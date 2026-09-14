// VulkanMesh-backed material-range draw file: buildMeshResources() (build()'s
// mesh-only step — uploads shader.meshShapes, already parsed ONCE by
// VulkanShaderCompiler::compile() (never re-parsed here, no matter how many times
// this execution rebuilds across a window-resize extent change), bakes the
// auto-fit model matrix, then builds every mesh GPU resource below) and its
// own steps buildMeshGpuResources()/createMeshDescriptorPoolAndSet(), plus
// buildMeshHookPipelines()/createMeshHookPipeline() (this execution's own
// hooked mesh pipelines, compiled from shader.meshShaderSource when present —
// mesh_shader= is a vertex-ANIMATION HOOK into the engine's own
// default mesh look, mirroring Shadertoy's mainImage paradigm one level
// down). Split out of jam_VulkanShaderInstance.cpp by concern — that file's
// own build() orchestrates a call into buildMeshResources() below via
// buildMeshIfDeclared(); see its own doc comment for the full call sequence.
// The engine-default mesh material-range/feature-edge draw has no render
// target/pipeline of its own to build here — it renders directly into
// imagePassGatherTarget (already built by buildImagePassTarget(),
// jam_VulkanShaderInstance.cpp) via the engine-owned mesh pipelines
// (VulkanShaderRegistry::getOrCreateMeshPipeline() et al., jam_VulkanShaderRegistryMesh.cpp);
// this file builds the mesh's own MVP/normal-matrix+standard-uniforms UBO
// and descriptor set (always), plus 3 hooked pipelines — fill/transparent-
// fill/edge (only when a mesh_shader= connection's own vertex-hook compiled
// successfully) — that draw into the SAME imagePassGatherTarget render pass
// too, just with their own instance-owned vk::Pipeline handles, built
// against the SAME engine-owned mesh pipeline layout
// (VulkanGraphics::getOrCreateMeshPipelineLayout()) the engine-default pipelines
// use, instead of engine-owned pipelines.

