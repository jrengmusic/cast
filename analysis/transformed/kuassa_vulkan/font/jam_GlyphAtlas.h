/**
 * @file jam_GlyphAtlas.h
 * @brief CPU-side glyph atlas with LRU cache, plus its GPU-resident mirror.
 *        Rasterizes registered (registerTypeface()) and lazily-resolved
 *        (getOrLoadSystemTypeface()) typefaces via FreeType (FT_LOAD_NO_HINTING +
 *        FT_RENDER_MODE_NORMAL, unhinted 8-bit grayscale coverage); every
 *        other typeface falls back to juce::Typeface::getLayersForGlyph()'s
 *        EdgeTable coverage. See the class doc's three-tier model.
 *
 *  Physically relocated from jam_graphics/fonts/ into jam_vulkan/font/,
 *  absorbing the GPU-mirror atlas images/upload that previously lived in the
 *  since-deleted jam::TextureCache (they could not live here while this
 *  file was in jam_graphics, a module jam_vulkan depends on but never the
 *  reverse). Namespace stays bare jam:: deliberately — module colocation is a
 *  build/dependency detail, not a conceptual merger into jam::vulkan; this type
 *  is not a Vulkan concept.
 */

namespace jam
{
/*____________________________________________________________________________*/
/*____________________________________________________________________________*/
/**
 * @struct GlyphConstraint
 * @brief Scaling, alignment, and padding descriptor for a Nerd Font icon glyph.
 *
 * A default-constructed GlyphConstraint has `scaleMode == ScaleMode::none` and
 * `isActive()` returns `false`, meaning no transformation is applied.
 *
 * @par Trivial copyability
 * GlyphConstraint is `std::is_trivially_copyable` so it can be stored directly
 * in the generated lookup table without heap allocation.
 */
struct GlyphConstraint
{
    /** @brief Controls how the glyph is scaled to fit the terminal cell. */
    enum class ScaleMode : uint8_t
    {
        none,          ///< Do not resize; render at natural glyph size.
        fit,           ///< Scale DOWN only to fit cell; preserve aspect ratio.
        cover,         ///< Scale to fill cell; preserve aspect ratio (may clip).
        adaptiveScale, ///< fit if wider than 1 cell, cover if narrower (NF 'pa' flag).
        stretch        ///< Stretch to fill both axes; no aspect ratio preservation.
    };

    /** @brief Horizontal or vertical alignment of the scaled glyph within the cell. */
    enum class Align : uint8_t
    {
        none,   ///< Do not move; use padding-based position.
        start,  ///< Align to left edge (H) or bottom edge (V).
        end,    ///< Align to right edge (H) or top edge (V).
        center  ///< Center in the cell on the respective axis.
    };

    /**
     * @brief Reference height used when computing the vertical scale target.
     *
     * - `cell`: Use the full cell height. Required for powerline/box glyphs
     *   that must tile seamlessly edge-to-edge.
     * - `icon`: Use the icon height (ascender to descender). Produces more
     *   visually balanced icons that don't touch the cell edges.
     */
    enum class HeightRef : uint8_t
    {
        cell, ///< Use full cell height (for powerline/box that must tile).
        icon  ///< Use icon height (ascender to descender).
    };

    /** @brief How the glyph is scaled to fit the cell. */
    ScaleMode scaleMode { ScaleMode::none };

    /** @brief Horizontal alignment of the scaled glyph within the cell. */
    Align alignH { Align::none };

    /** @brief Vertical alignment of the scaled glyph within the cell. */
    Align alignV { Align::none };

    /** @brief Reference dimension used for vertical scaling. */
    HeightRef heightRef { HeightRef::cell };

    /** @brief Fractional padding from the top cell edge (0.0-1.0), fraction of cellHeight. */
    float padTop { 0.0f };

    /** @brief Fractional padding from the left cell edge (0.0-1.0), fraction of cellWidth * cellSpan. */
    float padLeft { 0.0f };

    /** @brief Fractional padding from the right cell edge (0.0-1.0), fraction of cellWidth * cellSpan. */
    float padRight { 0.0f };

    /** @brief Fractional padding from the bottom cell edge (0.0-1.0), fraction of cellHeight. */
    float padBottom { 0.0f };

    /** @brief Relative width override as a fraction of the cell width (default 1.0). Reserved, not yet applied. */
    float relativeWidth { 1.0f };

    /** @brief Relative height override as a fraction of the cell height (default 1.0). Reserved, not yet applied. */
    float relativeHeight { 1.0f };

    /** @brief Relative X position offset as a fraction of the cell width (default 0.0). Reserved, not yet applied. */
    float relativeX { 0.0f };

    /** @brief Relative Y position offset as a fraction of the cell height (default 0.0). Reserved, not yet applied. */
    float relativeY { 0.0f };

    /**
     * @brief Maximum allowed aspect ratio (width / height) after scaling.
     *
     * When > 0, the horizontal scale factor is clamped so the scaled glyph's
     * aspect ratio does not exceed this value. 0 disables the clamp.
     */
    float maxAspectRatio { 0.0f };

    /** @brief Maximum number of terminal cells this icon may span horizontally. */
    uint8_t maxCellSpan { 2 };

    /**
     * @brief Fractional inset applied on all four sides before padding (~6.18%).
     *
     * The golden ratio conjugate (1/phi^2), chosen for aesthetically balanced margins.
     * Applied in addition to the per-side pad* values.
     */
    static constexpr float iconInset { 0.0618f };

    /** @brief Returns `true` if this constraint applies any transformation. */
    bool isActive() const noexcept { return scaleMode != ScaleMode::none; }

    /**
     * @brief Look up the constraint for a Unicode codepoint.
     *
     * Returns the GlyphConstraint record for the given codepoint from the
     * generated lookup table (jam_GlyphConstraintTable.cpp). Codepoints that
     * are not Nerd Font icons return a default-constructed (inactive) constraint.
     *
     * @note Implemented in jam_GlyphConstraintTable.cpp (generated file -- do
     *       not edit manually).
     */
    static GlyphConstraint getConstraint (char32_t codepoint) noexcept;
};

static_assert (std::is_trivially_copyable_v<GlyphConstraint>,
               "jam::GlyphConstraint must be trivially copyable");

/** @brief CPU-side glyph atlas — rasterizes and caches glyphs for GPU or software rendering.
 *
 *  Owns two juce::Image atlases (mono SingleChannel + emoji ARGB). Rasterizes
 *  on cache miss via rasterize(), which dispatches first on the active
 *  FontRasterizerBackend (edgeTable/freetype/native — see
 *  rasterizeByBackend's branchless dispatch table), set by setRasterization().
 *  The freetype backend additionally dispatches per-typeface across three tiers
 *  of its own (see rasterizeFreeType()'s doc comment for the internal
 *  EdgeTable-fallback tier, tier 3 below):
 *
 *  1. Registered memory fonts — END's shipped OOTB embedded defaults
 *     (Display/DisplayMono family), registered once via registerTypeface() at
 *     startup. This atlas owns a copy of the font bytes (FaceEntry::fontBytes).
 *  2. Lazily-resolved system fonts — any user-config-selected SYSTEM font not
 *     already registered. On first miss, getOrLoadSystemTypeface() resolves the
 *     typeface's on-disk font file via the platform (CoreText on macOS,
 *     DirectWrite on Windows — see findSystemFontFile()), reads it into a
 *     FaceEntry exactly like registerTypeface(), and inserts it into the same
 *     freetypeFaces registry so every subsequent glyph for that typeface hits
 *     tier 1's FreeType path above. The resolution attempt runs at most once
 *     per typeface identity — a failure inserts a null-face FaceEntry so it is
 *     never retried per glyph. Color fonts are deliberately excluded here: this
 *     FreeType path is grayscale coverage only, so a resolved face with
 *     FT_HAS_COLOR set is discarded immediately and cached as a null-face
 *     FaceEntry instead, same as a resolution failure.
 *  3. EdgeTable fallback — reached when tier 2's resolution failed (no on-disk
 *     font file could be resolved for that typeface, or the resolved face was
 *     a color font — both cached as a null-face FaceEntry) or for JUCE-internal
 *     fallback typefaces whose glyphs cannot be produced by an unhinted
 *     grayscale FreeType face. This is where color/emoji rendering (Apple
 *     Color Emoji, Segoe UI Emoji, Noto Color Emoji, bitmap/COLR fonts) stays
 *     — juce::Typeface::getLayersForGlyph()'s ImageLayer path renders them,
 *     unchanged. Also reached directly (skipping FreeType entirely) whenever
 *     FontRasterizerBackend::edgeTable is the active backend.
 *
 *  Both FreeType tiers render unhinted (FT_LOAD_NO_HINTING,
 *  FT_RENDER_MODE_NORMAL — 8-bit grayscale coverage, no autofitter, no stem
 *  darkening). LRU eviction.
 *
 *  Also owns the GPU-resident mirror of both atlas images (mono R8,
 *  emoji BGRA8), one jam::VulkanBindlessTexture per Type (see gpuImages),
 *  created by initialise() once the device is valid (device-lifetime
 *  invariant thereafter). The caller
 *  polls isDirty()/reads getAtlas() to re-upload via getTexture(type).upload() —
 *  BindlessTexture::upload() no longer owns a staging buffer of its own (a
 *  same-frame mono-then-emoji upload previously shared one fixed-offset-0
 *  buffer, so the second upload's memcpy silently overwrote the first's bytes
 *  before either copy executed, and growing that buffer mid-frame destroyed
 *  it while an earlier vk::CommandBuffer::copyBufferToImage call still
 *  referenced it — an AGX/MoltenVK crash at vk::Queue::submit) — it instead writes
 *  into a region of a staging buffer owned and passed in by the calling
 *  upload path itself (jam::VulkanLowLevelGraphicsContextGlyph.cpp:32),
 *  not a shared arena (see BindlessTexture::upload()'s parameters). This
 *  also removes the cross-window race the old shared stagingBuffer member
 *  had: each window's uploads now live in that path's own buffer, gated by
 *  that window's own in-flight fence. Bindless array-slot registries live
 *  INSIDE each BindlessTexture (see gpuImages), keyed by native window handle
 *  — the bindless descriptor set is per-window (per jam::VulkanGraphics
 *  instance) while this atlas's images are one VulkanEngine-wide shared
 *  instance, so the same image needs a different slot per window.
 *  jam::VulkanGraphics still assigns the slot and writes the descriptor
 *  (registerGlyphAtlasSlots()), recording the assigned index into the BindlessTexture
 *  it read from getTexture() — this atlas never assigns a slot itself.
 *
 *  @par Cell-box wiring (Key's codepoint/cellWidth/cellHeight/baseline/span)
 *  Key now carries the per-glyph codepoint and the span-aware terminal cell
 *  box alongside typeface/glyphIndex/fontSize — GlyphConstraint's own
 *  documented "required extension", now wired. Two rasterize()-time effects,
 *  both gated on `key.cellWidth > 0` (a real cell-run caller; see Key's own
 *  doc comment for the "no cell-run context" zeroed shape):
 *  - EdgeTable's emoji (ImageLayer) branch aspect-fits color-emoji ink into
 *    the box, centered (rasterizeEdgeTable(), jam_GlyphAtlasEdgeTable.cpp).
 *  - FreeType/native mono branches apply `GlyphConstraint::getConstraint
 *    (key.codepoint)` when active — scale/align/pad a PUA/Nerd-Font icon into
 *    a box-sized bitmap, `bearingX = 0, bearingY = baseline` (verbatim old
 *    constrained-glyph semantics — endless GlyphAtlas.mm @ 2e37f6d) — see
 *    computeConstraintPlacement()/renderConstrainedMonoGlyphIntoAtlas() below.
 *  Unconstrained mono glyphs (constraint inactive, or emoji with no cell-run
 *  context) render bit-for-bit as before this wiring.
 *
 *  @par Thread contract
 *  MESSAGE THREAD only — including initialise(), called synchronously from
 *  jam::VulkanEngine's constructor.
 */
class GlyphAtlas : public jam::Instance<GlyphAtlas>
{
public:
    /** @brief Unique identity of a rasterized glyph in the atlas.
     *
     *  @par codepoint/cellWidth/cellHeight/baseline/span
     *  These five fields thread the per-glyph Unicode codepoint and the
     *  span-aware terminal cell box through to rasterize() — GlyphConstraint's
     *  own documented "required extension" (emoji cell-fit + PUA/Nerd-Font
     *  GlyphConstraint application, see class doc's "Cell-box wiring" note).
     *  Set by jam::GlyphArrangement::draw() via the run-scoped setCellRun()
     *  seam (jam::VulkanLowLevelGraphicsContext / jam::LowLevelGraphicsGlyphRenderer);
     *  a call site with no cell-run context (plain juce::Graphics text, never
     *  routed through GlyphArrangement) leaves all five at their zero default.
     *
     *  Every field except the raw typeface pointer participates in identity
     *  (operator==/Hash) for every Key, including the zeroed "no cell-run
     *  context" shape — the simple-correct
     *  option: a terminal only ever has ONE (cellWidth, cellHeight, baseline)
     *  combination per fontSize (one font, one cell grid), so a real cell-run
     *  caller's Keys never fragment the cache across more than that one shape
     *  per fontSize; the zeroed shape is simply a second, always-distinct
     *  identity for callers with no cell context at all — not a source of
     *  cache fragmentation either way. */
    struct Key
    {
        /** @brief The typeface that owns this glyph. Payload for rasterization only —
         *  identity is carried by typefaceName/typefaceStyle below, since some
         *  platform backends mint a new Typeface instance per resolution. */
        juce::Typeface* typeface { nullptr };

        /** @brief Owning typeface's name — participates in identity. */
        juce::String typefaceName {};

        /** @brief Owning typeface's style — participates in identity. */
        juce::String typefaceStyle {};

        /** @brief Glyph index within the typeface (OpenType glyph ID). */
        uint32_t glyphIndex { 0 };

        /** @brief Point size at which the glyph was rasterized. */
        float fontSize { 0.0f };

        /** @brief Original Unicode codepoint this glyph was shaped from —
         *  0 with no cell-run context. Used by GlyphConstraint::getConstraint()
         *  lookup (FreeType/native mono backends) and by the emoji cell-fit box
         *  (EdgeTable backend) — see class doc's "Cell-box wiring" note. */
        char32_t codepoint { 0 };

        /** @brief Terminal cell width in physical pixels (single cell — span
         *  below carries the display-width multiplier) — 0 with no cell-run
         *  context. Every rasterize()-time cell-box effect is gated on this
         *  being > 0. */
        int cellWidth { 0 };

        /** @brief Terminal cell height in physical pixels — 0 with no
         *  cell-run context. */
        int cellHeight { 0 };

        /** @brief Baseline offset from the cell top, in physical pixels — 0
         *  with no cell-run context. */
        int baseline { 0 };

        /** @brief Display width in cells (1 = narrow, 2 = wide) — 0 with no
         *  cell-run context (distinct from a real 1, so a no-cell-run-context
         *  Key never collides with a real narrow glyph's Key at the same
         *  typeface/glyphIndex/fontSize/codepoint identity). */
        uint8_t span { 0 };

        /** @brief Rotation angle, in RADIANS, baked into the rasterized bitmap
         *  -- 0.0f for untransformed text. Every backend's rasterize() bakes
         *  this rotation directly into the glyph's coverage bitmap at rasterize
         *  time (rotation is part of glyph IDENTITY, not a runtime transform
         *  applied to an axis-aligned atlas quad), so a glyph drawn at any angle
         *  goes through the exact same atlas backend + gamma/hinting pipeline as
         *  unrotated text. Extracted by both drawGlyphs() call sites (the Vulkan
         *  LowLevelGraphicsContext and the CPU-fallback LowLevelGraphicsGlyphRenderer)
         *  from their own composed device transform via
         *  `std::atan2 (composedTransform.mat10, composedTransform.mat00)`. */
        float rotation { 0.0f };

        bool operator== (const Key& other) const noexcept
        {
            return typefaceName == other.typefaceName
                and typefaceStyle == other.typefaceStyle
                and glyphIndex == other.glyphIndex
                and fontSize == other.fontSize
                and codepoint == other.codepoint
                and cellWidth == other.cellWidth
                and cellHeight == other.cellHeight
                and baseline == other.baseline
                and span == other.span
                and rotation == other.rotation;
        }

        /** @brief Hash functor for Key — used by jam::HashMap. */
        struct Hash
        {
            size_t operator() (const Key& key) const noexcept
            {
                return static_cast<size_t> (key.typefaceName.hashCode64())
                     ^ (static_cast<size_t> (key.typefaceStyle.hashCode64()) << 1)
                     ^ (std::hash<uint32_t>{} (key.glyphIndex) << 2)
                     ^ (std::hash<float>{} (key.fontSize) << 3)
                     ^ (std::hash<uint32_t>{} (jam::toU32 (key.codepoint)) << 4)
                     ^ (std::hash<int>{} (key.cellWidth) << 5)
                     ^ (std::hash<int>{} (key.cellHeight) << 6)
                     ^ (std::hash<int>{} (key.baseline) << 7)
                     ^ (std::hash<uint8_t>{} (key.span) << 8)
                     ^ (std::hash<float>{} (key.rotation) << 9);
            }
        };

        /** @brief Builds a Key from JUCE font height + device scale, plus the
         *  per-glyph codepoint, span-aware cell-box metrics, and rotation — the
         *  one conversion site shared by both drawGlyphs() call sites (the Vulkan
         *  LowLevelGraphicsContext and the CPU-fallback LowLevelGraphicsGlyphRenderer).
         *  Units: @p fontHeight is juce::Font::getHeight()'s JUCE height unit;
         *  @p scale is the device/display scale factor (physical / logical pixels)
         *  — the product is the DPI-correct atlas key fontSize (the FreeType path
         *  converts this further to pixels-per-em downstream at
         *  loadFreeTypeGlyphBounds()'s one SSOT site).
         *  @param typeface    The typeface owning the glyph.
         *  @param glyphIndex  OpenType glyph index within @p typeface.
         *  @param fontHeight  JUCE font height (juce::Font::getHeight()).
         *  @param scale       Device/display scale factor.
         *  @param codepoint   Original Unicode codepoint (0 with no cell-run context).
         *  @param cellWidth   Terminal cell width, physical pixels (0 with no cell-run context).
         *  @param cellHeight  Terminal cell height, physical pixels (0 with no cell-run context).
         *  @param baseline    Cell-top-to-baseline offset, physical pixels (0 with no cell-run context).
         *  @param span        Display width in cells (0 with no cell-run context; 1 = narrow, 2 = wide otherwise).
         *  @param rotation    Rotation angle, in radians, baked into the rasterized bitmap (0.0f for untransformed text).
         *  @return             The constructed Key. */
        static Key make (juce::Typeface* typeface, uint32_t glyphIndex, float fontHeight, float scale,
                          char32_t codepoint, int cellWidth, int cellHeight, int baseline, uint8_t span,
                          float rotation) noexcept
        {
            return Key { typeface, typeface->getName(), typeface->getStyle(), glyphIndex, fontHeight * scale,
                        codepoint, cellWidth, cellHeight, baseline, span, rotation };
        }

        static float getScale (const juce::AffineTransform& transform) noexcept
        {
            return std::sqrt (transform.mat00 * transform.mat00 + transform.mat10 * transform.mat10);
        }
    };

    /** @brief Atlas glyph type — determines which atlas image the glyph resides in. */
    enum class Type : uint8_t
    {
        mono,
        emoji
    };

    /** @brief One horizontal packing strip within an atlas slot.
     *
     *  Ported from the deleted jam::glyph::AtlasPacker::Shelf (classic
     *  shelf/strip bin-packing): a shelf's height is fixed at the height of the
     *  first glyph placed on it; subsequent glyphs are placed on the first shelf
     *  whose height is >= the requested height and that has enough horizontal
     *  space remaining, so shorter glyphs can reuse space above later, taller rows
     *  instead of being forced into a brand-new shelf every time. */
    struct Shelf
    {
        int y { 0 };
        int height { 0 };
        int currentX { 0 };
    };

    /** @brief Per-atlas-type GPU-upload state: the CPU pixel buffer, dirty flag, and shelf packer. */
    struct Slot
    {
        juce::Image image;
        bool dirty { true };
        jam::Array<Shelf> shelves {};
    };

    /** @brief Rasterized glyph descriptor: atlas location and bearing metrics. */
    struct Region
    {
        /** @brief Normalized UV rectangle within the atlas texture. */
        juce::Rectangle<float> textureCoordinates;

        /** @brief Width/height of the rasterized bitmap, in physical pixels. */
        jam::Size<int> size { 0, 0 };

        /** @brief Horizontal (x, pen origin to left edge) and vertical (y, baseline
         *  to top edge) bearing, in physical pixels. */
        juce::Point<int> bearing { 0, 0 };

        /** @brief Whether this glyph is in the mono or emoji atlas. */
        Type type { Type::mono };
    };

    // =========================================================================
    // Lifecycle
    // =========================================================================

    /** @brief Constructs the CPU-side atlas images and the shared FreeType
     *  library (see freetypeLibrary); the GPU-resident mirror images (mono R8,
     *  emoji BGRA8) are created later by initialise().
     *  @param gpuDevice  Shared Vulkan device — not owned, must outlive this
     *                    GlyphAtlas. Used by the GPU-resident mirror (GPU
     *                    image creation in initialise(), getTexture()); every
     *                    CPU-only method (advanceFrame, getOrRasterize, getAtlas,
     *                    isDirty, clearDirty, composite) never touches it.
     */
    explicit GlyphAtlas (jam::VulkanDevice& gpuDevice) noexcept;

    /** @brief Releases every registered FT_Face, then the shared FreeType library. */
    ~GlyphAtlas();

    /** @brief Creates the GPU-resident mirror images (mono R8, emoji BGRA8) —
     *  deferred out of the constructor so the engine can call this only once
     *  its device has become valid.
     *  @note Called by VulkanEngine's synchronous constructor once its
     *  device has been validated. */
    void initialise();

    /** @brief Destroys the device-derived GPU-resident mirror images (mono R8,
     *  emoji BGRA8) and marks both CPU atlas slots dirty so a subsequent
     *  initialise() + re-upload restores the mirror from the CPU pixels still
     *  held in images — the object remains reinitialisable via initialise().
     *  CPU-side state (images, cache, freetypeLibrary/freetypeFaces/nativeFonts,
     *  backend/gamma/contrast/embolden) survives untouched; it embeds no
     *  device handle. Safe to call against a lost device: destroying gpuImages
     *  only runs each owned jam::VulkanBindlessTexture's RAII destroy calls,
     *  never a wait. */
    void shutdown();

    // =========================================================================
    // Core API
    // =========================================================================

    /** @brief Advance the LRU frame counter. Call once per paint cycle.
     *  @note MESSAGE THREAD. */
    void advanceFrame() noexcept;

    /** @brief Return a cached glyph or rasterize on demand.
     *  @param key  Unique glyph identity.
     *  @return Region descriptor, or nullptr if atlas is full or glyph is empty.
     *  @note MESSAGE THREAD. */
    Region* getOrRasterize (const Key& key) noexcept;

    /** @brief Sets the active rasterization backend and coverage LUT gamma/contrast,
     *  rebuilding the LUT (see rebuildCoverageLut()) and flushing every cached
     *  glyph (see flushCache()) whenever any parameter actually changed — a
     *  different backend, gamma, or contrast produces different bitmaps for the
     *  same glyph identity, so a stale cache entry must never survive the change.
     *
     *  No default parameter values — the framework never decides these on the caller's
     *  behalf (mirrors jam::VulkanEngine's targetFrameBudgetMs/pipelineCacheFile
     *  "no hidden default inside the framework" doctrine — see jam_VulkanEngine.h's
     *  constructor doc comment). END resolves the shipped defaults from user config.
     *  @param newBackend   Rasterization backend for all subsequent mono glyphs.
     *  @param newGamma     Coverage LUT gamma exponent (1.0 = no gamma correction).
     *  @param newContrast  Coverage LUT contrast (0.0 = no synthetic darkening).
     *  @note MESSAGE THREAD. */
    void setRasterization (map::FontRasterizerBackend::value newBackend, float newGamma, float newContrast) noexcept;

    /** @brief Enables or disables synthetic FreeType emboldening
     *  (`FT_Outline_Embolden`, verbatim old chain — restored from the deleted
     *  jam::glyph::Atlas's FreeType rasterization path) for every subsequent
     *  mono glyph rendered via rasterizeFreeType(). Flushes every cached glyph
     *  (flushCache()) when the value actually changes, same rationale as
     *  setRasterization() — a stale cache entry would otherwise keep pointing
     *  at pixels rasterized under the previous embolden state.
     *  @param newEmbolden  `true` to synthetically embolden FreeType-rasterized glyphs.
     *  @note MESSAGE THREAD. */
    void setEmbolden (bool newEmbolden) noexcept;

    /** @brief Returns the current synthetic-embolden state (see setEmbolden()). */
    bool getEmbolden() const noexcept { return embolden; }

    // =========================================================================
    // FreeType registration — unhinted grayscale raster in rasterize()
    // =========================================================================

    /** @brief Registers a typeface's font-file bytes with FreeType so rasterize()
     *  renders its mono glyphs through FreeType's unhinted 8-bit grayscale
     *  raster (FT_LOAD_NO_HINTING + FT_RENDER_MODE_NORMAL) instead of
     *  juce::Typeface::getLayersForGlyph()'s EdgeTable coverage.
     *
     *  Tier 1 of the three-tier model (see class doc): END's shipped OOTB
     *  embedded defaults are registered here explicitly, once, at startup.
     *  User-config-selected SYSTEM fonts reach the same freetypeFaces registry
     *  lazily instead, via getOrLoadSystemTypeface() on rasterize()'s first miss
     *  for that typeface — never through this method.
     *
     *  @param typeface   The already-created typeface this face data corresponds
     *                    to — used only as the lookup key identity; never queried
     *                    for glyph data itself once registered.
     *  @param fontData   Font-file bytes (TTF/OTF). Copied into this atlas's own
     *                    FaceEntry::fontBytes — FT_New_Memory_Face does not copy
     *                    its input buffer, so this atlas owns a copy for the
     *                    resulting FT_Face's entire lifetime; the caller's own
     *                    buffer may be freed or reused immediately after this
     *                    call returns.
     *  @param sizeBytes  Size of fontData in bytes.
     *  @note MESSAGE THREAD. */
    void registerTypeface (const juce::Typeface::Ptr& typeface, const void* fontData, size_t sizeBytes);

    // =========================================================================
    // Cell metrics — endless conformance restoration
    // =========================================================================

    /** @brief Terminal cell geometry (width/height/baseline) derived from a
     *  registered FreeType face, restoring the deleted endless terminal
     *  renderer's semantics (`Fonts::Metrics`, commit 2e37f6d,
     *  Source/terminal/rendering/Fonts.h) verbatim onto this atlas's faces.
     *
     *  Unlike the endless original, this struct carries only ONE coordinate
     *  space. Endless split logical (CSS pixel) and physical (device pixel)
     *  cell dimensions because it rasterized at a separate render DPI; this
     *  atlas's rasterize() path already folds device scale into the glyph
     *  Key (`Key::make()`'s `fontHeight * scale`), so calcMetrics() below is
     *  always called with the caller's own already-scale-correct
     *  fontHeightJuce and the logical/physical split does not need restoring
     *  here — every field is in the caller's pixel space, whatever that is. */
    struct Metrics
    {
        int cellWidth  { 0 }; ///< Maximum ASCII advance width, pixels.
        int cellHeight { 0 }; ///< Line height (ascender + descender), pixels.
        int baseline   { 0 }; ///< Distance from cell top to glyph baseline, pixels.
    };

    /** @brief Computes @p typeface's terminal cell metrics at @p fontHeightJuce,
     *  restoring the deleted endless `Fonts::calcMetrics()` semantics (commit
     *  2e37f6d, Source/terminal/rendering/FontsMetrics.cpp) onto this atlas's
     *  registered FreeType faces.
     *
     *  Sizes the face via the SAME pixelsPerEm conversion
     *  loadFreeTypeGlyphBounds() uses to size faces for rasterization
     *  (key.typeface->getMetrics()'s heightToPoints factor) — cell metrics
     *  MUST be measured at the exact FT size glyphs rasterize at, or grid
     *  cells and glyph bitmaps drift apart at fractional DPI scales (endless
     *  commit message: "fix cursor alignment at fractional DPI scales"), the
     *  whole point of this restoration.
     *
     *  Width is the maximum horizontal advance across printable ASCII glyphs
     *  (U+0020-U+007F, verbatim endless measureMaxCellWidth()), falling back
     *  to `face->size->metrics.max_advance` when no ASCII glyph loads. Height
     *  and baseline come directly from `face->size->metrics.height`/
     *  `ascender`. All three are 26.6 fixed-point values ceiling-converted to
     *  whole pixels (endless's `ceil26_6ToPx`, `ftFixedScale` == 64).
     *
     *  @param typeface       Typeface to measure — must already have a real
     *                        FT_Face in freetypeFaces (registerTypeface() or a
     *                        prior rasterize() miss's getOrLoadSystemTypeface()).
     *  @param fontHeightJuce juce::Font::getHeight() of the font this
     *                        typeface backs.
     *  @return               Populated Metrics, or a zeroed Metrics if no real
     *                        FT_Face is registered for @p typeface.
     *  @note MESSAGE THREAD. */
    Metrics calcMetrics (const juce::Typeface::Ptr& typeface, float fontHeightJuce) const noexcept;

    // =========================================================================
    // Atlas image accessors (for GPU upload)
    // =========================================================================

    /** @brief Returns the atlas image for the given slot type (mono SingleChannel or emoji ARGB). */
    juce::Image& getAtlas (Type type) noexcept { return images.at (type).image; }

    /** @brief Returns the atlas side length in texels. */
    int getDimension() const noexcept { return dimension; }

    /** @brief Returns true if the given slot's atlas image has been modified since last GPU upload. */
    bool isDirty (Type type) const noexcept { return images.at (type).dirty; }

    /** @brief Clears the given slot's dirty flag after GPU texture upload. */
    void clearDirty (Type type) noexcept { images.at (type).dirty = false; }

    // =========================================================================
    // Software composite helper (for non-GPU rendering path)
    // =========================================================================

    /** @brief Composite a glyph from the given atlas slot onto a render target using SIMD blending.
     *  Dispatches to the mono (tinted single-channel) or emoji (ARGB src-over) implementation
     *  registered for the given Type. colour is used by the mono path only; ignored by emoji.
     *  @param type        Which atlas slot to composite from.
     *  @param targetData  Locked BitmapData of the render target (readWrite).
     *  @param region      Region descriptor for the glyph.
     *  @param screenX     Destination X in the render target.
     *  @param screenY     Destination Y in the render target.
     *  @param colour      Foreground colour for tinting (mono only).
     *  @param clip        Device-space clip rectangle; rows/columns outside it
     *                     are skipped by compositeRows (bounds-intersection semantics). */
    void composite (Type type, juce::Image::BitmapData& targetData, const Region& region,
                    int screenX, int screenY, juce::Colour colour, juce::Rectangle<int> clip) noexcept;

    // =========================================================================
    // GPU-resident mirror (migrated from the since-deleted
    // jam::TextureCache, which owned this only because GlyphAtlas could
    // not legally hold a vk::Image while it lived in jam_graphics)
    // =========================================================================

    /** @brief Returns the BindlessTexture backing the given atlas GPU slot
     *  type — its upload() re-uploads pixel data (mono: pixelStride = 1;
     *  emoji: pixelStride = 4 BGRA) into a region of the caller's own staging
     *  buffer (jam::VulkanLowLevelGraphicsContextGlyph.cpp:32); its getBindlessIndex()/
     *  registerBindlessIndex() manage this window's own registered slot assignment
     *  in this texture's registry (see class doc comment); its getImage()/
     *  getView() back GPU upload commands
     *  and descriptor binding respectively.
     *  @param type  Which atlas slot (mono or emoji).
     *  @return Reference to the BindlessTexture for @p type — valid once
     *          initialise() has created both GPU images. */
    jam::VulkanBindlessTexture& getTexture (Type type) noexcept;

    /** @brief Returns true if @p type's own GPU image exists in gpuImages —
     *  true once initialise() has created both types, but
     *  retained for jam::VulkanBindlessInstance::~BindlessInstance()'s
     *  own call site (jam_VulkanGraphics.h) which may run during teardown.
     *  @param type  Which atlas slot (mono or emoji). */
    bool hasTexture (Type type) const noexcept { return gpuImages.contains (type); }

private:
    // =========================================================================
    // Internal types
    // =========================================================================

    struct CacheEntry
    {
        Region region;
        uint64_t lastUsedFrame { 0 };
    };

    /** @brief Temporary record used by evictLeastRecentlyUsed()'s age sort.
     *  Ported from the deleted jam::glyph::LRUCache::AgeEntry. */
    struct AgeEntry
    {
        Key key;
        uint64_t age;
    };

    /** @brief Glyph bounds extracted from the first juce::GlyphLayer of a rasterized glyph. */
    struct GlyphBounds
    {
        /** @brief Width/height of the rasterized bitmap, in physical pixels. */
        jam::Size<int> size { 0, 0 };

        /** @brief Horizontal (x, pen origin to left edge) and vertical (y, baseline
         *  to top edge) bearing, in physical pixels. */
        juce::Point<int> bearing { 0, 0 };

        bool isEmoji { false };
    };

    /** @brief One rasterizeNative() OS-native font handle, cached per typeface +
     *  point size — CTFontRef on macOS (jam_GlyphAtlas_mac.mm); DirectWrite has
     *  no equivalent cache yet (see rasterizeNative()'s Windows doc note).
     *  Declared void*-typed rather than CTFontRef/IDWriteFontFace so this
     *  shared, cross-platform header never needs to import CoreText/
     *  DirectWrite — jam_GlyphAtlas_mac.mm owns the cast at both write and
     *  release sites; released in ~GlyphAtlas() under \#if JUCE_MAC
     *  (jam_GlyphAtlas.cpp), mirroring freetypeFaces' FT_Done_Face release.
     *  pixelsPerEm invalidates the cached handle when a typeface is next
     *  requested at a different size (a CTFontRef bakes in its point size at
     *  creation, unlike FT_Face which is re-sized per call via FT_Set_Char_Size). */
    struct NativeFontEntry
    {
        void* fontRef { nullptr };
        float pixelsPerEm { 0.0f };

        /** @brief Cached juce::Typeface::getColourGlyphFormats() != 0 result for
         *  this typeface — set exactly once, on this NativeFontEntry's first
         *  miss (both rasterizeNative() halves test `not nativeFonts.contains
         *  (key.typeface)` before the map's operator[] inserts the default-
         *  constructed entry, so the query never repeats per glyph, including
         *  for a colour typeface, which never populates fontRef/pixelsPerEm
         *  above and so cannot rely on their nullness as a "not yet checked"
         *  sentinel). A colour font's CoreText/DirectWrite mono-only rendering
         *  path (DeviceGray fill / ClearType alpha-texture average) can only
         *  ever produce a monochrome silhouette of a colour glyph, so `true`
         *  here routes rasterizeNative() to rasterizeEdgeTable() instead —
         *  mirroring rasterizeFreeType()'s FT_HAS_COLOR rejection
         *  (getOrLoadSystemTypeface()) — "the only backend that ever produces
         *  Type::emoji glyphs" stays the sole colour/emoji producer. */
        bool isColourGlyph { false };
    };

    /** @brief One FreeType face plus this atlas's own copy of its font-file
     *  bytes — FT_New_Memory_Face does not copy the buffer it is given, so
     *  fontBytes must outlive face for as long as freetypeLibrary may still
     *  read from it (i.e. until this GlyphAtlas is destroyed).
     *
     *  Doubles as the negative-resolution cache for getOrLoadSystemTypeface():
     *  a null face (default) with an empty fontBytes means resolution was
     *  attempted for that typeface identity and failed — rasterize() then
     *  falls through to the EdgeTable path every time, never retrying. */
    struct FaceEntry
    {
        FT_Face face { nullptr };
        juce::MemoryBlock fontBytes;
    };

    /** @brief The two destination-pixel blend components shared by the mono and emoji composite paths.
     *  Fixed-point SIMD intermediate, deliberately confined to GlyphAtlas's private
     *  composite path (compositeMonoImpl/compositeEmojiImpl via computeDestBlend());
     *  never crosses the class boundary. */
    struct DestBlend
    {
        uint32_t a, r, g, b;
    };

    /** @brief JUCE EdgeTable::iterate() callback protocol implementation. Writes
     *  rasterized mono-glyph coverage directly into the atlas BitmapData subregion
     *  at offset, through the coverage-conditioning LUT (@p lut — see
     *  GlyphAtlas::coverageLut) — the EdgeTable backend's own per-pixel/per-run
     *  application of the same LUT table the FreeType/native backends apply via
     *  writeMonoCoverageRow(); different write shape, same LUT data (SSOT). Nested
     *  here — rather than file-local — so it does not need to be reconstructed on
     *  every GlyphAtlas::rasterize() call; tightly coupled to this atlas's own
     *  BitmapData layout. */
    struct MonoCoverageWriter
    {
        juce::Image::BitmapData& data;
        juce::Point<int> offset;
        const uint8_t* lut;
        int currentY { 0 };

        void setEdgeTableYPos (int y) noexcept
        {
            currentY = y;
        }

        void handleEdgeTablePixel (int x, int alphaLevel) noexcept
        {
            data.getLinePointer (currentY - offset.y)[x - offset.x] =
                lut[static_cast<uint8_t> (alphaLevel)];
        }

        void handleEdgeTablePixelFull (int x) noexcept
        {
            data.getLinePointer (currentY - offset.y)[x - offset.x] = lut[255];
        }

        void handleEdgeTableLine (int x, int width, int alphaLevel) noexcept
        {
            std::memset (data.getLinePointer (currentY - offset.y) + (x - offset.x),
                         lut[static_cast<uint8_t> (alphaLevel)], static_cast<size_t> (width));
        }

        void handleEdgeTableLineFull (int x, int width) noexcept
        {
            std::memset (data.getLinePointer (currentY - offset.y) + (x - offset.x),
                         lut[255], static_cast<size_t> (width));
        }
    };

    // =========================================================================
    // Rasterization
    // =========================================================================

    /** @brief Rasterize a glyph and write pixels into the atlas.
     *
     *  Dispatches on the active backend via rasterizeByBackend (branchless
     *  member-function-pointer table — see that table's doc comment): the
     *  selected rasterizeEdgeTable()/rasterizeFreeType()/rasterizeNative() call
     *  measures the glyph, packs it via packGlyph(), and writes its coverage
     *  bitmap into the atlas, returning its GlyphBounds (bearings + dimensions)
     *  and its packed position — the shared, backend-blind tail below then calls
     *  insertRasterizedGlyph() once packPos is valid. */
    Region* rasterize (const Key& key) noexcept;

    /** @brief EdgeTable coverage backend — juce::Typeface::getLayersForGlyph(),
     *  unhinted, cross-platform. Measures via extractGlyphBounds(), packs via
     *  packGlyph(), writes via renderMonoGlyphIntoAtlas() (mono) or
     *  renderEmojiGlyphIntoAtlas() (emoji — colour fonts always reach this
     *  backend's emoji branch, regardless of the active FontRasterizerBackend selection, since
     *  only this backend ever produces Type::emoji glyphs). key.rotation is
     *  composed into the transform passed to getLayersForGlyph() (after the
     *  pixel-size scale, about the pen origin), so both layer kinds rasterize
     *  already rotated — see this method's own definition comment.
     *  @param key      Unique glyph identity to rasterize.
     *  @param packPos  Out-parameter — see RasterizeGlyph's doc comment.
     *  @return GlyphBounds — bearings + dimensions + isEmoji. */
    GlyphBounds rasterizeEdgeTable (const Key& key, juce::Point<int>& packPos) noexcept;

    /** @brief FreeType coverage backend — unhinted 8-bit grayscale
     *  rasterization (class doc's tier 1/2): resolves key.typeface into
     *  freetypeFaces on first miss (getOrLoadSystemTypeface()), then measures via
     *  loadFreeTypeGlyphBounds(), packs via packGlyph(), and writes via
     *  renderFreeTypeGlyphIntoAtlas().
     *
     *  Falls back to rasterizeEdgeTable() directly (an internal call, never
     *  through rasterizeByBackend) when no usable FT_Face exists for this
     *  typeface — resolution failed (cached as a null-face FaceEntry), the
     *  resolved face was a colour font (rejected in getOrLoadSystemTypeface()), or
     *  this is a JUCE-internal fallback typeface this atlas never owns bytes
     *  for. This fallback is FreeType backend's own defined behavior regardless
     *  of which FontRasterizerBackend value is active — the only way to reach EdgeTable rendering
     *  for a face FreeType genuinely cannot rasterize. key.rotation (radians,
     *  0.0f for untransformed text) is baked in via FT_Set_Transform before
     *  glyph load/render, and the transform is reset immediately after
     *  (FT_Face is shared state) — see this method's own definition comment.
     *  @param key      Unique glyph identity to rasterize.
     *  @param packPos  Out-parameter — see RasterizeGlyph's doc comment.
     *  @return GlyphBounds — bearings + dimensions (isEmoji always false, unless
     *          the internal EdgeTable fallback produced a colour glyph). */
    GlyphBounds rasterizeFreeType (const Key& key, juce::Point<int>& packPos) noexcept;

    /** @brief OS-native coverage backend — CoreText on macOS
     *  (jam_GlyphAtlas_mac.mm), DirectWrite on Windows (this file, \#if
     *  JUCE_WINDOWS) — deliberately reproduces the platform's own
     *  font-smoothing boldening, distinct from FreeType's own unhinted raster.
     *  Mono only for a genuine native render — a colour typeface (detected
     *  once per typeface and cached, see NativeFontEntry::isColourGlyph's doc
     *  comment) never reaches CoreText's/DirectWrite's own mono-only
     *  rendering below at all, falling back to rasterizeEdgeTable() directly
     *  instead (an internal call, never through rasterizeByBackend, same
     *  policy as rasterizeFreeType()'s own EdgeTable fallback above) — "the
     *  only backend that ever produces Type::emoji glyphs" stays the sole
     *  colour/emoji producer.
     *
     *  macOS: builds the CTFontRef from the SAME font-file bytes freetypeFaces
     *  already owns for typefaces registered/resolved with real bytes
     *  (registerTypeface()'s embedded defaults, getOrLoadSystemTypeface()'s
     *  lazily-resolved system fonts) via CGDataProviderCreateWithData →
     *  CGFontCreateWithDataProvider → CTFontCreateWithGraphicsFont, rather than
     *  CTFontCreateWithName's family+style lookup, which would otherwise
     *  resolve whatever installed system font happens to match an embedded
     *  font's family+style name instead of the exact embedded font file
     *  itself. Typefaces with no registered bytes (genuine, uninstalled-copy
     *  system fonts) fall back to CTFontCreateWithName, correct there by
     *  definition. The resulting CTFontRef is cached per typeface + point size
     *  in nativeFonts (see its doc comment) — this backend used to create and
     *  release a CTFontRef on every glyph miss, a real per-glyph cost for any
     *  typeface, not only embedded ones.
     *
     *  Windows: DirectWrite's findDirectWriteFontFace equivalent has the
     *  same by-name substitution issue for custom-collection-loaded fonts
     *  (never in the system collection either) — not yet implemented; see the
     *  TODO at this method's Windows implementation site (this file, \#if
     *  JUCE_WINDOWS) for the IDWriteFactory::CreateCustomFontCollection
     *  equivalent.
     *
     *  key.rotation (radians, 0.0f for untransformed text) is baked in on both
     *  halves: macOS rotates the glyph draw about the pen origin via the
     *  CGBitmapContext CTM (rasterizeNative()'s definition, jam_GlyphAtlas_mac.mm);
     *  Windows passes a DWRITE_MATRIX rotation to CreateGlyphRunAnalysis()
     *  (createGlyphRunAnalysis(), jam_GlyphAtlasNative.cpp), whose measured
     *  bounds already reflect the rotated glyph.
     *  @param key      Unique glyph identity to rasterize.
     *  @param packPos  Out-parameter — see RasterizeGlyph's doc comment.
     *  @return GlyphBounds — bearings + dimensions (isEmoji always false,
     *          unless the colour-typeface EdgeTable fallback above produced
     *          a colour glyph). */
    GlyphBounds rasterizeNative (const Key& key, juce::Point<int>& packPos) noexcept;

    // =========================================================================
    // Constraint application (task B, GlyphConstraint's own documented
    // "required extension") — shared by rasterizeFreeType() and rasterizeNative()
    // (mac + Windows halves); rasterizeEdgeTable()'s own emoji cell-fit (task A)
    // needs neither of these (unconditional centered fit, no GlyphConstraint
    // table lookup) — see jam_GlyphAtlasEdgeTable.cpp's fitEmojiToCellBox().
    // =========================================================================

    /** @brief Scale factors + box-local top-left position for a glyph scaled/
     *  positioned per an active GlyphConstraint — computeConstraintPlacement()'s
     *  return value, consumed by renderConstrainedMonoGlyphIntoAtlas(). */
    struct ConstraintPlacement
    {
        float scaleX { 1.0f };
        float scaleY { 1.0f };
        float posX { 0.0f };
        float posY { 0.0f };
    };

    /** @brief Scale + box-local top-left placement for @p constraint applied to
     *  a glyph of natural size (@p naturalWidth, @p naturalHeight) within the
     *  span-aware cell box (@p cellWidth * @p span wide, @p cellHeight tall) —
     *  verbatim old scale/align/pad formula (endless GlyphAtlas.mm @ 2e37f6d),
     *  Y-axis flipped from that file's bottom-up CoreText convention to this
     *  atlas's top-down convention (see .cpp definition's doc comment for the
     *  flip derivation). Shared by rasterizeFreeType() and rasterizeNative()
     *  (mac + Windows halves).
     *  @param constraint     Nerd Font/PUA layout descriptor (already confirmed active).
     *  @param naturalWidth   Glyph's own natural (unconstrained) rasterized width, pixels.
     *  @param naturalHeight  Glyph's own natural (unconstrained) rasterized height, pixels.
     *  @param cellWidth      Terminal cell width, physical pixels.
     *  @param cellHeight     Terminal cell height, physical pixels.
     *  @param span           Display width in cells.
     *  @return                Scale factors + box-local top-left position for the scaled glyph. */
    static ConstraintPlacement computeConstraintPlacement (const GlyphConstraint& constraint,
                                                           int naturalWidth, int naturalHeight,
                                                           int cellWidth, int cellHeight, uint8_t span) noexcept;

    /** @brief Rescales @p naturalCoverage (single-channel, top-down,
     *  @p naturalStride bytes per row, @p naturalWidth x @p naturalHeight valid
     *  pixels) per @p placement into a zero-filled @p cellBitmapWidth x
     *  @p cellBitmapHeight canvas (juce::Image::rescaled() — same technique
     *  renderEmojiGlyphIntoAtlas() already uses for its own ink rescale), then
     *  writes the canvas into the mono atlas at @p packPos through coverageLut
     *  (writeMonoCoverageRow()) — shared tail for rasterizeFreeType()'s and
     *  rasterizeNative()'s (mac + Windows halves) constrained-icon path.
     *  @param naturalCoverage   Source coverage bytes, top-down, 1 byte/pixel.
     *  @param naturalStride     Bytes per source row (may exceed naturalWidth).
     *  @param naturalWidth      Valid pixel width within each source row.
     *  @param naturalHeight     Valid pixel row count.
     *  @param placement         Scale + box-local position, from computeConstraintPlacement().
     *  @param packPos           Atlas top-left position, from packGlyph().
     *  @param cellBitmapWidth   Destination canvas width (cellWidth * span).
     *  @param cellBitmapHeight  Destination canvas height (cellHeight). */
    void renderConstrainedMonoGlyphIntoAtlas (const uint8_t* naturalCoverage, int naturalStride,
                                              int naturalWidth, int naturalHeight,
                                              const ConstraintPlacement& placement,
                                              juce::Point<int> packPos,
                                              int cellBitmapWidth, int cellBitmapHeight) noexcept;

    /** @brief Fits @p natural emoji ink bounds aspect-preserving into @p key's
     *  span-aware cell box, centered on both axes, repositioning the bearing so
     *  the LLGCs' baseline-anchored composite lands the fitted ink inside the
     *  box — task A of the cell-box wiring (see class doc). Full fit-math doc
     *  at the definition (jam_GlyphAtlasEdgeTable.cpp). Member (not file-local)
     *  because GlyphBounds/Key are private nested types.
     *  @param natural  Natural (unconstrained) emoji bounds from extractGlyphBounds().
     *  @param key      Glyph identity carrying the span-aware cell box.
     *  @return          Fitted bounds — rescaled size, repositioned bearing. */
    static GlyphBounds fitEmojiToCellBox (const GlyphBounds& natural, const Key& key) noexcept;

    /** @brief Number of FontRasterizerBackend enumerators — sizes rasterizeByBackend and its
     *  static_assert below. */
    static constexpr uint8_t backendCount { 3 };

    /** @brief Pointer-to-member-function type shared by every rasterization
     *  backend — the ONE common signature rasterizeByBackend dispatches through.
     *  @param key      Unique glyph identity to rasterize.
     *  @param packPos  Out-parameter: the position packGlyph() assigned this
     *                  glyph, or left at its caller-supplied sentinel (-1, -1)
     *                  if the glyph measured empty or the atlas is full.
     *  @return GlyphBounds — bearings + dimensions (+ isEmoji, EdgeTable only). */
    using RasterizeGlyph = GlyphBounds (GlyphAtlas::*) (const Key& key, juce::Point<int>& packPos) noexcept;

    /** @brief One row per FontRasterizerBackend enumerator, in FontRasterizerBackend order — the branchless
     *  dispatch table rasterize() calls through as
     *  `(this->*rasterizeByBackend.at (static_cast<uint8_t> (backend))) (key, packPos)`.
     *  No switch/if-else chain for backend selection (Lean's 3-branch rule —
     *  a lookup replaces what would otherwise be a 3-way decision table). Declared
     *  after the three backend methods above — an in-class static initializer is
     *  not a complete-class context, so the pointed-to members must already be
     *  declared at this point in the class body. */
    static constexpr std::array<RasterizeGlyph, backendCount> rasterizeByBackend
    {
        &GlyphAtlas::rasterizeEdgeTable,
        &GlyphAtlas::rasterizeFreeType,
        &GlyphAtlas::rasterizeNative
    };

    static_assert (rasterizeByBackend.size() == backendCount);

    /** @brief One-time system-font resolution on a freetypeFaces miss — the
     *  single call site (SSOT) reached only from rasterize().
     *
     *  Resolves key.typeface's on-disk font file via the platform-specific
     *  findSystemFontFile() (CoreText on macOS, DirectWrite on Windows),
     *  keyed by typeface->getName() + typeface->getStyle(), reads it into a
     *  FaceEntry::fontBytes MemoryBlock, and creates its FT_Face exactly like
     *  registerTypeface() (own bytes, faceIndex 0). A successfully created face
     *  with FT_HAS_COLOR set (Apple Color Emoji, Segoe UI Emoji, Noto Color
     *  Emoji, or any other color-table font) is rejected immediately via
     *  FT_Done_Face — this FreeType path is grayscale coverage only, and color
     *  fonts must keep rasterizing through juce::Typeface::getLayersForGlyph()'s
     *  ImageLayer path. Inserts a real FaceEntry into freetypeFaces on success;
     *  inserts a null-face FaceEntry on failure or color-font rejection so this
     *  typeface's resolution is never attempted again — rasterize() falls
     *  through to the EdgeTable/ImageLayer path for it from then on.
     *  @param typeface  The typeface to resolve — used only for its name/style
     *                   and as the freetypeFaces lookup key identity. */
    void getOrLoadSystemTypeface (const juce::Typeface* typeface) noexcept;

    /** @brief Sizes the given FT_Face to key.fontSize (converted from JUCE height
     *  units to pixels-per-em via key.typeface->getMetrics()'s heightToPoints
     *  factor — the single conversion site for every FreeType rasterization),
     *  loads key.glyphIndex unhinted (FT_LOAD_NO_HINTING), and renders it
     *  (FT_RENDER_MODE_NORMAL — 8-bit grayscale). Returns the resulting bitmap's
     *  bounds; the rendered bitmap itself remains valid on face->glyph until the
     *  next FT_Load_Glyph call on this face, which renderFreeTypeGlyphIntoAtlas()
     *  consumes immediately afterward in rasterize(). */
    GlyphBounds loadFreeTypeGlyphBounds (FT_Face face, const Key& key) const noexcept;

    /** @brief Evicts the oldest 10% of cache entries (by frames since lastUsedFrame).
     *
     *  Ported from the deleted jam::glyph::LRUCache::evictLRU(): builds a
     *  temporary age list, std::partial_sort's it to find the maxCachedGlyphs/10
     *  oldest entries (O(n log k) rather than a full sort), then erases them.
     *  Called by rasterize() before inserting a new entry once cache.size() reaches
     *  maxCachedGlyphs — without this, cache was unbounded (this atlas's original
     *  shape had no eviction at all despite carrying lastUsedFrame per entry). */
    void evictLeastRecentlyUsed() noexcept;

    /** @brief Builds a flat (key, age) list from the current cache contents for evictLeastRecentlyUsed(). */
    int buildCacheAgeList (juce::HeapBlock<AgeEntry>& ageList) const noexcept;

    /** @brief Determines isEmoji, width, height, and bearing from the first glyph layer. */
    GlyphBounds extractGlyphBounds (const juce::GlyphLayer& firstLayer) const noexcept;

    /** @brief Iterates the mono glyph's EdgeTable coverage directly into the atlas at packPos. */
    void renderMonoGlyphIntoAtlas (const juce::GlyphLayer& firstLayer, juce::Point<int> packPos,
                                   int glyphWidth, int glyphHeight) noexcept;

    /** @brief Copies the emoji glyph's ARGB pixels directly into the atlas at packPos. */
    void renderEmojiGlyphIntoAtlas (const juce::GlyphLayer& firstLayer, juce::Point<int> packPos,
                                    int glyphWidth, int glyphHeight) noexcept;

    /** @brief Copies the FT_Face's just-rendered bitmap (face->glyph->bitmap, produced
     *  by loadFreeTypeGlyphBounds()'s FT_Render_Glyph call) into the mono atlas at
     *  packPos, through the coverage-conditioning LUT (writeMonoCoverageRow()).
     *  Handles both positive-pitch (top-down) and negative-pitch (bottom-up)
     *  FreeType bitmaps — mirrors the row-copy convention of the deleted
     *  jam::glyph::Atlas::Packer's copyBitmapRows() helper. */
    void renderFreeTypeGlyphIntoAtlas (FT_Face face, juce::Point<int> packPos,
                                       int glyphWidth, int glyphHeight) noexcept;

    /** @brief Applies coverageLut to `count` source coverage bytes, writing the
     *  transformed result into `dest` — the per-row transform site used by the
     *  FreeType rasterization backend (renderFreeTypeGlyphIntoAtlas(), copying
     *  contiguous 1-byte-per-pixel coverage rows from a backend-owned bitmap).
     *  EdgeTable's MonoCoverageWriter applies the same coverageLut table directly
     *  at its own per-pixel/per-run call sites instead (different write shape,
     *  same LUT data — SSOT). The Windows native backend (rasterizeNative(),
     *  jam_GlyphAtlasNative.cpp) applies coverageLut itself in
     *  writeNativeGlyphIntoAtlas() below rather than through this function — its
     *  source is 3-bytes-per-pixel ClearType data requiring an RGB average into a
     *  single coverage byte before the LUT applies, unlike this function's
     *  already-1-byte-per-pixel FreeType input. Emoji ARGB bytes never reach
     *  either function (see renderEmojiGlyphIntoAtlas()).
     *  @param dest    Atlas destination row (writeOnly BitmapData line pointer).
     *  @param source  Backend-owned source coverage row.
     *  @param count   Number of bytes to transform and copy. */
    void writeMonoCoverageRow (uint8_t* dest, const uint8_t* source, int count) const noexcept;

    /** @brief Windows native backend's atlas-write step (rasterizeNative(),
     *  jam_GlyphAtlasNative.cpp, \#if JUCE_WINDOWS) — averages each ClearType
     *  3-bytes-per-pixel (R,G,B subpixel coverage) source pixel down to one
     *  grayscale coverage byte, then applies coverageLut and writes the result
     *  into the mono atlas at packPos. Declared here (rather than file-local
     *  static, unlike jam_GlyphAtlasNative.cpp's findDirectWriteFontFace())
     *  because it needs private access to images/coverageLut/packGlyph() —
     *  its signature stays platform-neutral (uint8_t*, not a DirectWrite type)
     *  so this shared, cross-platform header never needs \#include \<dwrite.h\>.
     *  @param rgbTexture  Source ClearType 3-bytes-per-pixel buffer.
     *  @param packPos     Top-left position returned by packGlyph().
     *  @param width       Glyph width in pixels.
     *  @param height      Glyph height in pixels. */
    void writeNativeGlyphIntoAtlas (const uint8_t* rgbTexture, juce::Point<int> packPos,
                                    int width, int height) noexcept;

    /** @brief Write pixel data into the correct atlas image at the packing position.
     *  @param pixelData  Raw pixel bytes to copy.
     *  @param width      Width of the source pixel data.
     *  @param height     Height of the source pixel data.
     *  @param destRect   Destination rectangle within the atlas.
     *  @param type       Which atlas slot to write into. */
    void writePixels (const uint8_t* pixelData, int width, int height,
                      juce::Rectangle<int> destRect, Type type) noexcept;

    /** @brief Find space in the atlas for a glyph of the given dimensions.
     *  @param width   Glyph width in pixels.
     *  @param height  Glyph height in pixels.
     *  @param type    Which atlas slot to pack into.
     *  @return Top-left position, or (-1,-1) if atlas is full. */
    juce::Point<int> packGlyph (int width, int height, Type type) noexcept;

    /** @brief Builds the Region descriptor for a just-packed glyph, evicts if the
     *  cache is at capacity, and inserts the new cache entry — the backend-blind
     *  tail called once by rasterize() from any of the three rasterizeByBackend
     *  entries, once that entry's own packGlyph() call has already succeeded.
     *  @param key      Unique glyph identity, used as the cache key.
     *  @param bounds   Bounds produced by whichever backend rasterized this glyph
     *                  (loadFreeTypeGlyphBounds(), extractGlyphBounds(), or the
     *                  native backend's own CoreText/DirectWrite measurement).
     *  @param packPos  Top-left position returned by packGlyph().
     *  @param type     Which atlas slot the glyph was packed into.
     *  @return Pointer to the newly cached Region. */
    Region* insertRasterizedGlyph (const Key& key, const GlyphBounds& bounds,
                                   juce::Point<int> packPos, Type type) noexcept;

    /** @brief Rebuilds coverageLut from @p newGamma / @p newContrast — called by
     *  setRasterization() whenever either changed.
     *
     *  Formula (Skia SkMaskGamma-derived mask-gamma correction — see
     *  SkMaskGamma.h's own gamma + contrast shaping, adapted here to a single
     *  256-entry byte LUT rather than Skia's 3-channel LCD table):
     *  @code
     *    gammaCorrected = pow (coverage / 255, 1 / gamma)
     *    contrasted     = gammaCorrected + contrast * gammaCorrected * (1 - gammaCorrected)
     *    lut[coverage]  = round (255 * contrasted)
     *  @endcode
     *
     *  @p newGamma == identityGamma and @p newContrast == identityContrast is the
     *  identity mapping (lut[i] == i for every i) — verified once here via a
     *  jassert on lut[128] when both parameters are exactly identity.
     *  @param newGamma     Gamma exponent (1.0 = no gamma correction).
     *  @param newContrast  Contrast (0.0 = no synthetic darkening). */
    void rebuildCoverageLut (float newGamma, float newContrast) noexcept;

    /** @brief Fully evicts every cached glyph and resets both atlas types' shelf
     *  packers back to empty — called by setRasterization() when backend, gamma,
     *  or contrast change, since a different backend or LUT produces different
     *  bitmaps for the same glyph identity; a stale cache entry would otherwise
     *  keep pointing at pixels rasterized under the previous parameters. Atlas
     *  image pixel content is left untouched (harmless — no live Region ever
     *  points at it again); new glyphs simply resume packing from (0, 0). */
    void flushCache() noexcept;

    /** @brief Mono atlas composite implementation — tinted single-channel coverage blend.
     *  Registered into compositors for Type::mono in the constructor. */
    void compositeMonoImpl (juce::Image::BitmapData& targetData, const Region& region,
                            int screenX, int screenY, juce::Colour colour, juce::Rectangle<int> clip) noexcept;

    /** @brief Emoji atlas composite implementation — ARGB src-over blend.
     *  Registered into compositors for Type::emoji in the constructor. */
    void compositeEmojiImpl (juce::Image::BitmapData& targetData, const Region& region,
                             int screenX, int screenY, juce::Colour colour, juce::Rectangle<int> clip) noexcept;

    /** @brief Computes the inverse-alpha-weighted destination blend components shared by
     *  compositeMonoImpl and compositeEmojiImpl. */
    static DestBlend computeDestBlend (uint32_t destPixel, uint32_t invAlpha) noexcept;

    /** @brief Shared row-iteration scaffold for atlas-to-target glyph compositing.
     *  Iterates region rows, computes destY bounds + endCol once, then dispatches
     *  4-wide SIMD batches to simdBlend and the scalar remainder to scalarBlend.
     *  Both callables receive (row, col) into the atlas-local coordinate space —
     *  callers close over their own atlasData/atlasX/atlasY.
     */
    template <typename SimdBlend, typename ScalarBlend>
    void compositeRows (const Region& region, int screenX, int screenY,
                         juce::Image::BitmapData& targetData, juce::Rectangle<int> clip,
                         SimdBlend&& simdBlend, ScalarBlend&& scalarBlend) noexcept;

    // =========================================================================
    // GPU-resident mirror helpers (ported from the since-deleted
    // jam::TextureCache)
    // =========================================================================

    /** @brief Allocates one atlas GPU slot type's BindlessTexture (image sized
     *  to this atlas's own dimension) via jam::VulkanBindlessTexture::create().
     *  Inserts into gpuImages on success.
     *  @return true if the image+view were created successfully. */
    bool createAtlasImage (Type type, vk::Format format);

    // =========================================================================
    // Data
    // =========================================================================

    int dimension { 4096 };

    /** @brief Empty-pixel padding reserved on both axes between packed glyph
     *  slots (packGlyph()) — a defensive margin against a neighboring
     *  glyph's atlas texels bleeding into this slot's edge pixels under the
     *  atlas's nearest-sampled GPU reads (integer-snapped quads — see
     *  jam::VulkanLowLevelGraphicsContext::packGlyphQuadType()). */
    static constexpr int slotGutter { 1 };

    /** @brief Maximum cache entries before evictLeastRecentlyUsed() triggers.
     *  Ported bound: legacy jam::glyph::LRUCache split mono=19000/
     *  emoji=4000 budgets across two separate caches; this atlas already unifies
     *  both types into this one jam::HashMap<Key,CacheEntry>, so the combined
     *  legacy budget becomes one bound here. */
    static constexpr size_t maxCachedGlyphs { 23000 };

    /** @brief Identity gamma exponent — coverageLut's gamma pass is a no-op at
     *  this value. See rebuildCoverageLut()'s doc comment. */
    static constexpr float identityGamma { 1.0f };

    /** @brief Identity contrast — coverageLut's contrast pass is a no-op at this
     *  value. See rebuildCoverageLut()'s doc comment. */
    static constexpr float identityContrast { 0.0f };

    /** @brief Active rasterization backend, set by setRasterization(). Defaults to
     *  FontRasterizerBackend::edgeTable so a GlyphAtlas used before any setRasterization() call
     *  (the CPU-fallback LowLevelGraphicsGlyphRenderer path — no jam::VulkanEngine,
     *  hence no setRasterization() call site) still rasterizes through a defined,
     *  working backend rather than an unset one. */
    map::FontRasterizerBackend::value backend { map::FontRasterizerBackend::edgeTable };

    /** @brief Active coverage LUT gamma exponent, set by setRasterization(). */
    float gamma { identityGamma };

    /** @brief Active coverage LUT contrast, set by setRasterization(). */
    float contrast { identityContrast };

    /** @brief Synthetic FreeType embolden state, set by setEmbolden(). Applied
     *  at the rasterizeFreeType() rasterize site (jam_GlyphAtlasFreeType.cpp)
     *  via `FT_Outline_Embolden` before `FT_Render_Glyph`. */
    bool embolden { false };

    /** @brief Coverage-conditioning lookup table (gamma + contrast), applied to
     *  every mono coverage byte at write time — FreeType/native per-row copies
     *  via writeMonoCoverageRow(), EdgeTable's MonoCoverageWriter per-pixel/
     *  per-run writes — never applied to emoji ARGB bytes. Rebuilt by
     *  rebuildCoverageLut() whenever setRasterization() changes gamma/contrast;
     *  initialized to the identity mapping at construction. */
    std::array<uint8_t, 256> coverageLut {};

    uint64_t frameCounter { 0 };

    jam::HashMap<Type, Slot> images;

    /** @brief Per-atlas-type software composite implementations, registered in the constructor. */
    jam::Function::Map<Type, void> compositors;

    jam::HashMap<Key, CacheEntry, Key::Hash> cache;

    /** @brief Shared FreeType library instance — owns every FT_Face in
     *  freetypeFaces. Created in the constructor via FT_Init_FreeType with no
     *  further property configuration (unhinted raster — see
     *  loadFreeTypeGlyphBounds()); every FT_Face is released before this is,
     *  in the destructor. */
    FT_Library freetypeLibrary { nullptr };

    /** @brief FreeType faces, keyed by the juce::Typeface identity they were
     *  created from — populated by registerTypeface() (tier 1, embedded
     *  defaults) and getOrLoadSystemTypeface() (tier 2, lazily-resolved system
     *  fonts; also the negative-resolution cache, see FaceEntry). rasterize()
     *  consults this to choose the FreeType unhinted raster path over the
     *  EdgeTable coverage path. */
    jam::HashMap<const juce::Typeface*, FaceEntry> freetypeFaces;

    /** @brief rasterizeNative()'s per-typeface cached OS-native font handle —
     *  see NativeFontEntry's doc comment. Empty until rasterizeNative() first
     *  runs for a given typeface (FontRasterizerBackend::native only). */
    jam::HashMap<const juce::Typeface*, NativeFontEntry> nativeFonts;

#if JUCE_WINDOWS
    juce::ComSmartPtr<IDWriteFactory5> directWriteFactory;
    juce::ComSmartPtr<IDWriteInMemoryFontFileLoader> inMemoryFontFileLoader;
#endif

    /** @brief Shared Vulkan device — not owned, must outlive this GlyphAtlas.
     *  Used only by the GPU-resident mirror methods above. */
    jam::VulkanDevice& device;

    /** @brief GPU-resident glyph atlas textures (mono R8 + emoji BGRA8), keyed
     *  by Type — each entry owns its jam::VulkanImage plus the per-window
     *  bindless registry for that image (see jam::VulkanBindlessTexture's
     *  class doc comment). Originally migrated from
     *  jam::TextureCache::atlasImages; the value type became
     *  BindlessTexture when the upload + bindless registry was
     *  extracted out of this atlas (see class doc comment). */
    jam::HashMap<Type, jam::VulkanBindlessTexture> gpuImages {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GlyphAtlas)
};

#if JUCE_MAC
/** @brief Resolves the on-disk font file backing a system typeface via
 *  CoreText — the same family+style CTFontDescriptor construction JUCE's own
 *  CoreTextTypeface::from() uses (juce_Fonts_mac.mm), so the resolved file is
 *  the exact file JUCE itself would have rasterized from. Implemented in
 *  jam_GlyphAtlas_mac.mm.
 *  @param fontFamily  juce::Typeface::getName() of the typeface to resolve.
 *  @param fontStyle   juce::Typeface::getStyle() of the typeface to resolve.
 *  @return The resolved font file, or an invalid juce::File if CoreText could
 *          not resolve a descriptor, font, or file URL for this family/style.
 */
juce::File findSystemFontFile (const juce::String& fontFamily, const juce::String& fontStyle);
#endif

#if JUCE_WINDOWS
/** @brief Resolves the on-disk font file backing a system typeface via
 *  DirectWrite. Implemented in jam_GlyphAtlas.cpp under #if JUCE_WINDOWS —
 *  plain DirectWrite/COM calls need no separate translation unit, mirroring
 *  jam_LowLevelGraphicsGlyphRenderer.cpp's own Windows GDI presentation branch.
 *  @param fontFamily  juce::Typeface::getName() of the typeface to resolve.
 *  @param fontStyle   juce::Typeface::getStyle() of the typeface to resolve —
 *                     mapped best-effort to a DWRITE_FONT_WEIGHT/STYLE pair
 *                     ("Bold" substring → DWRITE_FONT_WEIGHT_BOLD, "Italic"
 *                     substring → DWRITE_FONT_STYLE_ITALIC, else regular).
 *  @return The resolved font file, or an invalid juce::File if DirectWrite
 *          could not resolve a matching family, font, face, or local file
 *          path for this family/style.
 */
juce::File findSystemFontFile (const juce::String& fontFamily, const juce::String& fontStyle);
#endif

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
