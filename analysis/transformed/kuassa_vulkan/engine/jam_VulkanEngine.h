namespace jam
{
/*____________________________________________________________________________*/
/** @brief Unified resource-ownership tree for the Vulkan rendering backend —
 *  the shared Device, every SharedResources<T> interning table (Typeface,
 *  Stamp, Grapheme, Hyperlink), the shared glyph atlas, and per-window Graphics
 *  instances, all rooted in one Application-owned object.
 *
 *  Sets juce::ComponentPeer::externalContextFactory at construction. Clears it
 *  at destruction. Lazily creates a Graphics per peer on first paint request.
 *
 *  Owns the shared Device (Vulkan instance, physical device, logical device,
 *  graphics queue, and VMA allocator). One VulkanEngine per application.
 *
 *  Owned by the application — typically as a member of the main window
 *  or application class.
 *
 *  Constructed unconditionally, regardless of GPU availability or the caller's
 *  configured preference — the constructor's gpuEnabled parameter controls
 *  whether GPU initialisation runs; isGpuAvailable() reflects that outcome
 *  for the engine's lifetime, selecting which engine createContext() picks
 *  per paint (native Vulkan vs the CPU-fallback
 *  jam::LowLevelGraphicsGlyphRenderer), never whether this VulkanEngine,
 *  its Device, or its shared glyphAtlas exist at all. This keeps the goal of
 *  JUCE's own default renderers
 *  (CoreGraphicsContext/Direct2DGraphicsContext) never running, for anything,
 *  structurally true in every configuration, including GPU categorically
 *  unavailable or explicitly disabled by the caller.
 */
class VulkanEngine : public jam::Instance<VulkanEngine>
{
public:
    /** @brief Constructs the engine synchronously on the message thread:
     *  registers the context factory, default-constructs every shared-resource
     *  interning table (Typeface, Stamp, Grapheme, Hyperlink — see their own member
     *  doc comments for teardown-order rationale), then — when the gpuEnabled
     *  parameter is true — initialises the shared Device and every GPU-side member
     *  (textureCache, glyphAtlas, pipelineCache, imageRenderer, blurRenderer)
     *  in sequence; gpuAvailable is set once that chain completes.  Finally,
     *  calls imageLoader.registerImages(), queueing every embedded resource for
     *  background decode into the image memo — unconditional, GPU and CPU
     *  lanes alike.
     *  @param maxImageExtent       Largest offscreen render-target extent this engine's
     *                              VulkanImageRenderer will serve (the layout-declared
     *                              full design editor size); seeds createOffscreen's
     *                              calibration probe and transparency-stack extent.
     *                              No default value — always set by the caller
     *                              (Explicit principle).
     *  @param deviceName           Vulkan device name. Defaults to projectName;
     *                              pass an explicit name to override.
     *  @param targetFrameBudgetMs  Per-frame time budget passed down into every Graphics
     *                              this VulkanEngine creates, consumed once by each surface's
     *                              own VulkanMsaaCalibration::calibrateSampleCount(). Defaults to
     *                              getFrameBudget()'s refresh-rate-resolved value.
     *  @param pipelineCacheFile    Exact Vulkan pipeline-cache file path passed down into
     *                              every Graphics this VulkanEngine creates. Defaults to
     *                              getPipelineCacheFile()'s user-settings-directory path.
     *  @param gpuEnabled           When true (default), the constructor initialises the
     *                              GPU device; when false, gpuAvailable stays false for
     *                              the engine's lifetime — no runtime call brings the GPU
     *                              route up retroactively.
     */
    explicit VulkanEngine (vk::Extent2D maxImageExtent, const juce::String& deviceName = ProjectInfo::projectName, double targetFrameBudgetMs = getFrameBudget(),
                           const juce::File& pipelineCacheFile = getPipelineCacheFile(), bool gpuEnabled = true)
        : textureCache (device),
          glyphAtlas (device),
          pipelineCache (device, pipelineCacheFile),
          imageRenderer (device),
          blurRenderer (device),
          targetFrameBudgetMs (targetFrameBudgetMs),
          maxImageExtent (maxImageExtent)
    {
        glyphAtlas.setRasterization (map::FontRasterizerBackend::freetype, defaultFontGamma, defaultFontContrast);

        juce::ComponentPeer::externalContextFactory = &createContext;

        if (gpuEnabled)
        {
            device.initialise (deviceName);

            if (device.isValid())
            {
                textureCache.initialise();
                glyphAtlas.initialise();
                pipelineCache.initialise();
                imageRenderer.initialise (targetFrameBudgetMs, pipelineCache.getPipelineCache(), maxImageExtent);
                blurRenderer.initialise (targetFrameBudgetMs, pipelineCache.getPipelineCache(), maxImageExtent);
            }

            gpuAvailable = device.isValid();
        }

        imageLoader.registerImages();
    }

    /** @brief Clears the context factory, destroys every peer's Graphics
     *  (contexts.clear()), then waits for the device to go fully idle.
     *  Reverse-declaration-order member destruction handles everything else
     *  below (VulkanDevice destructs last). */
    ~VulkanEngine()
    {
        juce::ComponentPeer::externalContextFactory = nullptr;

        contexts.clear();

        if (device.getDevice() != nullptr)
        {
            const vk::Result waitIdleResult { device.getDevice().waitIdle() };
            jassert (waitIdleResult == vk::Result::eSuccess or waitIdleResult == vk::Result::eErrorDeviceLost);
            juce::ignoreUnused (waitIdleResult);
        }
    }

    /** @brief Erases the Graphics already keyed by @p handle. The erase itself
     *  runs that Graphics's destructor — device waitIdle, then WSI teardown in
     *  spec order (semaphores, swapchain, surface) — while @p handle's native
     *  view is still alive; the peer has not yet left the desktop. The dtor
     *  also destroys the jam::VulkanBindlessInstance member — withdrawing
     *  the window's registered slot assignment from every shared glyph-atlas
     *  type's registry — without that, a future Graphics on a reused native
     *  handle would read a stale registered slot assignment and skip
     *  assigning/writing its own descriptor.
     *
     *  Called BEFORE the peer leaves the desktop (see the
     *  `removePeer (juce::Component&)` overload) — @p handle identifies
     *  the entry to erase and is not dereferenced.
     *  @param handle  The native peer handle whose Graphics entry should be removed.
     */
    void removePeer (void* handle)
    {
        textureCache.removePeer (handle);
        contexts.erase (handle);
    }

    /** @brief Tears down @p component's Vulkan-backed peer in this order:
     *  captures its native handle while the peer is still alive, marks
     *  teardownHandle so createContext() routes any synchronous teardown
     *  paint reaching it to the CPU lane, erases the Graphics keyed by that
     *  handle via `removePeer (void*)` — running the Graphics dtor while the
     *  native view is still alive — then calls `removeFromDesktop()`. This is
     *  the single teardown sequence used by `jam::PluginEditor`,
     *  `jam::Window`, `jam::PopupMenu::MenuWindow`, and
     *  `jam::ModalSheet::dismiss()`.
     *
     *  `removeFromDesktop()` can itself destroy child desktop windows whose
     *  own destructors call this overload re-entrantly (e.g.
     *  `jam::PopupMenu::MenuWindow`, `jam::Window`) — teardownHandle is
     *  saved and restored around the mark rather than unconditionally
     *  cleared, so the inner call cannot strip the outer window's mark while
     *  its own `removeFromDesktop()` is still executing.
     *  No-op when @p component has no peer.
     *  @param component  The component whose peer should be torn down.
     */
    void removePeer (juce::Component& component)
    {
        if (auto* peer { component.getPeer() })
        {
            auto* handle { peer->getNativeHandle() };
            auto* previousHandle { teardownHandle };
            teardownHandle = handle;
            removePeer (handle);
            component.removeFromDesktop();
            teardownHandle = previousHandle;
        }
    }

    /** @brief Recovers from a lost logical device by re-running the
     *  constructor's own device-initialisation chain against the surviving
     *  vk::Instance/vk::PhysicalDevice — never the constructor itself, never a
     *  new VulkanEngine. Deferred to the message loop (never called mid-paint)
     *  by every FrameDisposition::reinitialise arm via
     *  juce::MessageManager::callAsync(); idempotent by reconstruction, so
     *  multiple queued calls are harmless.
     *
     *  Sequence: waits the still-attached device idle (tolerating
     *  eErrorDeviceLost, same as the destructor); clears every window's
     *  Graphics (contexts.clear() — each rebuilds lazily on its next paint via
     *  getOrCreateGraphics()); releases blurStaging while the
     *  allocator is still alive; shuts down blurRenderer,
     *  imageRenderer, pipelineCache, glyphAtlas, and textureCache in that
     *  reverse-initialise order; shuts down and re-initialises the logical
     *  device and allocator (instance and physicalDevice survive); then
     *  replays the constructor's own initialise() chain on the five GPU-side
     *  members, exactly as the constructor body above does. CPU-side state
     *  (interning tables, imageLoader's decode memo, every Model) is untouched
     *  — the paint lane's own cacheImageTexture() calls re-upload already-
     *  decoded pixels on the next repaint, never re-decoding. */
    void reinitialiseDevice()
    {
        JUCE_ASSERT_MESSAGE_THREAD

        if (device.getDevice() != nullptr)
        {
            const vk::Result waitIdleResult { device.getDevice().waitIdle() };
            jassert (waitIdleResult == vk::Result::eSuccess or waitIdleResult == vk::Result::eErrorDeviceLost);
            juce::ignoreUnused (waitIdleResult);
        }

        contexts.clear();

        blurStaging = VulkanBuffer {};
        blurRenderer.shutdown();
        imageRenderer.shutdown();
        pipelineCache.shutdown();
        glyphAtlas.shutdown();
        textureCache.shutdown();
        device.shutdown();

        device.initialise (ProjectInfo::projectName);

        if (device.isValid())
        {
            textureCache.initialise();
            glyphAtlas.initialise();
            pipelineCache.initialise();
            imageRenderer.initialise (targetFrameBudgetMs, pipelineCache.getPipelineCache(), maxImageExtent);
            blurRenderer.initialise (targetFrameBudgetMs, pipelineCache.getPipelineCache(), maxImageExtent);
        }

        gpuAvailable = device.isValid();
    }

    /** @brief Blurs source into destination, resizing/reformatting destination
     *  to match source's dimensions and juce::Image::PixelFormat as needed.
     *
     *  GPU route (gated on isGpuAvailable() and
     *  device.isComputeCapable()): caches source into
     *  blurRenderer's own VulkanGraphics and resolves its bindless index; on a
     *  cache hit, records the two-dispatch compute stack blur plus its own
     *  readback into destination
     *  (recordBlurAndReadback()/copyStagingToDestination()) and returns.
     *  Falls through to the CPU stack-blur route (blurImageFallback()) both
     *  when the GPU is unavailable and when the source failed to resolve a
     *  bindless index this call.
     *  @param destination  Blurred output — reallocated to source's size/format
     *                      on mismatch.
     *  @param source       Image to blur.
     *  @param radius       Blur radius in pixels.
     */
    void blurImage (juce::Image& destination, const juce::Image& source, int radius)
    {
        computeImageOperation (destination, source,
            [&] (int width, int height, int srcBindlessIndex) -> vk::Result
            {
                return recordBlurAndReadback (width, height, radius, srcBindlessIndex);
            },
            [&] { blurImageFallback (destination, source, radius); });
    }

    /** @brief Erodes the alpha channel of @p source by @p radius pixels into @p destination.
     *
     *  GPU route (gated on isGpuAvailable() and
     *  device.isComputeCapable()): caches the source texture, records the compute
     *  choke dispatch plus readback into destination. Falls through to the CPU
     *  route (chokeImageFallback()) when the GPU is unavailable or the source
     *  fails to resolve a bindless index this call.
     *  @param destination  Choked output — reallocated to source's size/format on mismatch.
     *  @param source       Image to choke.
     *  @param radius       Erosion radius in pixels.
     */
    void chokeImage (juce::Image& destination, const juce::Image& source, int radius)
    {
        computeImageOperation (destination, source,
            [&] (int width, int height, int srcBindlessIndex) -> vk::Result
            {
                return recordChokeAndReadback (width, height, radius, srcBindlessIndex);
            },
            [&] { chokeImageFallback (destination, source, radius); });
    }

    /** @brief Feathers (softens) the alpha channel of @p source by @p radius and @p curve into @p destination.
     *
     *  GPU route (gated on isGpuAvailable() and
     *  device.isComputeCapable()): caches the source texture, records the compute
     *  feather dispatch plus readback into destination. Falls through to the CPU
     *  route (featherImageFallback()) when the GPU is unavailable or the source
     *  fails to resolve a bindless index this call.
     *  @param destination  Feathered output — reallocated to source's size/format on mismatch.
     *  @param source       Image to feather.
     *  @param radius       Feather radius in pixels.
     *  @param curve        Feather curve exponent controlling the falloff shape.
     */
    void featherImage (juce::Image& destination, const juce::Image& source, float radius, float curve)
    {
        computeImageOperation (destination, source,
            [&] (int width, int height, int srcBindlessIndex) -> vk::Result
            {
                return recordFeatherAndReadback (width, height, radius, curve, srcBindlessIndex);
            },
            [&] { featherImageFallback (destination, source, radius, curve); });
    }

    /** @brief Returns the engine-owned shared offscreen renderer for
     *  VulkanImagePixelData targets — the sole resolution path for
     *  VulkanImageType::create() (jam_VulkanImagePixelData.h), reached via
     *  getInstance()->getImageRenderer(). */
    VulkanImageRenderer& getImageRenderer() noexcept { return imageRenderer; }

    /** @brief Returns true once the synchronous GPU-initialisation chain has
     *  completed and the shared Device is valid — the single source of truth
     *  for GPU capability. */
    bool isGpuAvailable() const noexcept { return gpuAvailable.load(); }

    /** @brief Registers a typeface's font-file bytes into both glyphAtlas
     *  (rasterization) and typeface (HarfBuzz shaping) in one call — the two
     *  interning tables every embedded-font registration needs, kept in sync
     *  by construction instead of requiring two call sites. */
    void registerTypeface (const juce::Typeface::Ptr& typefacePtr, const void* fontData, size_t sizeBytes)
    {
        glyphAtlas.registerTypeface (typefacePtr, fontData, sizeBytes);
        typeface.registerTypeface (typefacePtr, fontData, sizeBytes);
    }

#if KUASSA_VULKAN_RUNTIME_SHADER_COMPILER
    /** @brief Bundle returned by getPostProcess() — the installed post-process
     *  chain (or nullptr) plus the two presentation parameters every
     *  Graphics::endFrame() call site needs alongside it, pulled together in
     *  one call (SSOT — Shader itself carries neither, see jam_VulkanShader.h's
     *  Shader doc comment). */
    struct PostProcess
    {
        /** @brief The installed post-process chain, or nullptr if none is installed. */
        const VulkanShader* shader;

        /** @brief Overall blend opacity, [0, 1] — meaningless when shader is nullptr. */
        float opacity;

        /** @brief Intermediate-pass extent fraction, [0, 1] — meaningless when shader is nullptr. */
        float resolutionScale;
    };

    /** @brief Installs (or, with nullptr, clears) the app-global post-process
     *  Shader chain, along with its opacity/resolutionScale. VulkanEngine owns
     *  shader for its entire lifetime after this call — every window's
     *  Graphics reads it back once per endFrame() via getPostProcess()
     *  (windows pull; VulkanEngine never pushes into a window).
     *
     *  Shader is pure data (jam_VulkanShader.h — no Vulkan handles, no
     *  opacity/resolutionScale fields). The GPU-side ShaderInstance each
     *  Graphics builds from it (ShaderInstance::build(),
     *  resource/jam_VulkanShaderInstance.cpp) consumes every field it needs
     *  synchronously inside that one build() call — SPIR-V is compiled into
     *  vk::ShaderModules — and retains no reference back to this Shader
     *  afterward. So replacing or clearing postProcess here, even while a
     *  prior content hash's ShaderInstance is still draining through a
     *  window's deferred-destroy queue (VulkanShaderRegistry::previousShaderInstances,
     *  owned by Graphics::getShaderRegistry()),
     *  destroys no GPU-side object those in-flight resources still depend on.
     *  @param shader           The compiled chain to install, or nullptr to clear it
     *                          (every window's endFrame() then runs its unmodified
     *                          identity composite).
     *  @param opacity          Overall blend opacity, [0, 1] — stored alongside shader.
     *  @param resolutionScale  Intermediate-pass extent fraction, [0, 1] — stored
     *                          alongside shader. */
    void setPostProcess (std::unique_ptr<VulkanShader> shader, float opacity, float resolutionScale) noexcept
    {
        postProcess = std::move (shader);
        postProcessOpacity = opacity;
        postProcessResolutionScale = resolutionScale;
    }

    /** @brief Cheap parameter-only update — no shader replace. Updates the
     *  stored opacity/resolutionScale read back by the next getPostProcess()
     *  call. Assert-free, positive-check simple: harmless to call with no
     *  chain currently installed (the stored values are simply read back
     *  unused by endFrame()'s no-postProcess branch until a chain is next
     *  installed via setPostProcess()).
     *  @param opacity          New overall blend opacity, [0, 1].
     *  @param resolutionScale  New intermediate-pass extent fraction, [0, 1]. */
    void setPostProcessParams (float opacity, float resolutionScale) noexcept
    {
        postProcessOpacity = opacity;
        postProcessResolutionScale = resolutionScale;
    }

    /** @brief Returns the currently installed app-global post-process chain
     *  together with its opacity/resolutionScale, pulled together in one call.
     *  Every window's Graphics calls this once per endFrame() (pull, never
     *  pushed) and compares the returned shader's Shader::contentHash against
     *  its own cached value to detect a chain that changed since the last
     *  frame. */
    PostProcess getPostProcess() const noexcept
    {
        return { postProcess.get(), postProcessOpacity, postProcessResolutionScale };
    }
#endif

private:
    static constexpr double highRefreshFrameBudgetMs { 5.8 };
    static constexpr double standardRefreshFrameBudgetMs { 11.1 };
    static constexpr double highRefreshRateThresholdHz { 120.0 };
    static constexpr double indeterminateRefreshRateHz { 60.0 };

    /** @brief Default value for the constructor's targetFrameBudgetMs
     *  parameter — resolves the per-frame time budget from the primary
     *  display's refresh rate. */
    static double getFrameBudget() noexcept
    {
        const auto* primaryDisplay { juce::Desktop::getInstance().getDisplays().getPrimaryDisplay() };
        const auto refreshRateHz { primaryDisplay != nullptr
                                       ? primaryDisplay->verticalFrequencyHz.value_or (indeterminateRefreshRateHz)
                                       : indeterminateRefreshRateHz };

        const auto budget { refreshRateHz >= highRefreshRateThresholdHz ? highRefreshFrameBudgetMs
                                                                       : standardRefreshFrameBudgetMs };

        return budget;
    }

    /** @brief Default value for the constructor's pipelineCacheFile parameter
     *  — resolves the Vulkan pipeline-cache file under the user's settings
     *  directory, named after projectName. */
    static juce::File getPipelineCacheFile()
    {
        const auto cacheDir { File::getOrCreateDirectory (File::getUserDirectory(), Id::cache) };
        const auto cacheFile { cacheDir.getChildFile (Format::toFileName (ProjectInfo::projectName, Id::cache)) };

        return cacheFile;
    }

    /** @brief GPU route (gated on isGpuAvailable() and device.isComputeCapable())
     *  with CPU fallback — runs exactly once, from a single tail call site —
     *  on GPU unavailability, on a source that fails to resolve a bindless
     *  index this call, or on any Result the GPU route's @p record returns
     *  other than eSuccess. This is the module's one GPU/CPU error boundary:
     *  a non-eSuccess Result is logged; eErrorDeviceLost additionally defers a
     *  VulkanEngine::reinitialiseDevice() call via
     *  juce::MessageManager::callAsync() — this method runs on the message
     *  thread mid-callback, so the deferral is the same reentrancy shape used
     *  by every FrameDisposition::reinitialise arm (never call
     *  reinitialiseDevice() synchronously from inside a call it was itself
     *  triggered by). @p fallback still runs this same call, completing the
     *  operation on the CPU route while the reinitialise is queued. */
    template <typename RecordOp, typename FallbackOp>
    void computeImageOperation (juce::Image& destination, const juce::Image& source,
                                RecordOp&& record, FallbackOp&& fallback)
    {
        const int width { source.getWidth() };
        const int height { source.getHeight() };

        if (isGpuAvailable() and device.isComputeCapable())
        {
            auto& graphics { blurRenderer.getGraphics() };
            graphics.cacheImageTexture (source);
            const int srcBindlessIndex { graphics.getBindlessIndex (source) };

            if (srcBindlessIndex >= 0)
            {
                const vk::Result result { record (width, height, srcBindlessIndex) };

                if (result == vk::Result::eSuccess)
                {
                    copyStagingToDestination (destination, source, width, height);
                    return;
                }

                debug::Log::write ("VulkanEngine: compute lane failed:", vk::to_string (result));

                if (result == vk::Result::eErrorDeviceLost)
                {
                    juce::MessageManager::callAsync ([]
                    {
                        if (auto* reinitialiseEngine { VulkanEngine::getInstance() })
                            reinitialiseEngine->reinitialiseDevice();
                    });
                }
            }
        }

        fallback();
    }



    /** @brief Records the compute dispatch plus its readback copy into
     *  blurStaging, submits the one-shot command buffer, and waits.
     *  @return submitAndWait()'s Result — eSuccess once the readback content
     *          is available in blurStaging. */
    template <typename ComputeDispatch>
    vk::Result recordComputeAndReadback (int width, int height, ComputeDispatch&& dispatch)
    {
        const vk::DeviceSize requiredBytes { static_cast<vk::DeviceSize> (width) * static_cast<vk::DeviceSize> (height) * bgraPixelStride };

        if (not blurStaging.isValid() or blurStaging.getSize() < requiredBytes)
            blurStaging = createReadbackStagingBuffer (device.getAllocator(), requiredBytes);

        const vk::CommandBuffer cmd { blurRenderer.beginCommands() };
        const vk::Buffer resolveBuffer { dispatch (cmd) };

        const vk::BufferCopy region { 0, 0, requiredBytes };
        cmd.copyBuffer (resolveBuffer, blurStaging.getBuffer(), region);

        return blurRenderer.submitAndWait();
    }

    vk::Result recordBlurAndReadback (int width, int height, int radius, int srcBindlessIndex)
    {
        return recordComputeAndReadback (width, height, [&] (vk::CommandBuffer cmd)
        {
            return blurRenderer.getGraphics().getImageEffects().blurImage (cmd, width, height, srcBindlessIndex, radius);
        });
    }

    vk::Result recordChokeAndReadback (int width, int height, int radius, int srcBindlessIndex)
    {
        return recordComputeAndReadback (width, height, [&] (vk::CommandBuffer cmd)
        {
            return blurRenderer.getGraphics().getImageEffects().applyMatteChoke (cmd, width, height, srcBindlessIndex, radius);
        });
    }

    vk::Result recordFeatherAndReadback (int width, int height, float radius, float curve, int srcBindlessIndex)
    {
        return recordComputeAndReadback (width, height, [&] (vk::CommandBuffer cmd)
        {
            return blurRenderer.getGraphics().getImageEffects().applyMatteFeather (cmd, width, height, srcBindlessIndex, radius, curve);
        });
    }

    /** @brief Copies recordBlurAndReadback()'s BGRA8 staging buffer content
     *  into destination, reallocating destination to source's own
     *  width/height/juce::Image::PixelFormat on mismatch — format-preserving:
     *  a SingleChannel source/destination extracts only the alpha byte
     *  (bgraAlphaOffset) per texel, any other format copies all four
     *  interleaved BGRA8 bytes verbatim per row.
     *  @param destination  Blurred output — reallocated on size/format mismatch.
     *  @param source       Original source image — supplies destination's target
     *                      format on reallocation; must not be juce::Image::RGB.
     *  @param width        Staged content width in pixels.
     *  @param height       Staged content height in pixels.
     */
    void copyStagingToDestination (juce::Image& destination, const juce::Image& source, int width, int height)
    {
        jassert (source.getFormat() != juce::Image::RGB);
        const bool destinationMismatched { not destination.isValid()
                                            or destination.getWidth() != width
                                            or destination.getHeight() != height
                                            or destination.getFormat() != source.getFormat() };

        if (destinationMismatched)
            destination = juce::Image (source.getFormat(), width, height, false, juce::SoftwareImageType());

        const juce::Image::BitmapData destinationData { destination, juce::Image::BitmapData::writeOnly };
        const auto* stagingBytes { static_cast<const uint8_t*> (blurStaging.getMapped()) };
        if (destination.getFormat() == juce::Image::SingleChannel)
        {
            for (int y = 0; y < height; ++y)
            {
                uint8_t* destRow { destinationData.getLinePointer (y) };
                const uint8_t* srcRow { stagingBytes + static_cast<size_t> (y) * static_cast<size_t> (width) * bgraPixelStride };
                for (int x = 0; x < width; ++x)
                    destRow[x] = srcRow[x * bgraPixelStride + bgraAlphaOffset];
            }
        }
        else
        {
            const size_t rowBytes { static_cast<size_t> (width) * bgraPixelStride };
            for (int y = 0; y < height; ++y)
                std::memcpy (destinationData.getLinePointer (y), stagingBytes + static_cast<size_t> (y) * rowBytes, rowBytes);
        }
    }

    /** @brief CPU stack-blur route — reallocates destination to source's own
     *  width/height/juce::Image::PixelFormat on mismatch, converts source into
     *  it, and blurs in place via jam::stackBlurSingleChannel (SingleChannel
     *  destination) or jam::stackBlurArgb (every other format). Reached by
     *  blurImage() whenever the GPU route is unavailable or the source failed
     *  to resolve a bindless index this call.
     *  @param destination  Blurred output — reallocated on size/format mismatch.
     *  @param source       Image to blur.
     *  @param radius       Blur radius in pixels.
     */
    void blurImageFallback (juce::Image& destination, const juce::Image& source, int radius)
    {
        const bool destinationMismatched { destination.getWidth() != source.getWidth()
                                            or destination.getHeight() != source.getHeight()
                                            or destination.getFormat() != source.getFormat() };

        if (destinationMismatched)
            destination = juce::Image (source.getFormat(), source.getWidth(), source.getHeight(), false, juce::SoftwareImageType());

        const juce::Image::BitmapData sourceData { source, juce::Image::BitmapData::readOnly };
        juce::Image::BitmapData destinationData { destination, juce::Image::BitmapData::readWrite };
        destinationData.convertFrom (sourceData);

        if (destination.getFormat() == juce::Image::SingleChannel)
            jam::stackBlurSingleChannel (destinationData, radius);
        else
            jam::stackBlurArgb (destinationData, radius);
    }

    void chokeImageFallback (juce::Image& destination, const juce::Image& source, int radius)
    {
        destination = source.createCopy();
        juce::Image::BitmapData destinationData { destination, juce::Image::BitmapData::readWrite };
        Graphics::applyMatteChoke (destinationData, radius);
    }

    void featherImageFallback (juce::Image& destination, const juce::Image& source, float radius, float curve)
    {
        destination = source.createCopy();
        juce::Image::BitmapData destinationData { destination, juce::Image::BitmapData::readWrite };
        Graphics::applyMatteFeather (destinationData, radius, curve);
    }

    /** @brief Returns the Graphics already keyed by @p handle, creating one via
     *  VulkanGraphics::create() when absent — the side effect is in the name.
     *  The found branch returns the stored Graphics immediately; the absent
     *  branch creates one, captures the
     *  raw pointer before ownership moves into contexts, and returns that
     *  pointer. Shared by every platform's createContext() below. Windows
     *  builds carry one further argument, opaquePeer, forwarded to
     *  VulkanGraphics::create() under the same conditional compilation as
     *  that factory's own signature.
     *  @param handle          The native peer handle to look up or create for.
     *  @param physicalWidth   Swapchain width in physical pixels for a new Graphics.
     *  @param physicalHeight  Swapchain height in physical pixels for a new Graphics.
     *  @return Raw, non-owning pointer to the stored Graphics, or nullptr when
     *          absent and creation failed.
     */
    VulkanGraphics* getOrCreateGraphics (void* handle, int physicalWidth, int physicalHeight
#if JUCE_WINDOWS
                                          , bool opaquePeer
#endif
        )
    {
        if (contexts.contains (handle))
            return contexts.at (handle).get();

        auto graphics { VulkanGraphics::create (device, handle, physicalWidth, physicalHeight,
                                                 targetFrameBudgetMs, pipelineCache.getPipelineCache()
#if JUCE_WINDOWS
                                                 , opaquePeer
#endif
                                                 ) };

        if (graphics == nullptr)
        {
            if (isGpuAvailable())
                debug::Log::write ("VulkanEngine: getOrCreateGraphics: VulkanGraphics::create failed");

            return nullptr;
        }

        auto* result { graphics.get() };
        contexts.emplace (handle, std::move (graphics));
        return result;
    }

    /** @brief Static factory registered on juce::ComponentPeer::externalContextFactory.
     *
     *  GPU lane requires BOTH engine->isGpuAvailable() AND the peer's native
     *  handle not being the engine's teardownHandle — a handle mid-teardown
     *  (removePeer (juce::Component&) has marked it, Graphics already erased,
     *  removeFromDesktop() not yet returned) takes the CPU lane instead of
     *  lazily creating a fresh Graphics (new surface/swapchain) on a window
     *  about to die. This VulkanEngine, its Device, and its glyphAtlas exist
     *  unconditionally regardless of the GPU route's enabled state — only
     *  this branch's selection changes (class doc comment).
     *  Lazily constructs a Graphics for the peer on first paint (emplace on miss),
     *  then handles swapchain resize and beginFrame in positive nesting.
     *  jam::VulkanLowLevelGraphicsContext destructor calls context.endFrame() after
     *  handlePaint returns.
     *
     *  Constructs a jam::LowLevelGraphicsGlyphRenderer (CPU fallback LLGC,
     *  atlas-backed glyph rendering) instead of ever returning
     *  nullptr, keeping the goal of JUCE's own default renderers
     *  (CoreGraphicsContext/Direct2DGraphicsContext) never running, for
     *  anything. One construction call site (SSOT) is reached both when GPU is
     *  categorically unavailable/disabled and when the GPU-available branch above
     *  transiently failed this frame (Graphics::create()/beginFrame() returning
     *  false) — either way, `result` is still nullptr when that call site is
     *  reached.
     */
#if JUCE_MAC
    static std::unique_ptr<juce::LowLevelGraphicsContext> createContext (juce::ComponentPeer& peer)
    {
        auto* engine { getInstance() };
        jassert (engine != nullptr);

        auto& component { peer.getComponent() };
        const auto width  { component.getWidth() };
        const auto height { component.getHeight() };

        const auto scaleFactor { getScaleFactor (peer.getNativeHandle()) };

        std::unique_ptr<juce::LowLevelGraphicsContext> result { nullptr };

        // Computed once here (moved out of the GPU-available branch below,
        // its sole reader until now) so the CPU-fallback branch can also size its
        // owned target image to physical pixels.
        const auto physicalWidth  { static_cast<int> (width  * scaleFactor) };
        const auto physicalHeight { static_cast<int> (height * scaleFactor) };

        if (engine->isGpuAvailable() and peer.getNativeHandle() != engine->teardownHandle)
        {
            auto* handle { peer.getNativeHandle() };

            if (auto* graphics { engine->getOrCreateGraphics (handle, physicalWidth, physicalHeight) })
            {
                const auto extent { graphics->getSwapchainExtent() };

                if (extent.width  != static_cast<uint32_t> (physicalWidth)
                    or extent.height != static_cast<uint32_t> (physicalHeight))
                {
                    graphics->resize (width, height, scaleFactor);
                }

                if (graphics->beginFrame())
                {
                    graphics->incrementContextCount();
                    result = std::make_unique<VulkanLowLevelGraphicsContext> (
                        *graphics, width, height, scaleFactor);

                    // Persisted scene content already holds every untouched pixel —
                    // narrow this frame's recording to the peer's own recovered
                    // dirty rect instead of the full-window clip the constructor
                    // above just established, AND transparent-clear that same
                    // device-space rect first (AppKit itself clears a non-opaque
                    // window's dirty rect before drawRect: — a LOAD frame here
                    // otherwise never clears color, so translucent repaints blend
                    // onto stale prior-frame pixels instead of starting fresh).
                    // Skipped when the scene target was just cleared
                    // (isSceneContentValid() false): that frame must record
                    // full-window regardless of what the peer reports dirty.
                    if (graphics->isSceneContentValid())
                    {
                        const auto dirtyBounds { jam::currentPeerDirtyBounds (peer) };

                        // dirtyBounds is logical (points); the scene target is
                        // physical pixels. Reproduces LowLevelGraphicsContext::
                        // clipToRectangle()'s own device-space transform exactly
                        // (jam_VulkanTransformState.h's boundsAfterTransform:
                        // AffineTransform::scale (scaleFactor) applied then
                        // rounded via getSmallestIntegerContainer) so the cleared
                        // region equals the clip clipToRectangle() below computes
                        // for the same dirtyBounds — never narrower than the clip.
                        const auto physicalDirtyBounds { dirtyBounds.toFloat()
                            .transformedBy (juce::AffineTransform::scale (scaleFactor))
                            .getSmallestIntegerContainer() };

                        graphics->clearSceneColorRegion (physicalDirtyBounds);
                        result->clipToRectangle (dirtyBounds);
                    }
                }
            }
        }

    #if JUCE_GRAPHICS_INCLUDE_COREGRAPHICS_HELPERS
        if (result == nullptr)
        {
            // CPU lane — built directly on the peer's own native CGContextRef, no
            // owned target image, no self-presentation.
            NSGraphicsContext* nsContext { [NSGraphicsContext currentContext] };
            jassert (nsContext != nullptr);

            CGContextRef cg { nsContext.CGContext };
            CGContextConcatCTM (cg, CGAffineTransformMake (1, 0, 0, -1, 0, height));

            result = std::make_unique<jam::LowLevelGraphicsGlyphRenderer> (
                cg, static_cast<float> (height), engine->glyphAtlas, scaleFactor);
        }
    #endif

        return result;
    }
#elif JUCE_WINDOWS
    static std::unique_ptr<juce::LowLevelGraphicsContext> createContext (juce::ComponentPeer& peer)
    {
        auto* engine { getInstance() };
        jassert (engine != nullptr);

        auto& component { peer.getComponent() };
        const auto width  { component.getWidth() };
        const auto height { component.getHeight() };

        const auto scaleFactor { static_cast<float> (peer.getPlatformScaleFactor()) };

        std::unique_ptr<juce::LowLevelGraphicsContext> result { nullptr };

        // Computed once here (moved out of the GPU-available branch below,
        // its sole reader until now) so the CPU-fallback branch can also size its
        // owned target image to physical pixels.
        const auto physicalWidth  { static_cast<int> (width  * scaleFactor) };
        const auto physicalHeight { static_cast<int> (height * scaleFactor) };

        // Selects WSI vs Composition inside Graphics::create below — non-opaque
        // peers enter the GPU branch and route through the Composition leg;
        // they reach the software glyph-renderer path only when
        // Graphics::create returns nullptr (interop unsupported for that peer).
        const bool opaquePeer { peer.getComponent().isOpaque() };

        if (engine->isGpuAvailable() and peer.getNativeHandle() != engine->teardownHandle)
        {
            auto* handle { peer.getNativeHandle() };

            if (auto* graphics { engine->getOrCreateGraphics (handle, physicalWidth, physicalHeight, opaquePeer) })
            {
                // HWND client rect is the true physical extent on Windows —
                // width * scaleFactor truncates and drifts from what the
                // compositor actually reports for the window's client area.
                const auto clientExtent    { graphics->getSurfaceExtent() };
                const auto swapchainExtent { graphics->getSwapchainExtent() };

                if ((clientExtent.width != swapchainExtent.width
                        or clientExtent.height != swapchainExtent.height)
                    and clientExtent.width > 0
                    and clientExtent.height > 0)
                {
                    graphics->resize (width, height, scaleFactor);
                }

                if (graphics->beginFrame())
                {
                    graphics->incrementContextCount();
                    result = std::make_unique<VulkanLowLevelGraphicsContext> (
                        *graphics, width, height, scaleFactor);

                    // Same sceneContentValid dirty-clip narrowing as the JUCE_MAC block
                    // above, inverted: peer.getLastPaintUpdateRect() is already physical
                    // (HWND client-rect px), so clearSceneColorRegion() consumes it
                    // directly and clipToRectangle() needs the logical conversion instead
                    // of mac's logical -> physical one. That logical scale is derived from
                    // component size vs swapchainExtent rather than 1 / scaleFactor —
                    // peer.getPlatformScaleFactor() reports 1 under System-DPI-Aware hosts
                    // (e.g. Pro Tools) even though the host's own DPI virtualization still
                    // scales the physical swapchain against the logical component bounds.
                    // Empty rect (no prior WM_PAINT update recorded yet) skips the clip
                    // entirely — same "must record full-window" semantics as mac's own guard.
                    if (graphics->isSceneContentValid())
                    {
                        const auto physicalDirtyBounds { peer.getLastPaintUpdateRect() };

                        if (not physicalDirtyBounds.isEmpty())
                        {
                            const auto logicalScaleX { static_cast<float> (width)  / static_cast<float> (swapchainExtent.width) };
                            const auto logicalScaleY { static_cast<float> (height) / static_cast<float> (swapchainExtent.height) };

                            const auto dirtyBounds { physicalDirtyBounds.toFloat()
                                .transformedBy (juce::AffineTransform::scale (logicalScaleX, logicalScaleY))
                                .getSmallestIntegerContainer() };

                            graphics->clearSceneColorRegion (physicalDirtyBounds);
                            result->clipToRectangle (dirtyBounds);
                        }
                    }
                }
            }
        }

        if (result == nullptr)
        {
            // CPU lane — fresh ARGB software target this frame, forced
            // SoftwareImageType for direct juce::Image::BitmapData access (both
            // jam::LowLevelGraphicsGlyphRenderer's own compositing and its
            // destructor's native-presentation call read pixels directly, which a
            // platform-native image type would not guarantee).
            juce::Image target (juce::Image::ARGB, physicalWidth, physicalHeight, true, juce::SoftwareImageType());

            result = std::make_unique<jam::LowLevelGraphicsGlyphRenderer> (
                target, engine->glyphAtlas, peer, scaleFactor);
        }

        return result;
    }
#else
    static std::unique_ptr<juce::LowLevelGraphicsContext> createContext (juce::ComponentPeer& peer)
    {
        auto* engine { getInstance() };
        jassert (engine != nullptr);

        auto& component { peer.getComponent() };
        const auto width  { component.getWidth() };
        const auto height { component.getHeight() };

        const auto scaleFactor { static_cast<float> (peer.getPlatformScaleFactor()) };

        std::unique_ptr<juce::LowLevelGraphicsContext> result { nullptr };

        // Computed once here (moved out of the GPU-available branch below,
        // its sole reader until now) so the CPU-fallback branch can also size its
        // owned target image to physical pixels.
        const auto physicalWidth  { static_cast<int> (width  * scaleFactor) };
        const auto physicalHeight { static_cast<int> (height * scaleFactor) };

        if (engine->isGpuAvailable() and peer.getNativeHandle() != engine->teardownHandle)
        {
            auto* handle { peer.getNativeHandle() };

            if (auto* graphics { engine->getOrCreateGraphics (handle, physicalWidth, physicalHeight) })
            {
                const auto extent { graphics->getSwapchainExtent() };

                if (extent.width  != static_cast<uint32_t> (physicalWidth)
                    or extent.height != static_cast<uint32_t> (physicalHeight))
                {
                    graphics->resize (width, height, scaleFactor);
                }

                if (graphics->beginFrame())
                {
                    graphics->incrementContextCount();
                    result = std::make_unique<VulkanLowLevelGraphicsContext> (
                        *graphics, width, height, scaleFactor);
                }
            }
        }

        if (result == nullptr)
        {
            // CPU lane — fresh ARGB software target this frame, forced
            // SoftwareImageType for direct juce::Image::BitmapData access (both
            // jam::LowLevelGraphicsGlyphRenderer's own compositing and its
            // destructor's native-presentation call read pixels directly, which a
            // platform-native image type would not guarantee).
            juce::Image target (juce::Image::ARGB, physicalWidth, physicalHeight, true, juce::SoftwareImageType());

            result = std::make_unique<jam::LowLevelGraphicsGlyphRenderer> (
                target, engine->glyphAtlas, peer, scaleFactor);
        }

        return result;
    }
#endif

    /** @brief Shared Vulkan device — one per application. Declared FIRST so it
     *  destructs LAST — every GPU resource below (glyphAtlas, per-window
     *  contexts) depends on it. Plain direct member (no unique_ptr) — member
     *  order alone is the teardown contract (reverse-declaration-order
     *  destruction). */
    VulkanDevice device;

    // Engine-owned device-lifetime image texture store. Declared immediately
    // after device (constructs after it, destructs before it) and before
    // contexts, so every window's Graphics dies while this store is still alive.
    VulkanTextureCache textureCache;

    /** @brief Typeface identity table — interning table, self-registers via
     *  jam::Instance<Typeface> (SharedResources<Typeface>). Declared before
     *  glyphAtlas so the typeface table outlives the atlas whose keys carry
     *  typeface pointers. */
    jam::Typeface typeface;

    /** @brief Style interning table for the KANJUT rendering pipeline —
     *  self-registers via jam::Instance<Stamp>. Declared before glyphAtlas,
     *  same rationale as typeface above. */
    jam::Stamp stamp;

    /** @brief Grapheme-cluster interning table — self-registers via
     *  jam::Instance<Grapheme>. Declared before glyphAtlas, same rationale as
     *  typeface above. */
    jam::Grapheme grapheme;

    /** @brief Hyperlink (OSC 8) interning table — self-registers via
     *  jam::Instance<Hyperlink>. Declared before glyphAtlas, same rationale as
     *  typeface above. */
    jam::Hyperlink hyperlink;

    /** @brief Shared CPU+GPU glyph atlas. Declared (and therefore destroyed) AFTER
     *  device, typeface, stamp, grapheme, and hyperlink, and BEFORE contexts is declared,
     *  so it destructs AFTER every window's Graphics — this atlas owns real
     *  GPU-resident images/staging buffer; destroying them while a window's command
     *  buffer could still be referencing the atlas's bindless descriptor slot would
     *  be a use-after-free hazard were this declared (and thus destroyed) any
     *  earlier than contexts. Plain direct member, constructed from the
     *  already-initialized device member above (declaration order guarantees it). */
    jam::GlyphAtlas glyphAtlas;

#if KUASSA_VULKAN_RUNTIME_SHADER_COMPILER
    /** @brief ShaderFormat vocabulary registry — engine-lifetime owner. Registers
     *  on the Instance<ShaderFormat> chain at construction, reachable process-wide
     *  via getInstance(). Constructed here (post-main) because its constructor
     *  reads Id:: identifier globals from another translation unit, which forbids
     *  file-scope static construction. No teardown-order dependency on the members
     *  above — placed after glyphAtlas. */
    jam::SharedInstance<VulkanShaderFormat> shaderFormat { std::in_place };
#endif

    /** @brief Shared vk::PipelineCache RAII holder — SSOT, its handle passed
     *  (never owned) into every VulkanPipelines::load() call this engine's
     *  Graphics instances make (imageRenderer, blurRenderer, every window
     *  context). Declared before imageRenderer/blurRenderer/contexts so it
     *  constructs first and destructs last among them. */
    VulkanPipelineCache pipelineCache;

    // Engine-owned shared offscreen renderer for VulkanImagePixelData targets.
    // Declared after glyphAtlas (createOffscreen registers atlas slots, requiring
    // the atlas to be alive) and before contexts (destructs after every window's
    // Graphics — same ordering contract as glyphAtlas itself).
    VulkanImageRenderer imageRenderer;

    // Dedicated shared offscreen renderer for blurImage()'s GPU route —
    // separate from imageRenderer so a blur's own one-shot command
    // buffer/fence never contends with a VulkanImagePixelData target's
    // render/readback cycle. Same device-lifetime ordering contract as
    // imageRenderer above.
    VulkanImageRenderer blurRenderer;

    // Persistent readback staging for recordBlurAndReadback — grow-only,
    // recreated via createReadbackStagingBuffer whenever a blurImage call's
    // required bytes exceed its current VulkanBuffer::getSize().
    VulkanBuffer blurStaging {};

    /** @brief Per-window Graphics instances, keyed by native window handle. */
    jam::HashMap<void*, std::unique_ptr<VulkanGraphics>> contexts;

    /** @brief The native handle removePeer (juce::Component&) is currently
     *  tearing down, or nullptr. Named threat: removeFromDesktop() can pump a
     *  synchronous teardown paint that reaches createContext() for the same
     *  peer before the call returns — without this mark, createContext()
     *  would find no entry in contexts (already erased) and lazily build a
     *  fresh Graphics (new surface/swapchain) on a native view about to die.
     *  createContext() checks this handle to route that paint to the CPU
     *  lane instead. */
    void* teardownHandle { nullptr };

    /** @brief Engine-owned decode/memoization cache for embedded images, keyed
     *  by resource byte pointer. registerImages() is driven once from this
     *  engine's constructor, queueing every embedded resource for background
     *  decode; owns its own juce::ThreadPool internally (see ImageLoader's
     *  own member doc comment for its internal teardown-order rationale). */
    ImageLoader imageLoader;

#if KUASSA_VULKAN_RUNTIME_SHADER_COMPILER
    /** @brief App-global post-process Shader chain, installed/cleared by
     *  setPostProcess(). Pure data — no Vulkan handles, no GPU-side
     *  dependency on this VulkanEngine's own device/glyphAtlas/contexts — so its
     *  declaration position carries no destruction-order constraint the way
     *  those carry; placed here purely because it is, like contexts, per-session
     *  mutable state rather than a constructor-fixed value. nullptr until a
     *  setPostProcess() call installs a chain — no call site in KANJUT does, so
     *  it stays nullptr and every window's endFrame() runs its unmodified
     *  identity composite. */
    std::unique_ptr<VulkanShader> postProcess;

    /** @brief postProcess's overall blend opacity, [0, 1] — set alongside
     *  postProcess by setPostProcess(), updated independently by the cheaper
     *  setPostProcessParams(). Meaningless while postProcess is nullptr. */
    float postProcessOpacity { 1.0f };

    /** @brief postProcess's intermediate-pass extent fraction, [0, 1] — same
     *  update pattern as postProcessOpacity. Meaningless while postProcess is
     *  nullptr. */
    float postProcessResolutionScale { 1.0f };
#endif

    /** @brief Per-frame time budget (ms) passed down into every Graphics this
     *  VulkanEngine creates — see the constructor's doc comment. createContext() is a
     *  fixed-signature JUCE callback (juce::ComponentPeer::externalContextFactory,
     *  ComponentPeer& only), so this value cannot be threaded through createContext()'s
     *  own parameter list; it crosses the KANJUT/application boundary via the constructor instead
     *  and is read back here at Graphics::create() call time. Set by the
     *  constructor's targetFrameBudgetMs parameter, which defaults to
     *  getFrameBudget()'s refresh-rate-resolved value — jam::PluginEditor
     *  constructs the engine with maxImageExtent only, so that default is what
     *  this member holds. */
    double targetFrameBudgetMs;

    /** @brief Largest offscreen render-target extent imageRenderer/blurRenderer
     *  are seeded with — set once by the constructor's maxImageExtent parameter,
     *  retained so reinitialiseDevice() can replay the exact same
     *  imageRenderer.initialise()/blurRenderer.initialise() call the constructor
     *  made, without requiring the caller to supply it again. */
    vk::Extent2D maxImageExtent;

    /** @brief Single source of truth for GPU capability — true once the
     *  synchronous GPU-initialisation chain completes and the shared Device is
     *  valid. Persistent for the engine's lifetime — reinitialiseDevice() is
     *  the only writer after construction. */
    std::atomic<bool> gpuAvailable { false };

    // Shipped OOTB font-rasterization defaults (freetype backend, gamma 2.2,
    // contrast 0.0) — this atlas's own hard, style.css-independent starting
    // point, applied unconditionally in the ctor body. edgeTable's thin aliased
    // identity default (jam_GlyphAtlas.h's own "used before any
    // setRasterization() call" doc comment on its `backend` member) never runs.
    static constexpr float defaultFontGamma { 2.2f };
    static constexpr float defaultFontContrast { 0.0f };

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VulkanEngine)
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
