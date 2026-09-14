/** @file jam_VulkanBindlessInstance.h
 *  @brief RAII withdrawal of one window's registered bindless glyph-atlas
 *         slot assignments — split out of jam_VulkanGraphics.h (its sole
 *         consumer, VulkanGraphics::bindlessInstance) since VulkanBindlessInstance is a
 *         self-contained concern of its own, not part of VulkanGraphics's own
 *         public/private API surface. Wired into jam_vulkan.h AFTER
 *         font/jam_GlyphAtlas.h (this class's dtor calls
 *         jam::GlyphAtlas::getInstance()->hasTexture()/getTexture(), needing
 *         that type complete) and BEFORE context/jam_VulkanGraphics.h (its
 *         sole consumer).
 */

namespace jam
{
/*____________________________________________________________________________*/
/** @brief RAII: this window's registered slot assignments in the shared
 *  glyph-atlas textures' registries. Stores only the opaque native window
 *  handle (identity key only, never dereferenced) and withdraws every
 *  registered slot assignment this window was ever assigned — across every
 *  jam::GlyphAtlas::Type — on destruction, so a stale registered slot
 *  assignment never survives this window's own jam::VulkanGraphics, and a
 *  future VulkanGraphics constructed on a reused native-handle address never reads
 *  it (see VulkanBindlessTexture::clearBindlessIndex()'s own doc comment).
 *
 *  Dtor body mirrors the former VulkanGraphics::~VulkanGraphics()'s own manual cleanup
 *  block: jam::GlyphAtlas::getInstance() + jassert non-null (the atlas
 *  outlives every VulkanGraphics — jam::VulkanEngine declares glyphAtlas BEFORE
 *  contexts, so it destructs AFTER — jam_VulkanEngine.h's glyphAtlas/contexts
 *  member-order doc comment) + for each atlas type, VulkanBindlessTexture::
 *  clearBindlessIndex (handle). clearBindlessIndex() tolerates an absent key,
 *  so a window that was never assigned a registered slot for a given
 *  type destructs clean.
 *
 *  Non-copyable, non-movable (mirrors VulkanGraphics itself — this member never
 *  needs to outlive its owning VulkanGraphics; the user-declared destructor already
 *  suppresses the implicit move operations).
 */
class VulkanBindlessInstance
{
public:
    /** @brief Stores @p windowHandle as this instance's identity key — never
     *  dereferenced, only ever passed back to VulkanBindlessTexture::
     *  clearBindlessIndex() at destruction.
     *  @param windowHandle  Native window handle (NSView* on macOS, HWND on
     *                       Windows). */
    explicit VulkanBindlessInstance (void* windowHandle) noexcept
        : handle (windowHandle)
    {
    }

    /** @brief Withdraws this window's registered slot assignment from every
     *  shared glyph-atlas type's registry — presence-checked per type via
     *  jam::GlyphAtlas::hasTexture() first (teardown ordering safety:
     *  the atlas may already be destroyed if this is the last window). */
    ~VulkanBindlessInstance()
    {
        auto* atlas { jam::GlyphAtlas::getInstance() };
        jassert (atlas != nullptr);

        for (auto type : { jam::GlyphAtlas::Type::mono, jam::GlyphAtlas::Type::emoji })
            if (atlas->hasTexture (type))
                atlas->getTexture (type).clearBindlessIndex (handle);
    }

    /** @brief Returns the native window handle this instance was constructed with. */
    void* getHandle() const noexcept { return handle; }

private:
    void* handle { nullptr };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VulkanBindlessInstance)
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam