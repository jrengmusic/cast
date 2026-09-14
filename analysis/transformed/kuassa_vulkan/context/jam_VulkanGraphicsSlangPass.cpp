//
// Slang-pass file: recordPostProcessCompositeDrawCommands() (endFrame's
// post-process swapchain composite — un-premultiplies the scene via
// recordStraightAlphaPass(), snapshots OriginalHistoryN via
// recordOriginalHistoryCopy(), records shader's buffer passes via
// recordShaderBufferPasses(), then the Image-pass gather + combine draw) and
// getSlangTextureBindings() (a slang pass's own reflected texture-name
// resolution — PassOutputN/PassFeedbackN/alias/Original/Source/
// OriginalHistoryN/named-LUT, see its own doc comment). Placed after
// jam_VulkanLowLevelGraphicsContextGlyph.cpp in jam_vulkan.cpp's include
// order — reuses setFullViewport(), file-local to
// jam_VulkanLowLevelGraphicsContext.cpp.

namespace jam
{
/*____________________________________________________________________________*/
//==============================================================================
// VulkanShader execution — post-process composite (endFrame's swapchain composite)
//==============================================================================

void VulkanGraphics::recordPostProcessCompositeDrawCommands (const VulkanShader& shader,
                                                       VulkanShaderInstance& execution,
                                                       float opacity,
                                                       float resolutionScale,
                                                       const std::array<float, 4>& mouse)
{
    // Same graceful-degradation gate as recordSceneCompositeDrawCommands():
    // sceneColorImage has no bindless slot when the array was at capacity
    // when it was created — nothing samplable exists for this composite (or
    // the identity one it replaces) either way. straightAlphaImage needs its
    // own bindless slot too (allocated separately from sceneColorImage's —
    // either can hit the capacity ceiling independently); when either is
    // missing the composite degrades gracefully exactly as documented above
    // (nothing samplable exists for this path).
    if (sceneColorBindlessIndex >= 0 and straightAlphaBindlessIndex >= 0)
    {
        // Un-premultiplies the resolved (premultiplied) scene into
        // straightAlphaImage FIRST — every downstream read on this composite
        // path (recordShaderBufferPasses()' sceneBindlessIndex parameter
        // below, and stampChannels()'s fallback for the Image pass further
        // down) resolves against THIS straight-alpha result, never
        // sceneColorImage directly (locked contract: user post-process
        // shaders see straight alpha; glass alpha is immutable; this
        // composite's own post_process_combine.frag re-premultiplies by
        // scene.a before its opaque write below — see
        // jam_vulkan/shaders/post_process_combine.frag).
        // recordStraightAlphaPass() barriers sceneColorImage for
        // sampling itself (own doc comment) — no separate barrier needed here.
        recordStraightAlphaPass();

        // Snapshots THIS frame's straight-alpha scene into execution's own
        // OriginalHistoryN ring buffer, immediately after
        // recordStraightAlphaPass() and BEFORE recordShaderBufferPasses()
        // below can read it — gated on getOriginalHistoryDepth() > 0 (a
        // chain reflecting no OriginalHistoryN texture at all has no ring to
        // write). See recordOriginalHistoryCopy()'s own doc comment for the
        // full ordering/cursor-arithmetic contract.
        if (execution.getOriginalHistoryDepth() > 0)
            recordOriginalHistoryCopy (execution);

        // No scene render pass is active at this call site (endFrame() ended
        // it just before reaching here) — resumeAfter = false, mirroring
        // recordShaderBufferPasses()'s documented contract for exactly this
        // call site. straightAlphaBindlessIndex (NOT sceneColorBindlessIndex —
        // see recordStraightAlphaPass()'s doc comment) is this composite's own
        // resolved-scene slot — the same index channels[]' fallback rule
        // below reads — stamped into iScene here (SSOT) and carried forward
        // unchanged into imageUniforms below.
        const VulkanShaderUniforms baseUniforms { recordShaderBufferPasses (
            shader, execution, false, straightAlphaBindlessIndex, resolutionScale, mouse) };

        const VulkanShaderUniforms imageUniforms { recordPostProcessGatherDraw (
            execution, baseUniforms, opacity) };

        recordPostProcessCombineDraw (execution, imageUniforms);
    }
}

VulkanShaderUniforms VulkanGraphics::recordPostProcessGatherDraw (VulkanShaderInstance& execution,
                                                      const VulkanShaderUniforms& baseUniforms,
                                                      float opacity)
{
    // Gather stage: the Image pass's own dedicated pipeline (built once,
    // eagerly, by VulkanShaderInstance::build() from this shader's compiled
    // Image-pass SPIR-V — VulkanShaderInstance::buildRenderResources()) draws
    // its raw, unmodified colour (no opacity math — see
    // jam_vulkan/shader/shadertoy_wrapper.frag for VulkanShaderFormat::
    // shadertoy, or the pass's own already-complete main() compiled
    // as-is for VulkanShaderFormat::slang — the zero-injection contract: real
    // .slang sources declare their own layout(set,binding) resources,
    // which the fixed-set-0 bindless macro wrapper would collide with;
    // bindings resolved by SPIR-V reflection downstream instead) into its own
    // offscreen gather target, at gatherTarget's own extent (always this
    // shader's own scaledExtent — see VulkanShaderInstance::build()'s own doc
    // comment), instead of compositeRenderPass/swapchainFramebuffers
    // directly. recordPostProcessCombineDraw() then samples this same target
    // back and draws the real composite.
    VulkanRenderResources& gatherTarget { execution.getImagePassGatherTarget() };

    // Channel mapping (SSOT — VulkanShaderInstance::stampChannels()): a channel
    // with no buffer pass falls back to the straight-alpha scene, the one
    // input this composite always has ready.
    VulkanShaderUniforms imageUniforms { baseUniforms };
    execution.stampChannels (imageUniforms, straightAlphaBindlessIndex, getNativeHandle());
    imageUniforms.opacity = opacity;

    const int writeHalf { recordPostProcessGatherPassBegin (execution) };

    recordPostProcessGatherBindAndPush (execution, imageUniforms, gatherTarget.extent);
    recordPostProcessGatherDrawAndEnd (execution, imageUniforms, writeHalf);

    return imageUniforms;
}

int VulkanGraphics::recordPostProcessGatherPassBegin (VulkanShaderInstance& execution)
{
    VulkanRenderResources& gatherTarget { execution.getImagePassGatherTarget() };

    // Write target: the other gather half (its own front half is a
    // self-feedback .slang shader's own read source this call,
    // VulkanGraphics::getSlangTextureBindings()'s own doc comment) — same
    // ping-pong toggle arithmetic as recordSingleBufferPass()'s own
    // writeHalf (degenerates to always 0 for a non-feedback,
    // single-image gather target, VulkanRenderResources' own doc comment).
    const int writeHalf { (gatherTarget.currentReadHalf + 1) % gatherTarget.images.size() };
    vk::Image writeImage { gatherTarget.images.at (writeHalf).getImage() };

    // Discard-transition — this draw fully overwrites the whole write-half
    // image every call (no prior content to preserve), but the wait side
    // still fences the previous frame's fragment-shader read of this same
    // image (WAR hazard, only reachable when a self-feedback .slang shader
    // actually samples it back) — same barrier shape
    // recordSingleBufferPass() records around its own ping-pong write.
    recordImageMemoryBarrier (commandBuffer,
                              writeImage,
                              vk::ImageLayout::eUndefined,
                              vk::ImageLayout::eColorAttachmentOptimal,
                              vk::ImageAspectFlagBits::eColor,
                              vk::AccessFlagBits::eShaderRead,
                              vk::AccessFlagBits::eColorAttachmentWrite,
                              vk::PipelineStageFlagBits::eFragmentShader,
                              vk::PipelineStageFlagBits::eColorAttachmentOutput);

    // Raw beginRenderPass against gatherTarget's own stored render pass —
    // same convention recordSingleBufferPass() uses for its own offscreen
    // write (target.renderPass); never touches renderPassActive (that flag
    // tracks only the scene render pass, and none is active at this call
    // site either way — see recordPostProcessCompositeDrawCommands()'s own
    // doc comment).
    //
    // clearValueCount must cover the depth attachment (index 1, always
    // loadOp=eClear — VulkanGraphics::getOrCreateShaderOffscreenRenderPass
    // (vk::Format)'s own doc comment) even though this target's own colour
    // attachment (index 0) stays loadOp=eDontCare — the post-process path
    // never wires a mesh into this gather target (VulkanLowLevelGraphicsContext::
    // renderShader()'s own background-only mesh integration), so this depth
    // attachment is always inert scratch space here.
    std::array<vk::ClearValue, 2> clearValues {};
    clearValues.at (1).depthStencil = vk::ClearDepthStencilValue { 1.0f, 0 };

    vk::RenderPassBeginInfo rpInfo {};
    rpInfo.renderPass = gatherTarget.renderPass;
    rpInfo.framebuffer = gatherTarget.framebuffers.at (writeHalf);
    rpInfo.renderArea.offset = vk::Offset2D { 0, 0 };
    rpInfo.renderArea.extent = gatherTarget.extent;
    rpInfo.clearValueCount = static_cast<uint32_t> (clearValues.size());
    rpInfo.pClearValues = clearValues.data();
    commandBuffer.beginRenderPass (&rpInfo, vk::SubpassContents::eInline);

    return writeHalf;
}

void VulkanGraphics::recordPostProcessGatherBindAndPush (VulkanShaderInstance& execution,
                                                   const VulkanShaderUniforms& imageUniforms,
                                                   vk::Extent2D gatherExtent)
{
    // VulkanShaderFormat::slang: the mandatory Image pass is just as
    // author-shaped as any buffer pass — same reflected descriptor
    // set/pipeline layout substitution as recordSingleBufferPass(), see
    // its own doc comment. isBackground = false unconditionally (this
    // draw only ever runs the post-process composite path).
    if (execution.usesSlangPipeline())
    {
        const int imagePassIndex { execution.getImagePassIndex() };
        const VulkanShaderTextureBindings textureBindings { getSlangTextureBindings (
            execution.getSlangPassTextures (imagePassIndex), execution, false, imagePassIndex) };

        const juce::MemoryBlock pushConstantBytes { execution.refreshSlangPass (
            imagePassIndex,
            gatherExtent,
            execution.getBuiltExtent(),
            static_cast<uint32_t> (imageUniforms.iFrame),
            textureBindings,
            shaderRegistry.getOrCreateSlangPassSampler (execution.getSlangPassSettings (imagePassIndex))) };

        if (const vk::DescriptorSet slangSet {
                execution.getSlangPassDescriptorSet (imagePassIndex) };
            slangSet != nullptr)
        {
            commandBuffer.bindDescriptorSets (vk::PipelineBindPoint::eGraphics,
                                              execution.getSlangPassPipelineLayout (imagePassIndex),
                                              0,
                                              slangSet,
                                              nullptr);
        }

        if (not pushConstantBytes.isEmpty())
            commandBuffer.pushConstants (
                execution.getSlangPassPipelineLayout (imagePassIndex),
                vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
                0,
                static_cast<uint32_t> (pushConstantBytes.getSize()),
                pushConstantBytes.getData());
    }
    else
    {
        commandBuffer.bindDescriptorSets (vk::PipelineBindPoint::eGraphics,
                                          shaderRegistry.getOrCreateShaderInstanceLayout(),
                                          0,
                                          bindlessTextureDescriptorSet,
                                          nullptr);

        commandBuffer.pushConstants (
            shaderRegistry.getOrCreateShaderInstanceLayout(),
            vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
            0,
            sizeof (imageUniforms),
            &imageUniforms);
    }
}

void VulkanGraphics::recordPostProcessGatherDrawAndEnd (VulkanShaderInstance& execution,
                                                  const VulkanShaderUniforms& imageUniforms,
                                                  int writeHalf)
{
    VulkanRenderResources& gatherTarget { execution.getImagePassGatherTarget() };

    commandBuffer.bindPipeline (vk::PipelineBindPoint::eGraphics, gatherTarget.pipeline);

    setFullViewport (commandBuffer, gatherTarget.extent);
    const vk::Rect2D scissor {
        { 0, 0 },
        gatherTarget.extent
    };
    commandBuffer.setScissor (0, scissor);

    // Slang: the mandatory Image pass's own pipeline was built against
    // slangQuadVertexBuffer's own vertex input state
    // (VulkanShaderInstance::createFullscreenPipeline()) — bind it and draw
    // its 4 vertices as a triangle strip. Shadertoy: shader_pass.vert
    // generates its own 3 vertices purely from gl_VertexIndex, needing
    // no vertex buffer at all — today's exact, unchanged draw.
    if (execution.usesSlangPipeline())
    {
        const vk::Buffer quadBuffers[1] { shaderRegistry.getSlangQuadVertexBuffer() };
        const vk::DeviceSize quadOffsets[1] { 0 };
        commandBuffer.bindVertexBuffers (0, quadBuffers, quadOffsets);
        commandBuffer.draw (VulkanShaderInstance::slangQuadVertexCount, 1, 0, 0);
    }
    else
    {
        commandBuffer.draw (VulkanPipelines::fullscreenTriangleVertexCount, 1, 0, 0);
    }

    commandBuffer.endRenderPass();

    // Same-layout memory-visibility barrier — the offscreen render pass's
    // finalLayout is already eShaderReadOnlyOptimal, but its implicit
    // vk::SubpassExternal dependency alone does not guarantee cross-stage
    // read visibility, exactly matching recordSingleBufferPass()'s own
    // post-pass barrier shape.
    vk::Image writeImage { gatherTarget.images.at (writeHalf).getImage() };
    recordImageMemoryBarrier (commandBuffer,
                              writeImage,
                              vk::ImageLayout::eShaderReadOnlyOptimal,
                              vk::ImageLayout::eShaderReadOnlyOptimal,
                              vk::ImageAspectFlagBits::eColor,
                              vk::AccessFlagBits::eColorAttachmentWrite,
                              vk::AccessFlagBits::eShaderRead,
                              vk::PipelineStageFlagBits::eColorAttachmentOutput,
                              vk::PipelineStageFlagBits::eFragmentShader);

    gatherTarget.currentReadHalf = writeHalf;
}

#if JUCE_WINDOWS
void VulkanGraphics::recordPostProcessCombineDraw (VulkanShaderInstance& execution,
                                             const VulkanShaderUniforms& imageUniforms)
{
    // Post-process combine draw — the real swapchain composite this
    // VulkanShader's post-process chain produces, replacing
    // recordSceneCompositeDrawCommands()'s identity composite for this
    // frame. No render pass is active at this call site (endFrame()
    // already ended the scene render pass before reaching here), so this
    // opens/closes compositeRenderPass itself, raw (not through
    // beginRenderPass()/resumeRenderPass() — those are hardwired to the
    // scene renderPass/renderPassLoad objects). Samples the just-written
    // gather target back through channels[0], stamped with its own
    // bindless index (this pass's own contract, distinct from
    // imageUniforms' channels[] above — the gather draw's channels[]
    // served the USER shader's own buffer-pass reads; this combine
    // shader is engine-authored and always reads its single input at
    // channels[0]) — read via the gather target's own currentReadHalf,
    // already advanced by recordPostProcessGatherDrawAndEnd() before this
    // runs. iScene carries straightAlphaBindlessIndex forward unchanged
    // from baseUniforms (SSOT, never re-stamped, folded into imageUniforms
    // above). Sets renderPassActive true/false around its own begin/end for
    // invariant symmetry, mirroring recordSceneCompositeDrawCommands()'s
    // exact terminal-draw convention (this call is always terminal —
    // endFrame()'s last composite step before commandBuffer.end()/present).
    VulkanRenderResources& gatherTarget { execution.getImagePassGatherTarget() };

    vk::PipelineLayout layout { shaderRegistry.getOrCreateShaderInstanceLayout() };
    vk::DescriptorSet bindlessSet { bindlessTextureDescriptorSet };

    VulkanShaderUniforms combineUniforms { imageUniforms };
    combineUniforms.channels[0] = gatherTarget.bindlessIndex.at (gatherTarget.currentReadHalf);

    const vk::Framebuffer combineFramebuffer { composition != nullptr
                                                    ? compositionFramebuffer
                                                    : swapchainFramebuffers.at (static_cast<int> (currentImageIndex)) };

    const vk::RenderPassBeginInfo combineRpInfo {
        compositeRenderPass,
        combineFramebuffer,
        vk::Rect2D { { 0, 0 }, swapchain.getExtent() }
    };
    commandBuffer.beginRenderPass (&combineRpInfo, vk::SubpassContents::eInline);
    renderPassActive = true;

    commandBuffer.bindDescriptorSets (
        vk::PipelineBindPoint::eGraphics, layout, 0, bindlessSet, nullptr);

    commandBuffer.pushConstants (
        layout,
        vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
        0,
        sizeof (combineUniforms),
        &combineUniforms);

    commandBuffer.bindPipeline (
        vk::PipelineBindPoint::eGraphics, shaderRegistry.getOrCreatePostProcessCombinePipeline (compositeRenderPass));

    setFullViewport (commandBuffer, swapchain.getExtent());
    const vk::Rect2D combineScissor {
        { 0, 0 },
        swapchain.getExtent()
    };
    commandBuffer.setScissor (0, combineScissor);

    commandBuffer.draw (VulkanPipelines::fullscreenTriangleVertexCount, 1, 0, 0);

    commandBuffer.endRenderPass();
    renderPassActive = false;
}
#else
void VulkanGraphics::recordPostProcessCombineDraw (VulkanShaderInstance& execution,
                                             const VulkanShaderUniforms& imageUniforms)
{
    // Post-process combine draw — the real swapchain composite this
    // VulkanShader's post-process chain produces, replacing
    // recordSceneCompositeDrawCommands()'s identity composite for this
    // frame. No render pass is active at this call site (endFrame()
    // already ended the scene render pass before reaching here), so this
    // opens/closes compositeRenderPass itself, raw (not through
    // beginRenderPass()/resumeRenderPass() — those are hardwired to the
    // scene renderPass/renderPassLoad objects). Samples the just-written
    // gather target back through channels[0], stamped with its own
    // bindless index (this pass's own contract, distinct from
    // imageUniforms' channels[] above — the gather draw's channels[]
    // served the USER shader's own buffer-pass reads; this combine
    // shader is engine-authored and always reads its single input at
    // channels[0]) — read via the gather target's own currentReadHalf,
    // already advanced by recordPostProcessGatherDrawAndEnd() before this
    // runs. iScene carries straightAlphaBindlessIndex forward unchanged
    // from baseUniforms (SSOT, never re-stamped, folded into imageUniforms
    // above). Sets renderPassActive true/false around its own begin/end for
    // invariant symmetry, mirroring recordSceneCompositeDrawCommands()'s
    // exact terminal-draw convention (this call is always terminal —
    // endFrame()'s last composite step before commandBuffer.end()/present).
    VulkanRenderResources& gatherTarget { execution.getImagePassGatherTarget() };

    vk::PipelineLayout layout { shaderRegistry.getOrCreateShaderInstanceLayout() };
    vk::DescriptorSet bindlessSet { bindlessTextureDescriptorSet };

    VulkanShaderUniforms combineUniforms { imageUniforms };
    combineUniforms.channels[0] = gatherTarget.bindlessIndex.at (gatherTarget.currentReadHalf);

    const vk::RenderPassBeginInfo combineRpInfo {
        compositeRenderPass,
        swapchainFramebuffers.at (static_cast<int> (currentImageIndex)),
        vk::Rect2D { { 0, 0 }, swapchain.getExtent() }
    };
    commandBuffer.beginRenderPass (&combineRpInfo, vk::SubpassContents::eInline);
    renderPassActive = true;

    commandBuffer.bindDescriptorSets (
        vk::PipelineBindPoint::eGraphics, layout, 0, bindlessSet, nullptr);

    commandBuffer.pushConstants (
        layout,
        vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
        0,
        sizeof (combineUniforms),
        &combineUniforms);

    commandBuffer.bindPipeline (
        vk::PipelineBindPoint::eGraphics, shaderRegistry.getOrCreatePostProcessCombinePipeline (compositeRenderPass));

    setFullViewport (commandBuffer, swapchain.getExtent());
    const vk::Rect2D combineScissor {
        { 0, 0 },
        swapchain.getExtent()
    };
    commandBuffer.setScissor (0, combineScissor);

    commandBuffer.draw (VulkanPipelines::fullscreenTriangleVertexCount, 1, 0, 0);

    commandBuffer.endRenderPass();
    renderPassActive = false;
}
#endif

//==============================================================================
// VulkanShader execution — slang texture-name resolution
//==============================================================================

VulkanShaderTextureBindings
VulkanGraphics::getSlangTextureBindings (const jam::Array<VulkanShaderReflection::TextureResource>& textures,
                                       VulkanShaderInstance& execution,
                                       bool isBackground,
                                       int passIndex) const
{
    VulkanShaderTextureBindings bindings;
    auto& externalTextures { execution.getExternalTextures() };

    for (auto& texture : textures)
    {
        // Named external LUT/texture (execution.getExternalTextures(),
        // VulkanShaderPreset::Texture::name) resolves FIRST, via direct name-map
        // lookup -- table dispatch, not
        // a fourth top-level if-chain arm (see this function's own header
        // doc comment). Binds this LUT's own per-window-built view/extent
        // directly (no per-window bindless-array INDEX needed here, unlike
        // the shadertoy channels[] path -- a slang pass samples through its
        // own dedicated descriptor set, VulkanShaderInstance::refreshSlangPass(),
        // never the bindless sampler2D[] array). Sampler: this same texture
        // binding is later written by refreshSlangPass() using THIS PASS's
        // own uniform sampler (getOrCreateSlangPassSampler (execution.
        // getSlangPassSettings (passIndex)), the caller's own argument) --
        // VulkanShaderPreset::Texture::filterLinear/wrapMode are NOT independently
        // consumed for a slang binding (unlike the shadertoy channelMacros()
        // path, which does pick a per-texture sampler array): RetroArch's
        // own per-LUT-filter default is unverifiable against a vendored
        // video_shader_parse.c source in this repo, so this pass adopts the
        // pass-parity default (the consuming pass's own resolved sampler)
        // rather than guess a per-texture matrix — flagged to ARCHITECT.
        if (externalTextures.contains (texture.name))
        {
            auto& lutTexture { externalTextures.at (texture.name) };

            bindings.views.emplace (texture.name, lutTexture.getView());
            bindings.extents.emplace (texture.name, lutTexture.getExtent());
        }
        else
        {
            int bufferPassOrdinal { -1 };
            bool isFeedbackRead { false };

            // PassFeedbackN resolves identically to PassOutputN — every buffer
            // pass is unconditionally self-feedback-capable (VulkanShaderPass's own
            // doc comment), so this is a name alias only, never a distinct
            // lookup against a different target. isFeedbackRead records which
            // spelling this texture used — consulted below (instead of
            // re-testing the prefix) by the mandatory Image pass's own
            // self-feedback branch, the one place output-vs-feedback actually
            // matters.
            if (texture.name.startsWith (VulkanShaderReflection::getPassOutputPrefix()))
                bufferPassOrdinal =
                    texture.name.substring (VulkanShaderReflection::getPassOutputPrefix().length())
                        .getIntValue();
            else if (texture.name.startsWith (VulkanShaderReflection::getPassFeedbackPrefix()))
            {
                bufferPassOrdinal =
                    texture.name.substring (VulkanShaderReflection::getPassFeedbackPrefix().length())
                        .getIntValue();
                isFeedbackRead = true;
            }

            // Neither engine-fixed prefix matched — fall back to this
            // execution's own author-alias vocabulary (aliasN preset directive
            // or its #pragma name fallback, VulkanShaderInstance::getPassAliases(),
            // jam_VulkanShaderInstance.h) before giving up: an exact hit is
            // RetroArch's own PASS_OUTPUT semantics (the alias names that pass's
            // CURRENT output, slang_process.cpp:177-207); a hit after stripping
            // VulkanShaderReflection::getFeedbackSuffix() ("<alias>Feedback") is
            // PASS_FEEDBACK — the SAME per-pass ordinal, resolving through the
            // exact same branches below as PassOutputN/PassFeedbackN.
            if (bufferPassOrdinal < 0)
            {
                const auto& passAliases { execution.getPassAliases() };
                const bool nameIsFeedbackAlias { texture.name.endsWith (
                    VulkanShaderReflection::getFeedbackSuffix()) };
                const auto aliasKey { nameIsFeedbackAlias
                                          ? texture.name.dropLastCharacters (
                                                VulkanShaderReflection::getFeedbackSuffix().length())
                                          : texture.name };

                if (passAliases.contains (aliasKey))
                {
                    bufferPassOrdinal = passAliases.at (aliasKey);
                    isFeedbackRead = nameIsFeedbackAlias;
                }
            }

            if (bufferPassOrdinal >= 0 and bufferPassOrdinal < execution.getBufferPassCount())
            {
                auto& target { execution.getBufferPassTarget (bufferPassOrdinal) };

                bindings.views.emplace (
                    texture.name, target.images.at (target.currentReadHalf).getView());
                bindings.extents.emplace (texture.name, target.extent);
            }
            else if (texture.name == VulkanShaderReflection::getOriginalName()
                     or texture.name == VulkanShaderReflection::getSourceName()
                     or texture.name.startsWith (VulkanShaderReflection::getOriginalHistoryPrefix()))
            {
                // Original always means the resolved scene (this engine has no
                // distinct "unprocessed input frame" concept RetroArch's own
                // Original/Source split assumes) — pp only (background mode has
                // no resolved scene to offer, matching sceneMacro()'s identical
                // GLSL-generation-time gate). Source means the stage immediately
                // PRECEDING @p passIndex within THIS shader's own chain — the
                // previous buffer pass's own current output when one exists
                // (@p passIndex > 0, works in background mode too — this is the
                // shader's own internal chaining, not scene-dependent), else the
                // exact same resolved-scene fallback as Original (pass 0 has no
                // previous stage in the chain — RetroArch's own pass-0-reads-the-
                // input-frame semantics), still pp only. OriginalHistoryN folds
                // into this SAME branch (never a fourth top-level resolution
                // branch — MANIFESTO's 3-branch max): OriginalHistory0 == the
                // exact same resolved-scene resolution as Original (RetroArch's
                // own semantic), OriginalHistoryN (N >= 1) instead reads
                // execution.getOriginalHistoryImages()'s own ring buffer.
                const bool wantsPreviousStageOutput { texture.name == VulkanShaderReflection::getSourceName()
                                                      and passIndex > 0 };

                // 0 for "Original"/"Source" (neither starts with
                // getOriginalHistoryPrefix(), substring() is never reached) and for
                // "OriginalHistory0" itself (OriginalHistory0 == Original) —
                // both resolve identically in the ordinal-0 branch below,
                // straight to the straight-alpha scene, never through the ring.
                const bool isOriginalHistoryName { texture.name.startsWith (
                    VulkanShaderReflection::getOriginalHistoryPrefix()) };
                const int originalHistoryOrdinal {
                    isOriginalHistoryName
                        ? texture.name.substring (VulkanShaderReflection::getOriginalHistoryPrefix().length())
                              .getIntValue()
                        : 0
                };

                if (wantsPreviousStageOutput)
                {
                    auto& previousTarget { execution.getBufferPassTarget (passIndex - 1) };

                    bindings.views.emplace (
                        texture.name,
                        previousTarget.images.at (previousTarget.currentReadHalf).getView());
                    bindings.extents.emplace (texture.name, previousTarget.extent);
                }
                else if (not isBackground and originalHistoryOrdinal == 0)
                {
                    // The SAME resolved-scene image this engine's existing
                    // straightAlphaBindlessIndex/iScene mechanism already feeds
                    // (VulkanGraphics::recordStraightAlphaPass()) — left absent from
                    // both maps in background mode, no entry, no error
                    // (graceful degradation, same contract this struct's own
                    // doc comment documents).
                    bindings.views.emplace (texture.name, getStraightAlphaImageView());
                    bindings.extents.emplace (texture.name, swapchain.getExtent());
                }
                else if (not isBackground
                         and originalHistoryOrdinal <= execution.getOriginalHistoryDepth())
                {
                    // OriginalHistoryN, N in [1, execution.getOriginalHistoryDepth()]
                    // — reads the ring buffer VulkanGraphics::recordOriginalHistoryCopy()
                    // writes into once per frame (recordPostProcessCompositeDrawCommands(),
                    // BEFORE this resolution ever runs this same frame).
                    //
                    // Cursor arithmetic: execution.getOriginalHistoryCursor() is
                    // read AFTER recordOriginalHistoryCopy()'s own end-of-call
                    // advance, so (cursor - 1 + ringCount) % ringCount is the
                    // slot THIS frame's own copy just landed at (frame n, never
                    // read here — N >= 1 never resolves to it); walking N slots
                    // further back from there gives frame n - N's own slot:
                    // (cursor - 1 - N) % ringCount, i.e. frame n - 1 at cursor - 2,
                    // frame n - 2 at cursor - 3, ..., frame n - N at
                    // cursor - 1 - N (all mod ringCount; + ringCount * 2 keeps
                    // the operand non-negative for every N in
                    // [1, execution.getOriginalHistoryDepth()], since ringCount
                    // == execution.getOriginalHistoryDepth() + 1 bounds
                    // N - 1 < ringCount).
                    auto& ringImages { execution.getOriginalHistoryImages() };
                    const int ringCount { ringImages.size() };
                    const int ringIndex { (execution.getOriginalHistoryCursor() - 1
                                           - originalHistoryOrdinal + ringCount * 2)
                                          % ringCount };

                    bindings.views.emplace (texture.name, ringImages.at (ringIndex).getView());
                    bindings.extents.emplace (texture.name, swapchain.getExtent());
                }
            }
            else if (bufferPassOrdinal == execution.getBufferPassCount() and isFeedbackRead
                     and execution.getImagePassGatherTarget().images.size() == 2)
            {
                // The mandatory Image pass's own self-feedback read
                // (PassFeedback<final pass ordinal>, or an author's own
                // "<alias>Feedback" resolving to that SAME final ordinal via
                // VulkanShaderInstance::getPassAliases() — RetroArch's real-world
                // 1-pass feedback preset shape, feedback.slangp -> feedback.slang)
                // — reading the Image pass's CURRENT-frame output
                // (its own PassOutput<N> or exact-alias spelling) is undefined
                // (this pass hasn't drawn yet this frame), so isFeedbackRead
                // (set only by the PassFeedback prefix or the "<alias>Feedback"
                // suffix above) gates this branch, never the output spelling.
                // Resolves against the gather target's OTHER half —
                // both gather draw sites (recordShaderImagePassDrawCommands(),
                // recordPostProcessCompositeDrawCommands() below) update
                // currentReadHalf only AFTER their own gather draw's
                // endRenderPass, and both call this resolution before that
                // update this same frame — so currentReadHalf here still
                // holds LAST frame's completed gather output, exactly what a
                // self-feedback read must sample. Gated on images.size() == 2
                // (history-capable, VulkanShaderInstance::build()'s own conditional
                // gatherImageCount) — a non-feedback single-image gather target
                // has no second, safely-stale half to read, so this stays
                // unresolved there (graceful degradation, same contract as the
                // Source/Original branch above).
                auto& gatherTarget { execution.getImagePassGatherTarget() };

                bindings.views.emplace (
                    texture.name, gatherTarget.images.at (gatherTarget.currentReadHalf).getView());
                bindings.extents.emplace (texture.name, gatherTarget.extent);
            }
        }
    }

    return bindings;
}

//==============================================================================
// VulkanShader execution
//==============================================================================

VulkanShaderInstance* VulkanGraphics::getOrCreateShaderInstance (const VulkanShader& shader, float resolutionScale, vk::Extent2D chainInputExtent)
{
    const vk::Extent2D scaledExtent { VulkanShaderInstance::computeScaledExtent (
        swapchain.getExtent(), resolutionScale) };

    const bool entryFound { shaderRegistry.hasShaderInstance (shader) };
    bool staleOrMissing { not entryFound };

    if (entryFound)
    {
        auto& existingInstance { shaderRegistry.getShaderInstance (shader) };

        staleOrMissing = existingInstance.getContentHash() != shader.contentHash
                          or existingInstance.getBuiltExtent().width != scaledExtent.width
                          or existingInstance.getBuiltExtent().height != scaledExtent.height;
    }

    if (staleOrMissing)
    {
        // Suspend the currently active render pass (if any) before this
        // (re)build's own transfer-command recording below —
        // shaderRegistry.buildExternalTexture()'s LUT uploads
        // (VulkanBindlessTexture::upload: copyBufferToImage + barriers, plus
        // shaderRegistry.recordMipChainGeneration()'s blits) and the mesh
        // path's VulkanMesh::build() staging copy (jam_VulkanShaderInstance.cpp,
        // reached via freshExecution->build() below) are illegal inside an
        // active render pass instance (this method's own @pre doc comment,
        // jam_VulkanGraphics.h). Captured before mutation, not re-read
        // after: VulkanLowLevelGraphicsContext::renderShader()'s in-scene
        // background call site always has the scene pass active on entry
        // (renderPassWasActive true here), while VulkanGraphics::endFrame()'s
        // post-process composite call site already ended it before reaching
        // this call (renderPassWasActive false, so neither branch below ever
        // fires there). A cache-hit call (staleOrMissing false) never reaches
        // this block at all — zero pass boundaries recorded, matching every
        // other lazily-built resource in this class.
        const bool renderPassWasActive { renderPassActive };

        if (renderPassWasActive)
            endRenderPass();

        auto freshExecution { std::make_unique<VulkanShaderInstance> (device) };

        // Per-pass render-pass factory (the per-pass render-target format
        // contract) — VulkanShaderInstance::build() resolves
        // each buffer pass's own format (srgb_framebufferN/float_framebufferN,
        // or this colorFormat fallback) and calls this factory with it,
        // rather than build() being handed one fixed render pass up front.
        if (freshExecution->build (shader, resolutionScale, swapchain.getFormat().format, swapchain.getExtent(),
                                   [this] (vk::Format passColorFormat)
                                   { return shaderRegistry.getOrCreateShaderOffscreenRenderPass (passColorFormat); },
                                   commandBuffer, shaderRegistry.getOrCreateMeshDescriptorSetLayout(),
                                   shaderRegistry.getOrCreateMeshPipelineLayout(),
                                   shaderRegistry.getOrCreateShaderInstanceLayout(),
                                   shaderRegistry.getOrCreateShaderPassVertModule(), chainInputExtent))
        {
            // Same unconditional bindless assignment convention as
            // getOrCreateTransparencyLayer()/getOrCreateWindingScratch(): a stable
            // slot per half, assigned once, written once — never per draw.
            // Every buffer pass is unconditionally ping-pong-feedback-capable
            // (VulkanShaderPass's own doc comment, jam_VulkanShaderPass.h).
            static constexpr int pingPongHalfCount { 2 };

            for (int passIndex = 0; passIndex < freshExecution->getBufferPassCount(); ++passIndex)
            {
                auto& target { freshExecution->getBufferPassTarget (passIndex) };

                for (int half = 0; half < pingPongHalfCount; ++half)
                {
                    if (target.bindlessIndex.at (half) < 0)
                    {
                        const int index { bindlessRegistry.assignBindlessIndex() };
                        target.bindlessIndex.at (half) = index;

                        writeBindlessTextureDescriptor (
                            static_cast<uint32_t> (index),
                            target.images.at (half).getView());
                    }
                }
            }

            // The Image pass's own gather target -- one slot per half it was
            // built with (VulkanRenderResources::bindlessIndex sized to
            // gatherTarget.images.size() -- 1 for a non-feedback target, 2
            // for a history-capable one, VulkanShaderInstance::build()'s own
            // conditional gatherImageCount) -- assigned the SAME way, once,
            // so the combine shaders (background_combine.frag/
            // post_process_combine.frag) can sample the just-written half
            // through channels[0] (stamped directly at each combine draw
            // site, mirroring recordStraightAlphaPass()'s own direct-stamp
            // technique -- never through stampChannels(), which is reserved
            // for buffer-pass/scene-fallback semantics).
            auto& gatherTarget { freshExecution->getImagePassGatherTarget() };
            const int gatherHalfCount { gatherTarget.images.size() };

            for (int half = 0; half < gatherHalfCount; ++half)
            {
                if (gatherTarget.bindlessIndex.at (half) < 0)
                {
                    const int index { bindlessRegistry.assignBindlessIndex() };
                    gatherTarget.bindlessIndex.at (half) = index;

                    writeBindlessTextureDescriptor (
                        static_cast<uint32_t> (index),
                        gatherTarget.images.at (half).getView());
                }
            }

            // External LUT/texture BindlessTextures (shader.preset.textures --
            // RetroArch textures= list merged with the host application's own
            // resource-manifest additions, VulkanShaderCompiler::compile()'s own merge/absolutize
            // step) -- built once here, same cadence as bufferPassTargets/
            // imagePassGatherTarget above: freshExecution->build() itself
            // creates no GPU-uploaded resource of its own (VulkanShaderInstance::
            // build()'s own doc comment) -- this VulkanGraphics's own staging
            // arena/command buffer (shaderRegistry.buildExternalTexture()'s own
            // upload requirement) are only available here, at the orchestrator.
            for (auto& texture : shader.preset.textures)
            {
                auto& lutTexture { freshExecution->getExternalTextures()[texture.name] };
                lutTexture = shaderRegistry.buildExternalTexture (texture, commandBuffer, stagingArena);

                if (lutTexture.isValid())
                {
                    const int index { bindlessRegistry.assignBindlessIndex() };

                    lutTexture.registerBindlessIndex (getNativeHandle(), index);
                    writeBindlessTextureDescriptor (static_cast<uint32_t> (index), lutTexture.getView());
                }
            }

            // A mesh-carrying execution (freshExecution->hasMesh()) needs no
            // bindless slot of its own — its own material-range/feature-edge
            // draws render directly into the Image pass's own gather target
            // (imagePassGatherTarget above), which already carries its own
            // slot assignment from the loop above this one; the combine
            // pipeline downstream samples that SAME slot either way, mesh
            // present or not (VulkanGraphics::recordMeshGatherDrawCommands()'s own
            // doc comment).
        }

        shaderRegistry.registerShaderInstance (&shader, std::move (freshExecution));

        // Resume the render pass suspended above — pairs with the
        // renderPassWasActive-gated endRenderPass() call at the top of this
        // block; see this block's own doc comment.
        if (renderPassWasActive)
            resumeRenderPass();
    }

    // SSOT stamp site — both call sites (LLGC's background render() and
    // VulkanGraphics::endFrame()'s post-process check) always reach
    // shaderRegistry.getShaderInstance() only after this call succeeds, so
    // this is the one place a live entry is proven still reachable this
    // frame. resetResources()'s orphan sweep (shaderRegistry.releaseRetiredInstances())
    // compares this stamp against frameCounter to move an owner-replaced
    // (destroyed) VulkanShader's now-unreachable entry into previousShaderInstances.
    auto& instance { shaderRegistry.getShaderInstance (shader) };
    instance.setLastUsedFrame (frameCounter);

    return instance.isReady() ? &instance : nullptr;
}

VulkanShaderUniforms VulkanGraphics::recordShaderBufferPasses (const VulkanShader& shader, VulkanShaderInstance& execution,
                                                   bool resumeAfter, int32_t sceneBindlessIndex,
                                                   float resolutionScale, const std::array<float, 4>& mouse)
{
    // Suspend the currently active render pass (if any) before recording any
    // offscreen pass — mirrors beginOffscreenTransparencyRenderPass()'s exact
    // suspend precedent. Whether to resume afterward is caller-controlled
    // (resumeAfter), not auto-inferred from this flag — the in-scene render()
    // call site knows a scene pass was active and passes true; the endFrame()
    // post-process call site knows none was and passes false.
    if (renderPassActive)
        endRenderPass();

    const vk::Extent2D scaledExtent { VulkanShaderInstance::computeScaledExtent (
        swapchain.getExtent(), resolutionScale) };
    VulkanShaderUniforms baseUniforms { execution.stampUniforms (scaledExtent, mouse) };

    // SSOT stamp site for iScene — every pass this call records (buffer
    // passes below, and the caller's own Image-pass push built from this
    // returned baseUniforms) copies this one value forward, never re-stamping
    // it.
    baseUniforms.iScene = sceneBindlessIndex;

    vk::DescriptorSet bindlessSet { bindlessTextureDescriptorSet };
    vk::PipelineLayout layout { shaderRegistry.getOrCreateShaderInstanceLayout() };

    for (int passIndex = 0; passIndex < execution.getBufferPassCount(); ++passIndex)
        recordSingleBufferPass (execution, passIndex, baseUniforms, layout, bindlessSet, sceneBindlessIndex);

    if (resumeAfter)
        resumeRenderPass();

    return baseUniforms;
}

void VulkanGraphics::recordSingleBufferPass (VulkanShaderInstance& execution, int passIndex, const VulkanShaderUniforms& baseUniforms,
                                       vk::PipelineLayout layout, vk::DescriptorSet bindlessSet,
                                       int32_t sceneBindlessIndex)
{
    auto& target { execution.getBufferPassTarget (passIndex) };

    // This pass's OWN render target extent (per the per-pass render-target
    // extent contract) — a preset-scaled buffer pass may legitimately differ from this
    // execution's own overall scaledExtent, so every renderArea/viewport/
    // scissor/passExtent use below reads target.extent directly, never a
    // shared extent parameter.
    const vk::Extent2D passExtent { target.extent };

    // Write target: the other ping-pong half (its own front half is this
    // call's read source, about to become stale) — every buffer pass is
    // unconditionally ping-pong-feedback-capable (VulkanShaderPass's own doc
    // comment, jam_VulkanShaderPass.h).
    const int writeHalf { 1 - target.currentReadHalf };
    vk::Image writeImage { target.images.at (writeHalf).getImage() };

    // Discard-transition — this pass's fullscreen draw fully overwrites the
    // whole image every call (no prior content to preserve), but the wait
    // side still fences the previous frame's fragment-shader read of this
    // same image (WAR hazard) — same convention
    // beginOffscreenTransparencyRenderPass() uses for its own color target.
    recordImageMemoryBarrier (commandBuffer, writeImage,
                              vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal,
                              vk::ImageAspectFlagBits::eColor,
                              vk::AccessFlagBits::eShaderRead, vk::AccessFlagBits::eColorAttachmentWrite,
                              vk::PipelineStageFlagBits::eFragmentShader, vk::PipelineStageFlagBits::eColorAttachmentOutput);

    // Raw beginRenderPass/endRenderPass, not VulkanGraphics's own
    // beginRenderPass()/resumeRenderPass() helpers — those are hardwired to
    // the scene renderPass/renderPassLoad objects; target.renderPass (this
    // pass's own resolved 2-attachment, single-sample offscreen render
    // pass — per the per-pass render-target format contract,
    // VulkanShaderInstance::buildRenderResources()) is fully opened and
    // closed within this one call, so it never touches renderPassActive
    // (which continues to track only the scene pass).
    //
    // clearValueCount must cover the depth attachment (index 1, always
    // loadOp=eClear — VulkanShaderRegistry::getOrCreateShaderOffscreenRenderPass
    // (vk::Format)'s own doc comment) even though this pass's own colour
    // attachment (index 0) stays loadOp=eDontCare and never reads its own
    // clear entry — this buffer pass's own pipeline never tests/writes
    // depth (VulkanPipelines::noStencilState()), so the depth attachment is inert
    // scratch space here, present only because every framebuffer built
    // against this one shared render pass shape must supply both.
    std::array<vk::ClearValue, 2> clearValues {};
    clearValues.at (1).depthStencil = vk::ClearDepthStencilValue { 1.0f, 0 };

    vk::RenderPassBeginInfo rpInfo {};
    rpInfo.renderPass = target.renderPass;
    rpInfo.framebuffer = target.framebuffers.at (writeHalf);
    rpInfo.renderArea.offset = vk::Offset2D { 0, 0 };
    rpInfo.renderArea.extent = passExtent;
    rpInfo.clearValueCount = static_cast<uint32_t> (clearValues.size());
    rpInfo.pClearValues = clearValues.data();
    commandBuffer.beginRenderPass (&rpInfo, vk::SubpassContents::eInline);

    // VulkanShaderFormat::slang: this pass's own reflected descriptor
    // set/pipeline layout (the slang zero-injection contract) replaces the
    // shared bindless set/VulkanShaderUniforms push entirely for this pass's draw
    // — a real .slang shader's own set-0 resources are incompatible with
    // this engine's fixed set-0 bindless contract (RetroArch's own
    // resource-usage rule: "All resources must be using descriptor set #0").
    // VulkanShaderFormat::
    // shadertoy takes the else branch below, byte-for-byte identical to
    // before slang support existed.
    if (execution.usesSlangPipeline())
    {
        const bool isBackground { sceneBindlessIndex == VulkanShaderUniforms::noScene };
        const VulkanShaderTextureBindings textureBindings { getSlangTextureBindings (
            execution.getSlangPassTextures (passIndex), execution, isBackground, passIndex) };

        const juce::MemoryBlock pushConstantBytes { execution.refreshSlangPass (
            passIndex, passExtent, execution.getBuiltExtent(), static_cast<uint32_t> (baseUniforms.iFrame),
            textureBindings, shaderRegistry.getOrCreateSlangPassSampler (execution.getSlangPassSettings (passIndex))) };

        if (const vk::DescriptorSet slangSet { execution.getSlangPassDescriptorSet (passIndex) }; slangSet != nullptr)
        {
            commandBuffer.bindDescriptorSets (vk::PipelineBindPoint::eGraphics,
                                              execution.getSlangPassPipelineLayout (passIndex), 0, slangSet, nullptr);
        }

        if (not pushConstantBytes.isEmpty())
            commandBuffer.pushConstants (execution.getSlangPassPipelineLayout (passIndex),
                                         vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
                                         0, static_cast<uint32_t> (pushConstantBytes.getSize()), pushConstantBytes.getData());
    }
    else
    {
        commandBuffer.bindDescriptorSets (vk::PipelineBindPoint::eGraphics, layout, 0, bindlessSet, nullptr);

        // channels[] = this buffer's current readable content per ordinal —
        // fresh (just-written) for a pass already recorded by an earlier
        // recordSingleBufferPass() call this recordShaderBufferPasses() loop,
        // last-available (previous frame) for a pass not yet reached (SSOT via
        // VulkanShaderInstance::stampChannels(), see its own doc comment).
        VulkanShaderUniforms passUniforms { baseUniforms };
        execution.stampChannels (passUniforms, sceneBindlessIndex, getNativeHandle());
        passUniforms.opacity = 1.0f;

        commandBuffer.pushConstants (layout, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
                                     0, sizeof (passUniforms), &passUniforms);
    }

    commandBuffer.bindPipeline (vk::PipelineBindPoint::eGraphics, target.pipeline);

    setFullViewport (commandBuffer, passExtent);
    const vk::Rect2D scissor { { 0, 0 }, passExtent };
    commandBuffer.setScissor (0, scissor);

    // Slang: this pass's own pipeline was built against
    // slangQuadVertexBuffer's own vertex input state
    // (VulkanShaderInstance::createFullscreenPipeline()) — bind it and draw its 4
    // vertices as a triangle strip. Shadertoy: shader_pass.vert generates its
    // own 3 vertices purely from gl_VertexIndex, needing no vertex buffer at
    // all — today's exact, unchanged draw.
    if (execution.usesSlangPipeline())
    {
        const vk::Buffer quadBuffers[1] { shaderRegistry.getSlangQuadVertexBuffer() };
        const vk::DeviceSize quadOffsets[1] { 0 };
        commandBuffer.bindVertexBuffers (0, quadBuffers, quadOffsets);
        commandBuffer.draw (VulkanShaderInstance::slangQuadVertexCount, 1, 0, 0);
    }
    else
    {
        commandBuffer.draw (VulkanPipelines::fullscreenTriangleVertexCount, 1, 0, 0);
    }

    commandBuffer.endRenderPass();

    // Level 0's own post-pass read-visibility barrier — unconditional,
    // unchanged from before this sweep, and this target's own mip chain
    // (below) DEPENDS on level 0 already being eShaderReadOnlyOptimal by the
    // time it runs (recordMipChainGeneration()'s own loop invariant), so
    // this must stay first, never reordered around the mip-chain gate below.
    recordImageMemoryBarrier (commandBuffer, writeImage,
                              vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageLayout::eShaderReadOnlyOptimal,
                              vk::ImageAspectFlagBits::eColor,
                              vk::AccessFlagBits::eColorAttachmentWrite, vk::AccessFlagBits::eShaderRead,
                              vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eFragmentShader);

    // mipmap_inputN (jam::VulkanShaderPreset::Pass::mipmapInput's own doc
    // comment) — every other pass on this shader (VulkanRenderResources::
    // numMipLevels == 1) never reaches this call at all, so the non-mip path
    // stays byte-identical to its own pre-existing behavior above.
    if (target.numMipLevels > 1)
        shaderRegistry.recordMipChainGeneration (commandBuffer, target.images.at (writeHalf).getImage(),
                                  target.extent, target.numMipLevels);

    target.currentReadHalf = writeHalf;
}

void VulkanGraphics::recordStraightAlphaPass()
{
    // Caller's gate (recordPostProcessCompositeDrawCommands()) guarantees
    // both indices are valid before reaching here — this makes that
    // precondition explicit.
    jassert (sceneColorBindlessIndex >= 0 and straightAlphaBindlessIndex >= 0);

    // Barrier sceneColorImage for sampling — this pass is now the FIRST read
    // of sceneColorImage on the post-process path (moved here from
    // recordPostProcessCompositeDrawCommands(), which used to record this
    // exact same-layout barrier itself before its own, then-direct, scene
    // read); recordSceneCompositeDrawCommands()'s identity-composite path
    // still records its own copy of this same barrier shape for its own,
    // entirely separate, call site.
    recordImageMemoryBarrier (commandBuffer, sceneColorImage.getImage(),
                              vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageLayout::eShaderReadOnlyOptimal,
                              vk::ImageAspectFlagBits::eColor,
                              vk::AccessFlagBits::eColorAttachmentWrite, vk::AccessFlagBits::eShaderRead,
                              vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eFragmentShader);

    // Discard-transition — this pass fully overwrites straightAlphaImage's
    // whole extent every call, same WAR-hazard shape recordSingleBufferPass()
    // barriers its own ping-pong write half with.
    recordImageMemoryBarrier (commandBuffer, straightAlphaImage.getImage(),
                              vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal,
                              vk::ImageAspectFlagBits::eColor,
                              vk::AccessFlagBits::eShaderRead, vk::AccessFlagBits::eColorAttachmentWrite,
                              vk::PipelineStageFlagBits::eFragmentShader, vk::PipelineStageFlagBits::eColorAttachmentOutput);

    // clearValueCount must cover the depth attachment (index 1, always
    // loadOp=eClear) even though this pass's own colour attachment (index 0)
    // stays loadOp=eDontCare — straightAlphaDepthImage is inert scratch
    // space here, present only because straightAlphaFramebuffer is built
    // against this same shared 2-attachment render pass shape (see its own
    // doc comment, jam_VulkanGraphics.h).
    std::array<vk::ClearValue, 2> clearValues {};
    clearValues.at (1).depthStencil = vk::ClearDepthStencilValue { 1.0f, 0 };

    vk::RenderPassBeginInfo rpInfo {};
    rpInfo.renderPass = shaderRegistry.getOrCreateShaderOffscreenRenderPass (swapchain.getFormat().format);
    rpInfo.framebuffer = straightAlphaFramebuffer;
    rpInfo.renderArea.offset = vk::Offset2D { 0, 0 };
    rpInfo.renderArea.extent = swapchain.getExtent();
    rpInfo.clearValueCount = static_cast<uint32_t> (clearValues.size());
    rpInfo.pClearValues = clearValues.data();
    commandBuffer.beginRenderPass (&rpInfo, vk::SubpassContents::eInline);

    vk::PipelineLayout layout { shaderRegistry.getOrCreateShaderInstanceLayout() };
    commandBuffer.bindDescriptorSets (vk::PipelineBindPoint::eGraphics, layout, 0, bindlessTextureDescriptorSet, nullptr);

    // Plain stamp, not VulkanShaderInstance::stampUniforms()/stampChannels() (no
    // VulkanShaderInstance executes this pass) — every channels[] slot and iScene
    // point at sceneColorBindlessIndex, the one input this pass reads
    // (straight_alpha.frag samples channels[0], see its own doc comment).
    VulkanShaderUniforms uniforms {};
    uniforms.iResolution[0] = static_cast<float> (swapchain.getExtent().width);
    uniforms.iResolution[1] = static_cast<float> (swapchain.getExtent().height);
    uniforms.iScene = sceneColorBindlessIndex;

    for (int32_t ordinal = 0; ordinal < VulkanShaderUniforms::maxChannelCount; ++ordinal)
        uniforms.channels[ordinal] = sceneColorBindlessIndex;

    commandBuffer.pushConstants (layout, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
                                 0, sizeof (uniforms), &uniforms);

    commandBuffer.bindPipeline (vk::PipelineBindPoint::eGraphics, shaderRegistry.getOrCreateStraightAlphaPipeline (swapchain.getFormat().format));

    setFullViewport (commandBuffer, swapchain.getExtent());
    const vk::Rect2D scissor { { 0, 0 }, swapchain.getExtent() };
    commandBuffer.setScissor (0, scissor);

    commandBuffer.draw (VulkanPipelines::fullscreenTriangleVertexCount, 1, 0, 0);

    commandBuffer.endRenderPass();

    // Same-layout memory-visibility barrier — the offscreen render pass's
    // finalLayout is already eShaderReadOnlyOptimal, but its implicit
    // vk::SubpassExternal dependency alone does not guarantee cross-stage
    // read visibility, exactly matching recordSingleBufferPass()'s own
    // post-pass barrier shape.
    recordImageMemoryBarrier (commandBuffer, straightAlphaImage.getImage(),
                              vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageLayout::eShaderReadOnlyOptimal,
                              vk::ImageAspectFlagBits::eColor,
                              vk::AccessFlagBits::eColorAttachmentWrite, vk::AccessFlagBits::eShaderRead,
                              vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eFragmentShader);
}

void VulkanGraphics::recordOriginalHistoryCopy (VulkanShaderInstance& execution)
{
    // Gated by the caller (recordPostProcessCompositeDrawCommands()) on
    // execution.getOriginalHistoryDepth() > 0 — see this method's own header
    // doc comment (jam_VulkanGraphics.h) for the full ordering/cursor-
    // arithmetic contract.
    auto& ringImages { execution.getOriginalHistoryImages() };
    const int cursor { execution.getOriginalHistoryCursor() };
    const int ringCount { ringImages.size() };
    vk::Image ringSlotImage { ringImages.at (cursor).getImage() };

    // straightAlphaImage: same-layout-chain memory-visibility barrier into
    // eTransferSrcOptimal — srcAccess combines eShaderRead (the layout this
    // image already carries, per recordStraightAlphaPass()'s own final
    // barrier) with eColorAttachmentWrite (that same barrier's own
    // predecessor access/stage) so this new barrier's wait is correct
    // regardless of which stage's caches still hold the pending write.
    recordImageMemoryBarrier (commandBuffer, straightAlphaImage.getImage(),
                              vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageLayout::eTransferSrcOptimal,
                              vk::ImageAspectFlagBits::eColor,
                              vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eColorAttachmentWrite,
                              vk::AccessFlagBits::eTransferRead,
                              vk::PipelineStageFlagBits::eFragmentShader | vk::PipelineStageFlagBits::eColorAttachmentOutput,
                              vk::PipelineStageFlagBits::eTransfer);

    // Ring slot [cursor]: discard-transition into eTransferDstOptimal — this
    // copy fully overwrites the whole image every call, but the wait side
    // still fences a PRIOR frame's fragment-shader read of this same slot
    // (WAR hazard, only reachable once the ring has wrapped at least once) —
    // same convention recordSingleBufferPass()'s own ping-pong write barrier
    // uses for its own write half.
    recordImageMemoryBarrier (commandBuffer, ringSlotImage,
                              vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal,
                              vk::ImageAspectFlagBits::eColor,
                              vk::AccessFlagBits::eShaderRead, vk::AccessFlagBits::eTransferWrite,
                              vk::PipelineStageFlagBits::eFragmentShader, vk::PipelineStageFlagBits::eTransfer);

    const vk::ImageSubresourceLayers copyLayers { vk::ImageAspectFlagBits::eColor, 0, 0, 1 };
    const vk::ImageCopy copyRegion { copyLayers, vk::Offset3D { 0, 0, 0 }, copyLayers, vk::Offset3D { 0, 0, 0 },
                                     vk::Extent3D { swapchain.getExtent(), 1 } };

    commandBuffer.copyImage (straightAlphaImage.getImage(), vk::ImageLayout::eTransferSrcOptimal,
                             ringSlotImage, vk::ImageLayout::eTransferDstOptimal, copyRegion);

    // Ring slot [cursor]: transfer-write -> shader-read, for a LATER frame's
    // OriginalHistoryK resolution (VulkanGraphics::getSlangTextureBindings())
    // to sample.
    recordImageMemoryBarrier (commandBuffer, ringSlotImage,
                              vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal,
                              vk::ImageAspectFlagBits::eColor,
                              vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eShaderRead,
                              vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader);

    // straightAlphaImage: back to eShaderReadOnlyOptimal — every downstream
    // read on this composite path (recordShaderBufferPasses()'s
    // sceneBindlessIndex, stampChannels()'s fallback, Original/Source/
    // OriginalHistory0 resolution) samples it at this layout, unchanged from
    // before this call.
    recordImageMemoryBarrier (commandBuffer, straightAlphaImage.getImage(),
                              vk::ImageLayout::eTransferSrcOptimal, vk::ImageLayout::eShaderReadOnlyOptimal,
                              vk::ImageAspectFlagBits::eColor,
                              vk::AccessFlagBits::eTransferRead, vk::AccessFlagBits::eShaderRead,
                              vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader);

    execution.setOriginalHistoryCursor ((cursor + 1) % ringCount);
}

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam