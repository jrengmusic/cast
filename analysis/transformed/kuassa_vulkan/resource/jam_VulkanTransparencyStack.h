namespace jam
{
/*____________________________________________________________________________*/
/** @brief Owns the stack of offscreen transparency-layer render targets for a VulkanGraphics surface.
 *
 *  One TransparencyLayer per nesting level, created on demand by getOrCreateLayer()
 *  at the current extent (set by resize()). Window-extent recreation lives exclusively
 *  in resize() — getOrCreateLayer() never compares or recreates.
 *
 *  Not copyable, not movable — holds a VulkanDevice& reference, same pattern as VulkanGraphics.
 */
class VulkanTransparencyStack
{
public:
    //==========================================================================
    // Nested types
    //==========================================================================

    /** @brief Offscreen render target for a single transparency layer nesting level.
     *  Owns a VulkanImage for compositing + a vk::Framebuffer — destroyed by this owning
     *  VulkanTransparencyStack's destructor, which holds the vk::Device needed for cleanup.
     *  Created on demand by getOrCreateLayer() at the stack's current extent (set by resize()).
     *
     *  `image` is the MSAA color attachment (activeSampleCount-sampled,
     *  matching the shared renderPass's attachment 0); `resolveImage` is the
     *  single-sample resolve target that is actually addressed through the
     *  bindless array (a multisample image cannot be sampled as sampler2D).
     *
     *  `stencil` is this level's own owned MSAA stencil attachment, created/
     *  destroyed together with `image` at the same sample count — never a
     *  caller-supplied/shared view.
     */
    struct TransparencyLayer
    {
        VulkanImage image;
        VulkanImage resolveImage;
        VulkanImage stencil;
        vk::Framebuffer framebuffer {};
        vk::Extent2D extent {};

        /** @brief Stable slot in the bindless texture array (set 1 binding 0),
         *  assigned once at creation by VulkanGraphics::getOrCreateTransparencyLayer() via
         *  VulkanGraphics::bindlessRegistry.assignBindlessIndex(). Reset to -1 by resize() whenever
         *  this level's images are recreated (window resize changes the
         *  vk::ImageView identity), which is this level's only re-assignment trigger. */
        int bindlessIndex { -1 };

        void create (VmaAllocator allocator, vk::Device device, vk::RenderPass renderPass,
                     vk::SurfaceFormatKHR colorFormat, vk::SampleCountFlagBits sampleCount, vk::Extent2D newExtent);

        /** @brief Destroys this level's own vk::Framebuffer against @p device.
         *  RAII Images self-destruct on this struct's own destruction, but the
         *  raw vk::Framebuffer needs an explicit vk::Device to release, which
         *  this struct does not itself own — shared cleanup step for every
         *  owner of a TransparencyLayer (VulkanTransparencyStack::~VulkanTransparencyStack()'s
         *  own sweep, resize()'s own recreate walk, VulkanGraphics::Texture's
         *  per-pixelData layer teardown).
         *  @param device  Logical device that owns framebuffer.
         */
        void destroy (vk::Device device) noexcept
        {
            device.destroyFramebuffer (framebuffer, nullptr);
            framebuffer = vk::Framebuffer {};
        }
    };

    //==========================================================================
    // Constructor / Destructor
    //==========================================================================

    /** @brief Stores the device reference. Owns no Vulkan handles until first use.
     *  @param device  Shared Vulkan device — not owned, must outlive this VulkanTransparencyStack.
     */
    explicit VulkanTransparencyStack (VulkanDevice& device);

    /** @brief Destroys every target's vk::Framebuffer; RAII Images self-destruct on vector clear. */
    ~VulkanTransparencyStack();

    //==========================================================================
    // Layer access
    //==========================================================================

    /** @brief Returns or creates the TransparencyLayer at the given nesting level.
     *  If the level does not yet exist, creates it at the current extent (set by
     *  resize()). Asserts on creation failure (VRAM exhaustion).
     *  @param level  Zero-based stack index.
     */
    TransparencyLayer& getOrCreateLayer (int level);

    /** @brief Returns the TransparencyLayer at the given nesting level.
     *  @param level  Stack index — must be within the range created by getOrCreateLayer().
     */
    TransparencyLayer& getTransparencyLayer (int level);

    //==========================================================================
    // Extent event
    //==========================================================================

    /** @brief Recreates every existing layer at the new extent. Stores all
     *  creation parameters (renderPass, colorFormat, sampleCount, extent) as
     *  stack state so getOrCreateLayer() can create future levels without
     *  per-call threading. The extent stored here is the layer-extent SSOT.
     *
     *  Called from VulkanGraphics::resize() (after waitIdle) and once from
     *  VulkanGraphics::create()/createOffscreen() to seed the initial params.
     *
     *  @param renderPass                   Render pass the framebuffers are created against.
     *  @param colorFormat                  Swapchain surface format.
     *  @param sampleCount                  Session-locked MSAA sample count.
     *  @param extent                       New pixel extent for all layers.
     *  @param outPreviousBindlessIndices   Receives the bindlessIndex of every
     *                                      existing layer that was recreated (caller
     *                                      releases them back to the bindless pool).
     */
    void resize (vk::RenderPass renderPass, vk::SurfaceFormatKHR colorFormat,
                 vk::SampleCountFlagBits sampleCount, vk::Extent2D extent,
                 jam::Array<int>& outPreviousBindlessIndices);

private:
    //==========================================================================
    // Members
    //==========================================================================

    /** @brief Shared Vulkan device — not owned, must outlive this VulkanTransparencyStack. */
    VulkanDevice& device;

    /** @brief Stack of offscreen targets — one entry per transparency nesting level.
     *  Owner sweep — TransparencyLayer owns 3 Images + a raw vk::Framebuffer handle
     *  per level (an RAII resource owner in its own right); vector-of-unique_ptr
     *  also keeps every level's address stable across growth (getOrCreateLayer()'s
     *  stack add() loop), so a reference returned by getTransparencyLayer() for an
     *  already-created level never dangles when a later call grows the stack for a
     *  deeper level (jam_core/utilities/jam_Owner.h). */
    jam::Owner<TransparencyLayer> stack {};

    /** @brief Layer-extent SSOT — set by resize(), read by getOrCreateLayer(). */
    vk::Extent2D currentExtent {};

    /** @brief Render pass stored by resize() for getOrCreateLayer() creation. */
    vk::RenderPass storedRenderPass {};

    /** @brief Swapchain surface format stored by resize() for getOrCreateLayer() creation. */
    vk::SurfaceFormatKHR storedColorFormat {};

    /** @brief Session-locked MSAA sample count stored by resize() for getOrCreateLayer() creation. */
    vk::SampleCountFlagBits storedSampleCount { vk::SampleCountFlagBits::e1 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VulkanTransparencyStack)
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam