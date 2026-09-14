namespace jam
{
/*____________________________________________________________________________*/
// =============================================================================
// load() — coordinator
// =============================================================================

bool VulkanPipelines::load (vk::Device device,
                       vk::RenderPass renderPass,
                       vk::Extent2D extent,
                       uint32_t bindlessTextureCapacity,
                       vk::SampleCountFlagBits sampleCount,
                       vk::PipelineCache pipelineCache)
{
    cache = pipelineCache;

    return createDescriptorSetLayouts (device, bindlessTextureCapacity)
        and createComputeStorageLayout (device)
        and createPipelineLayout (device)
        and createComputePipelineLayout (device)
        and loadShaderModules (device)
        and createGraphicsPipelines (device, renderPass, extent, sampleCount)
        and createComputePipelines (device);
}

// =============================================================================
// createDescriptorSetLayouts
// =============================================================================

bool VulkanPipelines::createDescriptorSetLayouts (vk::Device device, uint32_t bindlessTextureCapacity)
{
    // Projection layout: set 3, binding 0, storage buffer, vertex stage
    const vk::DescriptorSetLayoutBinding projectionBinding { 0, vk::DescriptorType::eStorageBuffer, 1,
                                                      vk::ShaderStageFlagBits::eVertex, nullptr };

    const vk::DescriptorSetLayoutCreateInfo projectionLayoutInfo { {}, projectionBinding };
    if (device.createDescriptorSetLayout (&projectionLayoutInfo, nullptr, &projectionLayout) != vk::Result::eSuccess)
        return false;

    // Sampler layout: set 1 — the bindless texture array. Binding 0: `texture2D[]`
    // sampled-image array (runtime-sized in the shader via GL_EXT_nonuniform_qualifier,
    // so the shader's SPIR-V is decoupled from bindlessTextureCapacity's concrete value —
    // sized only by this vk::DescriptorSetLayoutBinding). Binding 1: single shared linear
    // sampler (VulkanGraphics's one linearSampler covers every image; a sampler array would be
    // YAGNI). Binding 2: single shared nearest sampler (VulkanGraphics's one nearestSampler) —
    // a VulkanShader-execution pass's compile-time-resolved resample mode (map::ImageResample::value,
    // resolved by jam::VulkanShaderCompiler, never stored on VulkanShader itself) samples
    // through binding 1 or binding 2 directly in its own GLSL; this layout is reused
    // verbatim as set 0 of VulkanGraphics::getOrCreateShaderInstanceLayout() (see
    // jam_VulkanShaderUniforms.h's documented GLSL contract), so no per-execution
    // descriptor set of its own is ever allocated for resample mode selection. Bindings
    // 3/4: `sampler2D[]` combined-image-sampler arrays exposing the SAME image views as
    // binding 0, pre-paired with the linear (binding 3) / nearest (binding 4) sampler
    // respectively — a first-class passable sampler2D value user-shader GLSL can hand to
    // its own helper functions (glslang rejects the binding-0 `texture2D` + binding-1/2
    // `sampler` constructor form as a function argument — "sampler constructor must
    // appear at point of use" — the Shadertoy helper-function paste-compat hole this
    // pair closes; see jam::VulkanShaderCompiler::channelMacros()/sceneMacro()).
    const std::array<vk::DescriptorSetLayoutBinding, 5> samplerBindings
    { {
        { 0, vk::DescriptorType::eSampledImage,         bindlessTextureCapacity, vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eCompute, nullptr },
        { 1, vk::DescriptorType::eSampler,              1,                       vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eCompute, nullptr },
        { 2, vk::DescriptorType::eSampler,              1,                       vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eCompute, nullptr },
        { 3, vk::DescriptorType::eCombinedImageSampler, bindlessTextureCapacity, vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eCompute, nullptr },
        { 4, vk::DescriptorType::eCombinedImageSampler, bindlessTextureCapacity, vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eCompute, nullptr },
    } };

    // The queried Vulkan 1.2 descriptor-indexing capabilities are hard requirements
    // for this engine (asserted, enabled unconditionally at device creation) — these flags
    // are always set, no runtime branch. Bindings 1/2 (the two shared samplers, each
    // written once at init) never need update-after-bind semantics; binding 0's and
    // bindings 3/4's per-texture array slots are all rewritten together, once per texture,
    // by VulkanGraphics::writeBindlessTextureDescriptor() — all three carry the same flags as
    // binding 0.
    const std::array<vk::DescriptorBindingFlags, 5> samplerBindingFlags
    {
        vk::DescriptorBindingFlagBits::eUpdateAfterBind | vk::DescriptorBindingFlagBits::ePartiallyBound,
        {},
        {},
        vk::DescriptorBindingFlagBits::eUpdateAfterBind | vk::DescriptorBindingFlagBits::ePartiallyBound,
        vk::DescriptorBindingFlagBits::eUpdateAfterBind | vk::DescriptorBindingFlagBits::ePartiallyBound,
    };

    const vk::DescriptorSetLayoutBindingFlagsCreateInfo samplerBindingFlagsInfo { samplerBindingFlags };

    const vk::DescriptorSetLayoutCreateInfo samplerLayoutInfo { vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool,
                                                                samplerBindings, &samplerBindingFlagsInfo };
    if (device.createDescriptorSetLayout (&samplerLayoutInfo, nullptr, &samplerLayout) != vk::Result::eSuccess)
        return false;

    // Storage layout: set 2, binding 0, readonly VulkanPrimitiveRecord storage buffer, vertex stage
    const vk::DescriptorSetLayoutBinding storageBinding { 0, vk::DescriptorType::eStorageBuffer, 1,
                                                          vk::ShaderStageFlagBits::eVertex, nullptr };

    const vk::DescriptorSetLayoutCreateInfo storageLayoutInfo { {}, storageBinding };
    if (device.createDescriptorSetLayout (&storageLayoutInfo, nullptr, &storageLayout) != vk::Result::eSuccess)
        return false;

    // Empty layout: zero bindings, occupies set 0 in the shared pipeline layout
    const vk::DescriptorSetLayoutCreateInfo emptyLayoutInfo {};
    return device.createDescriptorSetLayout (&emptyLayoutInfo, nullptr, &emptyLayout) == vk::Result::eSuccess;
}

// =============================================================================
// createComputeStorageLayout
// =============================================================================

bool VulkanPipelines::createComputeStorageLayout (vk::Device device)
{
    const std::array<vk::DescriptorSetLayoutBinding, 2> computeStorageBindings
    { {
        { 0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr },
        { 1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr },
    } };

    const vk::DescriptorSetLayoutCreateInfo computeStorageLayoutInfo { {}, computeStorageBindings };
    return device.createDescriptorSetLayout (&computeStorageLayoutInfo, nullptr, &computeStorageLayout) == vk::Result::eSuccess;
}

// =============================================================================
// createPipelineLayout
// =============================================================================

// sharedPushConstantRangeSize — sized to the largest push-constant variant across
// all pipelines: VulkanImagePushConstants (color[4]=16 + clip[4]=16 + textureScale[2]=8
// = 40 bytes, defined in jam_VulkanGraphics.h). Change 1 (opacity fold) removed
// the intervening `float opacity` field — its removal also fixes a latent
// offset mismatch: with `opacity` present, GLSL std430 padded `clip` to offset 32
// and `textureScale` to offset 48 (verified via spirv-dis), 4 bytes beyond the
// prior 44-byte range; with `opacity` gone, `clip` lands at offset 16 and
// `textureScale` at offset 32 on both the C++ and GLSL sides, byte-identical.
// The rect/path/glyph/background push-constant variants — a bare VulkanColour for
// rect/path draws (see jam_VulkanLowLevelGraphicsContext.cpp's
// recordRectFillDrawCommands()) up through the 40-byte VulkanImagePushConstants —
// all fit within this window. Vulkan offers no per-stage variable push
// constant ranges without VK_KHR_push_descriptor, so one range covers all
// pipeline variants.
static constexpr uint32_t sharedPushConstantRangeSize { 40 };

bool VulkanPipelines::createPipelineLayout (vk::Device device)
{
    // Set order fixed by position: 0 empty, 1 samplerLayout, 2 storageLayout, 3 projectionLayout — see getLayout().
    const std::array<vk::DescriptorSetLayout, 4> setLayouts { emptyLayout, samplerLayout, storageLayout, projectionLayout };

    const vk::PushConstantRange pushRange { vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
                                            0, sharedPushConstantRangeSize };

    const vk::PipelineLayoutCreateInfo layoutInfo { {}, setLayouts, pushRange };
    return device.createPipelineLayout (&layoutInfo, nullptr, &layout) == vk::Result::eSuccess;
}

// =============================================================================
// createComputePipelineLayout
// =============================================================================

bool VulkanPipelines::createComputePipelineLayout (vk::Device device)
{
    // Set order fixed by position: 0 empty, 1 samplerLayout, 2 computeStorageLayout, 3 projectionLayout — see getComputeLayout().
    const std::array<vk::DescriptorSetLayout, 4> computeSetLayouts { emptyLayout, samplerLayout, computeStorageLayout, projectionLayout };

    const vk::PushConstantRange computePushRange { vk::ShaderStageFlagBits::eCompute,
                                                    0, sizeof (VulkanMatteFeatherPushConstants) };

    const vk::PipelineLayoutCreateInfo computeLayoutInfo { {}, computeSetLayouts, computePushRange };
    return device.createPipelineLayout (&computeLayoutInfo, nullptr, &computeLayout) == vk::Result::eSuccess;
}

// =============================================================================
// loadShaderModules
// =============================================================================

bool VulkanPipelines::loadShaderModules (vk::Device device)
{
    const std::array shaderNames
    {
        files::fillRectVert,
        files::fillRectFrag,
        files::imageFrag,
        files::tiledImageFrag,
        files::glyphMonoFrag,
        files::glyphEmojiFrag,
        files::backgroundFrag,
        files::imageAlphaMaskFrag,
        files::instancedVert,
        files::instancedRectVert,
        files::gradientFillFrag,
        files::instancedMaskedVert,
        files::maskedImageFrag,
        files::stackBlurTextureComp,
        files::stackBlurBufferComp,
        files::matteChokeComp,
        files::matteFeatherComp
    };

    for (const auto& name : shaderNames)
    {
        const BinaryData::Raw resource { name };

        if (resource.data != nullptr and resource.size > 0)
        {
            shaderModules.addOrReplace (name,
                createShaderModule (device, resource.data, resource.size));
        }
    }

    for (const auto& [name, module] : shaderModules)
    {
        if (module == nullptr)
            return false;
    }

    return shaderModules.size() == shaderNames.size();
}

// =============================================================================
// createGraphicsPipelines
// =============================================================================

bool VulkanPipelines::createGraphicsPipelines (vk::Device device,
                                          vk::RenderPass renderPass,
                                          vk::Extent2D extent,
                                          vk::SampleCountFlagBits sampleCount)
{
    // 24-way variation lives as data in pipelineSpecs — this orchestrator only
    // builds the shared/per-axis state once, then resolves each row into a
    // vk::GraphicsPipelineCreateInfo via buildPipelineCreateInfo().
    PipelineBuildState build {};
    initializePipelineBuildState (build, renderPass, extent, sampleCount);

    std::array<vk::GraphicsPipelineCreateInfo, pipelineCount> createInfos {};
    for (uint8_t i = 0; i < pipelineCount; ++i)
        createInfos.at (i) = buildPipelineCreateInfo (pipelineSpecs[i], build);

    // Result overload (never-null VulkanEngine factory contract) — batch call preserved
    // as-is; eErrorPipelineCompileRequired's opt-in-only semantics are unaffected by
    // this vocabulary conversion (this call site never opts in, so the driver can
    // only ever return eSuccess or a hard error here).
    return device.createGraphicsPipelines (cache, pipelineCount, createInfos.data(), nullptr, pipelines) == vk::Result::eSuccess;
}

// =============================================================================
// createComputePipelines
// =============================================================================

bool VulkanPipelines::createComputePipelines (vk::Device device)
{
    const auto stackBlurTextureModule { getShaderModule (files::stackBlurTextureComp) };
    const auto stackBlurBufferModule  { getShaderModule (files::stackBlurBufferComp) };
    const auto matteChokeModule       { getShaderModule (files::matteChokeComp) };
    const auto matteFeatherModule     { getShaderModule (files::matteFeatherComp) };

    const vk::PipelineShaderStageCreateInfo textureStage { {}, vk::ShaderStageFlagBits::eCompute, stackBlurTextureModule, "main" };
    const vk::PipelineShaderStageCreateInfo bufferStage  { {}, vk::ShaderStageFlagBits::eCompute, stackBlurBufferModule,  "main" };
    const vk::PipelineShaderStageCreateInfo chokeStage   { {}, vk::ShaderStageFlagBits::eCompute, matteChokeModule,       "main" };
    const vk::PipelineShaderStageCreateInfo featherStage { {}, vk::ShaderStageFlagBits::eCompute, matteFeatherModule,     "main" };

    std::array<vk::ComputePipelineCreateInfo, computePipelineCount> createInfos {};
    createInfos.at (static_cast<uint8_t> (ComputeID::stackBlurTexture)) = vk::ComputePipelineCreateInfo { {}, textureStage,  computeLayout };
    createInfos.at (static_cast<uint8_t> (ComputeID::stackBlurBuffer))  = vk::ComputePipelineCreateInfo { {}, bufferStage,   computeLayout };
    createInfos.at (static_cast<uint8_t> (ComputeID::matteChoke))       = vk::ComputePipelineCreateInfo { {}, chokeStage,    computeLayout };
    createInfos.at (static_cast<uint8_t> (ComputeID::matteFeather))     = vk::ComputePipelineCreateInfo { {}, featherStage,  computeLayout };

    return device.createComputePipelines (cache, computePipelineCount, createInfos.data(), nullptr, computePipelines) == vk::Result::eSuccess;
}

// =============================================================================
// initializePipelineBuildState — shared/per-axis state, built once per load()
// =============================================================================

void VulkanPipelines::initializePipelineBuildState (PipelineBuildState& build,
                                               vk::RenderPass renderPass,
                                               vk::Extent2D extent,
                                               vk::SampleCountFlagBits sampleCount) const
{
    build.renderPass = renderPass;
    initializeFixedFunctionState (build, extent, sampleCount);
    initializeVertexAndTopologyState (build);
    initializeDepthStencilAndBlendState (build);
    initializeDynamicState (build);
    initializeShaderStages (build);
}

void VulkanPipelines::initializeFixedFunctionState (PipelineBuildState& build, vk::Extent2D extent,
                                              vk::SampleCountFlagBits sampleCount) const
{
    build.viewport = viewportState (extent);
    build.rasterizer = rasterizationState();
    build.multisample = multisampleState (sampleCount, false);
    // ID::clipMaskInstanced alone selects this variant — see multisampleState()'s
    // doc comment for why alpha-to-coverage is needed there and nowhere else.
    build.multisampleAlphaToCoverage = multisampleState (sampleCount, true);
}

void VulkanPipelines::initializeVertexAndTopologyState (PipelineBuildState& build) const
{
    build.vertexInputByMode[static_cast<uint8_t> (VertexInput::position2D)] = position2DVertexInputState();
    build.vertexInputByMode[static_cast<uint8_t> (VertexInput::instanced)]  = instancedVertexInputState();
    build.inputAssemblyStrip = triangleStripInputAssemblyState();
    build.inputAssemblyList  = triangleListInputAssemblyState();
}

void VulkanPipelines::initializeDepthStencilAndBlendState (PipelineBuildState& build) const
{
    build.depthStencilByMode[static_cast<uint8_t> (DepthStencil::noStencil)]    = noStencilState();
    build.depthStencilByMode[static_cast<uint8_t> (DepthStencil::stencilWrite)] = stencilWriteState();
    build.depthStencilByMode[static_cast<uint8_t> (DepthStencil::stencilTest)]  = stencilTestState();
    build.depthStencilByMode[static_cast<uint8_t> (DepthStencil::windingAccumulate)] = windingAccumulateState();
    build.depthStencilByMode[static_cast<uint8_t> (DepthStencil::windingCover)]      = windingCoverState();

    // Blend attachments are named members (not temporaries) — colorBlendState()
    // stores a pointer to its argument, which must outlive the returned
    // create-info; binding it to a PipelineBuildState member (which lives for
    // the whole createGraphicsPipelines() call) keeps that pointer valid.
    build.opaqueAttachment = opaqueBlendAttachment();
    build.alphaAttachment  = alphaBlendAttachment();
    build.noColorAttachment = vk::PipelineColorBlendAttachmentState {};
    build.noColorAttachment.blendEnable    = false;
    build.noColorAttachment.colorWriteMask = {};
    build.colorBlendByMode[static_cast<uint8_t> (Blend::opaque)] = colorBlendState (build.opaqueAttachment);
    build.colorBlendByMode[static_cast<uint8_t> (Blend::alpha)]  = colorBlendState (build.alphaAttachment);
    build.colorBlendByMode[static_cast<uint8_t> (Blend::none)]   = colorBlendState (build.noColorAttachment);
}

void VulkanPipelines::initializeDynamicState (PipelineBuildState& build) const
{
    // Dynamic viewport+scissor+stencil — set per command buffer at draw time.
    build.dynamicStates.at (static_cast<size_t> (DynamicState::viewport))           = vk::DynamicState::eViewport;
    build.dynamicStates.at (static_cast<size_t> (DynamicState::scissor))            = vk::DynamicState::eScissor;
    build.dynamicStates.at (static_cast<size_t> (DynamicState::stencilCompareMask)) = vk::DynamicState::eStencilCompareMask;  // enum 6, core 1.0
    build.dynamicStates.at (static_cast<size_t> (DynamicState::stencilWriteMask))   = vk::DynamicState::eStencilWriteMask;    // enum 7, core 1.0
    build.dynamicStates.at (static_cast<size_t> (DynamicState::stencilReference))   = vk::DynamicState::eStencilReference;    // enum 8, core 1.0
    build.dynamicState = vk::PipelineDynamicStateCreateInfo { {}, build.dynamicStates };
}

void VulkanPipelines::initializeShaderStages (PipelineBuildState& build) const
{
    // Per-pipeline shader stages. Field order: flags, stage, module, pName, pSpecializationInfo.
    //
    // Every instanced stage pair below shares files::instancedVert for its
    // vertex stage — fragment stages are UNCHANGED — except stagesInstancedRect
    // and stagesBackground, which use files::instancedRectVert (no vTextureIndex
    // output; fill_rect.frag and background.frag never consume it).
    // files::fillRectVert is used by stagesTriList below instead.
    const auto instancedVert        { getShaderModule (files::instancedVert) };
    const auto instancedRectVert    { getShaderModule (files::instancedRectVert) };
    const auto instancedMaskedVert  { getShaderModule (files::instancedMaskedVert) };
    const auto fillRectVert         { getShaderModule (files::fillRectVert) };
    const auto fillRectFrag         { getShaderModule (files::fillRectFrag) };
    const auto imageFrag            { getShaderModule (files::imageFrag) };
    const auto maskedImageFrag      { getShaderModule (files::maskedImageFrag) };
    const auto tiledImageFrag       { getShaderModule (files::tiledImageFrag) };
    const auto glyphMonoFrag        { getShaderModule (files::glyphMonoFrag) };
    const auto glyphEmojiFrag       { getShaderModule (files::glyphEmojiFrag) };
    const auto backgroundFrag       { getShaderModule (files::backgroundFrag) };
    const auto imageAlphaMaskFrag   { getShaderModule (files::imageAlphaMaskFrag) };
    const auto gradientFillFrag     { getShaderModule (files::gradientFillFrag) };

    build.stagesInstancedRect[0]      = vk::PipelineShaderStageCreateInfo { {}, vk::ShaderStageFlagBits::eVertex,   instancedRectVert, "main" };
    build.stagesInstancedRect[1]      = vk::PipelineShaderStageCreateInfo { {}, vk::ShaderStageFlagBits::eFragment, fillRectFrag, "main" };
    build.stagesInstancedImage[0]      = vk::PipelineShaderStageCreateInfo { {}, vk::ShaderStageFlagBits::eVertex,   instancedVert, "main" };
    build.stagesInstancedImage[1]      = vk::PipelineShaderStageCreateInfo { {}, vk::ShaderStageFlagBits::eFragment, imageFrag, "main" };
    build.stagesTiled[0]               = vk::PipelineShaderStageCreateInfo { {}, vk::ShaderStageFlagBits::eVertex,   instancedVert, "main" };
    build.stagesTiled[1]               = vk::PipelineShaderStageCreateInfo { {}, vk::ShaderStageFlagBits::eFragment, tiledImageFrag, "main" };
    build.stagesInstancedGlyphMono[0]  = vk::PipelineShaderStageCreateInfo { {}, vk::ShaderStageFlagBits::eVertex,   instancedVert, "main" };
    build.stagesInstancedGlyphMono[1]  = vk::PipelineShaderStageCreateInfo { {}, vk::ShaderStageFlagBits::eFragment, glyphMonoFrag, "main" };
    build.stagesInstancedGlyphEmoji[0] = vk::PipelineShaderStageCreateInfo { {}, vk::ShaderStageFlagBits::eVertex,   instancedVert, "main" };
    build.stagesInstancedGlyphEmoji[1] = vk::PipelineShaderStageCreateInfo { {}, vk::ShaderStageFlagBits::eFragment, glyphEmojiFrag, "main" };
    build.stagesBackground[0]          = vk::PipelineShaderStageCreateInfo { {}, vk::ShaderStageFlagBits::eVertex,   instancedRectVert, "main" };
    build.stagesBackground[1]          = vk::PipelineShaderStageCreateInfo { {}, vk::ShaderStageFlagBits::eFragment, backgroundFrag, "main" };
    build.stagesInstancedClipMask[0]   = vk::PipelineShaderStageCreateInfo { {}, vk::ShaderStageFlagBits::eVertex,   instancedVert, "main" };
    build.stagesInstancedClipMask[1]   = vk::PipelineShaderStageCreateInfo { {}, vk::ShaderStageFlagBits::eFragment, imageAlphaMaskFrag, "main" };

    // Triangulated path draws (opaqueTriList/stencilWriteTriList/opaqueTriListStencil/
    // alphaBlendTriList/alphaBlendTriListStencil) keep the UNCHANGED position2D vertex
    // shader (files::fillRectVert) + UNCHANGED fragment shader
    // (files::fillRectFrag). windingStencilAccumulate
    // reuses this exact stage pair too (fragment output discarded,
    // colorWriteMask=0 via Blend::none), and fill_rect_alpha.frag's deletion (byte-identical
    // to fill_rect.frag) means alphaBlendTriList/alphaBlendTriListStencil now resolve to
    // this same pair as well — Blend::alpha is already carried on the PipelineSpec row,
    // not the StagePair. One stage pair now serves every triangulated path draw.
    build.stagesTriList[0] = vk::PipelineShaderStageCreateInfo { {}, vk::ShaderStageFlagBits::eVertex,   fillRectVert, "main" };
    build.stagesTriList[1] = vk::PipelineShaderStageCreateInfo { {}, vk::ShaderStageFlagBits::eFragment, fillRectFrag, "main" };

    // instanced_rect.vert (same quad shape as stagesInstancedRect) + shaders/gradient_fill.frag.
    build.stagesGradientFill[0] = vk::PipelineShaderStageCreateInfo { {}, vk::ShaderStageFlagBits::eVertex,   instancedRectVert, "main" };
    build.stagesGradientFill[1] = vk::PipelineShaderStageCreateInfo { {}, vk::ShaderStageFlagBits::eFragment, gradientFillFrag, "main" };

    build.stagesMaskedImage[0] = vk::PipelineShaderStageCreateInfo { {}, vk::ShaderStageFlagBits::eVertex,   instancedMaskedVert, "main" };
    build.stagesMaskedImage[1] = vk::PipelineShaderStageCreateInfo { {}, vk::ShaderStageFlagBits::eFragment, maskedImageFrag, "main" };

    build.stagesByPair[static_cast<uint8_t> (StagePair::instancedRect)]       = build.stagesInstancedRect;
    build.stagesByPair[static_cast<uint8_t> (StagePair::instancedImage)]      = build.stagesInstancedImage;
    build.stagesByPair[static_cast<uint8_t> (StagePair::tiled)]               = build.stagesTiled;
    build.stagesByPair[static_cast<uint8_t> (StagePair::instancedGlyphMono)]  = build.stagesInstancedGlyphMono;
    build.stagesByPair[static_cast<uint8_t> (StagePair::instancedGlyphEmoji)] = build.stagesInstancedGlyphEmoji;
    build.stagesByPair[static_cast<uint8_t> (StagePair::background)]         = build.stagesBackground;
    build.stagesByPair[static_cast<uint8_t> (StagePair::instancedClipMask)]  = build.stagesInstancedClipMask;
    build.stagesByPair[static_cast<uint8_t> (StagePair::triList)]            = build.stagesTriList;
    build.stagesByPair[static_cast<uint8_t> (StagePair::gradientFill)]       = build.stagesGradientFill;
    build.stagesByPair[static_cast<uint8_t> (StagePair::maskedImage)]        = build.stagesMaskedImage;
}

// =============================================================================
// buildPipelineCreateInfo — one PipelineSpec row -> one vk::GraphicsPipelineCreateInfo
// =============================================================================

vk::GraphicsPipelineCreateInfo VulkanPipelines::buildPipelineCreateInfo (
    const PipelineSpec& spec,
    const PipelineBuildState& build) const
{
    vk::GraphicsPipelineCreateInfo info {};
    info.layout = layout;
    info.renderPass = build.renderPass;
    info.subpass = 0;
    info.basePipelineIndex = -1;
    info.stageCount = 2;
    info.pStages = build.stagesByPair[static_cast<uint8_t> (spec.stagePair)];
    info.pVertexInputState = &build.vertexInputByMode[static_cast<uint8_t> (spec.vertexInput)];
    // vk::PipelineDynamicStateCreateInfo is not a pNext extension — it has its
    // own dedicated field on vk::GraphicsPipelineCreateInfo.
    info.pDynamicState = &build.dynamicState;
    info.pInputAssemblyState = spec.topology == vk::PrimitiveTopology::eTriangleStrip
                                 ? &build.inputAssemblyStrip
                                 : &build.inputAssemblyList;
    info.pViewportState = &build.viewport;
    info.pRasterizationState = &build.rasterizer;
    info.pMultisampleState = spec.alphaToCoverage
                                ? &build.multisampleAlphaToCoverage
                                : &build.multisample;
    info.pDepthStencilState = &build.depthStencilByMode[static_cast<uint8_t> (spec.depthStencil)];
    info.pColorBlendState = &build.colorBlendByMode[static_cast<uint8_t> (spec.blend)];
    return info;
}

// =============================================================================
// shutdown() — reverse order destruction
// =============================================================================

void VulkanPipelines::shutdown (vk::Device device)
{
    // 1. Destroy pipelines. The pipeline cache itself is not owned here —
    // VulkanEngine owns it and destroys it separately.
    for (uint8_t i = 0; i < pipelineCount; ++i)
    {
        if (pipelines[i] != nullptr)
        {
            device.destroyPipeline (pipelines[i], nullptr);
            pipelines[i] = vk::Pipeline {};
        }
    }

    for (uint8_t i = 0; i < computePipelineCount; ++i)
    {
        if (computePipelines[i] != nullptr)
        {
            device.destroyPipeline (computePipelines[i], nullptr);
            computePipelines[i] = vk::Pipeline {};
        }
    }

    cache = vk::PipelineCache {};

    // 2. Destroy pipeline layouts.
    if (layout != nullptr)
    {
        device.destroyPipelineLayout (layout, nullptr);
        layout = vk::PipelineLayout {};
    }
    if (computeLayout != nullptr)
    {
        device.destroyPipelineLayout (computeLayout, nullptr);
        computeLayout = vk::PipelineLayout {};
    }

    // 3. Destroy descriptor set layouts.
    if (computeStorageLayout != nullptr)
    {
        device.destroyDescriptorSetLayout (computeStorageLayout, nullptr);
        computeStorageLayout = vk::DescriptorSetLayout {};
    }
    if (emptyLayout != nullptr)
    {
        device.destroyDescriptorSetLayout (emptyLayout, nullptr);
        emptyLayout = vk::DescriptorSetLayout {};
    }
    if (storageLayout != nullptr)
    {
        device.destroyDescriptorSetLayout (storageLayout, nullptr);
        storageLayout = vk::DescriptorSetLayout {};
    }
    if (samplerLayout != nullptr)
    {
        device.destroyDescriptorSetLayout (samplerLayout, nullptr);
        samplerLayout = vk::DescriptorSetLayout {};
    }
    if (projectionLayout != nullptr)
    {
        device.destroyDescriptorSetLayout (projectionLayout, nullptr);
        projectionLayout = vk::DescriptorSetLayout {};
    }

    // 4. Destroy shader modules.
    for (auto& [name, module] : shaderModules)
    {
        if (module != nullptr)
            device.destroyShaderModule (module, nullptr);
    }
    shaderModules.clear();
}

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam