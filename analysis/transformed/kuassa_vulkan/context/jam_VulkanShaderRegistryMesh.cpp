//
// VulkanShaderRegistryMesh file: buildExternalTexture() (decodes/uploads one
// named RetroArch LUT/texture, jam::VulkanShaderPreset::Texture) and every
// mesh-backed draw helper — the engine-default descriptor set/pipeline
// layout, the engine's own default, unanimated lit/transparent-fill/
// wireframe pipeline variants (built against the unified offscreen shader
// render pass, VulkanShaderRegistry::getOrCreateShaderOffscreenRenderPass
// (vk::Format), their own vertex stage runtime-compiled from
// mesh_default.vert/mesh_edge.vert's own engine wrapper templates via
// VulkanShaderCompiler::compileMeshVertexStage() — mesh_shader= is a
// vertex-ANIMATION HOOK into this engine's own default mesh look, mirroring
// Shadertoy's mainImage paradigm one level down, never a full-stage
// replacement), and the per-frame setup + per-MaterialRange fill +
// whole-mesh feature-edge overlay draw — recordMeshMaterialRangeDraws()
// itself selects, per draw, this execution's own hooked pipeline
// (jam::VulkanShaderInstance::getMeshHookFillPipeline() et al., built by
// that class's own buildMeshHookPipelines()) when
// VulkanShaderInstance::hasMeshHook() is true, or one of this file's own
// engine-default pipelines otherwise — the SAME rendering contract either
// way, only the vertex-stage animation differs. Every mesh draw — hooked or
// default — records directly into execution.getImagePassGatherTarget()'s own
// already-active render-pass instance, right after the ordinary Image-pass
// fullscreen draw (VulkanLowLevelGraphicsContext::
// recordShaderImagePassDrawCommands()), never a dedicated mesh render
// target/pass of its own. jam_VulkanShaderRegistry.cpp is the sibling
// file for the shared VulkanShader-execution setup helpers and the
// VulkanShaderInstance cache.

namespace jam
{
/*____________________________________________________________________________*/
//==============================================================================
// External LUT/texture upload
//==============================================================================

VulkanBindlessTexture VulkanShaderRegistry::buildExternalTexture (const VulkanShaderPreset::Texture& texture,
                                                                   vk::CommandBuffer commandBuffer, VulkanStagingArena& stagingArena)
{
    VulkanBindlessTexture builtTexture {};

    const juce::Image decodedImage { juce::ImageFileFormat::loadFrom (juce::File (texture.path)) };

    if (decodedImage.isValid())
    {
        // Normalize to ARGB before upload -- the SAME BGRA-byte-order pixel
        // format cacheImageTexture()'s own generic-texture path already
        // standardizes a decoded/source juce::Image onto before BitmapData is
        // ever read (vk::Format::eB8G8R8A8Unorm, 4 bytes/pixel), regardless
        // of texture.path's own on-disk format (whichever
        // juce::ImageFileFormat::loadFrom's own registered decoder produced).
        const juce::Image argbImage { decodedImage.convertedToFormat (juce::Image::ARGB) };
        const int width { argbImage.getWidth() };
        const int height { argbImage.getHeight() };
        const juce::Image::BitmapData bitmap { argbImage, juce::Image::BitmapData::readOnly };

        jassert (bitmap.pixelStride == 4);// ARGB only -- matches vk::Format::eB8G8R8A8Unorm, guaranteed by convertedToFormat() above

        // texture.mipmapInput (jam_VulkanShaderPreset.h's own VulkanTexture::
        // mipmapInput doc comment) -- this texture IS its own producer
        // (unlike a buffer pass's own mipmap_inputN, declared on the
        // CONSUMER), so its own directive is read directly, no
        // consumer-ordinal lookup needed. computeMipLevelCount() --
        // jam_VulkanShaderInstance.cpp -- recordMipChainGeneration()'s
        // own identical cross-file reuse of minimumPixelExtent.
        const uint32_t mipLevels { texture.mipmapInput
            ? computeMipLevelCount (vk::Extent2D { static_cast<uint32_t> (width), static_cast<uint32_t> (height) })
            : 1 };

        // eTransferSrc/eTransferDst only when a real mip chain is requested
        // -- recordMipChainGeneration()'s own per-level vkCmdBlitImage
        // source/destination requirement, mirroring VulkanShaderInstance::
        // buildRenderResources()'s identical mipUsage gate.
        const vk::ImageUsageFlags mipUsage { mipLevels > 1
            ? vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst
            : vk::ImageUsageFlags {} };

        if (builtTexture.create (device, width, height, vk::Format::eB8G8R8A8Unorm,
                                 vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled | mipUsage,
                                 mipLevels))
        {
            const vk::DeviceSize stagingSize { static_cast<vk::DeviceSize> (bitmap.lineStride)
                                             * static_cast<vk::DeviceSize> (height) };
            const auto staging { stagingArena.getOrCreateAllocation (stagingSize) };

            if (staging.valid)
            {
                builtTexture.upload (commandBuffer, staging.buffer, staging.mapped, staging.offset, bitmap, width, height);

                if (mipLevels > 1)
                    recordMipChainGeneration (commandBuffer, builtTexture.getImage(), builtTexture.getExtent(), mipLevels);
            }
        }
    }
    else
    {
#if JUCE_DEBUG
        jam::debug::Log::write ("jam::VulkanShaderRegistry::buildExternalTexture: failed to decode \""
                                + texture.path + "\" (texture \"" + texture.name + "\")");
#endif
    }

    return builtTexture;
}

//==============================================================================
// VulkanMesh-backed material-range draw
//==============================================================================

vk::DescriptorSetLayout VulkanShaderRegistry::getOrCreateMeshDescriptorSetLayout()
{
    if (meshDescriptorSetLayout == nullptr)
    {
        const std::array<vk::DescriptorSetLayoutBinding, 3> bindings {
            vk::DescriptorSetLayoutBinding { 0, vk::DescriptorType::eUniformBuffer, 1,
                                             vk::ShaderStageFlagBits::eVertex },
            vk::DescriptorSetLayoutBinding { 1, vk::DescriptorType::eStorageBuffer, 1,
                                             vk::ShaderStageFlagBits::eVertex },
            // Feature-edge endpoint-index SSBO — read ONLY by mesh_edge.vert
            // (jam_vulkan/shader/mesh_edge.vert's own doc comment); the
            // fill/transparent-fill pipelines' own mesh_default.vert declares
            // no binding 2 at all, so this binding is simply unread by every
            // draw except the whole-mesh feature-edge overlay.
            vk::DescriptorSetLayoutBinding { 2, vk::DescriptorType::eStorageBuffer, 1,
                                             vk::ShaderStageFlagBits::eVertex }
        };

        const vk::DescriptorSetLayoutCreateInfo layoutInfo { {}, bindings };

        const vk::Result createLayoutResult { device.getDevice().createDescriptorSetLayout (
            &layoutInfo, nullptr, &meshDescriptorSetLayout) };
        jassert (createLayoutResult == vk::Result::eSuccess);
        juce::ignoreUnused (createLayoutResult);
    }

    return meshDescriptorSetLayout;
}

vk::PipelineLayout VulkanShaderRegistry::getOrCreateMeshPipelineLayout()
{
    if (meshPipelineLayout == nullptr)
    {
        const vk::DescriptorSetLayout setLayout { getOrCreateMeshDescriptorSetLayout() };

        const vk::PushConstantRange pushRange { vk::ShaderStageFlagBits::eFragment, 0,
                                                sizeof (VulkanShaderInstance::MeshMaterialPushConstants) };

        const vk::PipelineLayoutCreateInfo layoutInfo { {}, setLayout, pushRange };

        const vk::Result createLayoutResult { device.getDevice().createPipelineLayout (
            &layoutInfo, nullptr, &meshPipelineLayout) };
        jassert (createLayoutResult == vk::Result::eSuccess);
        juce::ignoreUnused (createLayoutResult);
    }

    return meshPipelineLayout;
}

const juce::MemoryBlock& VulkanShaderRegistry::getOrCreateMeshDefaultVertexSpirv()
{
    if (meshDefaultVertexSpirv.isEmpty())
        meshDefaultVertexSpirv = VulkanShaderCompiler::compileMeshVertexStage (
            files::meshDefaultVertex, {});

    return meshDefaultVertexSpirv;
}

const juce::MemoryBlock& VulkanShaderRegistry::getOrCreateMeshEdgeVertexSpirv()
{
    if (meshEdgeVertexSpirv.isEmpty())
        meshEdgeVertexSpirv = VulkanShaderCompiler::compileMeshVertexStage (
            files::meshEdgeVertex, {});

    return meshEdgeVertexSpirv;
}

vk::Pipeline VulkanShaderRegistry::getOrCreateMeshPipeline (vk::Format colorFormat)
{
    if (meshPipeline == nullptr)
        meshPipeline = buildMeshVariantPipeline (getOrCreateMeshDefaultVertexSpirv(), files::meshDefaultFrag,
                                                 VulkanPipelines::opaqueBlendAttachment(), colorFormat);

    jassert (meshPipeline != nullptr);
    return meshPipeline;
}

vk::Pipeline VulkanShaderRegistry::getOrCreateMeshTransparentFillPipeline (vk::Format colorFormat)
{
    if (meshTransparentFillPipeline == nullptr)
        meshTransparentFillPipeline = buildMeshVariantPipeline (getOrCreateMeshDefaultVertexSpirv(), files::meshDefaultFrag,
                                                                 VulkanPipelines::alphaBlendAttachment(), colorFormat);

    jassert (meshTransparentFillPipeline != nullptr);
    return meshTransparentFillPipeline;
}

vk::Pipeline VulkanShaderRegistry::getOrCreateMeshWireframePipeline (vk::Format colorFormat)
{
    if (meshWireframePipeline == nullptr)
    {
        // mesh_edge.vert-templated vertex-hook SPIR-V (hookless, the no-op
        // mainMesh splice) + mesh_edge.frag — NOT mesh_default.vert/.frag —
        // this pipeline's own screen-space thick-line quad expansion
        // (mesh_edge.vert's own doc comment for why:
        // vk::PipelineRasterizationStateCreateInfo::lineWidth greater than 1.0
        // requires the wideLines device feature, unsupported on MoltenVK/Metal)
        // and flat-colour output (mesh_edge.frag's own doc comment for why
        // mesh_default.frag's headlight lighting model does not apply here).
        // eTriangleList, not eLineList — this overlay's own thick-line quads
        // still rasterize as ordinary triangles (createMeshPipelineFixedFunctionState()'s
        // own doc comment).
        meshWireframePipeline = buildMeshVariantPipeline (getOrCreateMeshEdgeVertexSpirv(), files::meshEdgeFrag,
                                                          VulkanPipelines::opaqueBlendAttachment(), colorFormat);
    }

    jassert (meshWireframePipeline != nullptr);
    return meshWireframePipeline;
}

vk::Pipeline VulkanShaderRegistry::buildMeshVariantPipeline (const juce::MemoryBlock& vertexSpirv, const juce::String& fragFile,
                                                             const vk::PipelineColorBlendAttachmentState& blendAttachment,
                                                             vk::Format colorFormat)
{
    vk::ShaderModule vertModule { VulkanPipelines::createShaderModule (
        device.getDevice(), static_cast<const char*> (vertexSpirv.getData()), static_cast<int> (vertexSpirv.getSize())) };

    const BinaryData::Raw fragRaw { fragFile };
    vk::ShaderModule fragModule { VulkanPipelines::createShaderModule (device.getDevice(), fragRaw.data, fragRaw.size) };

    vk::Pipeline pipeline {};

    if (vertModule != nullptr and fragModule != nullptr)
        pipeline = createMeshPipeline (vertModule, fragModule, vk::PrimitiveTopology::eTriangleList,
                                       vk::PolygonMode::eFill, blendAttachment, colorFormat);

    // Unconditional destroy — a documented no-op against an empty handle,
    // this module's own stated convention (jam_VulkanShaderRegistry.cpp's
    // ~VulkanShaderRegistry() doc comment).
    device.getDevice().destroyShaderModule (vertModule, nullptr);
    device.getDevice().destroyShaderModule (fragModule, nullptr);

    return pipeline;
}

VulkanShaderRegistry::MeshPipelineFixedFunctionState VulkanShaderRegistry::createMeshPipelineFixedFunctionState (
    vk::PrimitiveTopology topology, vk::PolygonMode polygonMode,
    const vk::PipelineColorBlendAttachmentState& blendAttachment) const
{
    MeshPipelineFixedFunctionState state {};

    state.inputAssemblyInfo.topology = topology;

    state.viewportInfo.viewportCount = 1;
    state.viewportInfo.scissorCount = 1;

    state.rasterizerInfo.polygonMode = polygonMode;
    state.rasterizerInfo.cullMode = vk::CullModeFlagBits::eNone;
    state.rasterizerInfo.frontFace = vk::FrontFace::eCounterClockwise;
    state.rasterizerInfo.lineWidth = 1.0f;

    state.multisampleInfo.rasterizationSamples = vk::SampleCountFlagBits::e1;

    state.depthStencilInfo.depthTestEnable = vk::True;
    state.depthStencilInfo.depthWriteEnable = vk::True;
    state.depthStencilInfo.depthCompareOp = vk::CompareOp::eLess;

    state.blendAttachment = blendAttachment;
    state.dynamicStates = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };

    return state;
}

vk::Pipeline VulkanShaderRegistry::createMeshPipeline (vk::ShaderModule vertModule, vk::ShaderModule fragModule,
                                                       vk::PrimitiveTopology topology, vk::PolygonMode polygonMode,
                                                       const vk::PipelineColorBlendAttachmentState& blendAttachment,
                                                       vk::Format colorFormat)
{
    const std::array<vk::PipelineShaderStageCreateInfo, 2> stages {
        vk::PipelineShaderStageCreateInfo { {}, vk::ShaderStageFlagBits::eVertex, vertModule, "main" },
        vk::PipelineShaderStageCreateInfo { {}, vk::ShaderStageFlagBits::eFragment, fragModule, "main" }
    };

    const MeshPipelineFixedFunctionState fixedState { createMeshPipelineFixedFunctionState (topology, polygonMode, blendAttachment) };
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
    pipelineInfo.layout = getOrCreateMeshPipelineLayout();
    pipelineInfo.renderPass = getOrCreateShaderOffscreenRenderPass (colorFormat);
    pipelineInfo.subpass = 0;

    vk::Pipeline pipeline {};
    const vk::Result createPipelineResult { device.getDevice().createGraphicsPipelines (
        nullptr, 1, &pipelineInfo, nullptr, &pipeline) };
    jassert (createPipelineResult == vk::Result::eSuccess);
    juce::ignoreUnused (createPipelineResult);

    return pipeline;
}

void VulkanShaderRegistry::recordMeshGatherDrawCommands (VulkanShaderInstance& execution, const glm::mat4& view,
                                                         const glm::mat4& projection, const glm::mat4& normalMatrix,
                                                         const VulkanShaderUniforms& imageUniforms,
                                                         vk::CommandBuffer commandBuffer, vk::Format colorFormat)
{
    // mesh_shader= is a vertex-ANIMATION HOOK into the engine's own
    // default mesh look, never a full-stage replacement — the SAME setup +
    // per-MaterialRange fill + feature-edge overlay sequence always runs;
    // recordMeshMaterialRangeDraws() itself is what selects this execution's
    // own hooked pipeline vs the engine-default one, per draw (see this
    // method's own header doc comment).
    recordMeshGatherSetup (execution, view, projection, normalMatrix, imageUniforms, commandBuffer);
    recordMeshMaterialRangeDraws (execution, commandBuffer, colorFormat);
}

void VulkanShaderRegistry::recordMeshGatherSetup (VulkanShaderInstance& execution, const glm::mat4& view,
                                                  const glm::mat4& projection, const glm::mat4& normalMatrix,
                                                  const VulkanShaderUniforms& imageUniforms, vk::CommandBuffer commandBuffer)
{
    const vk::Extent2D gatherExtent { execution.getImagePassGatherTarget().extent };

    // Single named-constant tuning point for the whole-mesh feature-edge
    // overlay's own pixel thickness (mesh_edge.vert's own screen-space quad
    // expansion) — mirrors defaultMaterialFillColour/featureEdgeColour's own
    // local-named-constant convention in this same mesh-draw path.
    static constexpr float featureEdgeThicknessPixels { 2.5f };

    // @p imageUniforms carries the SAME iMouse/iResolution/iTime/iTimeDelta/
    // iFrame the caller's own ordinary Image-pass fullscreen draw already
    // pushed — refreshMeshUniforms() copies them verbatim into this
    // execution's own mesh UBO, the standard-uniform delivery mechanism a
    // mesh_shader= mainMesh snippet reads bare (zero bespoke uniform
    // vocabulary either way, hooked or default).
    execution.refreshMeshUniforms (projection * view * execution.getMeshModelMatrix(), normalMatrix,
                                   gatherExtent, featureEdgeThicknessPixels, imageUniforms);

    // The caller's own render-pass instance is already active — the ordinary
    // Image-pass fullscreen draw it just recorded into the SAME framebuffer
    // serves as this mesh's own backdrop, unchanged, its own normal pipeline
    // (VulkanLowLevelGraphicsContext::recordShaderImagePassDrawCommands()'s own
    // doc comment) — this method records no barrier, clear, or render-pass
    // begin/end of its own, only the state every MaterialRange draw below
    // needs already bound. getOrCreateMeshPipelineLayout() is the SAME
    // layout every mesh pipeline variant is built against, engine-default
    // AND hooked alike (VulkanShaderInstance::buildMeshHookPipelines()'s own doc
    // comment) — no per-variant descriptor rebind is ever needed.
    commandBuffer.bindDescriptorSets (vk::PipelineBindPoint::eGraphics, getOrCreateMeshPipelineLayout(), 0,
                                      execution.getMeshDescriptorSet(), nullptr);

    setFullViewport (commandBuffer, gatherExtent);
    const vk::Rect2D scissor { { 0, 0 }, gatherExtent };
    commandBuffer.setScissor (0, scissor);

    auto& mesh { execution.getMesh() };
    commandBuffer.bindIndexBuffer (mesh.getIndexBuffer(), 0, vk::IndexType::eUint32);
}

void VulkanShaderRegistry::recordMeshMaterialRangeDraws (VulkanShaderInstance& execution, vk::CommandBuffer commandBuffer,
                                                         vk::Format colorFormat)
{
    auto& mesh { execution.getMesh() };

    // mesh_shader= is a vertex-ANIMATION HOOK into the engine's own
    // default mesh look — every pipeline bind below selects this execution's
    // own hooked pipeline when present, the engine-default one otherwise; the
    // SAME rendering contract either way, only the vertex-stage animation
    // differs (this method's own header doc comment).
    const bool hasHook { execution.hasMeshHook() };

    // Push colours — jam::WavefrontObj::Material::name's own doc comment: an
    // empty name is the SSOT "no real MTL material" signal. Cyan-tinted,
    // matching mesh_default.frag's own headlight-lit diffuse.rgb *
    // diffuse.a premultiplied-alpha contract (its own doc comment) — alpha 0.12
    // for the low-alpha default-material fill, 1.0 (fully opaque) for the
    // whole-mesh feature-edge overlay drawn once, below, after every
    // MaterialRange's own fill draw.
    static constexpr std::array<float, 4> defaultMaterialFillColour { 0.0f, 1.0f, 1.0f, 0.12f };
    static constexpr std::array<float, 4> featureEdgeColour { 0.0f, 1.0f, 1.0f, 1.0f };

    // One indexed FILL draw per shape's own MaterialRange: a real material
    // draws once — simplest: the material's diffuse as a push
    // constant per drawIndexed range; a default (unnamed) material also
    // draws once, with the low-alpha transparent-fill pipeline instead — the
    // per-range wireframe outline this used to also draw here is gone,
    // replaced by the single whole-mesh overlay below (this method's own
    // header doc comment). MeshMaterialPushConstants::diffuse is a plain
    // float[4] — the inner braces initialize that single array member.
    for (auto& range : mesh.getMaterialRanges())
    {
        const bool isDefaultMaterial { range.material.name.isEmpty() };

        const VulkanShaderInstance::MeshMaterialPushConstants rangePushConstants { isDefaultMaterial
            ? VulkanShaderInstance::MeshMaterialPushConstants { { defaultMaterialFillColour.at (0), defaultMaterialFillColour.at (1),
                                                                  defaultMaterialFillColour.at (2), defaultMaterialFillColour.at (3) } }
            : VulkanShaderInstance::MeshMaterialPushConstants { { range.material.diffuse.x, range.material.diffuse.y,
                                                                  range.material.diffuse.z, 1.0f } } };

        commandBuffer.bindPipeline (vk::PipelineBindPoint::eGraphics, isDefaultMaterial
            ? (hasHook ? execution.getMeshHookTransparentFillPipeline() : getOrCreateMeshTransparentFillPipeline (colorFormat))
            : (hasHook ? execution.getMeshHookFillPipeline() : getOrCreateMeshPipeline (colorFormat)));
        commandBuffer.pushConstants (getOrCreateMeshPipelineLayout(), vk::ShaderStageFlagBits::eFragment,
                                     0, sizeof (rangePushConstants), &rangePushConstants);
        commandBuffer.drawIndexed (range.numIndices, 1, range.firstIndex, 0, 0);
    }

    // ONE whole-mesh feature-edge line-art overlay draw — never per range
    // (jam::VulkanMesh's own class doc comment, "Feature-edge line art"
    // section: feature edges are a property of the whole VulkanMesh). Non-indexed —
    // mesh_edge.vert reads mesh.getFeatureEdgeIndexBuffer() as an SSBO (set 0
    // binding 2, execution.getMeshDescriptorSet() — recordMeshGatherSetup()
    // already bound it), never through the fixed-function index fetch, so the
    // triangle index buffer that method bound earlier stays bound and simply
    // unused by this one draw. VulkanVertex count: 2 endpoint indices per
    // classified edge, 6 vertices (one screen-space quad's own 2 triangles)
    // per edge — mesh_edge.vert's own gl_VertexIndex decode.
    if (mesh.getNumFeatureEdgeIndices() > 0)
    {
        const VulkanShaderInstance::MeshMaterialPushConstants featureEdgePushConstants {
            { featureEdgeColour.at (0), featureEdgeColour.at (1), featureEdgeColour.at (2), featureEdgeColour.at (3) } };

        // Feature edges are declared as endpoint-index pairs (VulkanMesh::
        // getFeatureEdgeIndexBuffer()'s own doc comment) — one screen-space
        // quad (2 triangles, 6 vertices) drawn per classified edge.
        static constexpr uint32_t indicesPerFeatureEdge { 2u };
        static constexpr uint32_t verticesPerFeatureEdgeQuad { 6u };
        const uint32_t featureEdgeVertexCount {
            (mesh.getNumFeatureEdgeIndices() / indicesPerFeatureEdge) * verticesPerFeatureEdgeQuad };

        commandBuffer.bindPipeline (vk::PipelineBindPoint::eGraphics,
            hasHook ? execution.getMeshHookEdgePipeline() : getOrCreateMeshWireframePipeline (colorFormat));
        commandBuffer.pushConstants (getOrCreateMeshPipelineLayout(), vk::ShaderStageFlagBits::eFragment,
                                     0, sizeof (featureEdgePushConstants), &featureEdgePushConstants);
        commandBuffer.draw (featureEdgeVertexCount, 1, 0, 0);
    }
}

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
