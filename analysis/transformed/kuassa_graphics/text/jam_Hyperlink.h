/**
 * @file jam_Hyperlink.h
 * @brief Hyperlink document carrier table for OSC 8 (an informally-specified
 *        escape sequence, not part of any ratified terminal standard).
 *
 * `jam::Hyperlink::Entry` is the URI/id descriptor referenced by every
 * `jam::AttributedChar::hyperlinkId()`.  `jam::Hyperlink` is a `SharedResources<Hyperlink>` — the
 * third instance of the established interning shape (Stamp, Grapheme, Hyperlink).
 *
 * Interned on the reader thread at OSC 8 parse (`Video::setLink()`),
 * exactly like `jam::Stamp` at SGR resolution — reader interns, message
 * resolves.  The returned 0-based table index + 1 is stored in the Video
 * pen's `activeHyperlinkId` and stamped onto every subsequently written cell via
 * `AttributedChar::withHyperlinkId()`.
 *
 * ### hyperlinkId sentinel contract
 * `jam::AttributedChar::hyperlinkId() == 0` means "no link" (zero-init default). Because
 * this table's own index (`addIfNotAlreadyThere()`) is 0-based, the stored
 * `hyperlinkId` is always `tableIndex + 1` — never the raw index — so the FIRST
 * interned link cannot collide with the sentinel. Consumers resolve an
 * entry via `jam::Hyperlink::getInstance()->get(hyperlinkId - 1)`.
 *
 * ### Dedup contract
 * OSC 8 carries two fields: the URI and an optional `id=` parameter.
 * - **Explicit id** (`id` non-empty): entries with the same `id` dedupe to
 *   one `jam::Hyperlink` regardless of `uri` — matches the OSC 8 spec's
 *   same-id-same-link hover/highlight semantics.
 * - **Implicit id** (`id` empty): entries dedupe by `uri` alone.
 * - An explicit-id entry and an implicit-id entry are never equal to each
 *   other — they occupy separate dedup domains.
 */
#pragma once

namespace jam
{ /*____________________________________________________________________________*/

/**
 * @struct Hyperlink
 * @brief Shared hyperlink table — dedupes `Hyperlink::Entry` values.
 *
 * Inherits `addIfNotAlreadyThere(entry) -> int` from SharedResources; the
 * returned index is stored in a `jam::AttributedChar` via `AttributedChar::withHyperlinkId()` and
 * read back via `AttributedChar::hyperlinkId()`.
 */
struct Hyperlink : SharedResources<Hyperlink>
{
    /**
     * @struct Entry
     * @brief Hyperlink descriptor: OSC 8 URI and optional explicit id= param.
     *
     * The atom interned in `jam::Hyperlink` and referenced by `jam::AttributedChar::hyperlinkId()`.
     * Inherits `SharedResource` so it can be stored polymorphically by the
     * SharedResources interning container and compared/hashed via virtual dispatch.
     */
    struct Entry : SharedResource
    {
        juce::String uri;      ///< OSC 8 target URI.
        juce::String id;       ///< OSC 8 `id=` param; empty when the link is implicit.

        /** @brief Construct from uri/id. */
        Entry (juce::String newUri, juce::String newId) noexcept
            : uri (std::move (newUri)), id (std::move (newId)) {}

        /** @brief Default-construct (empty uri/id). */
        Entry() noexcept = default;

        /**
         * @brief Dedup equality — explicit ids compare by id alone, implicit by uri alone.
         *
         * Two entries with a non-empty `id` on both sides compare by `id`.
         * Two entries with an empty `id` on both sides compare by `uri`.
         * Mixed (one empty, one non-empty) are never equal — separate domains.
         */
        bool operator== (const SharedResource& other) const noexcept override
        {
            const auto& o { static_cast<const Entry&> (other) };
            bool equal;

            if (id.isNotEmpty() and o.id.isNotEmpty())
                equal = (id == o.id);
            else if (id.isEmpty() and o.id.isEmpty())
                equal = (uri == o.uri);
            else
                equal = false;

            return equal;
        }

        /**
         * @brief Polynomial hash over the dedup key (id when explicit, else uri).
         *
         * Runs `jam::hashBytes` over the UTF-8 bytes of whichever field
         * `operator==` compares by, keeping hash and equality consistent.
         */
        size_t hash() const noexcept override
        {
            size_t result;

            if (id.isNotEmpty())
                result = hashBytes (id.toRawUTF8(), static_cast<size_t> (id.getNumBytesAsUTF8()));
            else
                result = hashBytes (uri.toRawUTF8(), static_cast<size_t> (uri.getNumBytesAsUTF8()));

            return result;
        }
    };
};

/**______________________________END OF NAMESPACE______________________________*/
} // namespace jam
