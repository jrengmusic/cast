/**
 * @file jam_LowLevelGraphicsGlyphRenderer.h
 * @brief CPU fallback LowLevelGraphicsContext — atlas-backed glyph rendering.
 *
 *  jam::VulkanEngine::createContext()'s GPU-unavailable branch constructs
 *  this instead of returning nullptr, keeping the "externalContextFactory never
 *  returns nullptr" goal. Namespace stays bare jam:: deliberately (mirrors
 *  jam::GlyphAtlas) — this is not a Vulkan concept, merely colocated in
 *  jam_vulkan for its GlyphAtlas dependency.
 *
 *  macOS: subclasses juce::CoreGraphicsContext — the peer's own native context
 *  — overriding only drawGlyphs() to resolve glyphs through the shared
 *  jam::GlyphAtlas; every other verb (fillRect/drawImage/clipToPath/...) runs
 *  CG's own rendering unchanged, so parity with the peer's stock CG path
 *  (dirty-rect clip, retina, no blit) is inherited rather than reimplemented.
 *
 *  Non-macOS: subclasses juce::LowLevelGraphicsSoftwareRenderer against an
 *  owned target image, with glyph rendering routed through the shared
 *  jam::GlyphAtlas instead of JUCE's own glyph rasterization, then
 *  self-presents that image to the native window in its destructor, since
 *  returning a non-null context from externalContextFactory means JUCE
 *  performs zero presentation of its own (Windows peer never blits its own
 *  device context either).
 */

namespace jam
{
/*____________________________________________________________________________*/
#if JUCE_MAC

#if JUCE_GRAPHICS_INCLUDE_COREGRAPHICS_HELPERS

/** @brief LowLevelGraphicsContext backed by juce::CoreGraphicsContext, with glyph
 *  rendering routed through the shared jam::GlyphAtlas instead of CG's own text
 *  drawing.
 *
 *  Every virtual except drawGlyphs() is inherited unchanged from
 *  juce::CoreGraphicsContext. setOrigin()/addTransform()/saveState()/restoreState()/
 *  setFill()/setOpacity() forward to the base implementation and additionally
 *  maintain this class's own copy of the state drawGlyphs() needs (the base keeps
 *  its own copy private).
 *
 *  @par Thread contract
 *  MESSAGE THREAD only — constructed, drawn into, and destroyed within one
 *  synchronous native paint callback.
 */
class LowLevelGraphicsGlyphRenderer : public juce::CoreGraphicsContext
{
public:
    /** @brief Constructs the CoreGraphics-backed CPU fallback context.
     *  @param nativeContext  The peer's own native CoreGraphics context, already
     *                        CTM-adjusted by the caller — passed straight to the
     *                        base class constructor.
     *  @param flipHeight     Component height in physical pixels, passed straight
     *                        to the base class constructor for its own internal
     *                        y-flip.
     *  @param glyphAtlas     The shared CPU-side glyph atlas (VulkanEngine-owned,
     *                        not owned here) — must outlive this context.
     *  @param scaleFactor    Display scale factor (physical / logical) — used only
     *                        to scale the atlas key's font size to a DPI-correct
     *                        physical pixel size and to convert a resolved glyph's
     *                        physical-pixel bitmap geometry back to logical points —
     *                        never fed into addTransform(), since nativeContext is
     *                        already retina-aware on its own.
     */
    LowLevelGraphicsGlyphRenderer (CGContextRef nativeContext, float flipHeight,
                                   jam::GlyphAtlas& glyphAtlas, float scaleFactor);

    // ---- Overridden so drawGlyphs() can read state the base class keeps private ----

    /** @brief Forwards to the base class, then updates this class's own transform copy. */
    void setOrigin (juce::Point<int> o) override;

    /** @brief Forwards to the base class, then updates this class's own transform copy. */
    void addTransform (const juce::AffineTransform& t) override;

    /** @brief Forwards to the base class, then pushes this class's own cached state. */
    void saveState() override;

    /** @brief Forwards to the base class, then pops this class's own cached state. */
    void restoreState() override;

    /** @brief Forwards to the base class, then caches fill.colour for drawGlyphs(). */
    void setFill (const juce::FillType& f) override;

    /** @brief Forwards to the base class, then replaces this class's cached fill
     *  colour's alpha with @p o — mirrors juce::FillType::setOpacity()'s own
     *  replace (not multiply) semantics.
     *  @param o  New opacity, [0, 1].
     */
    void setOpacity (float o) override;

    /** @brief Sets run-scoped codepoint/span + cell-box metrics consumed by the
     *  immediately following drawGlyphs() call — same seam as
     *  jam::VulkanLowLevelGraphicsContext::setCellRun() (see that method's own
     *  doc comment for the full rationale); mirrors setFont()'s per-run idiom.
     *  GlyphArrangement::draw() is the sole caller (dynamic_cast dispatch).
     *  Consumed and cleared by the following drawGlyphs() call; @p codepoints /
     *  @p spans must stay valid until it returns — ownership stays with the
     *  caller (GlyphArrangement::Run's own HeapBlocks).
     *  @param codepoints  Per-glyph original Unicode codepoints, parallel to the
     *                      glyph indices the following drawGlyphs() call will receive.
     *  @param spans       Per-glyph display width in cells (1 = narrow, 2 = wide).
     *  @param count       Element count of both arrays — must equal the following
     *                      drawGlyphs() call's glyph count, or this state is ignored.
     *  @param cellWidth   Terminal cell width, physical pixels.
     *  @param cellHeight  Terminal cell height, physical pixels.
     *  @param baseline    Cell-top-to-baseline offset, physical pixels.
     */
    void setCellRun (const char32_t* codepoints, const uint8_t* spans, int count,
                     int cellWidth, int cellHeight, int baseline) noexcept
    {
        pendingCellRun = { codepoints, spans, count, cellWidth, cellHeight, baseline };
    }

    /** @brief Resolves each glyph through GlyphAtlas::getOrRasterize() and composites
     *  it onto the live CGContext through the inherited CoreGraphicsContext verbs —
     *  mirrors jam::VulkanLowLevelGraphicsContext::drawGlyphs()'s atlas resolution
     *  exactly, substituting a CG composite for a GPU quad/draw call.
     *  @param glyphs      OpenType glyph indices.
     *  @param positions   Per-glyph baseline positions in user space.
     *  @param t           Additional transform composed with this context's own
     *                     accumulated transform (setOrigin()/addTransform() history).
     */
    void drawGlyphs (juce::Span<const uint16_t> glyphs,
                     juce::Span<const juce::Point<float>> positions,
                     const juce::AffineTransform& t) override;

private:
    /** @brief Shared CPU-side glyph atlas — not owned, must outlive this context. */
    jam::GlyphAtlas& atlas;

    /** @brief Display scale factor, used only to compute the atlas key's DPI-correct
     *  physical pixel font size and to scale a resolved glyph's bitmap back down to
     *  logical points for placement. */
    float scale;

    /** @brief This class's own copy of the accumulated user-to-device transform —
     *  the base class tracks its own copy privately; drawGlyphs() needs to read it,
     *  so it is duplicated here, updated by setOrigin()/addTransform()/restoreState(). */
    juce::AffineTransform currentTransform;

    /** @brief This class's own copy of the current fill colour — the base class
     *  exposes no getter; duplicated here, updated by setFill()/restoreState(). */
    juce::Colour currentFillColour { juce::Colours::black };

    /** @brief Stack of cached states, grown by saveState() and drained by
     *  restoreState() — kept in lockstep with the base class's own state stack. */
    jam::Array<CachedState> stateStack;

    /** @brief Run-scoped cell-run state set by setCellRun(), consumed and
     *  cleared by drawGlyphs() — see setCellRun()'s own doc comment. */
    PendingCellRun pendingCellRun {};
};

#endif

/** @brief Returns the peer's current native paint-callback clip bounding box
 *  (CGContextGetClipBoundingBox of [NSGraphicsContext currentContext]), rounded
 *  to the smallest containing integer rectangle.
 *
 *  No flip needed: JUCE's NSView is flipped (isFlipped == true), so CG user
 *  space is already top-left-origin, y-down here — the same space
 *  juce::LowLevelGraphicsContext::clipToRectangle() expects, read straight from
 *  clipBox untouched. jam::VulkanEngine::createContext()'s own CTM concat is
 *  unrelated to this read — it pre-compensates CoreGraphicsContext's own
 *  internal flip for drawing, not this clip query.
 *
 *  Implemented in jam_LowLevelGraphicsGlyphRenderer_mac.mm; same "runs
 *  synchronously inside the native paint callback" precondition as
 *  jam::VulkanEngine::createContext() itself.
 *  @param peer  The peer being painted.
 *  @return       The peer's current clip bounds, in top-left-origin, y-down
 *                physical pixels.
 */
juce::Rectangle<int> currentPeerDirtyBounds (juce::ComponentPeer& peer);

#else // not JUCE_MAC

/** @brief LowLevelGraphicsContext backed by juce::LowLevelGraphicsSoftwareRenderer,
 *         with glyph rendering routed through the shared jam::GlyphAtlas instead of
 *         JUCE's own glyph rasterization.
 *
 *  Every virtual except drawGlyphs() is inherited unchanged from
 *  juce::LowLevelGraphicsSoftwareRenderer — fillRect/drawImage/clipToPath/etc. all
 *  run JUCE's own proven software rasterization against the owned target image.
 *  drawGlyphs() is overridden to resolve each glyph through GlyphAtlas::getOrRasterize()
 *  (cache hit, or rasterize on miss) and composite it directly onto the target image
 *  via GlyphAtlas::composite() — the same shared atlas the Vulkan LLGC uses, so glyph
 *  rendering is visually identical regardless of which engine is active.
 *
 *  The base class's own transform/fill/opacity state is private to its Impl and
 *  exposes no getters drawGlyphs() could read — setOrigin()/addTransform()/
 *  saveState()/restoreState()/setFill()/setOpacity() are therefore also overridden,
 *  each forwarding to the base implementation (so every inherited draw op keeps
 *  working exactly as before) and additionally maintaining this class's own copy,
 *  the only state drawGlyphs() needs. getFont() needed no such treatment — the base
 *  class already exposes it publicly.
 *
 *  Since this type is only ever returned from juce::ComponentPeer::externalContextFactory,
 *  JUCE performs zero presentation of its own once a non-null context is returned
 *  (confirmed: the Windows peer never blits its own device context either) — this
 *  type must present its own rendered image to the screen. The destructor does so
 *  (mirroring jam::VulkanLowLevelGraphicsContext's own destructor, which calls
 *  Graphics::endFrame() at the same point in its lifecycle) via native OS calls, using
 *  the juce::ComponentPeer's native handle captured at construction.
 *
 *  @par Thread contract
 *  MESSAGE THREAD only — constructed, drawn into, and destroyed within one
 *  synchronous native paint callback.
 */
class LowLevelGraphicsGlyphRenderer : public juce::LowLevelGraphicsSoftwareRenderer
{
public:
    /** @brief Constructs the CPU fallback context.
     *  @param target       Owned render target, already sized to physical pixels
     *                      (width * scaleFactor x height * scaleFactor) — passed
     *                      straight to the base class constructor.
     *  @param glyphAtlas   The shared CPU-side glyph atlas (VulkanEngine-owned, not
     *                      owned here) — must outlive this context.
     *  @param peer         The peer this context renders for — only its native
     *                      handle is captured (for the destructor's presentation
     *                      call); the peer itself is not retained.
     *  @param scaleFactor  Display scale factor (physical / logical) — seeds this
     *                      context's own transform (mirrors
     *                      jam::VulkanLowLevelGraphicsContext's identical
     *                      constructor-time scale seeding) and scales the atlas key's
     *                      font size to a DPI-correct physical pixel size.
     */
    LowLevelGraphicsGlyphRenderer (const juce::Image& target, jam::GlyphAtlas& glyphAtlas,
                                   juce::ComponentPeer& peer, float scaleFactor);

    /** @brief Presents the rendered target image to the native window (see file doc
     *  comment) — mirrors jam::VulkanLowLevelGraphicsContext's destructor, which
     *  ends the frame at the same point in its lifecycle. */
    ~LowLevelGraphicsGlyphRenderer() override;

    // ---- Overridden so drawGlyphs() can read state the base class keeps private ----

    /** @brief Forwards to the base class, then updates this class's own transform copy. */
    void setOrigin (juce::Point<int> o) override;

    /** @brief Forwards to the base class, then updates this class's own transform copy. */
    void addTransform (const juce::AffineTransform& t) override;

    /** @brief Forwards to the base class, then pushes this class's own cached state. */
    void saveState() override;

    /** @brief Forwards to the base class, then pops this class's own cached state. */
    void restoreState() override;

    /** @brief Forwards to the base class, then caches fill.colour for drawGlyphs(). */
    void setFill (const juce::FillType& f) override;

    // Replaces this class's cached fill colour's alpha with o — mirrors
    // juce::FillType::setOpacity()'s own replace (not multiply) semantics.
    void setOpacity (float o) override;

    /** @brief Forwards to the base class, then intersects this class's own
     *  device-space clip mirror with @p r transformed by currentTransform. */
    bool clipToRectangle (const juce::Rectangle<int>& r) override;

    /** @brief Forwards to the base class, then intersects this class's own
     *  device-space clip mirror with @p r's bounds transformed by currentTransform. */
    bool clipToRectangleList (const juce::RectangleList<int>& r) override;

    /** @brief Classifies the composed transform and routes accordingly: a
     *  pure translation/scale composites through
     *  jam::Graphics::drawScaledImage() against this class's own
     *  device-space clip mirror (currentClip); rotation or shear falls
     *  through to the inherited
     *  juce::LowLevelGraphicsSoftwareRenderer::drawImage().
     *  @param imageToDraw  Source image, composited premultiplied src-over
     *                      on the fast path.
     *  @param transform    Transform composed with this context's own
     *                      accumulated transform (setOrigin()/addTransform()
     *                      history) to decide the routing. */
    void drawImage (const juce::Image& imageToDraw, const juce::AffineTransform& transform) override;

    /** @brief Sets run-scoped codepoint/span + cell-box metrics consumed by the
     *  immediately following drawGlyphs() call — same seam as
     *  jam::VulkanLowLevelGraphicsContext::setCellRun() (see that method's own
     *  doc comment for the full rationale); mirrors setFont()'s per-run idiom.
     *  GlyphArrangement::draw() is the sole caller (dynamic_cast dispatch).
     *  Consumed and cleared by the following drawGlyphs() call; @p codepoints /
     *  @p spans must stay valid until it returns — ownership stays with the
     *  caller (GlyphArrangement::Run's own HeapBlocks).
     *  @param codepoints  Per-glyph original Unicode codepoints, parallel to the
     *                      glyph indices the following drawGlyphs() call will receive.
     *  @param spans       Per-glyph display width in cells (1 = narrow, 2 = wide).
     *  @param count       Element count of both arrays — must equal the following
     *                      drawGlyphs() call's glyph count, or this state is ignored.
     *  @param cellWidth   Terminal cell width, physical pixels.
     *  @param cellHeight  Terminal cell height, physical pixels.
     *  @param baseline    Cell-top-to-baseline offset, physical pixels.
     */
    void setCellRun (const char32_t* codepoints, const uint8_t* spans, int count,
                     int cellWidth, int cellHeight, int baseline) noexcept
    {
        pendingCellRun = { codepoints, spans, count, cellWidth, cellHeight, baseline };
    }

    /** @brief Resolves each glyph through GlyphAtlas::getOrRasterize() and composites
     *  it directly onto the target image via GlyphAtlas::composite() — mirrors
     *  jam::VulkanLowLevelGraphicsContext::drawGlyphs()'s atlas resolution exactly,
     *  substituting a direct CPU composite for a GPU quad/draw call.
     *  @param glyphs      OpenType glyph indices.
     *  @param positions   Per-glyph baseline positions in user space.
     *  @param t           Additional transform composed with this context's own
     *                     accumulated transform (setOrigin()/addTransform() history).
     */
    void drawGlyphs (juce::Span<const uint16_t> glyphs,
                     juce::Span<const juce::Point<float>> positions,
                     const juce::AffineTransform& t) override;

private:
    /** @brief Own reference to the render target — the base class keeps its own
     *  handle private, so direct juce::Image::BitmapData access for compositing
     *  (and the destructor's presentation call) needs this separate copy. juce::Image
     *  is a reference-counted handle, so this is not a pixel-data duplication. */
    juce::Image targetImage;

    /** @brief Shared CPU-side glyph atlas — not owned, must outlive this context. */
    jam::GlyphAtlas& atlas;

    /** @brief Native window handle captured from the peer at construction, for the
     *  destructor's presentation call (see file doc comment). */
    void* nativeHandle;

    /** @brief Display scale factor, used only to compute the atlas key's DPI-correct
     *  physical pixel font size (mirrors jam::VulkanLowLevelGraphicsContext's
     *  identical scale member and its identical use in glyph-key construction). */
    float scale;

    /** @brief This class's own copy of the accumulated user-to-device transform —
     *  the base class tracks its own copy privately; drawGlyphs() needs to read it,
     *  so it is duplicated here, updated by setOrigin()/addTransform()/restoreState(). */
    juce::AffineTransform currentTransform;

    /** @brief This class's own copy of the current fill colour — the base class
     *  exposes no getter; duplicated here, updated by setFill()/restoreState(). */
    juce::Colour currentFillColour { juce::Colours::black };

    /** @brief This class's own device-space clip mirror — the base class's own
     *  clip region is private and unreadable here; drawGlyphs() needs it to
     *  bound GlyphAtlas::composite()'s direct pixel writes. Initialized to the
     *  full target image bounds, narrowed by clipToRectangle()/
     *  clipToRectangleList(), restored by restoreState(). */
    juce::Rectangle<int> currentClip;

    /** @brief Stack of cached states, grown by saveState() and drained by
     *  restoreState() — kept in lockstep with the base class's own state stack. */
    jam::Array<CachedState> stateStack;

    /** @brief Run-scoped cell-run state set by setCellRun(), consumed and
     *  cleared by drawGlyphs() — see setCellRun()'s own doc comment. */
    PendingCellRun pendingCellRun {};
};

#endif

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
