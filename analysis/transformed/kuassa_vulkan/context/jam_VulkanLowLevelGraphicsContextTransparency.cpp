//
// Transparency file: beginTransparencyLayer(), endTransparencyLayer(), and their
// helpers. Depends on setFullViewport() defined in jam_VulkanLowLevelGraphicsContext.cpp,
// included before this file in jam_vulkan.cpp.

namespace jam
{
/*____________________________________________________________________________*/
// beginOffscreenTransparencyRenderPass — ensures the offscreen target for `level`
// exists, then begins a render pass into it.
//
// vk::CommandBuffer::clearColorImage/clearDepthStencilImage/copyImage are transfer
// commands, illegal inside an active render-pass instance — beginTransparencyLayer()
// already calls context.endRenderPass() before calling this method, so the transfer
// sequence below runs legally outside any render pass. Neither existing render-pass
// object supports the needed per-attachment shape (clear color unconditionally,
// load/preserve stencil conditionally) in one instance: the CLEAR-op renderPass
// clears BOTH attachments unconditionally (load ops are fixed per-vk::RenderPass-object,
// not switchable per beginRenderPass call); renderPassLoad loads both, which
// would incorrectly preserve stale color from this level's previous use. Fix: clear/
// copy both attachments manually via transfer commands, then begin renderPassLoad
// (LOAD both — content is already correct by construction).
void VulkanLowLevelGraphicsContext::beginOffscreenTransparencyRenderPass (int level)
{
    const vk::Extent2D extent { context.getSwapchainExtent() };

    context.getOrCreateTransparencyLayer (level);

    auto& target { context.getTransparencyLayer (level) };
    vk::CommandBuffer cmd { context.getCommandBuffer() };

    // --- Color: clear target.image (the MSAA color attachment) to transparent
    // black. oldLayout=UNDEFINED discards this level's previous content — this
    // level's color is unconditionally fully overwritten here every call, so
    // nothing needs preserving. Collapsed via recordImageMemoryBarrier().
    recordImageMemoryBarrier (cmd, target.image.getImage(),
                              vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal,
                              vk::ImageAspectFlagBits::eColor,
                              vk::AccessFlagBits::eColorAttachmentWrite, vk::AccessFlagBits::eTransferWrite,
                              vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eTransfer);

    const vk::ClearColorValue transparentBlack {};

    const vk::ImageSubresourceRange colorRange { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };

    cmd.clearColorImage (target.image.getImage(), vk::ImageLayout::eTransferDstOptimal,
                         transparentBlack, colorRange);

    // renderPassLoad's attachment 0 declares initialLayout COLOR_ATTACHMENT_OPTIMAL
    // — transition to match before the render pass begins.
    recordImageMemoryBarrier (cmd, target.image.getImage(),
                              vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eColorAttachmentOptimal,
                              vk::ImageAspectFlagBits::eColor,
                              vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eColorAttachmentWrite,
                              vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eColorAttachmentOutput);

    // --- Stencil: target.stencil's first-ever-use layout is vk::ImageLayout::eUndefined
    // (fresh from vmaCreateImage); on every subsequent call it is whatever the LAST
    // render pass targeting this framebuffer left it in. oldLayout=UNDEFINED is used
    // unconditionally to discard whatever was there (this level's stencil is
    // unconditionally fully overwritten every call, by copy or by clear).
    recordImageMemoryBarrier (cmd, target.stencil.getImage(),
                              vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal,
                              vk::ImageAspectFlagBits::eStencil,
                              vk::AccessFlagBits::eDepthStencilAttachmentWrite, vk::AccessFlagBits::eTransferWrite,
                              vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests,
                              vk::PipelineStageFlagBits::eTransfer);

    // Positive branch on currentState.stencilClipDepth (not a bail-out guard):
    // an active outer clip copies its shaped mask in; no active clip clears to zero.
    if (currentState.stencilClipDepth > 0)
    {
        // Nesting-aware copy source — level > 0 copies from the parent level's
        // own stencil image; level == 0 copies from the main scene's stencil.
        vk::Image sourceStencilImage { level > 0
            ? context.getTransparencyLayer (level - 1).stencil.getImage()
            : context.getStencilImage() };

        recordImageMemoryBarrier (cmd, sourceStencilImage,
                                  vk::ImageLayout::eDepthStencilAttachmentOptimal, vk::ImageLayout::eTransferSrcOptimal,
                                  vk::ImageAspectFlagBits::eStencil,
                                  vk::AccessFlagBits::eDepthStencilAttachmentWrite, vk::AccessFlagBits::eTransferRead,
                                  vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests,
                                  vk::PipelineStageFlagBits::eTransfer);

        vk::ImageCopy stencilCopyRegion {};
        stencilCopyRegion.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eStencil;
        stencilCopyRegion.srcSubresource.layerCount = 1;
        stencilCopyRegion.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eStencil;
        stencilCopyRegion.dstSubresource.layerCount = 1;
        stencilCopyRegion.extent = vk::Extent3D { extent.width, extent.height, 1 };

        cmd.copyImage (sourceStencilImage, vk::ImageLayout::eTransferSrcOptimal,
                       target.stencil.getImage(), vk::ImageLayout::eTransferDstOptimal,
                       stencilCopyRegion);

        recordImageMemoryBarrier (cmd, sourceStencilImage,
                                  vk::ImageLayout::eTransferSrcOptimal, vk::ImageLayout::eDepthStencilAttachmentOptimal,
                                  vk::ImageAspectFlagBits::eStencil,
                                  vk::AccessFlagBits::eTransferRead,
                                  vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite,
                                  vk::PipelineStageFlagBits::eTransfer,
                                  vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests);
    }
    else
    {
        const vk::ClearDepthStencilValue clearStencilValue { 0.0f, 0 };

        const vk::ImageSubresourceRange stencilRange { vk::ImageAspectFlagBits::eStencil, 0, 1, 0, 1 };

        cmd.clearDepthStencilImage (target.stencil.getImage(), vk::ImageLayout::eTransferDstOptimal,
                                   clearStencilValue, stencilRange);
    }

    // Transition target.stencil to DEPTH_STENCIL_ATTACHMENT_OPTIMAL before the
    // render pass begins (common to both branches above).
    recordImageMemoryBarrier (cmd, target.stencil.getImage(),
                              vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eDepthStencilAttachmentOptimal,
                              vk::ImageAspectFlagBits::eStencil,
                              vk::AccessFlagBits::eTransferWrite,
                              vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite,
                              vk::PipelineStageFlagBits::eTransfer,
                              vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests);

    // Resume rendering against this level's own framebuffer — both attachments are
    // already correct by construction (cleared/copied above), so LOAD is the
    // correct op for both.
    context.resumeRenderPass (target.framebuffer, extent);
}

/*____________________________________________________________________________*/
// beginTransparencyLayer — ends the current main render pass, begins an offscreen
// render pass for the transparency layer, and saves graphics state. All subsequent
// draw commands record into the offscreen target until endTransparencyLayer.
void VulkanLowLevelGraphicsContext::beginTransparencyLayer (float layerOpacity)
{
    saveState();
    transparencyOpacityStack.add (layerOpacity);

    context.endRenderPass();
    beginOffscreenTransparencyRenderPass (transparencyNestingLevel);

    ++transparencyNestingLevel;
}

/*____________________________________________________________________________*/
// transitionTransparencyLayerForSampling — target.image is the MSAA
// color attachment (never sampled directly); target.resolveImage is the actual
// composited output, and the shared renderPass's attachment 2 (resolve) already
// declares finalLayout = SHADER_READ_ONLY_OPTIMAL (no longer PRESENT_SRC_KHR — that
// layout belonged to the earlier swapchain-direct main pass). This barrier is
// now a same-layout memory dependency (not an actual transition): the render pass's
// implicit vk::SubpassExternal dependency does not by itself guarantee
// FRAGMENT_SHADER-stage read visibility of the resolve write.
void VulkanLowLevelGraphicsContext::transitionTransparencyLayerForSampling (VulkanTransparencyStack::TransparencyLayer& target) const
{
    // Collapsed via recordImageMemoryBarrier().
    recordImageMemoryBarrier (context.getCommandBuffer(), target.resolveImage.getImage(),
                              vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageLayout::eShaderReadOnlyOptimal,
                              vk::ImageAspectFlagBits::eColor,
                              vk::AccessFlagBits::eColorAttachmentWrite, vk::AccessFlagBits::eShaderRead,
                              vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eFragmentShader);
}

/*____________________________________________________________________________*/
// recordTransparencyCompositeDrawCommands — binds the persistent bindless texture
// descriptor set + record state (set 2), pushes layerOpacity, selects the
// imageInstanced pipeline (VertexInput::instanced, zero vertex attributes), and
// issues the full-screen composite draw via gl_InstanceIndex = recordIndex. Scissor
// always spans the full extent — the composite ignores the active clip (the layer's
// contents were already clipped when drawn into the offscreen target), matching
// drawImage's pipeline but not its scissor.
void VulkanLowLevelGraphicsContext::recordTransparencyCompositeDrawCommands (
    int recordIndex, vk::Extent2D extent, float layerOpacity)
{
    vk::CommandBuffer cmd { context.getCommandBuffer() };
    vk::PipelineLayout layout { context.getPipelines().getLayout() };
    vk::DescriptorSet projSet { context.getProjectionDescriptorSet() };
    vk::DescriptorSet recordSet { context.getRecordDescriptorSet() };
    vk::DescriptorSet imgSet { context.getBindlessTextureDescriptorSet() };

    cmd.bindDescriptorSets (vk::PipelineBindPoint::eGraphics, layout, 1, imgSet, nullptr);
    cmd.bindDescriptorSets (vk::PipelineBindPoint::eGraphics, layout, 2, recordSet, nullptr);
    cmd.bindDescriptorSets (vk::PipelineBindPoint::eGraphics, layout, 3, projSet, nullptr);

    // Push constant block matches imageInstanced pipeline layout (drawImage shape).
    const auto pc { makeImagePushConstants (layerOpacity) };
    cmd.pushConstants (layout, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, sizeof (pc), &pc);

    cmd.bindPipeline (vk::PipelineBindPoint::eGraphics, context.getPipelines()[VulkanPipelines::ID::imageInstanced]);

    setFullViewport (cmd, extent);
    const vk::Rect2D scissor { { 0, 0 }, extent };
    cmd.setScissor (0, scissor);

    cmd.draw (4, 1, 0, static_cast<uint32_t> (recordIndex));
}

/*____________________________________________________________________________*/
// compositeTransparencyLayer — draws `target` as a full-screen textured quad at
// `layerOpacity` onto the now-resumed main render pass. The full-screen quad's
// geometry (device-space {0,0,extent.width,extent.height}, full {0,0,1,1} uvRect,
// opaque white — matching appendImageQuadRecord's convention) is appended as one
// VulkanPrimitiveRecord; endTransparencyLayer can be called multiple times per frame, so
// each composite draws its own record.
void VulkanLowLevelGraphicsContext::compositeTransparencyLayer (VulkanTransparencyStack::TransparencyLayer& target, float layerOpacity)
{
    jassert (target.bindlessIndex >= 0);

    std::memcpy (context.getProjectionMappedPtr(), &orthoProjection, sizeof (orthoProjection));

    const vk::Extent2D extent { context.getSwapchainExtent() };

    auto& recordBuffer { context.getPrimitiveRecordBuffer() };

    context.reservePrimitiveRecords (recordBuffer.getUsedRecords() + 1);

    const juce::Rectangle<float> deviceBounds { 0.0f, 0.0f,
                                                  static_cast<float> (extent.width),
                                                  static_cast<float> (extent.height) };
    const int recordIndex { appendPrimitiveRecord (deviceBounds, { 0.0f, 0.0f, 1.0f, 1.0f },
                                                    juce::Colours::white, 1.0f,
                                                    static_cast<uint32_t> (target.bindlessIndex)) };

    recordTransparencyCompositeDrawCommands (recordIndex, extent, layerOpacity);
}

/*____________________________________________________________________________*/
// resumeRenderPassForNestingLevel — shared SSOT for the "resume with LOAD op after
// an offscreen pass has closed" branch, needed identically by endTransparencyLayer()
// (below) and recordWindingCompositeDrawCommands() (jam_VulkanLowLevelGraphicsContextPath.cpp)
// — the winding-scratch composite may itself be nested inside an active transparency
// layer, so it must resume that layer's own framebuffer rather than always the main
// scene, exactly like a nested transparency-layer close.
void VulkanLowLevelGraphicsContext::resumeRenderPassForNestingLevel (int nestingLevel)
{
    if (nestingLevel > 0)
    {
        auto& parentLayer { context.getTransparencyLayer (nestingLevel - 1) };
        context.resumeRenderPass (parentLayer.framebuffer, parentLayer.extent);
    }
    else
    {
        context.resumeRenderPass();
    }
}

/*____________________________________________________________________________*/
// endTransparencyLayer — ends the just-closed level's offscreen render pass,
// resumes either the parent transparency level's own offscreen framebuffer (LOAD
// op, nested case) or the main render pass (LOAD op, outermost case), composites
// the offscreen image back at the stored opacity, and restores graphics state.
void VulkanLowLevelGraphicsContext::endTransparencyLayer()
{
    jassert (not transparencyOpacityStack.isEmpty());

    --transparencyNestingLevel;

    const float layerOpacity { transparencyOpacityStack.last() };
    transparencyOpacityStack.remove (transparencyOpacityStack.size() - 1);

    // context.endRenderPass() — not a raw vk::CommandBuffer::endRenderPass call — so
    // renderPassActive stays correct for whichever pass is actually open (main or
    // offscreen, any nesting depth), matching beginOffscreenTransparencyRenderPass()'s
    // begin side.
    context.endRenderPass();

    auto& target { context.getTransparencyLayer (transparencyNestingLevel) };
    transitionTransparencyLayerForSampling (target);

    // Resume with LOAD op to preserve content drawn before beginTransparencyLayer.
    resumeRenderPassForNestingLevel (transparencyNestingLevel);

    compositeTransparencyLayer (target, layerOpacity);

    restoreState();
}

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam