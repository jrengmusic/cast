namespace jam
{
/*____________________________________________________________________________*/
/** @brief One window's bindless array slot allocator plus juce::ImagePixelData::
 *  Listener-driven recycling of the per-root slot cache. jam::VulkanGraphics
 *  remains the orchestrator that assigns/releases through this registry and
 *  writes every descriptor (VulkanGraphics::writeBindlessTextureDescriptor()) —
 *  this type answers "which index", never touches a vk::DescriptorSet.
 *
 *  registerBindlessIndex() registers this registry as a juce::ImagePixelData::
 *  Listener on the given root at slot-assign time (VulkanGraphics::
 *  cacheImageTexture()) so this window's own slot is released back to
 *  freeBindlessIndices when the root is deleted or dimension-changed —
 *  deferred via previousImageBindlessIndices/releaseRetiredIndices() (mirrors
 *  VulkanStagingArena::resetResources()'s exact deferred-release shape,
 *  jam_VulkanStagingArena.h) since the GPU may still be reading the slot
 *  from an already-recorded descriptor this frame.
 *
 *  Not copyable, not movable — holds no reference to VulkanDevice/VulkanEngine/
 *  VulkanGraphics, but does reach VulkanTextureCache::getInstance() in
 *  imageDataChanged() to distinguish a same-dimension content update from a
 *  dimension change: bookkeeping over juce::ImagePixelData and the index
 *  domain, keyed by the owning window's native handle (identity key only,
 *  never dereferenced — same idiom as VulkanBindlessInstance).
 */
class VulkanBindlessRegistry : private juce::ImagePixelData::Listener
{
public:
    //==========================================================================
    // Constructor / Destructor
    //==========================================================================

    /** @brief Stores @p windowHandle as this registry's identity key — never
     *  dereferenced, only ever passed to VulkanBindlessTexture::
     *  clearBindlessIndex() at destruction — and @p capacity as
     *  assignBindlessIndex()'s exhaustion bound for this registry's whole
     *  lifetime.
     *  @param windowHandle  This window's resolved native handle
     *                       (VulkanGraphics::getNativeHandle() — NSView* or HWND
     *                       for a windowed instance, the owning VulkanGraphics's
     *                       own address for an offscreen instance).
     *  @param capacity      Session-locked bindless array bound
     *                       (VulkanGraphics::bindlessTextureCapacity).
     */
    explicit VulkanBindlessRegistry (void* windowHandle, uint32_t capacity) noexcept;

    /** @brief Deregisters this listener from every root it registered on at
     *  registerBindlessIndex() time — prevents dangling listener pointers into
     *  roots whose juce::Image outlives this registry. Engine-backed roots
     *  also have their per-window registry entry cleared so a future window
     *  reusing this native handle sees no stale index. */
    ~VulkanBindlessRegistry();

    //==========================================================================
    // Bindless index allocation
    //==========================================================================

    /** @brief Assigns the next bindless array slot bounded by capacity (set
     *  once at construction). Asserts on exhaustion (capacity is
     *  hardware-budget-sized at create — exhaustion is a design bug, not a
     *  runtime branch).
     *  @return The assigned slot (always >= 0).
     */
    int assignBindlessIndex() noexcept;

    /** @brief Releases a previously assigned bindless slot back onto
     *  freeBindlessIndices for assignBindlessIndex() to reissue.
     *  @param index  The slot to release (must be >= 0).
     */
    void releaseBindlessIndex (int index) noexcept;

    /** @brief Records that @p root now holds @p index, and registers this
     *  registry as a juce::ImagePixelData::Listener on @p root so the slot is
     *  recycled when @p root is deleted or dimension-changed. Called by
     *  VulkanGraphics::cacheImageTexture() once it has assigned @p index
     *  (assignBindlessIndex()) and written its descriptor
     *  (VulkanGraphics::writeBindlessTextureDescriptor()) — mirrors
     *  VulkanBindlessTexture::registerBindlessIndex()'s exact division of
     *  labor, keyed by root instead of window handle.
     *  @param root   The resolved root pixel store (VulkanGraphics::
     *                getImageSourceRoot()) this window's slot belongs to.
     *  @param index  The slot assigned to @p root for this window.
     */
    void registerBindlessIndex (juce::ImagePixelData* root, int index);

    //==========================================================================
    // Drain
    //==========================================================================

    /** @brief Releases every slot retired by imageDataBeingDeleted()/
     *  imageDataChanged() since the last drain, back onto freeBindlessIndices.
     *  Call once per frame at the fence-safe point (VulkanGraphics::
     *  resetResources(), after beginFrame()'s fence wait;
     *  VulkanGraphics::submitOffscreenAndWait(), after its own fence wait) —
     *  mirrors VulkanStagingArena::resetResources()'s identical
     *  fence-safe-drain contract. */
    void releaseRetiredIndices() noexcept;

private:
    //==========================================================================
    // juce::ImagePixelData::Listener — per-window bindless slot recycling
    //==========================================================================

    /** @brief Retires this window's slot for @p pixelData (retireIndex()) and
     *  erases the bindlessIndices entry — the root is gone, so its slot is
     *  recycled and this registry's own listener registration on it is moot.
     *  @param pixelData  The root being deleted, previously registered via
     *                    registerBindlessIndex().
     */
    void imageDataBeingDeleted (juce::ImagePixelData* pixelData) override;

    /** @brief No-op for an engine-backed root (VulkanImagePixelData) — it
     *  repaints in place, so its view/descriptor/slot stay valid across
     *  content changes. For a foreign (software) root, consults
     *  VulkanTextureCache::getInstance() to distinguish a same-dimension
     *  content update (no slot action — the cache re-uploads into the same
     *  image in place) from a dimension change (retires this window's slot
     *  via retireIndex(), erases the bindlessIndices entry, and deregisters
     *  this listener — the cache's own imageDataChanged may erase its entry
     *  before or after this callback, ListenerList order not guaranteed).
     *  @param pixelData  The root that changed, previously registered via
     *                    registerBindlessIndex().
     */
    void imageDataChanged (juce::ImagePixelData* pixelData) override;

    //==========================================================================
    // Private helpers
    //==========================================================================

    /** @brief Moves @p index into previousImageBindlessIndices — deferred
     *  release, since the GPU may still be reading this slot from an
     *  already-recorded descriptor this frame; releasing now would let
     *  assignBindlessIndex() hand it to a different texture before the GPU is
     *  proven done. Drained post-fence-wait by releaseRetiredIndices().
     *  Precedent: VulkanStagingArena::retireBuffer() queues an outgoing
     *  resource for deferred destruction the same way
     *  (jam_VulkanStagingArena.cpp).
     *  @param index  The slot being retired.
     */
    void retireIndex (int index);

    //==========================================================================
    // Members
    //==========================================================================

    /** @brief This registry's identity key — the owning window's resolved
     *  native handle, captured once at construction and immutable for this
     *  registry's whole life, never dereferenced. Equal to bindlessInstance's
     *  own handle value (VulkanGraphics::bindlessInstance) — a creation-time
     *  identity copy, not shadow state: the source value never changes after
     *  construction, so there is nothing for this copy to drift from. */
    void* nativeHandle { nullptr };

    /** @brief Session-locked bindless array bound, captured once at
     *  construction — assignBindlessIndex()'s exhaustion check reads this
     *  member rather than a caller-supplied argument. */
    uint32_t capacity { 0 };

    /** @brief Next bindless array slot to assign once freeBindlessIndices is
     *  empty — bounded monotonic counter, no LRU. Shared by cached textures,
     *  transparency targets, jam::GlyphAtlas's GPU-mirror slots, and
     *  sceneColorImage (one array namespace, owned by VulkanGraphics via this
     *  registry). */
    int nextBindlessTextureIndex { 0 };

    /** @brief Slots released by releaseBindlessIndex() and available for
     *  assignBindlessIndex() to reissue before advancing
     *  nextBindlessTextureIndex — the recycle pool that closes the leak a bare
     *  monotonic counter otherwise has against every resize/replace event
     *  (window resize abandoning the old scene/straightAlpha/TransparencyLayer
     *  slot, VulkanWindingScratch growth, VulkanShaderInstance replacement). LIFO reuse
     *  (last()/remove() in assignBindlessIndex()) — recency has no bearing on
     *  correctness here, only on which physical slot a caller happens to land in. */
    jam::Array<int> freeBindlessIndices {};

    /** @brief Bindless slots freed by imageDataBeingDeleted()/
     *  imageDataChanged() mid-frame, held until releaseRetiredIndices()'s
     *  post-fence-wait drain releases them — same deferred pattern as
     *  VulkanStagingArena's previousStagingBuffers and VulkanGraphics's own
     *  previousShaderInstances. */
    jam::Array<int> previousImageBindlessIndices {};

    /** @brief Roots this window registered as juce::ImagePixelData::Listener
     *  on, mapped to this window's own bindless index for that root —
     *  deregistered in the destructor so no dangling listener pointer
     *  survives into whatever caller-owned juce::Image outlives this
     *  registry. VulkanBindlessTexture::registry (keyed by window handle) is
     *  the SSOT for a texture's per-window slot; this map is a root-keyed
     *  derivation of that same assignment, written at the single call site
     *  that owns both (VulkanGraphics::cacheImageTexture()) and asserted
     *  there to agree. The derivation exists because the listener callbacks
     *  below need to look up a slot to retire by root, not by texture object,
     *  without depending on VulkanTextureCache's entry still existing at
     *  callback time (ListenerList iteration order between cache and window
     *  listeners is not guaranteed). */
    jam::HashMap<juce::ImagePixelData*, int> bindlessIndices {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VulkanBindlessRegistry)
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
