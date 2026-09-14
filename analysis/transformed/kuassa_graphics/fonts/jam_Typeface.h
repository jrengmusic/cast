/**
 * @file jam_Typeface.h
 * @brief Typeface identity table — interns a `juce::Typeface::Ptr` alongside its
 *        HarfBuzz font, for cmap glyph lookup and ligature shaping.
 *
 * Conformance provenance: the CPU shaping half of the pre-rewrite glyph
 * pipeline (`jam::glyph::Arrangement`, deleted commit e8543bf1a^,
 * `jam_vulkan/fonts/font/glyph/jam_Run.h`/`.cpp`) drove its shaping through a
 * platform-native, static god-object `jam::Typeface`
 * (`jam_vulkan/fonts/typeface/jam_typeface.h`, 816 lines — CoreText/DirectWrite
 * shaping, per-style sub-fonts, emoji fallback). That type is NOT restored
 * here — only the API *shape* (a typeface identity table keyed for glyph
 * lookup) survives, rebuilt on top of today's `juce::Typeface` + HarfBuzz,
 * mirroring `jam::Stamp`'s `SharedResources<Stamp>` interning idiom exactly
 * (`jam_graphics/text/jam_Stamp.h`).
 *
 * The ligature-detection HarfBuzz flow this table feeds
 * (`jam::GlyphArrangement::tryLigature`) is recovered from END's own history
 * (`Source/terminal/rendering/ScreenRender.cpp` @ 40d2163 / FreeType+HarfBuzz
 * shaping precedent @ de22c5c) rather than from the deleted god object — that
 * history built its `hb_font_t` directly from font-file bytes (no `hb-ft`
 * bridge), the same construction this table performs per `Entry`.
 *
 * @par Emoji / generic-fallback conformance restoration
 * `Entry` gained a second, bytes-less constructor (`Entry (juce::Typeface::Ptr)`)
 * so `jam::GlyphArrangement`'s emoji-font and per-codepoint generic-fallback
 * resolution (the god object's `hasEmojiGlyph`/`shapeEmoji`/`shapeFallback`
 * semantics, restored in `jam_GlyphArrangement.cpp`) can intern its resolved
 * typefaces into this SAME table — `draw()` and `RunKey` need no special case.
 * Per-codepoint generic-fallback entries still carry `hbFont == nullptr`
 * (see "Bytes-less construction" below) — no raw font-file buffer is
 * available to this module for typefaces resolved via `juce::Font::
 * findSuitableFontForText()`.
 *
 * @par Grapheme-cluster shaping restoration (real `hb_font_t` for the emoji face)
 * The platform emoji typeface is the ONE exception: `Entry` gained a THIRD
 * constructor (`Entry (juce::Typeface::Ptr, juce::MemoryBlock&&)`)
 * that takes ownership of the emoji family's on-disk font-file bytes
 * (resolved via `jam::findSystemFontFile()`, the same route
 * `GlyphAtlas::getOrLoadSystemTypeface()`'s tier-2 uses) and builds a REAL
 * `hb_font_t` from them — the god object's own emoji `hb_font_t` (built from
 * `FT_Face`/`CTFontRef`, a platform-native handle this module otherwise never
 * acquires; today's public equivalent, `juce::Typeface::getNativeDetails()`,
 * stays `@internal`, see the "Bytes-less construction" doc comment below for
 * the full citation) is restored this way instead. `jam::GlyphArrangement`'s
 * constructor-time emoji discovery is the sole caller of this constructor (via the
 * `registerTypeface (Ptr, MemoryBlock&&)` overload below); the font-file
 * bytes have no static/`BinaryData` owner to alias (unlike tier-1's embedded
 * defaults), so this Entry must own its copy — mirrors `GlyphAtlas::
 * FaceEntry::fontBytes`'s identical "atlas owns its own copy" precedent
 * (`jam_GlyphAtlas.h`). This unblocks `jam::GlyphArrangement`'s cluster-shaping
 * restoration (see `jam_GlyphArrangement.h`'s file doc, "Known gap" section).
 *
 * @par Instance ownership
 * `Typeface` is a `SharedResources<Typeface>`, which inherits `Instance<Typeface>`
 * (`jam_lexicon/utils/jam_Instance.h`) — the SAME mechanism `jam::Stamp` and
 * `jam::Grapheme` already use. The owning instance is a plain member of
 * `jam::VulkanEngine` (`jam_vulkan/engine/jam_VulkanEngine.h`), declared
 * alongside `stamp`/`grapheme`/`link` — VulkanEngine is the unified
 * resource-ownership tree for the Vulkan rendering backend, superseding the
 * prior Application-owned placement.
 */
#pragma once

namespace jam
{ /*____________________________________________________________________________*/

/**
 * @struct Typeface
 * @brief Shared typeface identity table — interns `juce::Typeface::Ptr` + `hb_font_t`.
 *
 * Inherits `addIfNotAlreadyThere(entry) -> int` / `get(index)` from
 * `SharedResources<Typeface>`. `registerTypeface()` and `find()` below are the
 * ergonomic entry points consumers actually call.
 */
struct Typeface : SharedResources<Typeface>
{
    /**
     * @struct Entry
     * @brief One interned typeface: its `juce::Typeface::Ptr` plus the HarfBuzz
     *        font built from the same font-file bytes.
     *
     * Builds `hb_blob_t` → `hb_face_t` → `hb_font_t` at construction
     * (`hb_face_create`/`hb_font_create` each take their own reference, so the
     * intermediate blob/face are released immediately after — standard
     * HarfBuzz refcounting idiom); `hbFont` is released in the destructor
     * (RAII). Non-copyable — a copied `Entry` would double-destroy the same
     * `hb_font_t`; `SharedResources<Typeface>::addIfNotAlreadyThere
     * (std::unique_ptr<SharedResource>)` (the move-only overload, documented
     * there as "e.g. Typeface") is the interning path this Entry type uses.
     *
     * @note @p fontData is stored as a non-owning pointer, mirroring
     *       `GlyphAtlas::Key::typeface`'s own non-owning rasterization field
     *       (`jam_GlyphAtlas.h`) — the caller (typically an embedded
     *       `BinaryData` array with static storage duration, the same source
     *       `GlyphAtlas::registerTypeface()`'s tier-1 embedded defaults use)
     *       must guarantee @p fontData outlives this Entry. No defensive copy
     *       is made here.
     *
     * @par Bytes-less construction (no `hbFont`)
     * A second constructor interns a `juce::Typeface::Ptr` with no backing
     * font-file bytes — `hbFont` stays `nullptr`. This is the path
     * `jam::GlyphArrangement`'s per-codepoint generic-fallback resolution uses
     * (typefaces resolved via `juce::Font::findSuitableFontForText()`, where no
     * raw font-file buffer is available to this module — no on-disk-file
     * resolution route equivalent to `jam::findSystemFontFile()` exists for
     * an arbitrary fallback candidate typeface, only for a fixed, named
     * platform family such as the emoji font below).
     *
     * Conformance provenance note: the deleted god object
     * (`jam_vulkan/fonts/typeface/jam_typeface.h`, e8543bf1a^) built its own
     * `hb_font_t` for the emoji face directly from `FT_Face`
     * (`hb_ft_font_create`) or `CTFontRef` (`hb_coretext_font_create`) — a
     * platform-native handle this module never acquires. The equivalent today,
     * `juce::Font::getNativeDetails()` / `juce::Typeface::getNativeDetails()`,
     * is `@internal`: both return `Font::Native`/`Typeface::Native`, forward-declared
     * only in the public header (`juce_Font.h:614`, `juce_Typeface.h:317`) and
     * defined solely inside `juce_Font.cpp`/`juce_Typeface.cpp`
     * (`juce_Font.cpp:38`) — those definitions are visible only within
     * `juce_graphics`'s own amalgamated compile unit (confirmed: every
     * `getNativeDetails()` call site outside that unit is itself inside
     * `modules/juce_graphics/{native,detail}/`; none exists in any dependent
     * module). An external module such as this one cannot access `.font` on an
     * incomplete type via `getNativeDetails()` — for the per-codepoint generic
     * fallback path (no fixed family name to resolve a file for) this bytes-less
     * constructor is the resulting conformant shape: intern the identity (for
     * `draw()`/`RunKey` SSOT), resolve glyphs via
     * `juce::Typeface::getNominalGlyphForCodepoint()` instead of
     * `hb_font_get_nominal_glyph()` — see `GlyphArrangement::findFallbackGlyph()`'s
     * own doc comment for the full citation.
     *
     * @par Owned-bytes construction (real `hbFont`, no caller-side lifetime obligation)
     * A THIRD constructor takes a `juce::MemoryBlock&&` instead of a non-owning
     * `(data, size)` pair — the font-file bytes are MOVED into this Entry
     * (stored in @ref ownedFontBytes) rather than aliased from caller-owned
     * storage, and @ref fontData / @ref fontDataSize point into that owned block.
     * This is the platform emoji typeface's route (`jam::GlyphArrangement`'s
     * constructor-time emoji discovery, via `registerTypeface (Ptr, MemoryBlock&&)`
     * below): its bytes come from `jam::findSystemFontFile()` reading a
     * temporary on-disk file into a `juce::MemoryBlock` with no static/
     * `BinaryData` owner to alias — mirrors `GlyphAtlas::FaceEntry::fontBytes`'s
     * identical "atlas owns its own copy" precedent (`jam_GlyphAtlas.h`). This
     * restores a real `hb_font_t` for the emoji face specifically, closing
     * `jam_GlyphArrangement.h`'s "Known gap" (grapheme-cluster HarfBuzz shaping)
     * without touching the bytes-less generic-fallback path above.
     */
    struct Entry : SharedResource
    {
        juce::Typeface::Ptr juceTypeface;      ///< The interned typeface identity.

        /** @brief Owned font-file bytes for the owned-bytes constructor (see class
         *  doc comment's "Owned-bytes construction" section) — empty for the other
         *  two constructors. Declared before @ref fontData so it is constructed
         *  first (member-initialization order), letting @ref fontData safely
         *  point into it. */
        juce::MemoryBlock   ownedFontBytes;

        /** @brief Font-file bytes (TTF/OTF) for cmap/shaping. Two ownership
         *  modes: (1) caller-owned, not copied — the (data, size) constructor,
         *  caller must outlive this Entry; (2) points into @ref ownedFontBytes,
         *  owned by this Entry — the `MemoryBlock&&` constructor. `nullptr` for
         *  the bytes-less constructor. */
        const void*         fontData     { nullptr };
        size_t              fontDataSize { 0 };       ///< Size of @ref fontData in bytes. `0` for the bytes-less constructor.
        hb_font_t*          hbFont       { nullptr };  ///< HarfBuzz font built from @ref fontData, for cmap + shaping. `nullptr` for the bytes-less constructor.

        /** @brief Glyph-index -> codepoint reverse cmap, built at construction
         *  for entries with an @ref hbFont — empty for the bytes-less
         *  constructor (no @ref hbFont to enumerate). */
        jam::HashMap<uint16_t, char32_t> reverseCmap;

        /**
         * @brief Builds the HarfBuzz font from @p data / @p dataSize, stores
         *        @p typefacePtr, and builds @ref reverseCmap.
         *
         * @param typefacePtr  The typeface identity this entry represents.
         * @param data         Font-file bytes (TTF/OTF). Must outlive this Entry.
         * @param dataSize     Size of @p data in bytes.
         */
        Entry (juce::Typeface::Ptr typefacePtr, const void* data, size_t dataSize) noexcept
            : juceTypeface (std::move (typefacePtr)), fontData (data), fontDataSize (dataSize)
        {
            jassert (juceTypeface != nullptr);
            jassert (fontData != nullptr and fontDataSize > 0);

            hbFont = getFont (fontData, fontDataSize);
            reverseCmap = getReverseCmap (hbFont);
        }

        /**
         * @brief Interns @p typefacePtr's identity only — no `hb_font_t` is built.
         *
         * See the class doc comment's "Bytes-less construction" section for why:
         * no font-file bytes are available for typefaces resolved by family name
         * or `juce::Font::findSuitableFontForText()`. `hbFont` stays `nullptr`;
         * `~Entry()`'s existing null-guarded `hb_font_destroy` is unaffected.
         *
         * @param typefacePtr  The typeface identity this entry represents.
         */
        explicit Entry (juce::Typeface::Ptr typefacePtr) noexcept
            : juceTypeface (std::move (typefacePtr))
        {
            jassert (juceTypeface != nullptr);
        }

        /**
         * @brief Takes ownership of @p ownedData, builds the HarfBuzz font and
         *        @ref reverseCmap from it.
         *
         * See the class doc comment's "Owned-bytes construction" section. Same
         * `hb_blob_t` → `hb_face_t` → `hb_font_t` idiom as the caller-owned-bytes
         * constructor above, run against @ref ownedFontBytes (the moved-in
         * @p ownedData) instead of a caller-supplied buffer.
         *
         * @param typefacePtr  The typeface identity this entry represents.
         * @param ownedData    Font-file bytes (TTF/OTF), moved into @ref ownedFontBytes.
         */
        Entry (juce::Typeface::Ptr typefacePtr, juce::MemoryBlock&& ownedData) noexcept
            : juceTypeface (std::move (typefacePtr)), ownedFontBytes (std::move (ownedData)),
              fontData (ownedFontBytes.getData()), fontDataSize (ownedFontBytes.getSize())
        {
            jassert (juceTypeface != nullptr);
            jassert (fontData != nullptr and fontDataSize > 0);

            hbFont = getFont (fontData, fontDataSize);
            reverseCmap = getReverseCmap (hbFont);
        }

        /** @brief Non-copyable — see class doc comment. */
        Entry (const Entry&) = delete;
        /** @brief Non-copyable — see class doc comment. */
        Entry& operator= (const Entry&) = delete;

        /** @brief Releases the HarfBuzz font (RAII). */
        ~Entry() override
        {
            if (hbFont != nullptr)
                hb_font_destroy (hbFont);
        }

        // Reverse-cmap lookup, built at construction (see @ref reverseCmap).
        // Returns 0 on a miss (no hbFont, or the glyph has no cmap-covered
        // codepoint).
        char32_t getCodepoint (uint16_t glyphIndex) const noexcept
        {
            const auto found { reverseCmap.find (glyphIndex) };
            return found != reverseCmap.end() ? found->second : 0;
        }

        /** @brief Identity equality — same `juceTypeface` pointer. */
        bool operator== (const SharedResource& other) const noexcept override
        {
            return juceTypeface.get() == static_cast<const Entry&> (other).juceTypeface.get();
        }

        /** @brief Pointer-identity hash over `juceTypeface.get()`. */
        size_t hash() const noexcept override
        {
            return std::hash<const void*>{} (juceTypeface.get());
        }

    private:
        /**
         * @brief Builds an `hb_font_t` from @p data / @p dataSize.
         *
         * `hb_face_create`/`hb_font_create` each take their own reference, so
         * the intermediate blob/face are released immediately after — standard
         * HarfBuzz refcounting idiom.
         *
         * @param data      Font-file bytes (TTF/OTF).
         * @param dataSize  Size of @p data in bytes.
         * @return          The built HarfBuzz font.
         */
        static hb_font_t* getFont (const void* data, size_t dataSize) noexcept
        {
            hb_blob_t* blob { hb_blob_create (static_cast<const char*> (data),
                                              static_cast<unsigned int> (dataSize),
                                              HB_MEMORY_MODE_READONLY, nullptr, nullptr) };
            hb_face_t* face { hb_face_create (blob, 0) };
            hb_font_t* font { hb_font_create (face) };

            hb_face_destroy (face);
            hb_blob_destroy (blob);

            return font;
        }

        // Builds the inverse of this typeface's cmap: hb_face_collect_unicodes()
        // enumerates every codepoint the font covers (ascending, via
        // hb_set_next()), each resolved to its glyph via
        // hb_font_get_nominal_glyph(), then inverted with jam::Map::getKey().
        // Ascending enumeration + getKey()'s first-insertion-wins emplace()
        // means the smallest codepoint wins any many-to-one glyph collision.
        static jam::HashMap<uint16_t, char32_t> getReverseCmap (hb_font_t* font) noexcept
        {
            jam::HashMap<char32_t, uint16_t> forwardCmap;

            hb_set_t* unicodes { hb_set_create() };
            hb_face_collect_unicodes (hb_font_get_face (font), unicodes);

            hb_codepoint_t codepoint { HB_SET_VALUE_INVALID };

            while (hb_set_next (unicodes, &codepoint))
            {
                hb_codepoint_t glyph { 0 };

                if (hb_font_get_nominal_glyph (font, codepoint, &glyph) != 0)
                    forwardCmap.emplace (static_cast<char32_t> (codepoint), static_cast<uint16_t> (glyph));
            }

            hb_set_destroy (unicodes);

            return jam::Map::getKey (forwardCmap);
        }
    };

    /**
     * @brief Interns @p typefacePtr, building its HarfBuzz font from @p fontData/@p fontDataSize.
     *
     * Idempotent by `juceTypeface` pointer identity (Entry::operator==) — calling
     * this again with the same @p typefacePtr discards the newly-built (and
     * therefore redundant) `hb_font_t` and returns the existing index.
     *
     * @param typefacePtr  Typeface identity to intern.
     * @param fontData     Font-file bytes (TTF/OTF). Must outlive every Entry
     *                     sharing this typeface identity — no defensive copy.
     * @param fontDataSize Size of @p fontData in bytes.
     * @return             Interned index (existing or newly inserted).
     */
    int registerTypeface (const juce::Typeface::Ptr& typefacePtr, const void* fontData, size_t fontDataSize) noexcept
    {
        return addIfNotAlreadyThere (std::make_unique<Entry> (typefacePtr, fontData, fontDataSize));
    }

    /**
     * @brief Interns @p typefacePtr's identity only — no HarfBuzz font is built.
     *
     * Idempotent by `juceTypeface` pointer identity, same as the byte-backed
     * overload above. For typefaces with no available font-file bytes
     * (per-codepoint fallback fonts resolved via `juce::Font::
     * findSuitableFontForText()`, or the platform emoji font when
     * `jam::findSystemFontFile()` cannot resolve its on-disk file) — see
     * `Entry`'s bytes-less constructor doc comment for why `hb_font_t` cannot
     * be built for these. Consumers resolve glyphs for these entries via
     * `juce::Typeface::getNominalGlyphForCodepoint()` (`Entry::hbFont` is
     * `nullptr`) — see `GlyphArrangement::findEmojiGlyph()` /
     * `findFallbackGlyph()`.
     *
     * @param typefacePtr  Typeface identity to intern.
     * @return             Interned index (existing or newly inserted).
     */
    int registerTypeface (const juce::Typeface::Ptr& typefacePtr) noexcept
    {
        return addIfNotAlreadyThere (std::make_unique<Entry> (typefacePtr));
    }

    /**
     * @brief Interns @p typefacePtr, taking ownership of @p ownedFontData and
     *        building its HarfBuzz font from it.
     *
     * Idempotent by `juceTypeface` pointer identity, same as the other
     * overloads. Unlike the caller-owned-bytes overload above, @p ownedFontData
     * is MOVED into the interned `Entry` (@ref Entry::ownedFontBytes) — no
     * caller-side lifetime obligation. See `Entry`'s "Owned-bytes construction"
     * doc comment; the platform emoji typeface
     * (`GlyphArrangement`'s constructor-time emoji discovery) is this overload's sole caller.
     *
     * @param typefacePtr    Typeface identity to intern.
     * @param ownedFontData  Font-file bytes (TTF/OTF), moved into the new `Entry`.
     * @return               Interned index (existing or newly inserted).
     */
    int registerTypeface (const juce::Typeface::Ptr& typefacePtr, juce::MemoryBlock&& ownedFontData) noexcept
    {
        return addIfNotAlreadyThere (std::make_unique<Entry> (typefacePtr, std::move (ownedFontData)));
    }

    /**
     * @brief Finds the interned index for @p typefacePtr.
     *
     * Linear scan over interned entries, comparing `juceTypeface.get()` pointer
     * identity — the "small find method on the container" consumers use when
     * they hold only a raw `const juce::Typeface*` (e.g. `jam::GlyphArrangement::
     * Run::typeface`) and need to resolve back to the owning Entry (its
     * `hbFont`, or its `juceTypeface` `Ptr` for `juce::FontOptions::withTypeface`).
     *
     * @param typefacePtr  Typeface identity to look up.
     * @return             Interned index, or -1 if not registered.
     */
    int find (const juce::Typeface* typefacePtr) const noexcept
    {
        int result { -1 };

        for (int index { 0 }; index < size() and result < 0; ++index)
            if (get (index).juceTypeface.get() == typefacePtr)
                result = index;

        return result;
    }
};

/**______________________________END OF NAMESPACE______________________________*/
} // namespace jam
