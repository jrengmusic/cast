namespace jam
{
/*____________________________________________________________________________*/
//==============================================================================
// Constructor / Destructor
//==============================================================================

VulkanGraphics::VulkanGraphics (VulkanDevice& vulkanDevice, void* nativeHandle, uint32_t bindlessCapacity)
    : device (vulkanDevice)
    , swapchain (vulkanDevice)
    , msaaCalibration (vulkanDevice)
    , imageEffects (vulkanDevice, pipelines)
    , bindlessTextureCapacity (bindlessCapacity)
    , stagingArena (vulkanDevice)
    , bindlessInstance (nativeHandle != nullptr ? nativeHandle : this)
    , bindlessRegistry (bindlessInstance.getHandle(), bindlessCapacity)
    , transparencyStack (vulkanDevice)
    , windingScratch (vulkanDevice)
#if KUASSA_VULKAN_RUNTIME_SHADER_COMPILER
    , shaderRegistry (vulkanDevice, pipelines)
#endif
{
}

VulkanGraphics::~VulkanGraphics()
{
    if (device.getDevice() != nullptr)
    {
        const vk::Result waitResult { device.getDevice().waitIdle() };
        jassert (waitResult == vk::Result::eSuccess or waitResult == vk::Result::eErrorDeviceLost);
        juce::ignoreUnused (waitResult);
    }

    // bindlessInstance's own destructor and bindlessRegistry's own destructor
    // (RAII members, destroyed alongside every other member below) withdraw
    // this window's registered slot assignments — glyph-atlas types via
    // bindlessInstance, per-root image slots via bindlessRegistry — see each
    // class's own doc comment (jam_VulkanBindlessInstance.h,
    // jam_VulkanBindlessRegistry.h). No manual cleanup needed here.

    // Raw Vulkan handles — unconditional destroy (an empty handle is a no-op per Vulkan spec)
    device.getDevice().destroyDescriptorPool (bindlessDescriptorPool, nullptr);
    device.getDevice().destroyDescriptorPool (descriptorPool, nullptr);
    device.getDevice().destroyFence (inFlightFence, nullptr);
    device.getDevice().destroyFence (offscreenFence, nullptr);
    device.getDevice().destroyCommandPool (commandPool, nullptr);

    // Linear sampler teardown — shared by all drawImage calls.
    device.getDevice().destroySampler (linearSampler, nullptr);

    // VulkanShader-execution shared setup — every VulkanShaderInstance
    // instance (shaderRegistry's own shaderInstances/previousShaderInstances)
    // owns its own per-VulkanShader handles and self-destructs via RAII; the
    // SHARED handles the runtime-shader-compiler lane itself owns are torn
    // down by shaderRegistry's own destructor (a RAII member, destroyed
    // alongside every other member below).
    device.getDevice().destroySampler (nearestSampler, nullptr);

    for (auto& framebuffer : swapchainFramebuffers)
        device.getDevice().destroyFramebuffer (framebuffer, nullptr);
#if JUCE_WINDOWS
    device.getDevice().destroyFramebuffer (compositionFramebuffer, nullptr);
#endif

    device.getDevice().destroyFramebuffer (sceneFramebuffer, nullptr);
#if KUASSA_VULKAN_RUNTIME_SHADER_COMPILER
    device.getDevice().destroyFramebuffer (straightAlphaFramebuffer, nullptr);
#endif

    device.getDevice().destroyPipeline (compositePipeline, nullptr);

    pipelines.shutdown (device.getDevice());
    device.getDevice().destroyRenderPass (renderPassLoad, nullptr);
    device.getDevice().destroyRenderPass (renderPass, nullptr);
    device.getDevice().destroyRenderPass (compositeRenderPass, nullptr);

    // Remaining members are RAII and self-destruct in reverse declaration order —
    // including swapchain, whose own destructor orders WSI teardown per spec
    // (views, then semaphores, then swapchain, then surface).
}

//==============================================================================
// Static factory
//==============================================================================

std::unique_ptr<VulkanGraphics> VulkanGraphics::create (VulkanDevice& vulkanDevice,
                                                        void* nativeHandle,
                                                        int width,
                                                        int height,
                                                        double targetFrameBudgetMs,
                                                        vk::PipelineCache pipelineCache
#if JUCE_WINDOWS
                                                        ,
                                                        bool opaquePeer
#endif
)
{
    // Bindless texture array sizing, decided once here (against the device's
    // queried hardware ceiling) and never re-evaluated afterward. The queried descriptor-
    // indexing capabilities are hard requirements for this engine (asserted at device
    // creation) — no capability branch, only a sizing clamp. bindlessSampledImageArrayCount
    // bindless arrays (binding 0, 3, 4) all draw from the same hardware-queried
    // maxDescriptorSetUpdateAfterBindSampledImages budget (see bindlessSampledImageArrayCount's
    // doc comment) — the clamp divides that budget across them instead of assuming
    // binding 0 alone consumes it.
    const uint32_t bindlessTextureCapacity {
        std::min (maxBindlessTextures,
                  vulkanDevice.getMaxDescriptorSetUpdateAfterBindSampledImages() / bindlessSampledImageArrayCount)
    };

    auto graphics { std::make_unique<VulkanGraphics> (vulkanDevice, nativeHandle, bindlessTextureCapacity) };

    graphics->pathFrameBuffer = VulkanFrameBuffer (vulkanDevice.getAllocator(), initialPathCapacity, pathVertexStride);
    graphics->primitiveRecordBuffer =
        VulkanPrimitiveRecordBuffer (vulkanDevice.getAllocator(), initialPrimitiveRecordCapacity);

    // By deliberate ordering, calibrateSampleCount() runs right after
    // swapchain creation, BEFORE createRenderPass()/pipelines.load(). It only needs
    // commandPool (VulkanGraphics-owned, its own command buffer allocations) and
    // timestampPool (msaaCalibration-owned, its own timestamp queries) — neither
    // coupled to VulkanPipelines — so createCommandPool()/msaaCalibration.createTimestampPool()
    // are sequenced immediately before it.
    // measureCandidateSampleCount() is fully self-contained (own render pass,
    // framebuffer, empty pipeline layout, calibration.vert/frag pipeline — see its
    // doc comment) — this eliminates the prior circular dependency where calibration
    // needed the real VulkanPipelines layout, which itself needed createRenderPass(), which
    // needed activeSampleCount from calibration. createRenderPass() now builds the
    // MAIN render pass ONCE, correctly, at the now-known activeSampleCount — no
    // rebuild, no double-build.
#if JUCE_WINDOWS
    bool ready { false };

    if (opaquePeer)
    {
        ready = graphics->swapchain.create (nativeHandle, width, height);
    }
    else if (vulkanDevice.isCompositionInteropSupported())
    {
        graphics->composition =
            VulkanComposition::create (vulkanDevice.getCompositionDevice(), static_cast<HWND> (nativeHandle));

        ready = graphics->composition != nullptr;

        if (ready)
        {
            const vk::SurfaceFormatKHR compositionFormat {
                vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear
            };
            const vk::Extent2D compositionExtent {
                graphics->composition->getClientExtent (static_cast<HWND> (nativeHandle))
            };

            graphics->swapchain.setFormatAndExtent (compositionFormat, compositionExtent);
        }
    }
#else
    bool ready { graphics->swapchain.create (nativeHandle, width, height) };
#endif

    ready = ready and graphics->createCommandPool();

    ready = ready and graphics->msaaCalibration.createTimestampPool();

    if (ready)
        graphics->msaaCalibration.calibrateSampleCount (
            targetFrameBudgetMs, graphics->swapchain.getFormat(), graphics->swapchain.getExtent(), graphics->commandPool);

    // createStencilImage() (MSAA) and createSceneTarget() (scene MSAA
    // color + resolve) both need activeSampleCount, already locked above. Both run
    // BEFORE createRenderPass()/createFramebuffers() (the framebuffers need their
    // vk::ImageViews to already exist). createCompositePipeline() runs AFTER
    // pipelines.load()/createDrawState() — it needs the real pipeline layout and the
    // bindless/SSBO descriptor sets those two build.
    ready = ready and graphics->createStencilImage();

    ready = ready and graphics->createSceneTarget (graphics->swapchain.getExtent());

#if KUASSA_VULKAN_RUNTIME_SHADER_COMPILER
    ready = ready and graphics->createStraightAlphaTarget (graphics->swapchain.getExtent());
#endif

    ready = ready and graphics->createRenderPass();

#if JUCE_WINDOWS
    ready = ready and (graphics->composition == nullptr or graphics->importCompositionImage());
#endif

    ready = ready and graphics->createFramebuffers();

    ready = ready and graphics->createSyncObjects();

    ready = ready
            and graphics->pipelines.load (vulkanDevice.getDevice(),
                                          graphics->renderPass,
                                          graphics->swapchain.getExtent(),
                                          graphics->bindlessTextureCapacity,
                                          graphics->getActiveSampleCount(),
                                          pipelineCache);

    ready = ready and graphics->createDrawState();

    ready = ready and graphics->createCompositePipeline (pipelineCache);

    ready = ready and graphics->pathFrameBuffer.isValid() and graphics->primitiveRecordBuffer.isValid();

    // updateSceneColorBindlessDescriptor()/updateStraightAlphaBindlessDescriptor()
    // need bindlessTextureDescriptorSet, which createDrawState() only just created
    // above — cannot run any earlier (see createSceneTarget()'s doc comment).
    if (ready)
    {
        graphics->updateSceneColorBindlessDescriptor();
#if KUASSA_VULKAN_RUNTIME_SHADER_COMPILER
        graphics->updateStraightAlphaBindlessDescriptor();
#endif

        // Seed the transparency stack with the initial creation params — no layers
        // exist yet, so this stores renderPass/format/sampleCount/extent only.
        jam::Array<int> noIndices;
        graphics->transparencyStack.resize (graphics->renderPass,
                                            graphics->swapchain.getFormat(),
                                            graphics->getActiveSampleCount(),
                                            graphics->swapchain.getExtent(),
                                            noIndices);

        auto* atlas { jam::GlyphAtlas::getInstance() };
        jassert (atlas != nullptr);
        graphics->registerGlyphAtlasSlots (*atlas);
    }

    return ready ? std::move (graphics) : nullptr;
}

std::unique_ptr<VulkanGraphics> VulkanGraphics::createOffscreen (VulkanDevice& vulkanDevice,
                                                                 double targetFrameBudgetMs,
                                                                 vk::PipelineCache pipelineCache,
                                                                 vk::Extent2D maxImageExtent)
{
    // Each offscreen instance keys the bindless registry by its own address;
    // windows key by native handle; distinct by construction.
    const uint32_t bindlessTextureCapacity {
        std::min (maxBindlessTextures,
                  vulkanDevice.getMaxDescriptorSetUpdateAfterBindSampledImages() / bindlessSampledImageArrayCount)
    };

    auto graphics { std::make_unique<VulkanGraphics> (vulkanDevice, nullptr, bindlessTextureCapacity) };
    graphics->pathFrameBuffer = VulkanFrameBuffer (vulkanDevice.getAllocator(), initialPathCapacity, pathVertexStride);
    graphics->primitiveRecordBuffer =
        VulkanPrimitiveRecordBuffer (vulkanDevice.getAllocator(), initialPrimitiveRecordCapacity);

    jassert (maxImageExtent.width > 0 and maxImageExtent.height > 0);
    graphics->swapchain.setFormatAndExtent (
        vk::SurfaceFormatKHR { vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear }, maxImageExtent);

    bool ready { graphics->createCommandPool() };
    ready = ready and graphics->msaaCalibration.createTimestampPool();
    if (ready)
        graphics->msaaCalibration.calibrateSampleCount (
            targetFrameBudgetMs, graphics->swapchain.getFormat(), graphics->swapchain.getExtent(), graphics->commandPool);

    ready = ready and graphics->createRenderPass()
            and graphics->pipelines.load (vulkanDevice.getDevice(),
                                          graphics->renderPass,
                                          graphics->swapchain.getExtent(),
                                          graphics->bindlessTextureCapacity,
                                          graphics->getActiveSampleCount(),
                                          pipelineCache)
            and graphics->createDrawState() and graphics->pathFrameBuffer.isValid()
            and graphics->primitiveRecordBuffer.isValid();

    if (ready)
    {
        graphics->offscreenInstance = true;

        const auto [fenceResult, fence] { vulkanDevice.getDevice().createFence ({}) };
        const FrameDisposition fenceDisposition { getDisposition (
            getSuccessOnlyDispositions(),
            fenceResult,
            FrameDisposition::skip,
            "jam::VulkanGraphics::createOffscreen: unhandled createFence result ") };

        ready = fenceDisposition == FrameDisposition::proceed;
        graphics->offscreenFence = fence;
    }

    if (ready)
    {
        // Seed the transparency stack with renderPass/format/sampleCount and
        // maxImageExtent (the layout-declared design extent); beginOffscreenFrame
        // re-seeds per target bind when the extent differs.
        jam::Array<int> noIndices;
        graphics->transparencyStack.resize (graphics->renderPass,
                                            graphics->swapchain.getFormat(),
                                            graphics->getActiveSampleCount(),
                                            graphics->swapchain.getExtent(),
                                            noIndices);

        auto* atlas { jam::GlyphAtlas::getInstance() };
        jassert (atlas != nullptr);
        graphics->registerGlyphAtlasSlots (*atlas);
    }

    return ready ? std::move (graphics) : nullptr;
}

//==============================================================================
// Frame lifecycle
//==============================================================================

VulkanGraphics::FrameDisposition VulkanGraphics::getDisposition (
    const jam::HashMap<vk::Result, VulkanGraphics::FrameDisposition>& dispositions,
    vk::Result result,
    VulkanGraphics::FrameDisposition fallback,
    juce::StringRef context)
{
    if (dispositions.contains (result))
        return dispositions.at (result);

#if JUCE_DEBUG
    jam::debug::Log::write (juce::String (context) + juce::String (vk::to_string (result)));
#endif

    return fallback;
}

const jam::HashMap<vk::Result, VulkanGraphics::FrameDisposition>&
    VulkanGraphics::getDeviceLostTolerantDispositions()
{
    static const jam::HashMap<vk::Result, VulkanGraphics::FrameDisposition> dispositions {
        { vk::Result::eSuccess, VulkanGraphics::FrameDisposition::proceed },
        { vk::Result::eErrorOutOfHostMemory, VulkanGraphics::FrameDisposition::skip },
        { vk::Result::eErrorOutOfDeviceMemory, VulkanGraphics::FrameDisposition::skip },
        { vk::Result::eErrorDeviceLost, VulkanGraphics::FrameDisposition::reinitialise },
    };

    return dispositions;
}

const jam::HashMap<vk::Result, VulkanGraphics::FrameDisposition>&
    VulkanGraphics::getSuccessOnlyDispositions()
{
    static const jam::HashMap<vk::Result, VulkanGraphics::FrameDisposition> dispositions {
        { vk::Result::eSuccess, VulkanGraphics::FrameDisposition::proceed },
    };

    return dispositions;
}

void VulkanGraphics::deferDeviceReinitialise()
{
    juce::MessageManager::callAsync ([]
    {
        if (auto* reinitialiseEngine { VulkanEngine::getInstance() })
            reinitialiseEngine->reinitialiseDevice();
    });
}

bool VulkanGraphics::beginFrame()
{
    JUCE_ASSERT_MESSAGE_THREAD

    if (activeContextCount > 0)
        return true;

    const vk::Result waitFencesResult { device.getDevice().waitForFences (inFlightFence, vk::True, UINT64_MAX) };

    const FrameDisposition waitDisposition { getDisposition (
        getDeviceLostTolerantDispositions(),
        waitFencesResult,
        FrameDisposition::skip,
        "jam::VulkanGraphics::beginFrame: unhandled waitForFences result ") };

    if (waitDisposition == FrameDisposition::reinitialise)
        deferDeviceReinitialise();

    if (waitDisposition != FrameDisposition::proceed)
        return false;

#if JUCE_MAC
    // Self-heal against the surface's current reality before acquiring: metalLayerRef's
    // own frame tracks the backing NSView's bounds via CALayer autoresizing regardless
    // of whether resize() was ever called for this geometry (a plugin pane's NSView is
    // windowless/unlaid-out at VulkanGraphics::create() time, so creation-time sizing
    // can be stale or zero). isSwapchainStale folds into the same trigger — a stale
    // (out-of-date/surface-lost) swapchain heals through this logical-geometry resize
    // rather than a separate physical-extent/scale-1.0 call, which would corrupt the
    // metal layer's contentsScale.
    if (swapchain.getMetalLayerRef() != nullptr)
    {
        const auto [currentWidth, currentHeight] { getMetalLayerSize (swapchain.getMetalLayerRef()) };
        const float currentScale { getScaleFactor (getNativeHandle()) };

        const auto currentPhysicalWidth { static_cast<uint32_t> (currentWidth * currentScale) };
        const auto currentPhysicalHeight { static_cast<uint32_t> (currentHeight * currentScale) };

        const bool zeroSizeSkip { currentPhysicalWidth == 0 or currentPhysicalHeight == 0 };
        const bool healTriggered { not zeroSizeSkip
                                   and (currentPhysicalWidth != swapchain.getExtent().width
                                        or currentPhysicalHeight != swapchain.getExtent().height
                                        or isSwapchainStale) };

        if (zeroSizeSkip)
            return false;

        if (healTriggered)
        {
            resize (currentWidth, currentHeight, currentScale);
            isSwapchainStale = false;
        }
    }
#else
    if (isSwapchainStale)
    {
        resize (static_cast<int> (swapchain.getExtent().width), static_cast<int> (swapchain.getExtent().height));
        isSwapchainStale = false;
    }
#endif

    FrameDisposition disposition { FrameDisposition::proceed };
    bool acquiredSwapchainImage { false };

#if JUCE_WINDOWS
    if (composition == nullptr)
    {
#endif
        const auto [acquireResult, acquiredImageIndex] { swapchain.acquireNextImage() };

        static const jam::HashMap<vk::Result, VulkanGraphics::FrameDisposition> acquireDispositions {
            { vk::Result::eSuccess, VulkanGraphics::FrameDisposition::proceed },
            { vk::Result::eSuboptimalKHR, VulkanGraphics::FrameDisposition::proceed },
            { vk::Result::eErrorOutOfHostMemory, VulkanGraphics::FrameDisposition::skip },
            { vk::Result::eErrorOutOfDeviceMemory, VulkanGraphics::FrameDisposition::skip },
            { vk::Result::eErrorOutOfDateKHR, VulkanGraphics::FrameDisposition::stale },
            { vk::Result::eErrorSurfaceLostKHR, VulkanGraphics::FrameDisposition::stale },
#if JUCE_WINDOWS
            { vk::Result::eErrorFullScreenExclusiveModeLostEXT, VulkanGraphics::FrameDisposition::stale },
#endif
            { vk::Result::eErrorDeviceLost, VulkanGraphics::FrameDisposition::reinitialise },
        };

        disposition = getDisposition (acquireDispositions,
                                      acquireResult,
                                      FrameDisposition::skip,
                                      "jam::VulkanGraphics::beginFrame: unhandled acquireNextImageKHR result ");

        acquiredSwapchainImage = disposition == FrameDisposition::proceed;

        if (acquiredSwapchainImage)
            currentImageIndex = acquiredImageIndex;
#if JUCE_WINDOWS
    }
#endif

    if (disposition == FrameDisposition::reinitialise)
        deferDeviceReinitialise();

    if (disposition == FrameDisposition::stale)
        isSwapchainStale = true;

    if (disposition == FrameDisposition::proceed)
    {
        // No frame is recording on the message thread here — one waitForFences
        // safely covers every window's in-flight GPU work, so any texture the
        // shared cache retired since the last drain can be destroyed now.
        if (auto* retiredTextureCache { VulkanTextureCache::getInstance() }; retiredTextureCache != nullptr)
            retiredTextureCache->destroyRetiredTextures();

        if (beginRecording())
        {
            resetResources();
            beginSceneRenderPass();
        }
        else
        {
            if (acquiredSwapchainImage)
                isSwapchainStale = true;

            disposition = FrameDisposition::skip;
        }
    }

    return disposition == FrameDisposition::proceed;
}

void VulkanGraphics::resetResources()
{
    // Release every slot VulkanWindingScratch moved to previousBindlessIndices since
    // the last drain (deferred release — see
    // VulkanWindingScratch::getPreviousBindlessIndices()'s doc comment) BEFORE
    // resetPrevious() clears the list; beginFrame()'s fence wait (already
    // completed by the time this method runs) is windingScratch's own proof
    // the GPU is done with them.
    for (const int previousWindingScratchIndex : windingScratch.getPreviousBindlessIndices())
        bindlessRegistry.releaseBindlessIndex (previousWindingScratchIndex);

    windingScratch.resetPrevious();

    // Release every slot imageDataBeingDeleted()/imageDataChanged() moved to
    // bindlessRegistry's own retired list since the last drain — same
    // proven-safe post-fence-wait point as windingScratch above.
    bindlessRegistry.releaseRetiredIndices();

    stagingArena.resetResources();

    ++frameCounter;

    {
        auto* atlas { jam::GlyphAtlas::getInstance() };
        jassert (atlas != nullptr);
        atlas->advanceFrame();
    }

#if KUASSA_VULKAN_RUNTIME_SHADER_COMPILER
    // Drains previousShaderInstances (releasing every entry's buffer-pass
    // ping-pong/gather/external-texture bindless slots first, this same
    // post-fence-wait proven-safe point), releases every LIVE shaderInstances
    // entry's own pending mesh staging buffer, then sweeps shaderInstances
    // for any live entry not re-stamped within the swapchain's own image
    // count worth of frames (an owner-replaced VulkanShader's now-unreachable
    // entry — VulkanGraphics::getOrCreateShaderInstance()'s own doc comment),
    // moving each into previousShaderInstances exactly like a
    // stale-generation rebuild. getImageCount() is 0 on the composition/
    // offscreen lanes (no per-image views are created there), so the bound
    // is clamped to at least one frame inside releaseRetiredInstances() itself.
    shaderRegistry.releaseRetiredInstances (bindlessRegistry, frameCounter,
                                            static_cast<int> (swapchain.getImageCount()), getNativeHandle());
#endif
}

void VulkanGraphics::endFrame()
{
    JUCE_ASSERT_MESSAGE_THREAD

    --activeContextCount;

    if (offscreenInstance and not activeRecordings.isEmpty())
    {
        jassert (activeContextCount == activeRecordings.size() - 1);

        endRenderPass();

        const vk::Image innerResolveImage {
            recordings.at (static_cast<size_t> (activeRecordings.last()))->resolveImage
        };

        activeRecordings.remove (activeRecordings.size() - 1);

        if (not activeRecordings.isEmpty())
        {
            auto& outer { *recordings.at (static_cast<size_t> (activeRecordings.last())) };

            // Allocate a FRESH set2 written to primitiveRecordBuffer's
            // current handle — the inner recording may have grown the
            // buffer, changing its vk::Buffer handle. A new allocation
            // avoids updating a potentially-bound descriptor set.
            const vk::DescriptorSetLayout set2 { pipelines.getLayoutSet2() };
            const vk::DescriptorSetAllocateInfo recordAllocInfo { descriptorPool, set2 };

            vk::DescriptorSet freshRecordSet {};
            const vk::Result allocResult { device.getDevice().allocateDescriptorSets (
                &recordAllocInfo, &freshRecordSet) };
            const FrameDisposition allocDisposition { getDisposition (
                getSuccessOnlyDispositions(),
                allocResult,
                FrameDisposition::skip,
                "jam::VulkanGraphics::endFrame: unhandled allocateDescriptorSets (record) result ") };

            if (allocDisposition == FrameDisposition::proceed)
            {
                const vk::DescriptorBufferInfo recordBufferInfo { primitiveRecordBuffer.getBuffer(), 0, vk::WholeSize };
                const vk::WriteDescriptorSet recordWrite {
                    freshRecordSet, 0, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr, &recordBufferInfo
                };
                device.getDevice().updateDescriptorSets (recordWrite, nullptr);

                outer.recordDescriptorSet = freshRecordSet;

                projectionDescriptorSet = outer.projectionDescriptorSet;
                recordDescriptorSet = outer.recordDescriptorSet;

                // Memory dependency: the inner pass's resolve write must be visible
                // to the outer pass's fragment-shader sampled read.
                recordImageMemoryBarrier (commandBuffer, innerResolveImage,
                                          vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageLayout::eShaderReadOnlyOptimal,
                                          vk::ImageAspectFlagBits::eColor,
                                          vk::AccessFlagBits::eColorAttachmentWrite, vk::AccessFlagBits::eShaderRead,
                                          vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eFragmentShader);

                resumeRenderPass (outer.framebuffer, outer.extent);
            }
        }
        else
        {
            submitOffscreenAndWait();
        }

        return;
    }

    if (activeContextCount == 0)
    {
        if (renderPassActive)
            endRenderPass();

#if JUCE_WINDOWS
        if (composition != nullptr)
            recordCompositionAcquireBarrier();
#endif

#if KUASSA_VULKAN_RUNTIME_SHADER_COMPILER
        // Post-process chain (VulkanEngine-owned, app-global — a live
        // VulkanEngine is guaranteed for the lifetime of any VulkanGraphics, since
        // VulkanGraphics only exists inside VulkanEngine::contexts): when installed
        // and its GPU execution resources are ready,
        // recordPostProcessCompositeDrawCommands() runs INSTEAD of the
        // minimal identity composite below — this frame's swapchain receives
        // the post-process chain's own gather (offscreen, raw colour) +
        // combine (post_process_combine.frag, sampling the gather target,
        // applying the effect-intensity mix) output. Otherwise the minimal
        // identity composite below (the scene render pass just ended above,
        // its resolve having just written sceneColorImage) copies the scene
        // onto the swapchain unchanged.
        auto* engine { jam::VulkanEngine::getInstance() };
        jassert (engine != nullptr);
        const auto postProcess { engine->getPostProcess() };

        // chainInputExtent = swapchain.getExtent() -- the actual straight-alpha
        // scene extent, per the per-pass render-target extent contract
        // (each pass independently resolves its own scaled extent) -- this
        // is the post-process composite's own basis for pass 0's own "source"-typed
        // scale directive (VulkanGraphics::getOrCreateShaderInstance()'s own doc
        // comment).
        // Mouse capture into the post-process chain is a separate follow-up
        // task — no per-frame iMouse state exists to report yet.
        static constexpr std::array<float, 4> noMouseInteraction { 0.0f, 0.0f, 0.0f, 0.0f };

        // getOrCreateShaderInstance() self-manages its own render-pass bracket
        // (suspends/resumes only when a build actually records transfer
        // commands, gated on renderPassActive — its own doc comment,
        // jam_VulkanGraphics.h). Here the scene pass was already ended
        // unconditionally a few lines above, so the bracket is a no-op
        // either way.
        if (postProcess.shader != nullptr
            and getOrCreateShaderInstance (*postProcess.shader, postProcess.resolutionScale, swapchain.getExtent()))
            recordPostProcessCompositeDrawCommands (*postProcess.shader,
                                                    shaderRegistry.getShaderInstance (*postProcess.shader),
                                                    postProcess.opacity,
                                                    postProcess.resolutionScale,
                                                    noMouseInteraction);
        else
            recordSceneCompositeDrawCommands();
#else
        recordSceneCompositeDrawCommands();
#endif

#if JUCE_WINDOWS
        if (composition != nullptr)
            recordCompositionReleaseBarrier();
#endif

        const vk::Result endCommandBufferResult { commandBuffer.end() };
        const FrameDisposition endDisposition { getDisposition (
            getSuccessOnlyDispositions(),
            endCommandBufferResult,
            FrameDisposition::skip,
            "jam::VulkanGraphics::endFrame: unhandled commandBuffer.end result ") };

        if (endDisposition == FrameDisposition::proceed)
        {
            const vk::Result resetFenceResult { device.getDevice().resetFences (inFlightFence) };
            const FrameDisposition resetFenceDisposition { getDisposition (
                getSuccessOnlyDispositions(),
                resetFenceResult,
                FrameDisposition::skip,
                "jam::VulkanGraphics::endFrame: unhandled resetFences result ") };

            if (resetFenceDisposition == FrameDisposition::proceed)
            {
                FrameDisposition submitDisposition { FrameDisposition::proceed };

#if JUCE_WINDOWS
                if (composition != nullptr)
                {
                    const vk::SubmitInfo submitInfo { nullptr, nullptr, commandBuffer };

                    const vk::Result submitResult { device.getQueue().submit (1, &submitInfo, inFlightFence) };
                    submitDisposition = getDisposition (
                        getDeviceLostTolerantDispositions(),
                        submitResult,
                        FrameDisposition::skip,
                        "jam::VulkanGraphics::endFrame: unhandled queue.submit result ");

                    if (submitDisposition == FrameDisposition::proceed)
                    {
                        const vk::Result waitFencesResult { device.getDevice().waitForFences (
                            inFlightFence, vk::True, UINT64_MAX) };
                        submitDisposition = getDisposition (
                            getDeviceLostTolerantDispositions(),
                            waitFencesResult,
                            FrameDisposition::skip,
                            "jam::VulkanGraphics::endFrame: unhandled waitForFences result ");
                    }
                }
                else
                {
#endif
                    const vk::PipelineStageFlags waitStage { vk::PipelineStageFlagBits::eColorAttachmentOutput };
                    const vk::Semaphore imageAvailableSemaphore { swapchain.getImageAvailableSemaphore() };
                    const vk::Semaphore renderFinishedSemaphore { swapchain.getRenderFinishedSemaphore (static_cast<int> (currentImageIndex)) };
                    const vk::SubmitInfo submitInfo { imageAvailableSemaphore,
                                                      waitStage,
                                                      commandBuffer,
                                                      renderFinishedSemaphore };

                    const vk::Result submitResult { device.getQueue().submit (1, &submitInfo, inFlightFence) };
                    submitDisposition = getDisposition (
                        getDeviceLostTolerantDispositions(),
                        submitResult,
                        FrameDisposition::skip,
                        "jam::VulkanGraphics::endFrame: unhandled queue.submit result ");
#if JUCE_WINDOWS
                }
#endif

                if (submitDisposition == FrameDisposition::reinitialise)
                    deferDeviceReinitialise();

                if (submitDisposition == FrameDisposition::proceed)
                {
                    // This frame's scene attachments are now queued with real content —
                    // the next frame may resumeRenderPass() (LOAD) instead of clearing.
                    sceneContentValid = true;

#if JUCE_WINDOWS
                    if (composition != nullptr)
                    {
                        composition->present (device.getCompositionDevice());
                    }
                    else
                    {
#endif
                        const vk::Result presentResult { swapchain.present (
                            device.getQueue(),
                            currentImageIndex,
                            swapchain.getRenderFinishedSemaphore (static_cast<int> (currentImageIndex))) };

                        static const jam::HashMap<vk::Result, VulkanGraphics::FrameDisposition> presentDispositions {
                            { vk::Result::eSuccess, VulkanGraphics::FrameDisposition::proceed },
                            { vk::Result::eSuboptimalKHR, VulkanGraphics::FrameDisposition::proceed },
                            { vk::Result::eErrorOutOfHostMemory, VulkanGraphics::FrameDisposition::skip },
                            { vk::Result::eErrorOutOfDeviceMemory, VulkanGraphics::FrameDisposition::skip },
                            { vk::Result::eErrorOutOfDateKHR, VulkanGraphics::FrameDisposition::stale },
                            { vk::Result::eErrorSurfaceLostKHR, VulkanGraphics::FrameDisposition::stale },
#if JUCE_WINDOWS
                            { vk::Result::eErrorFullScreenExclusiveModeLostEXT, VulkanGraphics::FrameDisposition::stale },
#endif
                            { vk::Result::eErrorDeviceLost, VulkanGraphics::FrameDisposition::reinitialise },
                        };

                        const FrameDisposition disposition { getDisposition (
                            presentDispositions,
                            presentResult,
                            FrameDisposition::stale,
                            "jam::VulkanGraphics::endFrame: unhandled presentKHR result ") };

                        if (disposition == FrameDisposition::reinitialise)
                            deferDeviceReinitialise();

                        if (disposition == FrameDisposition::stale)
                            isSwapchainStale = true;
#if JUCE_WINDOWS
                    }
#endif
                }
            }
        }
    }
}

void VulkanGraphics::beginRenderPass (vk::Framebuffer framebuffer, vk::Extent2D extent)
{
    if (not renderPassActive)
    {
        // One entry per framebuffer attachment (MSAA color, MSAA stencil, resolve —
        // RenderPassAttachment). clearValueCount must equal attachmentCount;
        // RenderPassAttachment::resolve's entry is unused (that attachment is
        // LOAD_OP_DONT_CARE) but the array still needs an entry at that slot.
        std::array<vk::ClearValue, renderPassAttachmentCount> clearValues {};
        clearValues.at (static_cast<size_t> (RenderPassAttachment::msaaColor)).color =
            vk::ClearColorValue { 0.0f, 0.0f, 0.0f, 0.0f };
        clearValues.at (static_cast<size_t> (RenderPassAttachment::stencil)).depthStencil =
            vk::ClearDepthStencilValue { 0.0f, 0 };

        const vk::RenderPassBeginInfo renderPassInfo {
            renderPass, framebuffer, vk::Rect2D { { 0, 0 }, extent },
               clearValues
        };

        commandBuffer.beginRenderPass (&renderPassInfo, vk::SubpassContents::eInline);
        renderPassActive = true;

        // Initialise dynamic stencil state — MoltenVK may read these even when
        // stencilTestEnable is vk::False on the bound pipeline.
        commandBuffer.setStencilCompareMask (vk::StencilFaceFlagBits::eFrontAndBack, 0xFF);
        commandBuffer.setStencilWriteMask (vk::StencilFaceFlagBits::eFrontAndBack, 0x00);
        commandBuffer.setStencilReference (vk::StencilFaceFlagBits::eFrontAndBack, 0);
    }
}

void VulkanGraphics::beginRenderPass() { beginRenderPass (sceneFramebuffer, swapchain.getExtent()); }

void VulkanGraphics::endRenderPass()
{
    jassert (renderPassActive);

    if (renderPassActive)
    {
        commandBuffer.endRenderPass();
        renderPassActive = false;
    }
}

void VulkanGraphics::resumeRenderPass (vk::Framebuffer framebuffer, vk::Extent2D extent)
{
    if (not renderPassActive)
    {
        // All attachments use LOAD_OP_LOAD — clear values are never read, but
        // clearValueCount must still equal attachmentCount (renderPassAttachmentCount,
        // same renderPassLoad shape as every framebuffer built against it). Every
        // entry is dummy/unused.
        const std::array<vk::ClearValue, renderPassAttachmentCount> clearValues {};

        const vk::RenderPassBeginInfo renderPassInfo {
            renderPassLoad, framebuffer, vk::Rect2D { { 0, 0 }, extent },
               clearValues
        };

        commandBuffer.beginRenderPass (&renderPassInfo, vk::SubpassContents::eInline);
        renderPassActive = true;
    }
}

void VulkanGraphics::resumeRenderPass()
{
    if (offscreenInstance)
    {
        jassert (not activeRecordings.isEmpty());
        auto& rec { *recordings.at (static_cast<size_t> (activeRecordings.last())) };

        resumeRenderPass (rec.framebuffer, rec.extent);
    }
    else
    {
        resumeRenderPass (sceneFramebuffer, swapchain.getExtent());
    }
}

void VulkanGraphics::beginSceneRenderPass()
{
    if (sceneContentValid)
    {
        resumeRenderPass();

        // Persisted scene attachments still hold frame-N's stencil
        // values while the LLGC's own currentState.stencilClipDepth
        // restarts at 0 for this frame — clear the stencil aspect
        // now, once, before any clip is recorded against it. Reached
        // only from this frame-start LOAD begin, never from the
        // other resumeRenderPass() call sites (transparency
        // composites, image/glyph uploads, shader passes), which
        // resume nested clip state mid-frame and never route
        // through createContext.
        commandBuffer.clearAttachments (
            vk::ClearAttachment {
                vk::ImageAspectFlagBits::eStencil, 0, vk::ClearValue { vk::ClearDepthStencilValue { 0.0f, 0 } }
        },
            vk::ClearRect { vk::Rect2D { { 0, 0 }, swapchain.getExtent() }, 0, 1 });

        // MoltenVK may read these even when stencilTestEnable is
        // vk::False on the bound pipeline — beginRenderPass()
        // initialises them on the CLEAR path below; this LOAD path
        // never reaches beginRenderPass(), so it needs its own copy.
        commandBuffer.setStencilCompareMask (vk::StencilFaceFlagBits::eFrontAndBack, 0xFF);
        commandBuffer.setStencilWriteMask (vk::StencilFaceFlagBits::eFrontAndBack, 0x00);
        commandBuffer.setStencilReference (vk::StencilFaceFlagBits::eFrontAndBack, 0);
    }
    else
    {
        beginRenderPass();
    }
}

bool VulkanGraphics::beginOffscreenFrame (vk::Framebuffer framebuffer, vk::Extent2D extent, vk::Image resolveImage)
{
    JUCE_ASSERT_MESSAGE_THREAD

    jassert (offscreenInstance);
    jassert (activeRecordings.size() < static_cast<int> (maxRecordingDepth));

    const int depth { activeRecordings.size() };

    if (depth == 0)
    {
        // Depth-0: reset+begin THE commandBuffer, reset descriptor pool, reset arenas.
        const vk::Result resetCommandBufferResult { commandBuffer.reset ({}) };
        const FrameDisposition resetDisposition { getDisposition (
            getSuccessOnlyDispositions(),
            resetCommandBufferResult,
            FrameDisposition::skip,
            "jam::VulkanGraphics::beginOffscreenFrame: unhandled commandBuffer.reset result ") };

        if (resetDisposition != FrameDisposition::proceed)
            return false;

        const vk::CommandBufferBeginInfo beginInfo { vk::CommandBufferUsageFlagBits::eOneTimeSubmit };
        const vk::Result beginCommandBufferResult { commandBuffer.begin (&beginInfo) };
        const FrameDisposition beginDisposition { getDisposition (
            getSuccessOnlyDispositions(),
            beginCommandBufferResult,
            FrameDisposition::skip,
            "jam::VulkanGraphics::beginOffscreenFrame: unhandled commandBuffer.begin result ") };

        if (beginDisposition != FrameDisposition::proceed)
            return false;

        const vk::Result resetDescriptorPoolResult { device.getDevice().resetDescriptorPool (descriptorPool) };
        const FrameDisposition resetPoolDisposition { getDisposition (
            getSuccessOnlyDispositions(),
            resetDescriptorPoolResult,
            FrameDisposition::skip,
            "jam::VulkanGraphics::beginOffscreenFrame: unhandled resetDescriptorPool result ") };

        if (resetPoolDisposition != FrameDisposition::proceed)
            return false;

        if (not imageEffects.createDescriptorSets (descriptorPool, bindlessTextureDescriptorSet))
            return false;

        stagingArena.resetResources();
        imageEffects.releaseRetiredBuffers();
        recordingsUsed = 0;

        pathFrameBuffer.resetUsage();
        primitiveRecordBuffer.resetUsage();
    }

    jassert (recordingsUsed < static_cast<int> (maxRecordingsPerFrame));

    const int takenIndex { recordingsUsed };
    ++recordingsUsed;

    // Get-or-create the recording entry for this index.
    while (recordings.size() <= takenIndex)
    {
        auto rec { std::make_unique<Recording>() };

        // CPU_ONLY + MAPPED_BIT — same coherency rationale as VulkanGraphics::projectionBuffer's
        // own allocation site (jam_VulkanGraphicsSetupDrawState.cpp).
        const vk::BufferCreateInfo projInfo {
            {}, projectionBufferSize, vk::BufferUsageFlagBits::eStorageBuffer, vk::SharingMode::eExclusive
        };
        VmaAllocationCreateInfo projAllocInfo {};
        projAllocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
        projAllocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
        rec->projectionBuffer = VulkanBuffer (device.getAllocator(), projInfo, projAllocInfo);
        jassert (rec->projectionBuffer.isValid());

        recordings.add (std::move (rec));
    }

    auto& rec { *recordings.at (static_cast<size_t> (takenIndex)) };

    rec.framebuffer = framebuffer;
    rec.extent = extent;
    rec.resolveImage = resolveImage;

    // Allocate this recording's projection storage buffer set from the pool.
    const vk::DescriptorSetLayout projectionSetLayout { pipelines.getProjectionLayout() };
    const vk::DescriptorSetAllocateInfo allocInfo { descriptorPool, projectionSetLayout };

    const vk::Result allocProjectionSetResult { device.getDevice().allocateDescriptorSets (
        &allocInfo, &rec.projectionDescriptorSet) };
    const FrameDisposition allocProjectionDisposition { getDisposition (
        getSuccessOnlyDispositions(),
        allocProjectionSetResult,
        FrameDisposition::skip,
        "jam::VulkanGraphics::beginOffscreenFrame: unhandled allocateDescriptorSets (projection) result ") };

    if (allocProjectionDisposition != FrameDisposition::proceed)
        return false;

    const vk::DescriptorBufferInfo bufferDescInfo { rec.projectionBuffer.getBuffer(),
                                                    0,
                                                    projectionBufferSize };
    const vk::WriteDescriptorSet write {
        rec.projectionDescriptorSet, 0, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr, &bufferDescInfo
    };
    device.getDevice().updateDescriptorSets (write, nullptr);

    // Allocate this recording's set2 (primitiveRecordBuffer SSBO) from the pool.
    const vk::DescriptorSetLayout set2 { pipelines.getLayoutSet2() };
    const vk::DescriptorSetAllocateInfo recordAllocInfo { descriptorPool, set2 };

    const vk::Result allocRecordSetResult { device.getDevice().allocateDescriptorSets (
        &recordAllocInfo, &rec.recordDescriptorSet) };
    const FrameDisposition allocRecordDisposition { getDisposition (
        getSuccessOnlyDispositions(),
        allocRecordSetResult,
        FrameDisposition::skip,
        "jam::VulkanGraphics::beginOffscreenFrame: unhandled allocateDescriptorSets (record) result ") };

    if (allocRecordDisposition != FrameDisposition::proceed)
        return false;

    const vk::DescriptorBufferInfo recordBufferInfo { primitiveRecordBuffer.getBuffer(), 0, vk::WholeSize };
    const vk::WriteDescriptorSet recordWrite {
        rec.recordDescriptorSet, 0, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr, &recordBufferInfo
    };
    device.getDevice().updateDescriptorSets (recordWrite, nullptr);

    // Point the mirrors at this recording's sets.
    projectionDescriptorSet = rec.projectionDescriptorSet;
    recordDescriptorSet = rec.recordDescriptorSet;

    // Write the projection ortho for this target's extent into the recording's own storage buffer.
    const glm::mat4 ortho { glm::ortho (
        0.0f, static_cast<float> (extent.width), 0.0f, static_cast<float> (extent.height), -1.0f, 1.0f) };
    std::memcpy (rec.projectionBuffer.getMapped(), &ortho, sizeof (ortho));

    activeRecordings.add (takenIndex);

    // Offscreen targets vary per render — re-seed the transparency stack when the
    // incoming extent differs from the last extent passed to transparencyStack.resize()
    // (tracked by swapchain's own extent, which the createOffscreen seed and this path
    // both maintain as the caller-side proxy for the stack's own stored extent).
    if (extent.width != swapchain.getExtent().width or extent.height != swapchain.getExtent().height)
    {
        const vk::Result waitIdleResult { device.getDevice().waitIdle() };
        const FrameDisposition waitDisposition { getDisposition (
            getDeviceLostTolerantDispositions(),
            waitIdleResult,
            FrameDisposition::skip,
            "jam::VulkanGraphics::beginOffscreenFrame: unhandled waitIdle result ") };

        if (waitDisposition == FrameDisposition::reinitialise)
            deferDeviceReinitialise();

        if (waitDisposition == FrameDisposition::proceed)
        {
            jam::Array<int> previousTransparencyBindlessIndices;
            transparencyStack.resize (
                renderPass, swapchain.getFormat(), getActiveSampleCount(), extent, previousTransparencyBindlessIndices);

            for (const int previousIndex : previousTransparencyBindlessIndices)
                bindlessRegistry.releaseBindlessIndex (previousIndex);

            swapchain.setFormatAndExtent (swapchain.getFormat(), extent);
        }
    }

    if (depth > 0)
    {
        // Suspend the outer's render pass — every fallible allocation for this
        // inner recording has already succeeded above, so the inner's complete
        // pass will be recorded in-stream before the outer resumes with LOAD.
        endRenderPass();
    }

    beginRenderPass (framebuffer, extent);

    return true;
}

bool VulkanGraphics::beginRecording()
{
    const vk::Result resetCommandBufferResult { commandBuffer.reset ({}) };
    const FrameDisposition resetDisposition { getDisposition (
        getSuccessOnlyDispositions(),
        resetCommandBufferResult,
        FrameDisposition::skip,
        "jam::VulkanGraphics::beginRecording: unhandled commandBuffer.reset result ") };

    if (resetDisposition != FrameDisposition::proceed)
        return false;

    const vk::CommandBufferBeginInfo beginInfo { vk::CommandBufferUsageFlagBits::eOneTimeSubmit };
    const vk::Result beginCommandBufferResult { commandBuffer.begin (&beginInfo) };
    const FrameDisposition beginDisposition { getDisposition (
        getSuccessOnlyDispositions(),
        beginCommandBufferResult,
        FrameDisposition::skip,
        "jam::VulkanGraphics::beginRecording: unhandled commandBuffer.begin result ") };

    if (beginDisposition != FrameDisposition::proceed)
        return false;

    const vk::Result resetDescriptorPoolResult { device.getDevice().resetDescriptorPool (descriptorPool) };
    const FrameDisposition resetPoolDisposition { getDisposition (
        getSuccessOnlyDispositions(),
        resetDescriptorPoolResult,
        FrameDisposition::skip,
        "jam::VulkanGraphics::beginRecording: unhandled resetDescriptorPool result ") };

    if (resetPoolDisposition != FrameDisposition::proceed)
        return false;

    if (not imageEffects.createDescriptorSets (descriptorPool, bindlessTextureDescriptorSet))
        return false;

    const vk::DescriptorSetLayout projectionSetLayout { pipelines.getProjectionLayout() };
    const vk::DescriptorSetAllocateInfo allocInfo { descriptorPool, projectionSetLayout };

    const vk::Result allocProjectionSetResult { device.getDevice().allocateDescriptorSets (
        &allocInfo, &projectionDescriptorSet) };
    const FrameDisposition allocProjectionDisposition { getDisposition (
        getSuccessOnlyDispositions(),
        allocProjectionSetResult,
        FrameDisposition::skip,
        "jam::VulkanGraphics::beginRecording: unhandled allocateDescriptorSets (projection) result ") };

    if (allocProjectionDisposition != FrameDisposition::proceed)
        return false;

    const vk::DescriptorBufferInfo bufferDescInfo { projectionBuffer.getBuffer(),
                                                    0,
                                                    projectionBufferSize };
    const vk::WriteDescriptorSet write { projectionDescriptorSet, 0, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr,
                                         &bufferDescInfo };
    device.getDevice().updateDescriptorSets (write, nullptr);

    const vk::DescriptorSetLayout set2 { pipelines.getLayoutSet2() };
    const vk::DescriptorSetAllocateInfo recordAllocInfo { descriptorPool, set2 };

    const vk::Result allocRecordSetResult { device.getDevice().allocateDescriptorSets (
        &recordAllocInfo, &recordDescriptorSet) };
    const FrameDisposition allocRecordDisposition { getDisposition (
        getSuccessOnlyDispositions(),
        allocRecordSetResult,
        FrameDisposition::skip,
        "jam::VulkanGraphics::beginRecording: unhandled allocateDescriptorSets (record) result ") };

    if (allocRecordDisposition != FrameDisposition::proceed)
        return false;

    updateRecordDescriptorSet();

    pathFrameBuffer.resetUsage();
    primitiveRecordBuffer.resetUsage();

    return true;
}

void VulkanGraphics::submitOffscreenAndWait()
{
    JUCE_ASSERT_MESSAGE_THREAD

    const vk::Result endResult { commandBuffer.end() };
    const FrameDisposition endDisposition { getDisposition (
        getSuccessOnlyDispositions(),
        endResult,
        FrameDisposition::skip,
        "jam::VulkanGraphics::submitOffscreenAndWait: unhandled commandBuffer.end result ") };

    if (endDisposition != FrameDisposition::proceed)
        return;

    const vk::SubmitInfo submitInfo { nullptr, nullptr, commandBuffer };

    const vk::Result submitResult { device.getQueue().submit (1, &submitInfo, offscreenFence) };
    const FrameDisposition submitDisposition { getDisposition (
        getDeviceLostTolerantDispositions(),
        submitResult,
        FrameDisposition::skip,
        "jam::VulkanGraphics::submitOffscreenAndWait: unhandled queue.submit result ") };

    if (submitDisposition == FrameDisposition::reinitialise)
        deferDeviceReinitialise();

    if (submitDisposition != FrameDisposition::proceed)
        return;

    const vk::Result waitResult { device.getDevice().waitForFences (offscreenFence, vk::True, UINT64_MAX) };
    const FrameDisposition waitDisposition { getDisposition (
        getDeviceLostTolerantDispositions(),
        waitResult,
        FrameDisposition::skip,
        "jam::VulkanGraphics::submitOffscreenAndWait: unhandled waitForFences result ") };

    if (waitDisposition == FrameDisposition::reinitialise)
        deferDeviceReinitialise();

    if (waitDisposition != FrameDisposition::proceed)
        return;

    const vk::Result resetFenceResult { device.getDevice().resetFences (offscreenFence) };
    const FrameDisposition resetFenceDisposition { getDisposition (
        getSuccessOnlyDispositions(),
        resetFenceResult,
        FrameDisposition::skip,
        "jam::VulkanGraphics::submitOffscreenAndWait: unhandled resetFences result ") };

    if (resetFenceDisposition != FrameDisposition::proceed)
        return;

    // beginOffscreenFrame() never calls resetResources() — this fence wait
    // above is the offscreen instance's own proven-complete point, so drain
    // here instead.
    bindlessRegistry.releaseRetiredIndices();
}

void VulkanGraphics::clearSceneColorRegion (juce::Rectangle<int> area)
{
    jassert (renderPassActive);

    const juce::Rectangle<int> sceneBounds {
        0, 0, static_cast<int> (swapchain.getExtent().width), static_cast<int> (swapchain.getExtent().height)
    };
    const auto clampedArea { area.getIntersection (sceneBounds) };

    commandBuffer.clearAttachments (
        vk::ClearAttachment {
            vk::ImageAspectFlagBits::eColor, 0, vk::ClearValue { vk::ClearColorValue { 0.0f, 0.0f, 0.0f, 0.0f } }
    },
        vk::ClearRect { vk::Rect2D { { clampedArea.getX(), clampedArea.getY() },
                                     { static_cast<uint32_t> (clampedArea.getWidth()),
                                       static_cast<uint32_t> (clampedArea.getHeight()) } },
                        0,
                        1 });
}

void VulkanGraphics::resize (int width, int height, float scale)
{
    JUCE_ASSERT_MESSAGE_THREAD

    if (activeContextCount == 0)
    {
#if JUCE_MAC
        if (swapchain.getMetalLayerRef() != nullptr)
            updateMetalLayerFrame (swapchain.getMetalLayerRef(), width, height, scale);
#endif

        const vk::Result waitIdleResult { device.getDevice().waitIdle() };
        const FrameDisposition waitDisposition { getDisposition (
            getDeviceLostTolerantDispositions(),
            waitIdleResult,
            FrameDisposition::skip,
            "jam::VulkanGraphics::resize: unhandled waitIdle result ") };

        if (waitDisposition == FrameDisposition::reinitialise)
            deferDeviceReinitialise();

        if (waitDisposition == FrameDisposition::proceed)
        {
#if JUCE_WINDOWS
            if (composition != nullptr)
            {
                device.getDevice().destroyFramebuffer (compositionFramebuffer, nullptr);
                compositionFramebuffer = nullptr;

                // Destroyed before composition->resize() below replaces the D3D11 shared
                // texture this image's memory was imported from — the import holds no
                // reference of its own past this point (jam_VulkanComposition.h's
                // releaseTextureExport() comment).
                compositionImage = VulkanImage {};
            }
            else
#endif
            {
                // Destroy swapchain framebuffers
                for (auto& framebuffer : swapchainFramebuffers)
                    device.getDevice().destroyFramebuffer (framebuffer, nullptr);
                swapchainFramebuffers.clear();
            }

            // Destroy stencil — RAII move-assign with empty VulkanImage destroys the old one
            stencilImage = VulkanImage {};

            bool swapchainReady { true };

#if JUCE_WINDOWS
            if (composition != nullptr)
            {
                const auto clientExtent { composition->getClientExtent (static_cast<HWND> (getNativeHandle())) };
                composition->resize (device.getCompositionDevice(), clientExtent.width, clientExtent.height);
                swapchain.setFormatAndExtent (swapchain.getFormat(), clientExtent);

                importCompositionImage();
            }
            else
#endif
            {
                auto [recreateReady, previousSwapchain, previousRenderFinishedSemaphores] {
                    swapchain.recreate (static_cast<int> (width * scale), static_cast<int> (height * scale))
                };

                for (auto semaphore : previousRenderFinishedSemaphores)
                    device.getDevice().destroySemaphore (semaphore, nullptr);

                device.getDevice().destroySwapchainKHR (previousSwapchain, nullptr);

                swapchainReady = recreateReady;

                if (not swapchainReady)
                    isSwapchainStale = true;
            }

            if (swapchainReady)
            {
                // Recreate stencil image with updated swapchain extent
                createStencilImage();

                // Recreate the scene target (color+stencil+resolve) at the new
                // extent, alongside the existing stencil/framebuffer recreation above/below.
                // No re-calibration here (activeSampleCount stays locked for the session's lifetime by design).
                createSceneTarget (swapchain.getExtent());

#if KUASSA_VULKAN_RUNTIME_SHADER_COMPILER
                // Recreate the post-process path's un-premultiplied scene target at the
                // same new extent — same lifecycle as sceneColorImage above.
                createStraightAlphaTarget (swapchain.getExtent());
#endif

                createFramebuffers();

                // bindlessTextureDescriptorSet already exists by resize() time (create() finished
                // long ago) — safe to assign+write immediately, unlike at create() time.
                updateSceneColorBindlessDescriptor();
#if KUASSA_VULKAN_RUNTIME_SHADER_COMPILER
                updateStraightAlphaBindlessDescriptor();
#endif

                // Recreate every existing transparency layer at the new swapchain extent.
                // waitIdle already completed above — safe to destroy/recreate immediately.
                jam::Array<int> previousTransparencyBindlessIndices;
                transparencyStack.resize (renderPass, swapchain.getFormat(), getActiveSampleCount(),
                                          swapchain.getExtent(), previousTransparencyBindlessIndices);

                for (const int previousIndex : previousTransparencyBindlessIndices)
                    bindlessRegistry.releaseBindlessIndex (previousIndex);
            }
        }
    }
}

void VulkanGraphics::incrementContextCount() { ++activeContextCount; }

//==============================================================================
// Accessors
//==============================================================================

vk::Device VulkanGraphics::getDevice() const noexcept { return device.getDevice(); }
vk::PhysicalDevice VulkanGraphics::getPhysicalDevice() const noexcept { return device.getPhysicalDevice(); }
vk::CommandBuffer VulkanGraphics::getCommandBuffer() const noexcept { return commandBuffer; }
vk::RenderPass VulkanGraphics::getRenderPass() const noexcept { return renderPass; }
vk::Extent2D VulkanGraphics::getSwapchainExtent() const noexcept { return swapchain.getExtent(); }

#if JUCE_WINDOWS
vk::Extent2D VulkanGraphics::getSurfaceExtent() const
{
    if (composition != nullptr)
        return composition->getClientExtent (static_cast<HWND> (getNativeHandle()));

    return swapchain.getSurfaceExtent();
}
#else
vk::Extent2D VulkanGraphics::getSurfaceExtent() const { return swapchain.getSurfaceExtent(); }
#endif

uint32_t VulkanGraphics::getGraphicsQueueFamily() const noexcept { return device.getQueueFamily(); }
vk::Queue VulkanGraphics::getGraphicsQueue() const noexcept { return device.getQueue(); }
VmaAllocator VulkanGraphics::getAllocator() const noexcept { return device.getAllocator(); }
VulkanPipelines& VulkanGraphics::getPipelines() noexcept { return pipelines; }
vk::ImageView VulkanGraphics::getStencilView() const noexcept { return stencilImage.getView(); }
vk::Image VulkanGraphics::getStencilImage() const noexcept { return stencilImage.getImage(); }
vk::DescriptorSet VulkanGraphics::getProjectionDescriptorSet() const noexcept { return projectionDescriptorSet; }
void* VulkanGraphics::getProjectionMappedPtr() const noexcept
{
    if (not activeRecordings.isEmpty())
        return recordings.at (static_cast<size_t> (activeRecordings.last()))->projectionBuffer.getMapped();

    return projectionBuffer.getMapped();
}
vk::DescriptorSet VulkanGraphics::getRecordDescriptorSet() const noexcept { return recordDescriptorSet; }

VulkanFrameBuffer& VulkanGraphics::getPathFrameBuffer() { return pathFrameBuffer; }
VulkanPrimitiveRecordBuffer& VulkanGraphics::getPrimitiveRecordBuffer() { return primitiveRecordBuffer; }

void VulkanGraphics::reservePrimitiveRecords (int requiredRecords)
{
    const vk::Buffer bufferBeforeGrow { primitiveRecordBuffer.getBuffer() };

    primitiveRecordBuffer.reserve (requiredRecords);

    // Descriptor sets are bound to a specific vk::Buffer handle — a grow (new handle)
    // mid-frame leaves the already-allocated recordDescriptorSet stale. Set 2 has no
    // update-after-bind, so a bound set cannot be rewritten; allocate a FRESH set2
    // against the new handle instead and swap it in.
    if (primitiveRecordBuffer.getBuffer() != bufferBeforeGrow)
    {
        const vk::DescriptorSetLayout set2 { pipelines.getLayoutSet2() };
        const vk::DescriptorSetAllocateInfo recordAllocInfo { descriptorPool, set2 };

        vk::DescriptorSet freshRecordSet {};
        const vk::Result allocResult { device.getDevice().allocateDescriptorSets (&recordAllocInfo, &freshRecordSet) };
        const FrameDisposition allocDisposition { getDisposition (
            getSuccessOnlyDispositions(),
            allocResult,
            FrameDisposition::skip,
            "jam::VulkanGraphics::reservePrimitiveRecords: unhandled allocateDescriptorSets result ") };

        if (allocDisposition == FrameDisposition::proceed)
        {
            const vk::DescriptorBufferInfo recordBufferInfo { primitiveRecordBuffer.getBuffer(), 0, vk::WholeSize };
            const vk::WriteDescriptorSet recordWrite {
                freshRecordSet, 0, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr, &recordBufferInfo
            };
            device.getDevice().updateDescriptorSets (recordWrite, nullptr);

            recordDescriptorSet = freshRecordSet;
        }
    }
}

void VulkanGraphics::updateRecordDescriptorSet()
{
    const vk::DescriptorBufferInfo recordBufferInfo { primitiveRecordBuffer.getBuffer(), 0, vk::WholeSize };

    const vk::WriteDescriptorSet recordWrite {
        recordDescriptorSet, 0, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr, &recordBufferInfo
    };

    device.getDevice().updateDescriptorSets (recordWrite, nullptr);
}

VulkanTransparencyStack::TransparencyLayer& VulkanGraphics::getTransparencyLayer (int level)
{
    return transparencyStack.getTransparencyLayer (level);
}

void VulkanGraphics::updateSceneColorBindlessDescriptor()
{
    // sceneColorBindlessIndex is lifetime-stable (see its doc comment) — only
    // ever assigned once, but createSceneTarget() gives sceneColorImage a
    // fresh vk::ImageView identity on every resize, so the descriptor at that
    // same slot must be rewritten unconditionally, not only on first
    // assignment.
    if (sceneColorBindlessIndex < 0)
        sceneColorBindlessIndex = bindlessRegistry.assignBindlessIndex();

    if (sceneColorBindlessIndex >= 0)
        writeBindlessTextureDescriptor (static_cast<uint32_t> (sceneColorBindlessIndex), sceneColorImage.getView());
}

#if KUASSA_VULKAN_RUNTIME_SHADER_COMPILER
void VulkanGraphics::updateStraightAlphaBindlessDescriptor()
{
    // Mirrors updateSceneColorBindlessDescriptor()'s exact
    // assign-once/rewrite-always convention.
    if (straightAlphaBindlessIndex < 0)
        straightAlphaBindlessIndex = bindlessRegistry.assignBindlessIndex();

    if (straightAlphaBindlessIndex >= 0)
        writeBindlessTextureDescriptor (
            static_cast<uint32_t> (straightAlphaBindlessIndex), straightAlphaImage.getView());
}
#endif

void VulkanGraphics::recordSceneCompositeDrawCommands()
{
    if (sceneColorBindlessIndex >= 0)
    {
        // Barrier — mirrors transitionTransparencyLayerForSampling's established
        // shape (jam_VulkanLowLevelGraphicsContextTransparency.cpp): the scene render
        // pass's attachment 2 (the resolve target) already declares finalLayout =
        // SHADER_READ_ONLY_OPTIMAL, but the render pass's implicit vk::SubpassExternal
        // dependency (no explicit vk::SubpassDependency array is supplied) does not by
        // itself guarantee FRAGMENT_SHADER-stage read visibility of the resolve write —
        // an explicit same-layout barrier supplies that memory dependency, exactly like
        // every other composite-read in this engine. recordImageMemoryBarrier() is the
        // shared SSOT helper for this shape (jam_VulkanUploadHelpers.h) — same
        // same-layout/aspect/access/stage parameters as
        // recordPostProcessCompositeDrawCommands()'s identical sceneColorImage read
        // (jam_VulkanGraphicsSlangPass.cpp).
        recordImageMemoryBarrier (commandBuffer,
                                  sceneColorImage.getImage(),
                                  vk::ImageLayout::eShaderReadOnlyOptimal,
                                  vk::ImageLayout::eShaderReadOnlyOptimal,
                                  vk::ImageAspectFlagBits::eColor,
                                  vk::AccessFlagBits::eColorAttachmentWrite,
                                  vk::AccessFlagBits::eShaderRead,
                                  vk::PipelineStageFlagBits::eColorAttachmentOutput,
                                  vk::PipelineStageFlagBits::eFragmentShader);

        // A fresh, self-contained ortho projection at the current swapchain.getExtent() —
        // this composite draw does not depend on any particular LLGC's cached copy
        // (VulkanGraphics::endFrame() runs after every per-frame LLGC has already been
        // destroyed).
        const glm::mat4 compositeProjection { glm::ortho (0.0f,
                                                          static_cast<float> (swapchain.getExtent().width),
                                                          0.0f,
                                                          static_cast<float> (swapchain.getExtent().height),
                                                          -1.0f,
                                                          1.0f) };
        std::memcpy (projectionBuffer.getMapped(), &compositeProjection, sizeof (compositeProjection));

        reservePrimitiveRecords (primitiveRecordBuffer.getUsedRecords() + 1);
        {
            auto* records { static_cast<VulkanPrimitiveRecord*> (primitiveRecordBuffer.getMapped()) };
            const int recordIndex { primitiveRecordBuffer.getUsedRecords() };

            VulkanPrimitiveRecord& record { records[recordIndex] };
            record.position.x = 0.0f;
            record.position.y = 0.0f;
            record.size = jam::Size<float> { static_cast<float> (swapchain.getExtent().width),
                                                static_cast<float> (swapchain.getExtent().height) };
            record.uvRect = juce::Rectangle<float> { 0.0f, 0.0f, 1.0f, 1.0f };
            // Full-screen composite quad — always fully opaque, no currentState.opacity
            // to compose here (this draw is issued after every per-frame LLGC has
            // already been destroyed; see the surrounding comment above).
            record.color = VulkanColour::fromHex (juce::Colours::white);
            record.clip = juce::Rectangle<int> {};
            record.textureIndex = static_cast<uint32_t> (sceneColorBindlessIndex);
            record.stencilRef = 0;
            record.flags = 0;
            record.maskTextureIndex = noMaskIndex;

            primitiveRecordBuffer.setUsedRecords (recordIndex + 1);

#if JUCE_WINDOWS
            const vk::Framebuffer sceneCompositeFramebuffer {
                composition != nullptr ? compositionFramebuffer
                                       : swapchainFramebuffers.at (static_cast<int> (currentImageIndex))
            };
#else
            const vk::Framebuffer sceneCompositeFramebuffer {
                swapchainFramebuffers.at (static_cast<int> (currentImageIndex))
            };
#endif

            const vk::RenderPassBeginInfo rpInfo {
                compositeRenderPass, sceneCompositeFramebuffer, vk::Rect2D { { 0, 0 }, swapchain.getExtent() }
            };

            commandBuffer.beginRenderPass (&rpInfo, vk::SubpassContents::eInline);
            renderPassActive = true;

            const vk::PipelineLayout layout { pipelines.getLayout() };
            const vk::DescriptorSet projSet { projectionDescriptorSet };
            const vk::DescriptorSet imgSet { bindlessTextureDescriptorSet };
            const vk::DescriptorSet recordSet { recordDescriptorSet };

            commandBuffer.bindDescriptorSets (vk::PipelineBindPoint::eGraphics, layout, 1, imgSet, nullptr);
            commandBuffer.bindDescriptorSets (vk::PipelineBindPoint::eGraphics, layout, 2, recordSet, nullptr);
            commandBuffer.bindDescriptorSets (vk::PipelineBindPoint::eGraphics, layout, 3, projSet, nullptr);

            // compositePipeline reuses image.frag verbatim (createCompositePipeline()'s
            // doc comment) — it reads the opacity-premultiplied color.a from push-constant
            // memory exactly like ID::imageInstanced, so it needs the same push before its
            // draw as recordTransparencyCompositeDrawCommands() issues for that pipeline.
            // Opaque (1.0f) matches this composite's "minimal identity" contract —
            // full opacity, no additional blend.
            const auto pc { makeImagePushConstants (1.0f) };
            commandBuffer.pushConstants (
                layout, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, sizeof (pc), &pc);

            commandBuffer.bindPipeline (vk::PipelineBindPoint::eGraphics, compositePipeline);

            const vk::Viewport viewport {
                0.0f, 0.0f, static_cast<float> (swapchain.getExtent().width), static_cast<float> (swapchain.getExtent().height),
                0.0f, 1.0f
            };
            commandBuffer.setViewport (0, viewport);
            const vk::Rect2D scissor {
                { 0, 0 },
                swapchain.getExtent()
            };
            commandBuffer.setScissor (0, scissor);

            commandBuffer.draw (4, 1, 0, static_cast<uint32_t> (recordIndex));

            commandBuffer.endRenderPass();
            renderPassActive = false;
        }
    }
}

#if JUCE_WINDOWS
bool VulkanGraphics::importCompositionImage()
{
    const HANDLE textureExport { composition->releaseTextureExport() };

    compositionImage = VulkanImage::importD3D11 (
        device.getDevice(), textureExport, swapchain.getFormat().format, swapchain.getExtent());
    CloseHandle (textureExport);

    return compositionImage.isValid();
}

void VulkanGraphics::recordCompositionAcquireBarrier()
{
    const vk::ImageMemoryBarrier acquireBarrier {
        {},
        vk::AccessFlagBits::eColorAttachmentWrite,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eGeneral,
        vk::QueueFamilyExternal,
        device.getQueueFamily(),
        compositionImage.getImage(),
        vk::ImageSubresourceRange { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 }
    };

    commandBuffer.pipelineBarrier (vk::PipelineStageFlagBits::eTopOfPipe,
                                   vk::PipelineStageFlagBits::eColorAttachmentOutput,
                                   {},
                                   {},
                                   {},
                                   acquireBarrier);
}

void VulkanGraphics::recordCompositionReleaseBarrier()
{
    const vk::ImageMemoryBarrier releaseBarrier {
        vk::AccessFlagBits::eColorAttachmentWrite,
        {},
        vk::ImageLayout::eGeneral,
        vk::ImageLayout::eGeneral,
        device.getQueueFamily(),
        vk::QueueFamilyExternal,
        compositionImage.getImage(),
        vk::ImageSubresourceRange { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 }
    };

    commandBuffer.pipelineBarrier (vk::PipelineStageFlagBits::eColorAttachmentOutput,
                                   vk::PipelineStageFlagBits::eBottomOfPipe,
                                   {},
                                   {},
                                   {},
                                   releaseBarrier);
}
#endif

//==============================================================================
// Staging
//==============================================================================

VulkanStagingArena::StagingAllocation VulkanGraphics::allocateStaging (vk::DeviceSize sizeBytes)
{
    return stagingArena.getOrCreateAllocation (sizeBytes);
}

//==============================================================================
// Rendering partner interface
//==============================================================================

VulkanGraphics::ImageSourceRoot VulkanGraphics::getImageSourceRoot (const juce::Image& image) noexcept
{
    juce::ImagePixelData::Ptr current { image.getPixelData() };
    jassert (current != nullptr);// every caller (cacheImageTexture/getCachedImageView/
    // getBindlessIndex/getImageUVRect) is only ever reached once
    // its own caller has already gated on image.getWidth() > 0
    // (drawImage()/clipToImageAlpha(), jam_VulkanLowLevelGraphicsContextImage.cpp),
    // which is only true for a non-null backing pixel store.

    // `area` starts as `current`'s own full local-space bounds, then each loop
    // iteration translates it one hop up into its immediate source's own
    // coordinate space by adding that hop's own getSubsection() offset —
    // see ImageSourceRoot's own doc comment (jam_VulkanGraphics.h) for the
    // two/three-level worked example this accumulation is derived from.
    juce::Rectangle<int> area { current->width, current->height };
    juce::ImagePixelData::Ptr source { current->getSourcePixelData() };

    while (source.get() != current.get())
    {
        area = area.withPosition (area.getPosition() + current->getSubsection().getPosition());
        current = source;
        source = current->getSourcePixelData();
    }

    return { current.get(), area };
}

vk::ImageView VulkanGraphics::getCachedImageView (const juce::Image& src) const noexcept
{
    const auto resolved { getImageSourceRoot (src) };
    const juce::Image rootImage { juce::ImagePixelData::Ptr { resolved.root } };

    // Engine-backed images are already GPU-resident — return the resolve
    // texture's view directly, bypassing the cache.
    if (auto* texture { VulkanImageType::getTextureFrom (rootImage) }; texture != nullptr)
        return texture->getView();

    // Foreign (software) images — existing cache path.
    if (auto* cache { VulkanTextureCache::getInstance() }; cache != nullptr and cache->hasEntry (resolved.root))
    {
        auto& entry { cache->getEntry (resolved.root) };

        if (not entry.dirty)
            return entry.texture.getView();
    }

    return {};
}

int VulkanGraphics::getBindlessIndex (const juce::Image& src) const noexcept
{
    const auto resolved { getImageSourceRoot (src) };
    const juce::Image rootImage { juce::ImagePixelData::Ptr { resolved.root } };

    // Engine-backed images — per-window slot on the resolve texture directly.
    if (auto* texture { VulkanImageType::getTextureFrom (rootImage) }; texture != nullptr)
        return texture->getBindlessIndex (getNativeHandle());

    // Foreign (software) images — existing cache path.
    if (auto* cache { VulkanTextureCache::getInstance() }; cache != nullptr and cache->hasEntry (resolved.root))
        return cache->getEntry (resolved.root).texture.getBindlessIndex (getNativeHandle());

    return -1;
}

juce::Rectangle<float> VulkanGraphics::getImageUVRect (const juce::Image& src) const noexcept
{
    const auto resolved { getImageSourceRoot (src) };
    const juce::Image rootImage { juce::ImagePixelData::Ptr { resolved.root } };

    int textureWidth { 0 };
    int textureHeight { 0 };

    if (auto* texture { VulkanImageType::getTextureFrom (rootImage) }; texture != nullptr)
    {
        const auto extent { texture->getExtent() };
        textureWidth = static_cast<int> (extent.width);
        textureHeight = static_cast<int> (extent.height);
    }
    else if (auto* cache { VulkanTextureCache::getInstance() }; cache != nullptr and cache->hasEntry (resolved.root))
    {
        const auto [w, h] { cache->getEntry (resolved.root).size };
        textureWidth = w;
        textureHeight = h;
    }

    if (textureWidth > 0 and textureHeight > 0)
        return juce::Rectangle<float> (
            static_cast<float> (resolved.area.getX()) / static_cast<float> (textureWidth),
            static_cast<float> (resolved.area.getY()) / static_cast<float> (textureHeight),
            static_cast<float> (resolved.area.getWidth()) / static_cast<float> (textureWidth),
            static_cast<float> (resolved.area.getHeight()) / static_cast<float> (textureHeight));

    return { 0.0f, 0.0f, 1.0f, 1.0f };
}

void VulkanGraphics::cacheImageTexture (const juce::Image& src)
{
    const ImageSourceRoot resolved { getImageSourceRoot (src) };
    const juce::Image rootImage { juce::ImagePixelData::Ptr { resolved.root } };

    VulkanBindlessTexture* target { VulkanImageType::getTextureFrom (rootImage) };

    if (target == nullptr)
    {
        // Foreign (software) images — cache + upload path.
        const juce::Image argbImage { rootImage.convertedToFormat (juce::Image::ARGB) };

        const int width { argbImage.getWidth() };
        const int height { argbImage.getHeight() };
        const juce::Image::BitmapData bmp { argbImage, juce::Image::BitmapData::readOnly };

        jassert (width > 0 and height > 0);
        jassert (bmp.data != nullptr);
        jassert (bmp.pixelStride == 4);

        if (width > 0 and height > 0 and bmp.data != nullptr and bmp.pixelStride == 4)
        {
            auto* cache { VulkanTextureCache::getInstance() };
            jassert (cache != nullptr);

            cache->cacheImageTexture (rootImage, width, height);

            jassert (cache->hasEntry (resolved.root));
            target = &cache->getEntry (resolved.root).texture;
        }
    }

    if (target != nullptr)
    {
        const int existingIndex { target->getBindlessIndex (getNativeHandle()) };

        if (existingIndex < 0)
        {
            const int index { bindlessRegistry.assignBindlessIndex() };
            target->registerBindlessIndex (getNativeHandle(), index);
            writeBindlessTextureDescriptor (static_cast<uint32_t> (index), target->getView());

            // target->registry (window-keyed) is the SSOT for this
            // (texture,window) slot — just written above and read back here.
            // Registers this window's own listener on the root and records
            // that same slot, root-keyed, as an explicit derivation of the
            // SSOT read-back — recycled by bindlessRegistry on deletion/
            // dimension change (jam_VulkanBindlessRegistry.h).
            const int derivedIndex { target->getBindlessIndex (getNativeHandle()) };
            jassert (derivedIndex == index);
            bindlessRegistry.registerBindlessIndex (resolved.root, derivedIndex);
        }
    }
}

vk::Sampler VulkanGraphics::getOrCreateLinearSampler()
{
    if (linearSampler == nullptr)
    {
        const vk::SamplerCreateInfo samplerInfo { {},
                                                  vk::Filter::eLinear,
                                                  vk::Filter::eLinear,
                                                  vk::SamplerMipmapMode::eLinear,
                                                  vk::SamplerAddressMode::eClampToEdge,
                                                  vk::SamplerAddressMode::eClampToEdge,
                                                  vk::SamplerAddressMode::eClampToEdge,
                                                  0.0f,
                                                  vk::False,
                                                  0.0f,
                                                  vk::False,
                                                  vk::CompareOp::eNever,
                                                  0.0f,
                                                  0.0f,
                                                  vk::BorderColor::eIntOpaqueBlack };

        const vk::Result createSamplerResult { device.getDevice().createSampler (
            &samplerInfo, nullptr, &linearSampler) };
        const FrameDisposition createSamplerDisposition { getDisposition (
            getSuccessOnlyDispositions(),
            createSamplerResult,
            FrameDisposition::skip,
            "jam::VulkanGraphics::getOrCreateLinearSampler: unhandled createSampler result ") };
        juce::ignoreUnused (createSamplerDisposition);
    }

    return linearSampler;
}

void VulkanGraphics::writeBindlessTextureDescriptor (uint32_t index, vk::ImageView view)
{
    jassert (index < bindlessTextureCapacity);

    // Three writes, one call: binding 0's plain `texture2D` slot, plus bindings 3/4's
    // combined-image-sampler slots pre-pairing the SAME view with the linear/nearest
    // sampler respectively (VulkanPipelines::createDescriptorSetLayouts()'s doc comment) — all
    // three stay in lockstep at every texture upload/(re)creation, never per draw.
    const vk::DescriptorImageInfo sampledImageInfo { nullptr, view, vk::ImageLayout::eShaderReadOnlyOptimal };
    const vk::DescriptorImageInfo linearCombinedInfo { getOrCreateLinearSampler(),
                                                       view,
                                                       vk::ImageLayout::eShaderReadOnlyOptimal };
    const vk::DescriptorImageInfo nearestCombinedInfo { nearestSampler, view, vk::ImageLayout::eShaderReadOnlyOptimal };

    const std::array<vk::WriteDescriptorSet, 3> writes {
        vk::WriteDescriptorSet {
                                bindlessTextureDescriptorSet, 0, index, 1, vk::DescriptorType::eSampledImage,         &sampledImageInfo    },
        vk::WriteDescriptorSet {
                                bindlessTextureDescriptorSet, 3, index, 1, vk::DescriptorType::eCombinedImageSampler, &linearCombinedInfo  },
        vk::WriteDescriptorSet {
                                bindlessTextureDescriptorSet, 4, index, 1, vk::DescriptorType::eCombinedImageSampler, &nearestCombinedInfo }
    };

    device.getDevice().updateDescriptorSets (writes, nullptr);
}

void VulkanGraphics::registerGlyphAtlasSlots (jam::GlyphAtlas& atlas)
{
    for (auto type : { jam::GlyphAtlas::Type::mono, jam::GlyphAtlas::Type::emoji })
    {
        auto& texture { atlas.getTexture (type) };
        const int existingIndex { texture.getBindlessIndex (getNativeHandle()) };

        if (existingIndex < 0)
        {
            const int index { bindlessRegistry.assignBindlessIndex() };
            texture.registerBindlessIndex (getNativeHandle(), index);
            writeBindlessTextureDescriptor (static_cast<uint32_t> (index), texture.getView());
        }
    }
}

void VulkanGraphics::getOrCreateTransparencyLayer (int level)
{
    auto& target { transparencyStack.getOrCreateLayer (level) };

    // Stable per-level bindless slot, written once at creation (bindlessIndex
    // starts at -1). The RESOLVE image is what gets written — a multisample
    // image cannot be sampled as sampler2D.
    if (target.bindlessIndex < 0)
    {
        target.bindlessIndex = bindlessRegistry.assignBindlessIndex();
        writeBindlessTextureDescriptor (static_cast<uint32_t> (target.bindlessIndex), target.resolveImage.getView());
    }
}

VulkanWindingScratch& VulkanGraphics::getOrCreateWindingScratch (vk::Extent2D requiredExtent)
{
    windingScratch.reserve (renderPass, swapchain.getFormat(), getActiveSampleCount(), requiredExtent);

    if (windingScratch.getBindlessIndex() < 0)
    {
        const int index { bindlessRegistry.assignBindlessIndex() };
        windingScratch.setBindlessIndex (index);
        writeBindlessTextureDescriptor (static_cast<uint32_t> (index), windingScratch.getResolveView());
    }

    return windingScratch;
}

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