namespace jam
{
/*____________________________________________________________________________*/
//==============================================================================
// VulkanMesh-backed material-range draw
//==============================================================================

bool VulkanShaderInstance::buildMeshResources (const jam::Owner<jam::WavefrontObj::Shape>& meshShapes,
                                        vk::CommandBuffer commandBuffer,
                                        vk::DescriptorSetLayout meshDescriptorSetLayout)
{
    bool built { mesh.build (device, commandBuffer, meshShapes) };
    meshStagingPendingRelease = built;

    if (built)
        meshModelMatrix = VulkanOrbitCamera::computeAutoFitModelMatrix (mesh.getAabbMin(), mesh.getAabbMax());

    if (built)
        built = buildMeshGpuResources (meshDescriptorSetLayout);

    return built;
}

bool VulkanShaderInstance::buildMeshGpuResources (vk::DescriptorSetLayout meshDescriptorSetLayout)
{
    static constexpr vk::DeviceSize meshUniformBufferSize { sizeof (MeshUniforms) };

    // CPU_ONLY + MAPPED_BIT — same coherency rationale as VulkanGraphics::projectionBuffer's
    // own allocation site (jam_VulkanGraphicsSetupDrawState.cpp).
    const vk::BufferCreateInfo uniformBufferInfo { {}, meshUniformBufferSize, vk::BufferUsageFlagBits::eUniformBuffer,
                                                   vk::SharingMode::eExclusive };

    VmaAllocationCreateInfo uniformAllocInfo {};
    uniformAllocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
    uniformAllocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

    meshUniformBuffer = VulkanBuffer (device.getAllocator(), uniformBufferInfo, uniformAllocInfo);
    bool built { meshUniformBuffer.isValid() };

    if (built)
        built = createMeshDescriptorPoolAndSet (meshDescriptorSetLayout);

    if (built)
    {
        // Both fixed bindings written ONCE, here — neither buffer's own
        // vk::Buffer handle ever changes afterward (meshUniformBuffer is
        // refreshed only via its mapped bytes, refreshMeshUniforms();
        // mesh.getVertexBuffer() is built once by mesh.build() and never
        // replaced) — unlike refreshSlangPass()'s own per-frame texture-
        // binding rewrite, this execution's mesh descriptor set needs no
        // per-frame descriptor write at all, only the UBO memcpy.
        const vk::DescriptorBufferInfo uniformDescInfo { meshUniformBuffer.getBuffer(), 0, meshUniformBufferSize };
        const vk::DescriptorBufferInfo vertexDescInfo { mesh.getVertexBuffer(), 0, VK_WHOLE_SIZE };

        const std::array<vk::WriteDescriptorSet, 2> writes {
            vk::WriteDescriptorSet { meshDescriptorSet, 0, 0, 1, vk::DescriptorType::eUniformBuffer,
                                     nullptr, &uniformDescInfo },
            vk::WriteDescriptorSet { meshDescriptorSet, 1, 0, 1, vk::DescriptorType::eStorageBuffer,
                                     nullptr, &vertexDescInfo }
        };

        device.getDevice().updateDescriptorSets (writes, nullptr);

        // Binding 2 (feature-edge endpoint-index SSBO, mesh_edge.vert's own
        // doc comment) — written ONLY when this mesh actually classified a
        // feature edge (jam::VulkanMesh::getFeatureEdgeIndexBuffer()'s own
        // doc comment: an empty handle otherwise, never a valid descriptor to
        // write). VulkanGraphics::recordMeshMaterialRangeDraws()'s own
        // getNumFeatureEdgeIndices() > 0 gate is the ONLY place this binding
        // is ever read (the fill/transparent-fill pipelines' own
        // mesh_default.vert declares no binding 2 at all), so an unwritten
        // binding 2 on a feature-edge-less mesh is never reached by any draw.
        if (mesh.getNumFeatureEdgeIndices() > 0)
        {
            const vk::DescriptorBufferInfo featureEdgeDescInfo { mesh.getFeatureEdgeIndexBuffer(), 0, VK_WHOLE_SIZE };
            const vk::WriteDescriptorSet featureEdgeWrite { meshDescriptorSet, 2, 0, 1,
                                                            vk::DescriptorType::eStorageBuffer,
                                                            nullptr, &featureEdgeDescInfo };

            device.getDevice().updateDescriptorSets (featureEdgeWrite, nullptr);
        }
    }

    return built;
}

vk::Pipeline VulkanShaderInstance::createMeshHookPipeline (vk::ShaderModule vertModule, vk::ShaderModule fragModule,
                                                     const vk::PipelineColorBlendAttachmentState& blendAttachment,
                                                     vk::PipelineLayout meshPipelineLayout) const
{
    const vk::PipelineShaderStageCreateInfo stages[2] {
        { {}, vk::ShaderStageFlagBits::eVertex, vertModule, "main" },
        { {}, vk::ShaderStageFlagBits::eFragment, fragModule, "main" }
    };

    // Zero vertex-input bindings/attributes — this hooked vertex stage pulls
    // {position,normal,uv} from the SAME mesh vertex-pulling SSBO (set 0
    // binding 1) via gl_VertexIndex, fed by the bound INDEX buffer —
    // identical technique to VulkanGraphics::getOrCreateMeshPipeline()'s own
    // engine-default mesh pipeline.
    const vk::PipelineVertexInputStateCreateInfo vertexInputInfo {};
    const vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo { {}, vk::PrimitiveTopology::eTriangleList };

    vk::PipelineViewportStateCreateInfo viewportInfo {};
    viewportInfo.viewportCount = 1;
    viewportInfo.scissorCount = 1;

    vk::PipelineRasterizationStateCreateInfo rasterizerInfo {};
    rasterizerInfo.polygonMode = vk::PolygonMode::eFill;
    rasterizerInfo.cullMode = vk::CullModeFlagBits::eNone;
    rasterizerInfo.frontFace = vk::FrontFace::eCounterClockwise;
    rasterizerInfo.lineWidth = 1.0f;

    vk::PipelineMultisampleStateCreateInfo multisampleInfo {};
    multisampleInfo.rasterizationSamples = vk::SampleCountFlagBits::e1;

    // Depth test/write ON — same depth contract as VulkanGraphics::
    // createMeshPipelineFixedFunctionState()'s own engine-default mesh
    // pipelines (this hooked pipeline draws the SAME rendering contract,
    // only the vertex-stage animation differs).
    vk::PipelineDepthStencilStateCreateInfo depthStencilInfo {};
    depthStencilInfo.depthTestEnable = vk::True;
    depthStencilInfo.depthWriteEnable = vk::True;
    depthStencilInfo.depthCompareOp = vk::CompareOp::eLess;

    vk::PipelineColorBlendStateCreateInfo colorBlendInfo {};
    colorBlendInfo.attachmentCount = 1;
    colorBlendInfo.pAttachments = &blendAttachment;

    const std::array<vk::DynamicState, 2> dynamicStates { vk::DynamicState::eViewport, vk::DynamicState::eScissor };
    const vk::PipelineDynamicStateCreateInfo dynamicStateInfo { {}, dynamicStates };

    vk::GraphicsPipelineCreateInfo pipelineInfo {};
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = stages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
    pipelineInfo.pViewportState = &viewportInfo;
    pipelineInfo.pRasterizationState = &rasterizerInfo;
    pipelineInfo.pMultisampleState = &multisampleInfo;
    pipelineInfo.pDepthStencilState = &depthStencilInfo;
    pipelineInfo.pColorBlendState = &colorBlendInfo;
    pipelineInfo.pDynamicState = &dynamicStateInfo;
    pipelineInfo.layout = meshPipelineLayout;

    // The SAME unified offscreen shader render pass every mesh draw
    // targets — already resolved by buildImagePassTarget() (this
    // execution's own build() call sequence, buildMeshIfDeclared() runs
    // strictly after it), never re-resolved through a second factory call.
    pipelineInfo.renderPass = imagePassGatherTarget.renderPass;
    pipelineInfo.subpass = 0;

    vk::Pipeline pipeline {};
    const vk::Result createPipelineResult { device.getDevice().createGraphicsPipelines (
        nullptr, 1, &pipelineInfo, nullptr, &pipeline) };
    jassert (createPipelineResult == vk::Result::eSuccess);
    juce::ignoreUnused (createPipelineResult);

    return pipeline;
}

bool VulkanShaderInstance::buildMeshHookPipelines (const VulkanShader& shader, vk::PipelineLayout meshPipelineLayout)
{
    // Compiled ONCE each, shared by the fill and transparent-fill variants
    // below (mesh_default.vert-template's own hook, mesh_default.frag
    // unchanged) and the edge variant (mesh_edge.vert-template's own hook,
    // mesh_edge.frag unchanged) — VulkanShaderCompiler::compileMeshVertexStage()'s
    // own template-plus-replaceholder-plus-compileSpirv precedent.
    const juce::MemoryBlock fillVertexSpirv { VulkanShaderCompiler::compileMeshVertexStage (
        files::meshDefaultVertex, shader.meshShaderSource) };
    const juce::MemoryBlock edgeVertexSpirv { VulkanShaderCompiler::compileMeshVertexStage (
        files::meshEdgeVertex, shader.meshShaderSource) };

    bool built { not fillVertexSpirv.isEmpty() and not edgeVertexSpirv.isEmpty() };

    if (built)
    {
        vk::ShaderModule fillVertModule { VulkanPipelines::createShaderModule (
            device.getDevice(), static_cast<const char*> (fillVertexSpirv.getData()),
            static_cast<int> (fillVertexSpirv.getSize())) };
        const BinaryData::Raw meshDefaultFrag { files::meshDefaultFrag };
        vk::ShaderModule fillFragModule { VulkanPipelines::createShaderModule (
            device.getDevice(), meshDefaultFrag.data, meshDefaultFrag.size) };
        vk::ShaderModule edgeVertModule { VulkanPipelines::createShaderModule (
            device.getDevice(), static_cast<const char*> (edgeVertexSpirv.getData()),
            static_cast<int> (edgeVertexSpirv.getSize())) };
        const BinaryData::Raw meshEdgeFrag { files::meshEdgeFrag };
        vk::ShaderModule edgeFragModule { VulkanPipelines::createShaderModule (
            device.getDevice(), meshEdgeFrag.data, meshEdgeFrag.size) };

        built = fillVertModule != nullptr and fillFragModule != nullptr
              and edgeVertModule != nullptr and edgeFragModule != nullptr;

        if (built)
        {
            meshHookFillPipeline = createMeshHookPipeline (
                fillVertModule, fillFragModule, VulkanPipelines::opaqueBlendAttachment(), meshPipelineLayout);
            meshHookTransparentFillPipeline = createMeshHookPipeline (
                fillVertModule, fillFragModule, VulkanPipelines::alphaBlendAttachment(), meshPipelineLayout);
            meshHookEdgePipeline = createMeshHookPipeline (
                edgeVertModule, edgeFragModule, VulkanPipelines::opaqueBlendAttachment(), meshPipelineLayout);

            built = meshHookFillPipeline != nullptr and meshHookTransparentFillPipeline != nullptr
                  and meshHookEdgePipeline != nullptr;

            if (not built)
            {
                // ALL three or none — a partial build (some, not all, of the
                // 3 createGraphicsPipelines calls above succeeded) is torn
                // down rather than left half-hooked, so recordMeshMaterialRangeDraws()'s
                // own hasMeshHook() gate never binds a mix of hooked and
                // engine-default pipelines within the same mesh draw.
                device.getDevice().destroyPipeline (meshHookFillPipeline, nullptr);
                device.getDevice().destroyPipeline (meshHookTransparentFillPipeline, nullptr);
                device.getDevice().destroyPipeline (meshHookEdgePipeline, nullptr);
                meshHookFillPipeline = nullptr;
                meshHookTransparentFillPipeline = nullptr;
                meshHookEdgePipeline = nullptr;

#if JUCE_DEBUG
                jam::debug::Log::write ("jam::VulkanShaderInstance::buildMeshHookPipelines: "
                                        "createGraphicsPipelines failed -- engine default mesh pipelines retained");
#endif
            }
        }
        else
        {
#if JUCE_DEBUG
            jam::debug::Log::write ("jam::VulkanShaderInstance::buildMeshHookPipelines: "
                                    "failed to create a shader module from mesh_shader's compiled SPIR-V "
                                    "-- engine default mesh pipelines retained");
#endif
        }

        if (fillVertModule != nullptr)
            device.getDevice().destroyShaderModule (fillVertModule, nullptr);

        if (fillFragModule != nullptr)
            device.getDevice().destroyShaderModule (fillFragModule, nullptr);

        if (edgeVertModule != nullptr)
            device.getDevice().destroyShaderModule (edgeVertModule, nullptr);

        if (edgeFragModule != nullptr)
            device.getDevice().destroyShaderModule (edgeFragModule, nullptr);
    }
    else
    {
#if JUCE_DEBUG
        jam::debug::Log::write ("jam::VulkanShaderInstance::buildMeshHookPipelines: "
                                "mesh_shader vertex-hook compile failed -- engine default mesh pipelines retained");
#endif
    }

    return built;
}

bool VulkanShaderInstance::createMeshDescriptorPoolAndSet (vk::DescriptorSetLayout meshDescriptorSetLayout)
{
    // eStorageBuffer count 2 — the vertex-pulling SSBO (binding 1, always
    // written) and the feature-edge endpoint-index SSBO (binding 2, written
    // only when this mesh classified at least one feature edge,
    // buildMeshGpuResources()'s own doc comment) — sized for the max, never
    // re-sized down for a feature-edge-less mesh (an unused pool slot is
    // harmless, exactly like this pool's own single fixed descriptor set).
    const std::array<vk::DescriptorPoolSize, 2> poolSizes {
        vk::DescriptorPoolSize { vk::DescriptorType::eUniformBuffer, 1 },
        vk::DescriptorPoolSize { vk::DescriptorType::eStorageBuffer, 2 }
    };

    static constexpr uint32_t meshDescriptorSetCount { 1 };
    const vk::DescriptorPoolCreateInfo poolInfo { {}, meshDescriptorSetCount, poolSizes };

    bool built { device.getDevice().createDescriptorPool (&poolInfo, nullptr, &meshDescriptorPool)
                == vk::Result::eSuccess };

    if (built)
    {
        const vk::DescriptorSetAllocateInfo allocInfo { meshDescriptorPool, meshDescriptorSetLayout };
        built = device.getDevice().allocateDescriptorSets (&allocInfo, &meshDescriptorSet) == vk::Result::eSuccess;
    }

    return built;
}

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam