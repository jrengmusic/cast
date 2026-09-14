//
// Render file: renderShader() and its Image-pass gather+combine draw helper —
// the background/post-process shader injection seam (jam_VulkanRender.h's
// render() free function forwards here). Depends on
// VulkanGraphics::getOrCreateShaderInstance()/recordShaderBufferPasses()
// (jam_VulkanGraphicsSlangPass.cpp) and VulkanShaderRegistry's
// getShaderInstance()/getOrCreateShaderInstanceLayout()/
// getOrCreateShaderOffscreenRenderPass()/getOrCreateBackgroundCombinePipeline()
// (jam_VulkanShaderRegistry.cpp) — placed after all of them in
// jam_vulkan.cpp's include order.

namespace jam
{
/*____________________________________________________________________________*/
// renderShader — ensures shader's GPU execution resources are current, records
// its buffer passes (mid-paint: suspends the active scene pass, resumes it
// afterward — a scene pass is always active here, this call only ever happens
// from within an LLGC paint), then emits the Image pass. getOrCreateShaderInstance()
// returning false is a transient build failure (e.g. this generation's SPIR-V
// modules failed to compile into pipelines) — the draw is skipped entirely for
// this call rather than drawing stale or partial GPU state; getOrCreateShaderInstance()
// is retried unconditionally on every call, so a later frame recovers on its own
// once the underlying resource issue clears.
//
// getOrCreateShaderInstance() is self-managed (its own @pre doc comment,
// jam_VulkanGraphics.h): on a (re)build it suspends/resumes the active scene
// pass itself, entirely around its own transfer-command recording
// (buildExternalTexture()'s LUT uploads and the mesh path's VulkanMesh::build()
// staging copy); a cache-hit call records zero pass boundaries. This call
// site no longer brackets it — recordShaderBufferPasses() below still
// suspends/resumes the pass independently for its own offscreen buffer-pass
// recording, unrelated to getOrCreateShaderInstance()'s own internal bracket.
void VulkanLowLevelGraphicsContext::renderShader (const VulkanShader& shader, float opacity, float resolutionScale,
                                            const std::array<float, 4>& mouse, const VulkanOrbitCamera& camera)
{
    // chainInputExtent = this shader's own scaled extent, per the per-pass
    // render-target extent contract (each pass independently resolves its
    // own scaled extent from its own resolution-scale directive) -- the
    // engine-defined virtual input for pass 0's own "source"-typed scale
    // directive, since no resolved scene exists on
    // this in-scene background path (VulkanGraphics::getOrCreateShaderInstance()'s own
    // doc comment).
    const vk::Extent2D chainInputExtent {
        VulkanShaderInstance::computeScaledExtent (context.getSwapchainExtent(), resolutionScale) };

    auto* execution { context.getOrCreateShaderInstance (shader, resolutionScale, chainInputExtent) };

    if (execution != nullptr)
    {
        const VulkanShaderUniforms baseUniforms { context.recordShaderBufferPasses (
            shader, *execution, true, VulkanShaderUniforms::noScene, resolutionScale, mouse) };

        recordShaderImagePassDrawCommands (*execution, baseUniforms, opacity, camera);
    }
    else
    {
#if JUCE_DEBUG
        jam::debug::Log::write ("VulkanLowLevelGraphicsContext::renderShader: "
                                "getOrCreateShaderInstance failed (contentHash "
                                + juce::String (shader.contentHash)
                                + ") -- skipping this call's draw");
#endif
    }
}

/*____________________________________________________________________________*/
// recordShaderImagePassDrawCommands — the Image pass's own draw, followed by
// the background combine draw that composites its result into the scene.
//
// Gather stage: the Image pass's own dedicated pipeline (built once, eagerly,
// by VulkanShaderInstance::build(), VulkanShaderInstance::buildRenderResources()) draws
// its raw, unmodified colour (no opacity math — see
// jam_vulkan/shader/shadertoy_wrapper.frag for VulkanShaderFormat::shadertoy,
// or the pass's own already-complete main() compiled as-is for
// VulkanShaderFormat::slang, per the slang zero-injection contract (real .slang
// sources declare their own layout(set,binding) resources, which the
// fixed-set-0 bindless macro wrapper would collide with — compiled as-is,
// bindings resolved by SPIR-V reflection downstream)) into
// execution.getImagePassGatherTarget()'s own offscreen framebuffer, against
// that same target's own stored gatherTarget.renderPass (resolved at
// VulkanShaderInstance::build() time via VulkanGraphics::getOrCreateShaderOffscreenRenderPass
// (vk::Format), always this VulkanGraphics's own colorFormat entry for the gather
// target — resolved independently per pass, per the per-pass
// render-target format contract).
//
// Combine stage: the scene render pass — active on entry to this function
// (recordShaderBufferPasses()'s resumeAfter=true already resumed it in
// renderShader() above) — is explicitly suspended (context.endRenderPass())
// before, and resumed (context.resumeRenderPass()) after, the gather draw's
// own raw begin/endRenderPass pair below: Vulkan does not allow a render pass
// instance to nest inside another on the same command buffer, so the scene
// pass cannot stay active while the offscreen gather pass is. It binds
// context.getOrCreateBackgroundCombinePipeline() (background_combine.frag,
// static engine GLSL, built once and shared across every VulkanShaderInstance),
// samples the gather target through channels[0] (stamped directly with its
// own bindless index — mirrors VulkanGraphics::recordStraightAlphaPass()'s own
// direct-stamp technique, never through VulkanShaderInstance::stampChannels(),
// which is reserved for buffer-pass/scene-fallback semantics), and applies
// the exact component-transparency formula the Image pass's compiled shader
// used to compute itself (jam_VulkanShaderUniforms.h's opacity doc comment)
// via VulkanPipelines::alphaBlendAttachment() — compositing over whatever the
// scene render pass already painted beneath it.
//
// Viewport AND scissor for the combine draw are both set to the current clip
// bounds (device space, computeScissor()'s exact rect) rather than the full
// swapchain extent: the shared vertex stage (shaders/shader_pass.vert) emits
// a fullscreen triangle whose clip-space position spans NDC [-1, 1]
// unconditionally, with a 0-1 uv varying that is an affine function of that
// same NDC position — interpolation of an affine function of position equals
// that function evaluated at the interpolated position, so this holds true
// at every rasterised fragment regardless of viewport size. Shrinking the
// dynamic viewport to the clip rect therefore places that same NDC range
// exactly onto the clip rect on screen, and the interpolated uv still spans
// exactly [0, 1] across it — reproducing "quad at clip bounds, full 0-1 uv,
// honouring the active clip" with zero extra geometry, zero per-draw
// transform, and no MVP push (same technique the pre-gather/combine
// background draw used). No explicit viewport/scissor restore follows: every
// other draw method in this class sets its own viewport/scissor fresh before
// its own draw, so this call's narrowed dynamic state never leaks into a
// later command.
//
// Pipeline layout/bindless descriptor set are the SAME objects
// VulkanGraphics::recordShaderBufferPasses() binds for the buffer passes (set 0 =
// bindless texture array, reused verbatim) — no duplicate binding logic, no
// per-execution descriptor set of its own.
//
// baseUniforms is reused verbatim for iTime/iTimeDelta/iFrame/iResolution/iMouse
// (VulkanGraphics::recordShaderBufferPasses()'s documented SSOT contract — one stamp
// per render() call, shared by every pass it records); only channels[] (this
// pass's readable buffer outputs) and opacity (the caller's own blend opacity,
// not the buffer passes' fixed 1.0) vary here. The gather draw's own
// viewport/scissor/render-area extent is read directly from
// execution.getImagePassGatherTarget().extent (per the per-pass
// render-target extent contract — always this shader's own scaledExtent, VulkanShaderInstance::
// build()'s own doc comment), the same extent imagePassGatherTarget's
// images/framebuffer were built at — never re-derived from baseUniforms.
// iResolution.
void VulkanLowLevelGraphicsContext::recordShaderImagePassDrawCommands (
    VulkanShaderInstance& execution, const VulkanShaderUniforms& baseUniforms, float opacity, const VulkanOrbitCamera& camera)
{
    vk::CommandBuffer cmd { context.getCommandBuffer() };
    vk::PipelineLayout layout { context.getShaderRegistry().getOrCreateShaderInstanceLayout() };
    vk::DescriptorSet bindlessSet { context.getBindlessTextureDescriptorSet() };

    // gatherTarget's own extent (per the per-pass render-target extent contract) — always
    // this shader's own scaledExtent (VulkanShaderInstance::build()'s own doc
    // comment: the Image pass's gather target never resizes per-preset),
    // read directly here rather than re-derived from baseUniforms.iResolution.
    VulkanRenderResources& gatherTarget { execution.getImagePassGatherTarget() };

    // No resolved scene exists on this in-scene background path — every
    // channels[] slot beyond this shader's own buffer-pass count falls back
    // to VulkanShaderUniforms::noScene (SSOT via VulkanShaderInstance::stampChannels()).
    VulkanShaderUniforms imageUniforms { baseUniforms };
    execution.stampChannels (imageUniforms, VulkanShaderUniforms::noScene, context.getNativeHandle());
    imageUniforms.opacity = opacity;

    // Suspend the scene render pass, active on entry to this function (see
    // this function's own doc comment above) — same
    // endRenderPass()/resumeRenderPass() bracket convention
    // uploadDirtyAtlasSlots() uses around its own transfer commands
    // (jam_VulkanLowLevelGraphicsContextGlyph.cpp), required here because a
    // render pass instance cannot nest inside another on the same command
    // buffer. The bracketed raw beginRenderPass/endRenderPass pair below
    // targets the shared single-sample offscreen render pass — same
    // convention VulkanGraphics::recordSingleBufferPass() uses for its own
    // offscreen write.
    context.endRenderPass();

    // Write target: the other gather half (its own front half is a
    // self-feedback .slang shader's own read source this call,
    // VulkanGraphics::getSlangTextureBindings()'s own doc comment) — same
    // ping-pong toggle arithmetic as recordSingleBufferPass()'s own
    // writeHalf (degenerates to always 0 for a non-feedback, single-image
    // gather target, VulkanRenderResources' own doc comment).
    const int writeHalf { (gatherTarget.currentReadHalf + 1) % gatherTarget.images.size() };
    vk::Image writeImage { gatherTarget.images.at (writeHalf).getImage() };

    // Discard-transition — this draw fully overwrites the whole write-half
    // image every call (no prior content to preserve), but the wait side
    // still fences the previous frame's fragment-shader read of this same
    // image (WAR hazard, only reachable when a self-feedback .slang shader
    // actually samples it back) — same barrier shape
    // VulkanGraphics::recordSingleBufferPass() records around its own ping-pong
    // write (jam_VulkanGraphicsSlangPass.cpp).
    recordImageMemoryBarrier (cmd, writeImage,
                              vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal,
                              vk::ImageAspectFlagBits::eColor,
                              vk::AccessFlagBits::eShaderRead, vk::AccessFlagBits::eColorAttachmentWrite,
                              vk::PipelineStageFlagBits::eFragmentShader, vk::PipelineStageFlagBits::eColorAttachmentOutput);

    // clearValueCount must cover the depth attachment (index 1, always
    // loadOp=eClear — VulkanGraphics::getOrCreateShaderOffscreenRenderPass
    // (vk::Format)'s own doc comment) even though this target's own colour
    // attachment (index 0) stays loadOp=eDontCare — the mesh's own
    // material-range/feature-edge draws (appended below, when
    // execution.hasMesh()) test/write this SAME depth attachment; a
    // meshless shader leaves it inert scratch space.
    std::array<vk::ClearValue, 2> clearValues {};
    clearValues.at (1).depthStencil = vk::ClearDepthStencilValue { 1.0f, 0 };

    vk::RenderPassBeginInfo rpInfo {};
    rpInfo.renderPass = gatherTarget.renderPass;
    rpInfo.framebuffer = gatherTarget.framebuffers.at (writeHalf);
    rpInfo.renderArea.offset = vk::Offset2D { 0, 0 };
    rpInfo.renderArea.extent = gatherTarget.extent;
    rpInfo.clearValueCount = static_cast<uint32_t> (clearValues.size());
    rpInfo.pClearValues = clearValues.data();
    cmd.beginRenderPass (&rpInfo, vk::SubpassContents::eInline);

    // VulkanShaderFormat::slang: the mandatory Image pass is just as
    // author-shaped as any buffer pass — same reflected descriptor
    // set/pipeline layout substitution as VulkanGraphics::recordSingleBufferPass()/
    // recordPostProcessCompositeDrawCommands(), see their own doc comments
    // (the slang zero-injection contract: real .slang sources declare
    // their own layout(set,binding) resources, which the fixed-set-0
    // bindless macro wrapper would collide with — compiled as-is,
    // bindings resolved by SPIR-V reflection downstream). isBackground = true
    // unconditionally (this in-scene background path has no resolved scene
    // to offer "Source"/"Original", matching this function's own
    // VulkanShaderUniforms::noScene channels[] fallback just above).
    if (execution.usesSlangPipeline())
    {
        const int imagePassIndex { execution.getImagePassIndex() };
        const VulkanShaderTextureBindings textureBindings { context.getSlangTextureBindings (
            execution.getSlangPassTextures (imagePassIndex), execution, true, imagePassIndex) };

        const juce::MemoryBlock pushConstantBytes { execution.refreshSlangPass (
            imagePassIndex, gatherTarget.extent, execution.getBuiltExtent(),
            static_cast<uint32_t> (imageUniforms.iFrame), textureBindings,
            context.getShaderRegistry().getOrCreateSlangPassSampler (execution.getSlangPassSettings (imagePassIndex))) };

        if (const vk::DescriptorSet slangSet { execution.getSlangPassDescriptorSet (imagePassIndex) };
            slangSet != nullptr)
        {
            cmd.bindDescriptorSets (vk::PipelineBindPoint::eGraphics,
                                    execution.getSlangPassPipelineLayout (imagePassIndex), 0, slangSet, nullptr);
        }

        if (not pushConstantBytes.isEmpty())
            cmd.pushConstants (execution.getSlangPassPipelineLayout (imagePassIndex),
                               vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
                               0, static_cast<uint32_t> (pushConstantBytes.getSize()), pushConstantBytes.getData());
    }
    else
    {
        cmd.bindDescriptorSets (vk::PipelineBindPoint::eGraphics, layout, 0, bindlessSet, nullptr);

        cmd.pushConstants (layout, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
                           0, sizeof (imageUniforms), &imageUniforms);
    }

    cmd.bindPipeline (vk::PipelineBindPoint::eGraphics, gatherTarget.pipeline);

    // setFullViewport() — same offscreen-full-extent viewport/scissor
    // convention VulkanGraphics::recordSingleBufferPass()/
    // recordPostProcessCompositeDrawCommands() use for their own
    // VulkanRenderResources targets (jam_VulkanLowLevelGraphicsContext.cpp).
    setFullViewport (cmd, gatherTarget.extent);
    const vk::Rect2D scissor { { 0, 0 }, gatherTarget.extent };
    cmd.setScissor (0, scissor);

    // Slang: the mandatory Image pass's own pipeline was built against
    // context.getSlangQuadVertexBuffer()'s own vertex input state
    // (VulkanShaderInstance::createFullscreenPipeline()) — bind it and draw its 4
    // vertices as a triangle strip. Shadertoy: shader_pass.vert generates
    // its own 3 vertices purely from gl_VertexIndex, needing no vertex
    // buffer at all — today's exact, unchanged draw.
    if (execution.usesSlangPipeline())
    {
        const vk::Buffer quadBuffers[1] { context.getShaderRegistry().getSlangQuadVertexBuffer() };
        const vk::DeviceSize quadOffsets[1] { 0 };
        cmd.bindVertexBuffers (0, quadBuffers, quadOffsets);
        cmd.draw (VulkanShaderInstance::slangQuadVertexCount, 1, 0, 0);
    }
    else
    {
        cmd.draw (VulkanPipelines::fullscreenTriangleVertexCount, 1, 0, 0);
    }

    // VulkanMesh-backed draws — recorded directly INTO
    // this SAME render-pass instance, immediately after the ordinary
    // Image-pass fullscreen draw above (which now serves as the mesh's own
    // backdrop, unchanged, its own normal pipeline — no clone), gated on
    // execution.hasMesh() (this shader's own VulkanShader::meshPath parsed/
    // uploaded successfully, VulkanShaderInstance::build()'s own doc comment) — a
    // no-op for every meshless shader. imageUniforms is threaded through
    // verbatim — VulkanGraphics::recordMeshGatherDrawCommands()'s own
    // refreshMeshUniforms() call copies these SAME iMouse/iResolution/iTime/
    // iTimeDelta/iFrame values into this execution's own mesh UBO, the
    // standard-uniform delivery mechanism a mesh_shader= mainMesh snippet
    // reads bare (mesh_shader= is a vertex-ANIMATION HOOK into the
    // engine's own default mesh look, zero bespoke uniform vocabulary). See
    // VulkanGraphics::recordMeshGatherDrawCommands()'s own doc comment for the
    // exact hooked/engine-default draw dispatch.
    if (execution.hasMesh())
        context.getShaderRegistry().recordMeshGatherDrawCommands (execution, camera.getViewMatrix(),
                                                                  camera.getProjectionMatrix (static_cast<float> (gatherTarget.extent.width)
                                                                      / static_cast<float> (gatherTarget.extent.height)),
                                                                  camera.getNormalMatrix(), imageUniforms, cmd, context.getSwapchainFormat());

    cmd.endRenderPass();

    // Same-layout memory-visibility barrier — the offscreen render pass's
    // finalLayout is already eShaderReadOnlyOptimal, but its implicit
    // vk::SubpassExternal dependency alone does not guarantee cross-stage
    // read visibility, exactly matching recordSingleBufferPass()'s own
    // post-pass barrier shape. depthImage needs no equivalent barrier —
    // never sampled outside its own render pass's own depth test
    // (VulkanRenderResources::depthImage's own doc comment).
    recordImageMemoryBarrier (cmd, writeImage,
                              vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageLayout::eShaderReadOnlyOptimal,
                              vk::ImageAspectFlagBits::eColor,
                              vk::AccessFlagBits::eColorAttachmentWrite, vk::AccessFlagBits::eShaderRead,
                              vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eFragmentShader);

    gatherTarget.currentReadHalf = writeHalf;
    const int32_t combineChannelZeroBindlessIndex { gatherTarget.bindlessIndex.at (writeHalf) };

    // Resume the scene render pass suspended above (LOAD_OP_LOAD, preserving
    // everything already painted onto it) — the combine draw below
    // composites onto the scene, so the scene pass must be active again
    // before any of its commands are recorded. Pairs with the
    // context.endRenderPass() call above this gather draw's own
    // beginRenderPass.
    context.resumeRenderPass();

    // Background combine draw — samples the just-written gather target back
    // through channels[0], stamped with its own bindless index (this pass's
    // own contract, distinct from imageUniforms' channels[] above — the
    // gather draw's channels[] served the USER shader's own buffer-pass
    // reads; this combine shader is engine-authored and always reads its
    // single input at channels[0]). bindlessSet is rebound on layout
    // unconditionally, mirroring recordPostProcessCompositeDrawCommands()'s
    // own combine draw (jam_VulkanGraphicsSlangPass.cpp): the slang/mesh
    // gather stage above binds its own descriptor set with its own pipeline
    // layout, which under Vulkan pipeline-layout compatibility rules
    // disturbs set 0 for this shared layout. The shadertoy gather's
    // else-branch binding would still be intact at this point, but this
    // rebind stays unconditional — one rule for every format/mesh
    // combination, not a per-branch special case.
    VulkanShaderUniforms combineUniforms { imageUniforms };
    combineUniforms.channels[0] = combineChannelZeroBindlessIndex;

    cmd.bindDescriptorSets (vk::PipelineBindPoint::eGraphics, layout, 0, bindlessSet, nullptr);

    cmd.bindPipeline (vk::PipelineBindPoint::eGraphics,
                      context.getShaderRegistry().getOrCreateBackgroundCombinePipeline (context.getRenderPass(), context.getActiveSampleCount()));

    cmd.pushConstants (layout, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
                       0, sizeof (combineUniforms), &combineUniforms);

    const vk::Rect2D combineScissor { computeScissor() };

    vk::Viewport combineViewport {};
    combineViewport.x        = static_cast<float> (combineScissor.offset.x);
    combineViewport.y        = static_cast<float> (combineScissor.offset.y);
    combineViewport.width    = static_cast<float> (combineScissor.extent.width);
    combineViewport.height   = static_cast<float> (combineScissor.extent.height);
    combineViewport.minDepth = 0.0f;
    combineViewport.maxDepth = 1.0f;
    cmd.setViewport (0, combineViewport);
    cmd.setScissor (0, combineScissor);

    cmd.draw (VulkanPipelines::fullscreenTriangleVertexCount, 1, 0, 0);
}

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam