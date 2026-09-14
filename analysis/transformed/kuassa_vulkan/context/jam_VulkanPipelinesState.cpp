namespace jam
{
/*____________________________________________________________________________*/
// =============================================================================
// Inline state constructors
// =============================================================================

vk::ShaderModule VulkanPipelines::createShaderModule (vk::Device device,
                                                 const char* bytes,
                                                 int sizeBytes)
{
    const vk::ShaderModuleCreateInfo moduleInfo { {}, static_cast<size_t> (sizeBytes),
                                                  reinterpret_cast<const uint32_t*> (bytes) };

    vk::ShaderModule module {};
    if (device.createShaderModule (&moduleInfo, nullptr, &module) != vk::Result::eSuccess)
        return {};

    return module;
}

vk::PipelineVertexInputStateCreateInfo VulkanPipelines::position2DVertexInputState()
{
    // Used exclusively by triangulated path draws (fillPath/clipToPath) —
    // single binding, vec2 device-space position, vertex-rate input, TRIANGLE_LIST
    // topology fed by the path VulkanFrameBuffer's vertex+index buffers.
    static const vk::VertexInputBindingDescription binding { 0, sizeof (float) * 2, vk::VertexInputRate::eVertex };
    static const vk::VertexInputAttributeDescription attribute { 0, 0, vk::Format::eR32G32Sfloat, 0 };

    return { {}, binding, attribute };
}

vk::PipelineVertexInputStateCreateInfo VulkanPipelines::instancedVertexInputState()
{
    // SSBO-pulled quad primitives (rect fill, image, glyph, clip mask) bind
    // NO vertex buffer at all: the instanced.vert shader reads VulkanPrimitiveRecord data via
    // gl_InstanceIndex from the set-2 storage buffer and expands the quad corner from
    // gl_VertexIndex. Zero bindings/attributes — the default-constructed create-info
    // already carries zero counts and null pointers.
    return {};
}

vk::PipelineInputAssemblyStateCreateInfo VulkanPipelines::triangleStripInputAssemblyState()
{
    return { {}, vk::PrimitiveTopology::eTriangleStrip, false };
}

vk::PipelineInputAssemblyStateCreateInfo VulkanPipelines::triangleListInputAssemblyState()
{
    return { {}, vk::PrimitiveTopology::eTriangleList, false };
}

vk::PipelineViewportStateCreateInfo VulkanPipelines::viewportState (vk::Extent2D extent)
{
    juce::ignoreUnused (extent);

    // vk::DynamicState::eViewport + vk::DynamicState::eScissor are set via
    // commandBuffer.setViewport()/setScissor() at draw time. Per Vulkan spec §9.2,
    // the driver ignores pViewports/pScissors when those states are dynamic;
    // only viewportCount / scissorCount are read from the create-info.
    // Setting the pointers to nullptr eliminates dangling stack references.
    return { {}, 1, nullptr, 1, nullptr };
}

vk::PipelineRasterizationStateCreateInfo VulkanPipelines::rasterizationState()
{
    return { {}, false, false, vk::PolygonMode::eFill, vk::CullModeFlagBits::eNone,
             vk::FrontFace::eCounterClockwise, false, 0.0f, 0.0f, 0.0f, 1.0f };
}

// Reads the caller-supplied session-locked MSAA sample count
// (VulkanDevice::getActiveSampleCount()) instead of a hardcoded single-sample count. 20 of
// the 21 pipelines built by load() share ONE vk::PipelineMultisampleStateCreateInfo
// (PipelineBuildState.multisample, alphaToCoverage = false, built once in
// initializeFixedFunctionState()), including the pipelines TransparencyLayer's and
// VulkanWindingScratch's own offscreen framebuffers are built against for
// pipeline-compatibility (both reuse the SAME `renderPass` passed to load() — see
// getOrCreateTransparencyLayer()/reserve()). This is safe: TransparencyLayer's
// and VulkanWindingScratch's own color/stencil images are also activeSampleCount-
// sampled as of this same step, each gaining a separate single-sample resolve
// target that is what actually gets sampled (a sampler2D cannot read a multisample
// image directly) — see jam_VulkanTransparencyStack.{h,cpp} and
// jam_VulkanWindingScratch.{h,cpp}.
//
// ID::clipMaskInstanced is the one exception, selecting the second variant
// (PipelineBuildState.multisampleAlphaToCoverage, alphaToCoverage = true — see
// buildPipelineCreateInfo()): image_alpha_mask.frag's fragment output is a single
// per-fragment alpha decision (`discard` at alpha<=0.0, else vec4(1,1,1,alpha)),
// which by default is broadcast identically to every MSAA sample. The stencil
// write that follows reads that broadcast decision, so the clipToImageAlpha mask
// boundary rasterizes with no sub-pixel coverage despite MSAA being active
// elsewhere. Alpha-to-coverage converts the shader's output alpha into a
// per-sample coverage mask instead — the stencil write then applies only to
// covered samples, restoring MSAA at the mask edge. No other pipeline's fragment
// shader gates a stencil write off a single continuous alpha value this way, so
// no other row needs the second variant.
vk::PipelineMultisampleStateCreateInfo VulkanPipelines::multisampleState (vk::SampleCountFlagBits sampleCount,
                                                                     bool alphaToCoverage)
{
    return { {}, sampleCount, false, 0.0f, nullptr, alphaToCoverage, false };
}

vk::PipelineDepthStencilStateCreateInfo VulkanPipelines::noStencilState()
{
    return { {}, false, false, vk::CompareOp::eAlways, false, false };
}

vk::PipelineDepthStencilStateCreateInfo VulkanPipelines::stencilWriteState()
{
    // A clip write only takes effect where the EXISTING
    // stencil value equals the parent depth (reference, set by the caller to
    // stencilClipDepth - 1 at draw time), and INCREMENT_AND_CLAMP bumps that
    // existing value by 1 rather than replacing it with the reference. This
    // serializes nested clip writes (0->1->2->...) so an inner clip only ever
    // reaches its own depth where the outer clip's write already passed —
    // true intersection instead of independent bbox overwrite.
    const vk::StencilOpState front { vk::StencilOp::eKeep, vk::StencilOp::eIncrementAndClamp, vk::StencilOp::eKeep,
                                     vk::CompareOp::eEqual, 0xFF, 0xFF, 1 };

    return { {}, false, false, vk::CompareOp::eNever, false, true, front, front };
}

vk::PipelineDepthStencilStateCreateInfo VulkanPipelines::stencilTestState()
{
    const vk::StencilOpState front { vk::StencilOp::eKeep, vk::StencilOp::eKeep, vk::StencilOp::eKeep,
                                     vk::CompareOp::eEqual, 0xFF, 0x00, 1 };

    return { {}, false, false, vk::CompareOp::eNever, false, true, front, front };
}

vk::PipelineDepthStencilStateCreateInfo VulkanPipelines::windingAccumulateState()
{
    // Isolated scratch-target winding accumulation. compareOp
    // ALWAYS: there is no prior gating value to test against (the scratch stencil is
    // freshly cleared to 0 every complex fillPath() call, completely decoupled from
    // the clip-depth stencil — see VulkanWindingScratch). Front/back passOp
    // diverge (INCREMENT_AND_WRAP / DECREMENT_AND_WRAP) — classic stencil winding-number
    // accumulation: rasterizationState()'s cullMode is NONE, so both facings of every
    // per-subpath fan triangle rasterize, and their wraparound counts combine into the
    // signed winding number (mod 256) at each covered pixel.
    const vk::StencilOpState front { vk::StencilOp::eKeep, vk::StencilOp::eIncrementAndWrap, vk::StencilOp::eKeep,
                                     vk::CompareOp::eAlways, 0xFF, 0xFF, 0 };

    vk::StencilOpState back { front };
    back.passOp = vk::StencilOp::eDecrementAndWrap;

    return { {}, false, false, vk::CompareOp::eNever, false, true, front, back };
}

vk::PipelineDepthStencilStateCreateInfo VulkanPipelines::windingCoverState()
{
    // Cover pass tests the value windingAccumulateState() just wrote. NOT_EQUAL
    // against reference 0 discards fragments outside the filled region. compareMask is
    // set DYNAMICALLY per draw (0xFF for nonzero winding, 0x01 for even-odd — see
    // VulkanLowLevelGraphicsContext::recordWindingAccumulateAndCoverDrawCommands()) — the
    // baked values below are placeholders, exactly mirroring stencilTestState()'s
    // existing convention (front/back masks/reference are all declared dynamic state;
    // vk::PipelineDepthStencilStateCreateInfo still requires some value at pipeline-
    // creation time even though it is ignored at draw time).
    const vk::StencilOpState front { vk::StencilOp::eKeep, vk::StencilOp::eKeep, vk::StencilOp::eKeep,
                                     vk::CompareOp::eNotEqual, 0xFF, 0x00, 0 };

    return { {}, false, false, vk::CompareOp::eNever, false, true, front, front };
}

vk::PipelineColorBlendAttachmentState VulkanPipelines::opaqueBlendAttachment()
{
    return { false, {}, {}, {}, {}, {}, {},
             vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG
                 | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA };
}

vk::PipelineColorBlendAttachmentState VulkanPipelines::alphaBlendAttachment()
{
    return { true, vk::BlendFactor::eOne, vk::BlendFactor::eOneMinusSrcAlpha, vk::BlendOp::eAdd,
             vk::BlendFactor::eOne, vk::BlendFactor::eOneMinusSrcAlpha, vk::BlendOp::eAdd,
             vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG
                 | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA };
}

vk::PipelineColorBlendStateCreateInfo VulkanPipelines::colorBlendState (
    const vk::PipelineColorBlendAttachmentState& attachment)
{
    return { {}, false, vk::LogicOp::eClear, 1, &attachment };
}

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam