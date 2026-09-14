//
// VulkanShaderRegistry file: the runtime-shader-compiler lane's own GPU
// execution cache and shared setup helpers — getOrCreateShaderInstance()'s
// own cache surface (getShaderInstance()/hasShaderInstance()/
// registerShaderInstance()/releaseRetiredInstances(), consumed by
// VulkanGraphics::getOrCreateShaderInstance(), which stays in VulkanGraphics —
// it touches renderPassActive/endRenderPass()/resumeRenderPass()/
// writeBindlessTextureDescriptor(), Graphics frame fabric this registry never
// back-references), the shared VulkanShader-execution setup helpers (pipeline
// layout, offscreen render pass, shared vertex module, straight-alpha/
// background/post-process combine pipelines), the slang quad vertex buffer
// and per-pass sampler cache, and recordMipChainGeneration() (shared by
// buildExternalTexture() below in jam_VulkanShaderRegistryMesh.cpp and by
// VulkanGraphics::recordSingleBufferPass(), which stays in VulkanGraphics).
// jam_VulkanShaderRegistryMesh.cpp is the sibling file for the
// mesh-backed material-range draw and external LUT/texture upload.

namespace jam
{
/*____________________________________________________________________________*/
//==============================================================================
// Constructor / Destructor
//==============================================================================

VulkanShaderRegistry::VulkanShaderRegistry (VulkanDevice& vulkanDevice, VulkanPipelines& vulkanPipelines)
    : device (vulkanDevice), pipelines (vulkanPipelines) {}

VulkanShaderRegistry::~VulkanShaderRegistry()
{
    for (auto& [key, sampler] : slangPassSamplers)
        device.getDevice().destroySampler (sampler, nullptr);

    device.getDevice().destroyPipeline (straightAlphaPipeline, nullptr);
    device.getDevice().destroyPipeline (backgroundCombinePipeline, nullptr);
    device.getDevice().destroyPipeline (postProcessCombinePipeline, nullptr);
    device.getDevice().destroyShaderModule (shaderPassVertModule, nullptr);

    for (auto& [format, renderPassEntry] : shaderOffscreenRenderPasses)
        device.getDevice().destroyRenderPass (renderPassEntry, nullptr);

    device.getDevice().destroyPipelineLayout (shaderInstanceLayout, nullptr);

    // VulkanMesh-backed material-range draw —
    // every one of the following is empty/null for a VulkanShaderRegistry that
    // never executed a mesh-carrying VulkanShader (getOrCreateMeshXxx() lazily-built,
    // never called at all otherwise), every destroy call below a documented
    // no-op against an empty handle, mirroring shaderInstanceLayout's own
    // identical unconditional-destroy convention immediately above. No
    // dedicated mesh render pass exists anymore — the mesh's own
    // material-range/feature-edge draw renders directly into
    // execution.getImagePassGatherTarget(), built against
    // shaderOffscreenRenderPasses above.
    device.getDevice().destroyPipeline (meshPipeline, nullptr);
    device.getDevice().destroyPipeline (meshTransparentFillPipeline, nullptr);
    device.getDevice().destroyPipeline (meshWireframePipeline, nullptr);
    device.getDevice().destroyPipelineLayout (meshPipelineLayout, nullptr);
    device.getDevice().destroyDescriptorSetLayout (meshDescriptorSetLayout, nullptr);

    // meshDefaultVertexSpirv/meshEdgeVertexSpirv are plain juce::MemoryBlock —
    // no Vulkan handle to destroy, RAII self-destructs. The mesh_shader=
    // hooked pipelines built from them (jam::VulkanShaderInstance::
    // meshHookFillPipeline et al.) are destroyed by that VulkanShaderInstance's own
    // destructor instead, never here.
    //
    // shaderInstances/previousShaderInstances: every VulkanShaderInstance instance
    // owns its own per-VulkanShader handles and self-destructs via RAII —
    // no manual cleanup needed here. slangQuadVertexBuffer is a plain
    // VulkanBuffer — RAII, self-destructs.
}

//==============================================================================
// VulkanShader execution — shared setup (lazy, built once per VulkanShaderRegistry lifetime)
//==============================================================================

vk::PipelineLayout VulkanShaderRegistry::getOrCreateShaderInstanceLayout()
{
    if (shaderInstanceLayout == nullptr)
    {
        // Set 0: the bindless texture array, reused verbatim (same layout
        // object as the main 21-pipeline layout's set 1) — buffer-pass and
        // Image-pass fragment stages sample channels[] through this same
        // array, and select linear (binding 1) or nearest (binding 2)
        // filtering directly in their own GLSL, resolved compile-time by
        // jam::VulkanShaderCompiler (baked into the compiled GLSL — no filter field
        // on VulkanShader itself) — see jam_VulkanShaderUniforms.h's documented
        // GLSL contract. No per-execution filter descriptor set.
        vk::DescriptorSetLayout setLayout { pipelines.getLayoutSet1() };

        const vk::PushConstantRange pushRange { vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
                                                0, sizeof (VulkanShaderUniforms) };

        const vk::PipelineLayoutCreateInfo layoutInfo { {}, setLayout, pushRange };

        const vk::Result createLayoutResult { device.getDevice().createPipelineLayout (
            &layoutInfo, nullptr, &shaderInstanceLayout) };
        jassert (createLayoutResult == vk::Result::eSuccess);
        juce::ignoreUnused (createLayoutResult);
    }

    return shaderInstanceLayout;
}

vk::RenderPass VulkanShaderRegistry::getOrCreateShaderOffscreenRenderPass (vk::Format colorFormat)
{
    const int formatKey { static_cast<int> (colorFormat) };

    if (not shaderOffscreenRenderPasses.contains (formatKey))
    {
        // Single-sample colour attachment — every buffer pass/Image-pass
        // gather draw fully overwrites its whole target every draw, so
        // loadOp is DONT_CARE; finalLayout is the layout the very next
        // channel read (a later buffer pass this same call, the Image pass,
        // or a mesh draw's own backdrop sample) needs.
        vk::AttachmentDescription colorAttachment {};
        colorAttachment.format = colorFormat;
        colorAttachment.samples = vk::SampleCountFlagBits::e1;
        colorAttachment.loadOp = vk::AttachmentLoadOp::eDontCare;
        colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
        colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
        colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
        colorAttachment.finalLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

        // Unconditional depth attachment — every target this render pass
        // shape backs (buffer pass ping-pong pairs, the mandatory VulkanImage
        // pass's own gather target) carries one now, even for a meshless
        // VulkanShader (VulkanRenderResources::depthImage's own doc comment): the
        // mandatory Image pass's own gather target is where a mesh, when
        // present, draws its own depth-tested material-range/feature-edge
        // draws directly AFTER the ordinary fullscreen backdrop draw, in the
        // SAME render-pass instance — one render-pass shape for every
        // offscreen shader target, mesh-carrying or not. Never sampled
        // outside its own render pass's own depth test (finalLayout stays
        // eDepthStencilAttachmentOptimal, storeOp eDontCare — this pass's
        // own depth content is irrelevant the instant the pass ends).
        vk::AttachmentDescription depthAttachment {};
        depthAttachment.format = VulkanShaderInstance::offscreenDepthFormat;
        depthAttachment.samples = vk::SampleCountFlagBits::e1;
        depthAttachment.loadOp = vk::AttachmentLoadOp::eClear;
        depthAttachment.storeOp = vk::AttachmentStoreOp::eDontCare;
        depthAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
        depthAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        depthAttachment.initialLayout = vk::ImageLayout::eUndefined;
        depthAttachment.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

        const std::array<vk::AttachmentDescription, 2> attachments { colorAttachment, depthAttachment };

        const vk::AttachmentReference colorRef { 0, vk::ImageLayout::eColorAttachmentOptimal };
        const vk::AttachmentReference depthRef { 1, vk::ImageLayout::eDepthStencilAttachmentOptimal };

        vk::SubpassDescription subpass {};
        subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorRef;
        subpass.pDepthStencilAttachment = &depthRef;

        const vk::RenderPassCreateInfo rpInfo { {}, attachments, subpass };

        vk::RenderPass createdRenderPass {};
        const vk::Result createRenderPassResult { device.getDevice().createRenderPass (
            &rpInfo, nullptr, &createdRenderPass) };
        jassert (createRenderPassResult == vk::Result::eSuccess);
        juce::ignoreUnused (createRenderPassResult);

        shaderOffscreenRenderPasses.emplace (formatKey, createdRenderPass);
    }

    return shaderOffscreenRenderPasses.at (formatKey);
}

vk::ShaderModule VulkanShaderRegistry::getOrCreateShaderPassVertModule()
{
    if (shaderPassVertModule == nullptr)
    {
        const BinaryData::Raw shaderPassVert { files::shaderPassVert };
        shaderPassVertModule = VulkanPipelines::createShaderModule (
            device.getDevice(), shaderPassVert.data, shaderPassVert.size);
    }

    return shaderPassVertModule;
}

VulkanShaderRegistry::FullscreenPipelineFixedFunctionState VulkanShaderRegistry::createFullscreenPipelineFixedFunctionState (
    vk::SampleCountFlagBits sampleCount, const vk::PipelineColorBlendAttachmentState& blendAttachment) const
{
    FullscreenPipelineFixedFunctionState state {};

    state.inputAssemblyInfo.topology = vk::PrimitiveTopology::eTriangleList;

    state.viewportInfo.viewportCount = 1;
    state.viewportInfo.scissorCount = 1;

    state.rasterizerInfo.polygonMode = vk::PolygonMode::eFill;
    state.rasterizerInfo.cullMode = vk::CullModeFlagBits::eNone;
    state.rasterizerInfo.frontFace = vk::FrontFace::eCounterClockwise;
    state.rasterizerInfo.lineWidth = 1.0f;

    state.multisampleInfo.rasterizationSamples = sampleCount;

    // Every fullscreen combine/un-premultiply pass — straightAlphaPipeline,
    // backgroundCombinePipeline, postProcessCombinePipeline alike — never
    // touches depth or stencil, same rationale as VulkanShaderInstance::
    // createFullscreenPipeline()'s own VulkanPipelines::noStencilState() usage.
    state.depthStencilInfo = VulkanPipelines::noStencilState();

    state.blendAttachment = blendAttachment;
    state.dynamicStates = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };

    return state;
}

vk::Pipeline VulkanShaderRegistry::createFullscreenPipeline (vk::ShaderModule fragModule, vk::SampleCountFlagBits sampleCount,
                                                              const vk::PipelineColorBlendAttachmentState& blendAttachment,
                                                              vk::RenderPass renderPass)
{
    // Same fullscreen-triangle pipeline shape as VulkanShaderInstance::
    // createFullscreenPipeline() (zero vertex input, triangle list, dynamic
    // viewport/scissor, VulkanPipelines::noStencilState()) — the shared vert
    // module/pipeline layout every VulkanShader-execution pipeline already shares.
    const std::array<vk::PipelineShaderStageCreateInfo, 2> stages {
        vk::PipelineShaderStageCreateInfo { {}, vk::ShaderStageFlagBits::eVertex, getOrCreateShaderPassVertModule(), "main" },
        vk::PipelineShaderStageCreateInfo { {}, vk::ShaderStageFlagBits::eFragment, fragModule, "main" }
    };

    const FullscreenPipelineFixedFunctionState fixedState {
        createFullscreenPipelineFixedFunctionState (sampleCount, blendAttachment) };
    const vk::PipelineColorBlendStateCreateInfo colorBlendInfo { {}, false, vk::LogicOp::eClear, 1, &fixedState.blendAttachment };
    const vk::PipelineDynamicStateCreateInfo dynamicStateInfo { {}, fixedState.dynamicStates };

    vk::GraphicsPipelineCreateInfo pipelineInfo {};
    pipelineInfo.stageCount = static_cast<uint32_t> (stages.size());
    pipelineInfo.pStages = stages.data();
    pipelineInfo.pVertexInputState = &fixedState.vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &fixedState.inputAssemblyInfo;
    pipelineInfo.pViewportState = &fixedState.viewportInfo;
    pipelineInfo.pRasterizationState = &fixedState.rasterizerInfo;
    pipelineInfo.pMultisampleState = &fixedState.multisampleInfo;
    pipelineInfo.pDepthStencilState = &fixedState.depthStencilInfo;
    pipelineInfo.pColorBlendState = &colorBlendInfo;
    pipelineInfo.pDynamicState = &dynamicStateInfo;
    pipelineInfo.layout = getOrCreateShaderInstanceLayout();
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;

    vk::Pipeline pipeline {};
    const vk::Result createPipelineResult { device.getDevice().createGraphicsPipelines (
        nullptr, 1, &pipelineInfo, nullptr, &pipeline) };
    jassert (createPipelineResult == vk::Result::eSuccess);
    juce::ignoreUnused (createPipelineResult);

    return pipeline;
}

