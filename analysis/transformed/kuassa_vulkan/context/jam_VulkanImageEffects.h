namespace jam
{
/*____________________________________________________________________________*/

/** @brief Owns the compute-shader image-effect passes (stack blur, matte
 *  choke, matte feather) shared by every VulkanGraphics frame — the blur/
 *  matte device-local storage buffers, their set-2 descriptor sets, and the
 *  bindless texture array descriptor set (set 1) each dispatch binds. Called
 *  only from VulkanGraphics, which owns one instance per surface and threads
 *  its own descriptor pool and bindless set into createDescriptorSets(). */
class VulkanImageEffects
{
public:
    //==========================================================================
    // Constructor
    //==========================================================================

    /** @brief Stores the shared device/pipelines references. Allocates
     *  nothing — buffers and descriptor sets are created lazily on first
     *  use (reserveBlurBuffers()/reserveMatteBuffer()) and by
     *  createDescriptorSets().
     *  @param device     Shared Vulkan device.
     *  @param pipelines  Shared pipeline collection — supplies the compute
     *                    pipelines and layouts every dispatch binds. */
    explicit VulkanImageEffects (VulkanDevice& device, VulkanPipelines& pipelines);

    //==========================================================================
    // Descriptor sets
    //==========================================================================

    /** @brief Allocates the blur-transpose, blur-output, and matte-output set-2
     *  descriptor sets from @p pool, and re-writes them against the
     *  already-grown blurTranspose/blurOutput/matteOutput buffers when those
     *  buffers are already valid.
     *
     *  Re-arm-after-pool-reset contract: the caller's descriptor pool
     *  (VulkanGraphics::descriptorPool) is reset once per depth-0 frame
     *  (device.resetDescriptorPool()), which implicitly frees every set
     *  previously allocated from it — including the three sets this class
     *  owns. This method is called again immediately after every such reset
     *  (beginRecording(), beginOffscreenFrame() depth 0) as well as once at
     *  createDrawState() time, before any buffer has grown. It always
     *  re-allocates fresh set handles; it re-writes them against the
     *  underlying buffers only when those buffers already exist from a prior
     *  frame — a first-time call with no buffers yet allocated writes
     *  nothing, and the first reserveBlurBuffers()/reserveMatteBuffer() call
     *  after that performs the initial write.
     *  @param pool                          Descriptor pool to allocate from —
     *                                       VulkanGraphics::descriptorPool.
     *  @param newBindlessTextureDescriptorSet  The bindless texture array
     *                                       descriptor set (set 1), stored and
     *                                       bound by every dispatch in this class.
     *  @return true if every allocateDescriptorSets call succeeded, false on
     *          the first failure — the caller must not dispatch when false. */
    bool createDescriptorSets (vk::DescriptorPool pool, vk::DescriptorSet newBindlessTextureDescriptorSet);

    //==========================================================================
    // Blur
    //==========================================================================

    /** @brief Records the two-dispatch compute stack blur and returns the
     *  device-local buffer holding the result.
     *
     *  Pass 1 (ComputeID::stackBlurTexture) reads @p bindlessSourceIndex through
     *  the bindless sampled-image array and writes transposed into blurTranspose
     *  (one invocation per source row, output `[x * height + y]`). A buffer
     *  memory barrier (`eShaderWrite -> eShaderRead`) separates the two passes.
     *  Pass 2 (ComputeID::stackBlurBuffer) walks blurTranspose's now-`height x
     *  width` layout and writes transposed again into blurOutput, restoring the
     *  original `width x height` row-major orientation — the second transpose is
     *  why the caller receives a buffer, not an image: neither pass ever writes
     *  through an image view. A final barrier (`eShaderWrite -> eTransferRead`)
     *  leaves blurOutput ready for the caller's own copy command.
     *  @param cmd                 Active command buffer to record both dispatches into.
     *  @param width               Source/destination width in pixels.
     *  @param height              Source/destination height in pixels.
     *  @param bindlessSourceIndex Bindless array slot of the cached source texture.
     *  @param radius              Blur radius in pixels.
     *  @return blurOutput's buffer handle, valid until the next blurImage() call
     *          on this VulkanImageEffects reserves a larger buffer.
     */
    vk::Buffer blurImage (vk::CommandBuffer cmd, int width, int height, int bindlessSourceIndex, int radius);

    /** @brief Records a single ComputeID::matteChoke dispatch — erodes the alpha
     *  channel of the source texture by @p radius pixels (two-pass sliding-window
     *  minimum, hardware-side) and returns the device-local output buffer.
     *  A buffer memory barrier (`eShaderWrite -> eTransferRead`) is appended so the
     *  caller can immediately issue a copy command.
     *  @param cmd                 Active command buffer to record the dispatch into.
     *  @param width               Source/destination width in pixels.
     *  @param height              Source/destination height in pixels.
     *  @param sourceBindlessIndex Bindless array slot of the source texture.
     *  @param radius              Erosion radius in pixels.
     *  @return Device-local output buffer handle, valid until the next applyMatteChoke()
     *          call that requires a larger buffer. */
    vk::Buffer applyMatteChoke (vk::CommandBuffer cmd, int width, int height,
                                int sourceBindlessIndex, int radius);

    /** @brief Records a single ComputeID::matteFeather dispatch — distance-field
     *  feathers the alpha channel of the source texture by @p radius pixels and returns
     *  the device-local output buffer. A buffer memory barrier (`eShaderWrite ->
     *  eTransferRead`) is appended so the caller can immediately issue a copy command.
     *  @param cmd                 Active command buffer to record the dispatch into.
     *  @param width               Source/destination width in pixels.
     *  @param height              Source/destination height in pixels.
     *  @param sourceBindlessIndex Bindless array slot of the source texture.
     *  @param radius              Feather radius in pixels.
     *  @param curve               Exponent applied to the distance-to-alpha mapping.
     *  @return Device-local output buffer handle, valid until the next applyMatteFeather()
     *          call that requires a larger buffer. */
    vk::Buffer applyMatteFeather (vk::CommandBuffer cmd, int width, int height,
                                  int sourceBindlessIndex, float radius, float curve);

    //==========================================================================
    // Deferred release
    //==========================================================================

    /** @brief Destroys previousBlurBuffers and previousMatteBuffers — the
     *  buffers retired by reserveBlurBuffers()/reserveMatteBuffer() on growth,
     *  kept alive until the command buffer that last referenced them has
     *  finished executing. Called once, at the start of a depth-0
     *  beginOffscreenFrame() — the offscreen render cycle submits and waits
     *  on offscreenFence synchronously before returning, so any prior
     *  submission referencing the retired buffers has already completed by
     *  the time this call is reached. */
    void releaseRetiredBuffers();

private:
    //==========================================================================
    // Private helpers
    //==========================================================================

    /** @brief Grows blurTranspose/blurOutput to at least @p width * @p height *
     *  bgraPixelStride bytes and rewrites blurTransposeDescriptorSet/
     *  blurOutputDescriptorSet only when growth actually occurs. Both buffers are
     *  device-local (`VMA_MEMORY_USAGE_GPU_ONLY`) and never mapped — the compute
     *  passes are the only writers, and blurOutput's contents leave the GPU only
     *  via the caller's own copy command. Grow-only: never shrinks or reallocates
     *  on a call that fits within the current capacity.
     *  @param width   Blur output width in pixels.
     *  @param height  Blur output height in pixels.
     */
    void reserveBlurBuffers (int width, int height);

    /** @brief Grows matteOutput to at least @p width * @p height *
     *  bgraPixelStride bytes and rewrites matteOutputDescriptorSet only when
     *  growth actually occurs. Device-local (`VMA_MEMORY_USAGE_GPU_ONLY`) and
     *  never mapped — applyMatteChoke()/applyMatteFeather()'s dispatch is the
     *  only writer, and matteOutput's contents leave the GPU only via the
     *  caller's own copy command. Grow-only: never shrinks or reallocates on
     *  a call that fits within the current capacity.
     *  @param width   Matte output width in pixels.
     *  @param height  Matte output height in pixels.
     */
    void reserveMatteBuffer (int width, int height);

    /** @brief Records one compute-stack-blur dispatch: binds @p id's pipeline,
     *  the bindless texture array (set 1) and @p storageSet (set 2), pushes
     *  VulkanStackBlurPushConstants, then dispatches
     *  `ceil (lineCount / stackBlurWorkgroupSize)` workgroups along X — one
     *  invocation per line, so lineCount alone (not lineLength) determines the
     *  dispatch size.
     *  @param cmd                Active command buffer to record the dispatch into.
     *  @param id                 Which compute pipeline to bind (texture or buffer variant).
     *  @param storageSet         Set 2 descriptor set bound for this pass — the caller
     *                            selects blurTransposeDescriptorSet or blurOutputDescriptorSet.
     *  @param radius             Blur radius in elements.
     *  @param lineCount          Number of lines this dispatch walks.
     *  @param lineLength         Number of elements per line.
     *  @param sourceTextureIndex Bindless array index; consumed only by the
     *                            texture-variant pipeline's loadElement().
     */
    void dispatchStackBlur (vk::CommandBuffer cmd, VulkanPipelines::ComputeID id, vk::DescriptorSet storageSet,
                            int radius, int lineCount, int lineLength, uint32_t sourceTextureIndex);

    /** @brief Reserves matteOutput at @p width * @p height, binds @p id's
     *  compute pipeline plus the bindless texture array (set 1) and
     *  matteOutputDescriptorSet (set 2), pushes @p pushConstants, dispatches
     *  `ceil (width / matteWorkgroupSize) x ceil (height / matteWorkgroupSize)`
     *  workgroups, and appends a buffer memory barrier
     *  (`eShaderWrite -> eTransferRead`) so the caller can immediately issue a
     *  copy command. Shared by applyMatteChoke() and applyMatteFeather() —
     *  the two differ only in which compute pipeline and push-constant type
     *  they supply.
     *  @param cmd            Active command buffer to record the dispatch into.
     *  @param id             Which compute pipeline to bind (matteChoke or matteFeather).
     *  @param pushConstants  The caller's own push-constant block for @p id.
     *  @param width          Source/destination width in pixels.
     *  @param height         Source/destination height in pixels.
     */
    template <typename MattePushConstants>
    void dispatchMatte (vk::CommandBuffer cmd, VulkanPipelines::ComputeID id,
                        const MattePushConstants& pushConstants, int width, int height)
    {
        reserveMatteBuffer (width, height);

        const vk::PipelineLayout layout { pipelines.getComputeLayout() };

        cmd.bindPipeline (vk::PipelineBindPoint::eCompute, pipelines.getComputePipeline (id));
        cmd.bindDescriptorSets (vk::PipelineBindPoint::eCompute, layout, 1, bindlessTextureDescriptorSet, nullptr);
        cmd.bindDescriptorSets (vk::PipelineBindPoint::eCompute, layout, 2, matteOutputDescriptorSet, nullptr);

        cmd.pushConstants (layout, vk::ShaderStageFlagBits::eCompute, 0, sizeof (pushConstants), &pushConstants);

        const uint32_t groupCountX { static_cast<uint32_t> ((width + matteWorkgroupSize - 1) / matteWorkgroupSize) };
        const uint32_t groupCountY { static_cast<uint32_t> ((height + matteWorkgroupSize - 1) / matteWorkgroupSize) };
        cmd.dispatch (groupCountX, groupCountY, 1);

        const vk::BufferMemoryBarrier outputBarrier { vk::AccessFlagBits::eShaderWrite, vk::AccessFlagBits::eTransferRead,
            VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, matteOutput.getBuffer(), 0, vk::WholeSize };
        cmd.pipelineBarrier (vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eTransfer,
                             {}, nullptr, outputBarrier, nullptr);
    }

    //==========================================================================
    // Members
    //==========================================================================

    VulkanDevice& device;
    VulkanPipelines& pipelines;

    /** @brief Device-local storage buffer holding blurImage() pass 1's
     *  transposed output and pass 2's read source. Sized/grown by
     *  reserveBlurBuffers(); never mapped. */
    VulkanBuffer blurTranspose {};

    /** @brief Device-local storage buffer holding blurImage() pass 2's final,
     *  original-orientation output — the buffer copied from by the caller
     *  (VulkanEngine::recordBlurAndReadback()). Sized/grown by
     *  reserveBlurBuffers(); never mapped. */
    VulkanBuffer blurOutput {};
    VulkanBuffer matteOutput {};

    jam::Owner<VulkanBuffer> previousBlurBuffers {};
    jam::Owner<VulkanBuffer> previousMatteBuffers {};

    /** @brief Set 2 descriptor set for blurImage()'s pass 1 — binding 0 is left
     *  unwritten (pass 1's texture-variant shader reads through the bindless
     *  array, never its own input SSBO, so binding 0 is not statically used),
     *  binding 1 writes blurTranspose. Rewritten by reserveBlurBuffers() only
     *  on growth. */
    vk::DescriptorSet blurTransposeDescriptorSet {};

    /** @brief Set 2 descriptor set for blurImage()'s pass 2 — binding 0 reads
     *  blurTranspose, binding 1 writes blurOutput. Two descriptor sets exist over
     *  one computeStorageLayout because the two passes bind opposite roles
     *  (pass 1 writes what pass 2 reads) — one set cannot express both binding
     *  combinations simultaneously. Rewritten by reserveBlurBuffers() only on growth. */
    vk::DescriptorSet blurOutputDescriptorSet {};

    vk::DescriptorSet matteOutputDescriptorSet {};

    vk::DescriptorSet bindlessTextureDescriptorSet {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VulkanImageEffects)
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
