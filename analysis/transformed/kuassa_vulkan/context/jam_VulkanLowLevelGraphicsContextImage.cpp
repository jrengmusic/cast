//
// VulkanImage file: drawImage() and its helpers. Depends on setViewportAndScissor(),
// setStencilTestStateIfActive(), and makeImagePushConstants() defined in
// jam_VulkanLowLevelGraphicsContext.cpp, included before this file in jam_vulkan.cpp.

namespace jam
{
/*____________________________________________________________________________*/
// computeImageDeviceBounds — transforms the image-space bounds (0, 0, width, height)
// by currentTransform composed with `t`, returning the result in device space.
juce::Rectangle<float> VulkanLowLevelGraphicsContext::computeImageDeviceBounds (const juce::Image& image, const juce::AffineTransform& t) const
{
    const auto composedTransform { currentState.currentTransform.getTransformWith (t) };
    const auto imageBounds { juce::Rectangle<float> (0.0f, 0.0f,
                                                      static_cast<float> (image.getWidth()),
                                                      static_cast<float> (image.getHeight())) };
    return imageBounds.transformedBy (composedTransform);
}

/*____________________________________________________________________________*/
// appendImageQuadRecord — transforms the image-space quad (0,0,width,height) by
// currentTransform composed with `t` to device space, then appends one
// VulkanPrimitiveRecord via appendPrimitiveRecord(). `image` may itself be a
// juce::Image::getClippedImage() subsection of the ROOT image actually cached
// (see VulkanGraphics::getImageSourceRoot()'s doc comment, jam_VulkanGraphics.h) —
// uvRect is therefore image's own proportional sub-rect within the cached
// ROOT texture (VulkanGraphics::getImageUVRect()), the full {0,0,1,1} rect when
// image IS the root; colour is opaque white (the image's own pixels carry
// colour — matches makeImagePushConstants's existing "opaque white"
// convention) since image.frag/image_alpha_mask.frag don't consume the
// vertex-forwarded vColor. textureIndex is the bindless array slot for
// image's cached root — caller has already cached and slotted the image via
// context.cacheImageTexture(). Caller has already reserved capacity.
int VulkanLowLevelGraphicsContext::appendImageQuadRecord (const juce::Image& image, const juce::AffineTransform& t)
{
    const auto deviceBounds { computeImageDeviceBounds (image, t) };

    const int rawIndex { context.getBindlessIndex (image) };
    jassert (rawIndex >= 0);// residency invariant: image cached at bootstrap or first draw
    const uint32_t textureIndex { static_cast<uint32_t> (rawIndex) };

    return appendPrimitiveRecord (deviceBounds, context.getImageUVRect (image), juce::Colours::white, 1.0f, textureIndex);
}

/*____________________________________________________________________________*/
// recordImageDrawCommands — binds the record state, pushes colour (opacity
// pre-multiplied into its alpha channel by makeImagePushConstants — image.frag
// reads color.a directly, no separate opacity field), selects the imageInstanced
// pipeline (stencil/no-stencil), and issues the instanced draw (4 vertices,
// 1 instance, gl_InstanceIndex = recordIndex).
//
// Set 1 is unconditionally the persistent bindless texture array (bound
// once, same handle every draw — VulkanPrimitiveRecord::textureIndex, baked by
// appendImageQuadRecord, selects the sampled image).
void VulkanLowLevelGraphicsContext::recordImageDrawCommands (int recordIndex, const juce::Image& image, float opacity)
{
    const int textureIndex { context.getBindlessIndex (image) };

    vk::CommandBuffer cmd { context.getCommandBuffer() };
    vk::PipelineLayout layout { context.getPipelines().getLayout() };
    vk::DescriptorSet projSet { context.getProjectionDescriptorSet() };
    vk::DescriptorSet recordSet { context.getRecordDescriptorSet() };
    vk::DescriptorSet imgSet { context.getBindlessTextureDescriptorSet() };

    cmd.bindDescriptorSets (vk::PipelineBindPoint::eGraphics, layout, 1, imgSet, nullptr);
    cmd.bindDescriptorSets (vk::PipelineBindPoint::eGraphics, layout, 2, recordSet, nullptr);
    cmd.bindDescriptorSets (vk::PipelineBindPoint::eGraphics, layout, 3, projSet, nullptr);

    auto pc { makeImagePushConstants (opacity) };
    cmd.pushConstants (layout, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, sizeof (pc), &pc);

    const auto imageId { currentState.maskTextureIndex != noMaskIndex
        ? VulkanPipelines::ID::maskedImageInstancedStencil
        : currentState.stencilClipDepth > 0
            ? VulkanPipelines::ID::imageInstancedStencil
            : VulkanPipelines::ID::imageInstanced };

    setStencilTestStateIfActive (cmd);
    cmd.bindPipeline (vk::PipelineBindPoint::eGraphics, context.getPipelines()[imageId]);
    setViewportAndScissor (cmd);

    cmd.draw (4, 1, 0, static_cast<uint32_t> (recordIndex));
}

/*____________________________________________________________________________*/
// drawImage — native Vulkan textured-quad draw via imageInstanced pipeline.
//
// The quad's device-space geometry is pre-computed on CPU by appendImageQuadRecord().
// The MVP storage buffer holds only orthoProjection mapping device space to NDC.
// Scissor comes from computeScissor() — no separate view/model split.
void VulkanLowLevelGraphicsContext::drawImage (const juce::Image& image,
                                          const juce::AffineTransform& t)
{
    if (image.getWidth() > 0 and image.getHeight() > 0)
    {
        context.cacheImageTexture (image);
        updateProjection();

        auto& recordBuffer { context.getPrimitiveRecordBuffer() };
        context.reservePrimitiveRecords (recordBuffer.getUsedRecords() + 1);

        const int recordIndex { appendImageQuadRecord (image, t) };
        recordImageDrawCommands (recordIndex, image, currentState.opacity);
    }
}

/*____________________________________________________________________________*/
// recordClipToImageAlphaDrawCommands — binds the record state (same descriptor
// selection strategy as recordImageDrawCommands — see its doc comment), then the
// clipMaskInstanced pipeline and stencil write state (same masks/reference
// sequencing as recordClipToPathDrawCommands), and issues the non-indexed
// instanced draw. Writes currentState.stencilClipDepth into the stencil buffer.
void VulkanLowLevelGraphicsContext::recordClipToImageAlphaDrawCommands (int recordIndex, const juce::Image& image)
{
    vk::CommandBuffer cmd { context.getCommandBuffer() };
    vk::PipelineLayout layout { context.getPipelines().getLayout() };
    vk::DescriptorSet projSet { context.getProjectionDescriptorSet() };
    vk::DescriptorSet recordSet { context.getRecordDescriptorSet() };
    vk::DescriptorSet imgSet { context.getBindlessTextureDescriptorSet() };

    cmd.bindDescriptorSets (vk::PipelineBindPoint::eGraphics, layout, 1, imgSet, nullptr);
    cmd.bindDescriptorSets (vk::PipelineBindPoint::eGraphics, layout, 2, recordSet, nullptr);
    cmd.bindDescriptorSets (vk::PipelineBindPoint::eGraphics, layout, 3, projSet, nullptr);

    auto pc { makeImagePushConstants (1.0f) };
    cmd.pushConstants (layout, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, sizeof (pc), &pc);

    cmd.bindPipeline (vk::PipelineBindPoint::eGraphics, context.getPipelines()[VulkanPipelines::ID::clipMaskInstanced]);

    // The write draw's stencil EQUAL test gates on the PARENT depth
    // (stencilClipDepth - 1, computed in signed int before the cast, so depth
    // 0->1's first-ever write compares against 0, matching the LOAD_OP_CLEAR
    // stencil buffer). Content draws still bake/test the NEW depth via
    // record.stencilRef / setStencilTestStateIfActive — unchanged.
    cmd.setStencilReference   (vk::StencilFaceFlagBits::eFrontAndBack, static_cast<uint32_t> (currentState.stencilClipDepth - 1));
    cmd.setStencilWriteMask  (vk::StencilFaceFlagBits::eFrontAndBack, 0xFF);
    cmd.setStencilCompareMask (vk::StencilFaceFlagBits::eFrontAndBack, 0xFF);

    setViewportAndScissor (cmd);

    cmd.draw (4, 1, 0, static_cast<uint32_t> (recordIndex));
}

/*____________________________________________________________________________*/
// clipToImageAlpha — caches sourceImage as a texture (shared cache with drawImage),
// then draws its quad through the clipMaskInstanced pipeline: the alpha-mask
// fragment shader discards fragments where the source image's alpha is zero, so only
// the covered region writes currentState.stencilClipDepth into the stencil buffer.
// Mirrors clipToPath's control flow exactly, substituting an image quad for
// triangulated path geometry.
void VulkanLowLevelGraphicsContext::clipToImageAlpha (const juce::Image& sourceImage,
                                                 const juce::AffineTransform& transform)
{
    if (sourceImage.getWidth() > 0 and sourceImage.getHeight() > 0)
    {
        context.cacheImageTexture (sourceImage);
        updateProjection();

        auto& recordBuffer { context.getPrimitiveRecordBuffer() };
        context.reservePrimitiveRecords (recordBuffer.getUsedRecords() + 1);

        // Increment BEFORE appending the record — matches clipToPath's ordering
        // (++currentState.stencilClipDepth before the write draw), so the record's
        // stencilRef captures the NEW depth this clip write establishes, not the
        // prior one. Lives in State (saveState/restoreState-covered) — a clip
        // established inside a save/restore block can no longer leak past the
        // matching restoreState() (fixes defect #1).
        ++currentState.stencilClipDepth;
        const int recordIndex { appendImageQuadRecord (sourceImage, transform) };
        recordClipToImageAlphaDrawCommands (recordIndex, sourceImage);

        const auto deviceBounds { computeImageDeviceBounds (sourceImage, transform) };
        currentState.deviceSpaceClipList.clipTo (deviceBounds.getSmallestIntegerContainer());
        currentState.maskTextureIndex = static_cast<uint32_t> (context.getBindlessIndex (sourceImage));
    }
}

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