vk::Pipeline VulkanShaderRegistry::getOrCreateStraightAlphaPipeline (vk::Format colorFormat)
{
    if (straightAlphaPipeline == nullptr)
    {
        const BinaryData::Raw straightAlphaFrag { files::straightAlphaFrag };
        vk::ShaderModule fragModule { VulkanPipelines::createShaderModule (
            device.getDevice(), straightAlphaFrag.data, straightAlphaFrag.size) };

        if (fragModule != nullptr)
        {
            // straightAlphaFramebuffer's own colour attachment matches
            // shaderOffscreenRenderPasses' own single-sample contract — full
            // overwrite every call, no blend needed.
            straightAlphaPipeline = createFullscreenPipeline (fragModule, vk::SampleCountFlagBits::e1,
                VulkanPipelines::opaqueBlendAttachment(), getOrCreateShaderOffscreenRenderPass (colorFormat));

            device.getDevice().destroyShaderModule (fragModule, nullptr);
        }
    }

    // Loud in debug when fragModule creation failed above (outer positive
    // check leaves straightAlphaPipeline unset) — release keeps the same
    // nullptr tolerance every other lazily-built pipeline in this file ships with.
    jassert (straightAlphaPipeline != nullptr);
    return straightAlphaPipeline;
}

vk::Pipeline VulkanShaderRegistry::getOrCreateBackgroundCombinePipeline (vk::RenderPass renderPass, vk::SampleCountFlagBits activeSampleCount)
{
    if (backgroundCombinePipeline == nullptr)
    {
        const BinaryData::Raw backgroundCombineFrag { files::backgroundCombineFrag };
        vk::ShaderModule fragModule { VulkanPipelines::createShaderModule (
            device.getDevice(), backgroundCombineFrag.data, backgroundCombineFrag.size) };

        if (fragModule != nullptr)
        {
            // Targets the caller's own SCENE render pass (MSAA at
            // activeSampleCount, unlike every other VulkanShader-execution
            // pipeline's single-sample offscreen targets) with
            // VulkanPipelines::alphaBlendAttachment() — must composite over the
            // scene content already painted beneath it (jam_VulkanShaderUniforms.h's
            // opacity doc comment). Static engine GLSL, never per-instance-derived —
            // built once, shared across every VulkanShaderInstance/VulkanShader.
            backgroundCombinePipeline = createFullscreenPipeline (fragModule, activeSampleCount,
                VulkanPipelines::alphaBlendAttachment(), renderPass);

            device.getDevice().destroyShaderModule (fragModule, nullptr);
        }
    }

    // Loud in debug when fragModule creation failed above (outer positive
    // check leaves backgroundCombinePipeline unset) — release keeps the same
    // nullptr tolerance every other lazily-built pipeline in this file ships with.
    jassert (backgroundCombinePipeline != nullptr);
    return backgroundCombinePipeline;
}

vk::Pipeline VulkanShaderRegistry::getOrCreatePostProcessCombinePipeline (vk::RenderPass compositeRenderPass)
{
    if (postProcessCombinePipeline == nullptr)
    {
        const BinaryData::Raw postProcessCombineFrag { files::postProcessCombineFrag };
        vk::ShaderModule fragModule { VulkanPipelines::createShaderModule (
            device.getDevice(), postProcessCombineFrag.data, postProcessCombineFrag.size) };

        if (fragModule != nullptr)
        {
            // Single-sample — compositeRenderPass's sole attachment IS the
            // swapchain image. VulkanPipelines::opaqueBlendAttachment() — this
            // combine shader's own mix() already does the blending math, so
            // the fixed-function stage stays a direct overwrite. Static engine
            // GLSL, never per-instance-derived — built once, shared across
            // every VulkanShaderInstance/VulkanShader.
            postProcessCombinePipeline = createFullscreenPipeline (fragModule, vk::SampleCountFlagBits::e1,
                VulkanPipelines::opaqueBlendAttachment(), compositeRenderPass);

            device.getDevice().destroyShaderModule (fragModule, nullptr);
        }
    }

    // Loud in debug when fragModule creation failed above (outer positive
    // check leaves postProcessCombinePipeline unset) — release keeps the same
    // nullptr tolerance every other lazily-built pipeline in this file ships with.
    jassert (postProcessCombinePipeline != nullptr);
    return postProcessCombinePipeline;
}

void VulkanShaderRegistry::recordMipChainGeneration (vk::CommandBuffer commandBuffer, vk::Image image,
                                                     vk::Extent2D extent, uint32_t numMipLevels)
{
    int32_t sourceWidth { static_cast<int32_t> (extent.width) };
    int32_t sourceHeight { static_cast<int32_t> (extent.height) };

    for (uint32_t level = 1; level < numMipLevels; ++level)
    {
        // minimumPixelExtent (jam_VulkanShaderInstance.cpp) -- one shared
        // "never below 1x1" constant instead of a second same-value constant
        // local to this function.
        const int32_t destWidth { juce::jmax (static_cast<int32_t> (minimumPixelExtent), sourceWidth / 2) };
        const int32_t destHeight { juce::jmax (static_cast<int32_t> (minimumPixelExtent), sourceHeight / 2) };

        // Source level (level - 1): eShaderReadOnlyOptimal -> eTransferSrcOptimal.
        // This function's own loop invariant: level 0 was already left at
        // eShaderReadOnlyOptimal by the caller's own post-draw barrier
        // (recordSingleBufferPass(), for level == 1); every level > 0 was
        // left at eShaderReadOnlyOptimal by THIS SAME function's own prior
        // iteration (below), for level > 1.
        recordImageMemoryBarrier (commandBuffer, image,
                                  vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageLayout::eTransferSrcOptimal,
                                  vk::ImageAspectFlagBits::eColor,
                                  vk::AccessFlagBits::eShaderRead, vk::AccessFlagBits::eTransferRead,
                                  vk::PipelineStageFlagBits::eFragmentShader, vk::PipelineStageFlagBits::eTransfer,
                                  level - 1, 1);

        // Destination level: discard-transition (this level's own PRIOR
        // FRAME content is irrelevant — every level is fully rewritten every
        // time this function runs), but the wait side still fences that
        // prior frame's own fragment-shader read of this same level (WAR
        // hazard, same shape as recordSingleBufferPass()'s own ping-pong
        // write barrier).
        recordImageMemoryBarrier (commandBuffer, image,
                                  vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal,
                                  vk::ImageAspectFlagBits::eColor,
                                  vk::AccessFlagBits::eShaderRead, vk::AccessFlagBits::eTransferWrite,
                                  vk::PipelineStageFlagBits::eFragmentShader, vk::PipelineStageFlagBits::eTransfer,
                                  level, 1);

        const vk::ImageSubresourceLayers sourceLayers { vk::ImageAspectFlagBits::eColor, level - 1, 0, 1 };
        const vk::ImageSubresourceLayers destLayers { vk::ImageAspectFlagBits::eColor, level, 0, 1 };

        const std::array<vk::Offset3D, 2> sourceOffsets {
            vk::Offset3D { 0, 0, 0 }, vk::Offset3D { sourceWidth, sourceHeight, 1 } };
        const std::array<vk::Offset3D, 2> destOffsets {
            vk::Offset3D { 0, 0, 0 }, vk::Offset3D { destWidth, destHeight, 1 } };

        const vk::ImageBlit blitRegion { sourceLayers, sourceOffsets, destLayers, destOffsets };

        commandBuffer.blitImage (image, vk::ImageLayout::eTransferSrcOptimal,
                                 image, vk::ImageLayout::eTransferDstOptimal,
                                 blitRegion, vk::Filter::eLinear);

        // Both levels touched this iteration end at eShaderReadOnlyOptimal —
        // level (level - 1) because this loop never reads it again, level
        // (level) because it becomes the NEXT iteration's own source (the
        // loop invariant above), satisfying this function's own "every level
        // ends readable" contract regardless of numMipLevels.
        recordImageMemoryBarrier (commandBuffer, image,
                                  vk::ImageLayout::eTransferSrcOptimal, vk::ImageLayout::eShaderReadOnlyOptimal,
                                  vk::ImageAspectFlagBits::eColor,
                                  vk::AccessFlagBits::eTransferRead, vk::AccessFlagBits::eShaderRead,
                                  vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader,
                                  level - 1, 1);

        recordImageMemoryBarrier (commandBuffer, image,
                                  vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal,
                                  vk::ImageAspectFlagBits::eColor,
                                  vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eShaderRead,
                                  vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader,
                                  level, 1);

        sourceWidth = destWidth;
        sourceHeight = destHeight;
    }
}

//==============================================================================
// VulkanShader instance cache
//==============================================================================

VulkanShaderInstance& VulkanShaderRegistry::getShaderInstance (const VulkanShader& shader)
{
    return *shaderInstances.at (&shader);
}

bool VulkanShaderRegistry::hasShaderInstance (const VulkanShader& shader) const noexcept
{
    return shaderInstances.contains (&shader);
}

void VulkanShaderRegistry::registerShaderInstance (const VulkanShader* shader, std::unique_ptr<VulkanShaderInstance> instance)
{
    // Previous content hash's resources moved to previousShaderInstances —
    // deferred destroy, never destroyed mid-frame (mirrors
    // VulkanStagingArena::previousStagingBuffers/VulkanFrameBuffer::previousBuffers's
    // exact pattern, drained by releaseRetiredInstances() after the fence wait).
    if (shaderInstances.contains (shader))
        previousShaderInstances.add (std::move (shaderInstances.at (shader)));

    shaderInstances[shader] = std::move (instance);
}

void VulkanShaderRegistry::releaseRetiredInstances (VulkanBindlessRegistry& bindlessRegistry, uint64_t frameCounter,
                                                    int swapchainImageCount, void* nativeHandle)
{
    // Release every previousShaderInstances entry's buffer-pass ping-pong
    // bindless slots before the instances themselves are destroyed by clear()
    // below — this same post-fence-wait point is previousShaderInstances' own
    // proven-safe drain site, called from the caller's own resetResources()
    // after its fence wait has returned.
    for (auto& previousShaderInstance : previousShaderInstances)
    {
        for (int passIndex = 0; passIndex < previousShaderInstance->getBufferPassCount(); ++passIndex)
            for (const int pingPongBindlessIndex :
                 previousShaderInstance->getBufferPassTarget (passIndex).bindlessIndex)
                bindlessRegistry.releaseBindlessIndex (pingPongBindlessIndex);

        // The Image pass's own gather target slot (VulkanGraphics::getOrCreateShaderInstance()'s
        // symmetric assignment counterpart) — released the same way, same
        // post-fence-wait safe point.
        for (const int gatherBindlessIndex : previousShaderInstance->getImagePassGatherTarget().bindlessIndex)
            bindlessRegistry.releaseBindlessIndex (gatherBindlessIndex);

        // Every external LUT/texture's own THIS-window bindless slot
        // (VulkanGraphics::getOrCreateShaderInstance()'s own buildExternalTexture()
        // orchestration) — released the same way, same post-fence-wait safe
        // point; this window's own registered slot is also cleared on the
        // texture's own per-window registry (VulkanBindlessTexture::
        // clearBindlessIndex()) — the texture itself is about to be
        // destroyed by previousShaderInstances.clear() below regardless, but
        // this keeps every registerBindlessIndex() call paired with a
        // clearBindlessIndex() the same way VulkanBindlessInstance's own
        // destructor already pairs the glyph atlas's identical registry.
        for (auto& [textureName, lutTexture] : previousShaderInstance->getExternalTextures())
        {
            const int lutIndex { lutTexture.getBindlessIndex (nativeHandle) };

            if (lutIndex >= 0)
                bindlessRegistry.releaseBindlessIndex (lutIndex);

            lutTexture.clearBindlessIndex (nativeHandle);
        }
    }

    previousShaderInstances.clear();

    releaseMeshStagingBuffers();
    retireOrphanedInstances (frameCounter, swapchainImageCount);
}

void VulkanShaderRegistry::releaseMeshStagingBuffers()
{
    // VulkanMesh-backed gather target — releases every LIVE shaderInstances
    // entry's own mesh staging VulkanBuffer once its build()-time upload command
    // buffer is proven complete (this exact post-fence-wait point) — reuses
    // the existing post-fence release seam to wire staging release in. A no-op
    // for every entry that never built a mesh, or already released one —
    // releaseMeshStagingIfPending()'s own doc comment. Deliberately sweeps
    // shaderInstances (this registry's own LIVE per-VulkanShader cache), not
    // previousShaderInstances above — a freshly-built mesh execution is the
    // CURRENT entry, never moved to previousShaderInstances until ITS OWN
    // content later goes stale.
    for (auto& [shaderKey, liveInstance] : shaderInstances)
        liveInstance->releaseMeshStagingIfPending();
}

void VulkanShaderRegistry::retireOrphanedInstances (uint64_t frameCounter, int swapchainImageCount)
{
    // Orphan sweep — an owner (VulkanEngine post-process, Background) that has
    // replaced its unique_ptr<VulkanShader> never calls VulkanGraphics::
    // getOrCreateShaderInstance()/getShaderInstance() again for the destroyed
    // VulkanShader's now-dangling key, so its GPU execution resources would
    // otherwise leak for this registry's lifetime: the map entry is
    // unreachable, but nothing ever removes it. Any live entry not re-stamped
    // within the swapchain's own image count worth of frames is moved into
    // previousShaderInstances here exactly like a stale-generation rebuild
    // (VulkanGraphics::getOrCreateShaderInstance()) — erased from the live
    // map, and destroyed by this same clear() call one frame from now (no
    // maxFramesInFlight constant exists in this engine; @p swapchainImageCount
    // is the frames-in-flight bound already available).
    const uint64_t shaderInstanceStaleFrameBound { static_cast<uint64_t> (std::max (swapchainImageCount, 1)) };

    // Partition by move — one structured-binding pass moves every stale
    // entry's VulkanShaderInstance into previousShaderInstances (deferred destroy)
    // and every live entry into a fresh map that then replaces the old one
    // wholesale. No iterator/erase surgery, no intermediate key collection —
    // the sweep IS a partition, expressed as one.
    jam::HashMap<const VulkanShader*, std::unique_ptr<VulkanShaderInstance>> liveInstances;

    for (auto& [shaderKey, instance] : shaderInstances)
    {
        if (frameCounter - instance->getLastUsedFrame() >= shaderInstanceStaleFrameBound)
            previousShaderInstances.add (std::move (instance));
        else
            liveInstances.emplace (shaderKey, std::move (instance));
    }

    shaderInstances = std::move (liveInstances);
}

//==============================================================================
// Slang quad vertex buffer / per-pass sampler cache
//==============================================================================

bool VulkanShaderRegistry::createSlangVertexBuffer()
{
    // Slang quad vertex buffer — a single, VulkanShaderRegistry-owned, build-once
    // vertex buffer shared by every slang-format VulkanShaderPass in every
    // VulkanShader this registry ever executes (RetroArch's fixed
    // vertex-input contract) — mirrors projectionBuffer's own
    // persistent-mapped shape (VMA_MEMORY_USAGE_CPU_ONLY +
    // VMA_ALLOCATION_CREATE_MAPPED_BIT): content is written once, right
    // here, and never rewritten afterward — no per-frame refresh exists
    // anywhere for this buffer.
    const vk::DeviceSize slangQuadVertexBufferSize {
        sizeof (VulkanShaderInstance::SlangQuadVertex) * VulkanShaderInstance::slangQuadVertexCount };

    const vk::BufferCreateInfo slangQuadBufferInfo { {}, slangQuadVertexBufferSize,
                                                     vk::BufferUsageFlagBits::eVertexBuffer, vk::SharingMode::eExclusive };

    VmaAllocationCreateInfo slangQuadAllocInfo {};
    slangQuadAllocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
    slangQuadAllocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

    slangQuadVertexBuffer = VulkanBuffer (device.getAllocator(), slangQuadBufferInfo, slangQuadAllocInfo);
    if (not slangQuadVertexBuffer.isValid())
        return false;

    std::memcpy (slangQuadVertexBuffer.getMapped(), VulkanShaderInstance::slangQuadVertices.data(),
                static_cast<size_t> (slangQuadVertexBufferSize));

    return true;
}

// jam::VulkanShaderFormat's own wrap-mode vocabulary size (clampToBorder/clampToEdge/repeat/
// mirroredRepeat, jam_vulkan/bimap/jam_VulkanShaderFormat.h) — named once here, never a repeated magic "4" at either
// reference below.
static constexpr int slangPassWrapModeCount { 4 };

// RetroArch's wrap_modeN vocabulary (jam_vulkan/bimap/jam_VulkanShaderFormat.h's own VulkanShaderFormat::wrapModes doc
// comment), resolved to its vk::SamplerAddressMode counterpart — direct-indexed by
// VulkanShaderFormat's own wrap-mode ordinal, mirrors jam_VulkanShaderInstance.cpp's
// scaleAxisRuleLut / jam_VulkanShaderCompiler.cpp's optimizationLevelLut dispatch-table
// idiom for this exact ordinal family.
static constexpr jam::LookupTable<int, vk::SamplerAddressMode, slangPassWrapModeCount> slangPassWrapModeLut {
    {
                                          { VulkanShaderFormat::clampToBorder, vk::SamplerAddressMode::eClampToBorder },
                                          { VulkanShaderFormat::clampToEdge, vk::SamplerAddressMode::eClampToEdge },
                                          { VulkanShaderFormat::repeat, vk::SamplerAddressMode::eRepeat },
                                          { VulkanShaderFormat::mirroredRepeat, vk::SamplerAddressMode::eMirroredRepeat },
                                          }
};

// slangPassSamplers' own composite key — (filter_linearN resolved bool) x (wrap_modeN ordinal) —
// never a raw `(filterLinear ? 1 : 0) * slangPassWrapModeCount + wrapModeOrdinal` expression
// repeated at a call site.
static int slangPassSamplerKey (bool filterLinear, int wrapMode) noexcept
{
    return (filterLinear ? 1 : 0) * slangPassWrapModeCount + wrapMode;
}

vk::Sampler VulkanShaderRegistry::getOrCreateSlangPassSampler (const VulkanShaderPreset::Pass& settings)
{
    // filterLinearN is always concrete for a well-formed VulkanShaderFormat::slang compile
    // (VulkanShaderCompiler::compile()'s own compile-time UNSPEC-falls-back-to-global-filter
    // resolution loop, jam_VulkanShaderCompiler.cpp) — the jassert below is the one
    // boundary where this invariant is established; every downstream read trusts it
    // unconditionally, never re-doubted with a fallback value.
    jassert (settings.filterLinear.has_value());
    const bool filterLinear { *settings.filterLinear };

    const int key { slangPassSamplerKey (filterLinear, settings.wrapMode) };

    if (slangPassSamplers.contains (key))
        return slangPassSamplers.at (key);

    const vk::Filter filter { filterLinear ? vk::Filter::eLinear : vk::Filter::eNearest };
    const vk::SamplerMipmapMode mipmapMode { filterLinear ? vk::SamplerMipmapMode::eLinear
                                                          : vk::SamplerMipmapMode::eNearest };
    const vk::SamplerAddressMode addressMode { slangPassWrapModeLut[settings.wrapMode] };

    // clampToBorder pairs with vk::BorderColor::eFloatTransparentBlack — RetroArch's own
    // documented RARCH_WRAP_BORDER sampling result (border reads transparent, verified this
    // session); immaterial for every other wrap mode, whose addressing never samples the border
    // color at all.
    //
    // maxLod = vk::LodClampNone (was 0.0f) — this ONE sampler is shared
    // across every slang pass this registry ever executes (slangPassSamplers'
    // own cache, keyed only by filter/wrap, never by pass), including a pass
    // whose own texture binding now resolves to a real mip chain
    // (VulkanRenderResources::numMipLevels > 1, mipmap_inputN sweep) — clamping
    // maxLod to 0.0f would silently force every sample from such a chain back
    // to level 0 regardless of the GLSL's own textureLod()/implicit-LOD
    // sampling, defeating the whole chain. vk::LodClampNone (VK_LOD_CLAMP_NONE)
    // spans any mip count a bound view actually carries — harmless no-op for
    // every single-mip target this sampler ALSO serves (mipLodBias stays
    // 0.0f, minLod stays 0.0f either way).
    vk::Sampler sampler {};
    const vk::SamplerCreateInfo samplerInfo { {},
                                              filter,
                                              filter,
                                              mipmapMode,
                                              addressMode,
                                              addressMode,
                                              addressMode,
                                              0.0f,
                                              vk::False,
                                              0.0f,
                                              vk::False,
                                              vk::CompareOp::eNever,
                                              0.0f,
                                              vk::LodClampNone,
                                              vk::BorderColor::eFloatTransparentBlack };

    const vk::Result createSamplerResult { device.getDevice().createSampler (&samplerInfo, nullptr, &sampler) };
    jassert (createSamplerResult == vk::Result::eSuccess or createSamplerResult == vk::Result::eErrorDeviceLost);

    if (createSamplerResult == vk::Result::eSuccess)
    {
        slangPassSamplers.emplace (key, sampler);
    }
#if JUCE_DEBUG
    else
    {
        jam::debug::Log::write ("jam::VulkanShaderRegistry::getOrCreateSlangPassSampler: unhandled createSampler result "
                                + juce::String (vk::to_string (createSamplerResult)));
    }
#endif

    return sampler;
}

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
