/**
 * @file jam_SharedResource.h
 * @brief SharedResource (polymorphic entry base).
 */
#pragma once

namespace jam
{

static constexpr size_t hashMultiplier { 31 };

/** @brief Polynomial rolling hash (multiplier 31) over raw bytes. BinaryData-inspired. */
inline size_t hashBytes (const void* data, size_t byteCount) noexcept
{
    auto* bytes = static_cast<const uint8_t*> (data);
    size_t hash { 0 };

    for (size_t i { 0 }; i < byteCount; ++i)
        hash = hashMultiplier * hash + static_cast<size_t> (bytes[i]);

    return hash;
}

/*____________________________________________________________________________*/

/**
 * @struct SharedResource
 * @brief Polymorphic base for all interned entry types.
 *
 * Every entry type stored in a `SharedResources<Derived>` container (e.g.
 * `Grapheme::Entry`, `Stamp::Entry`, `Typeface`) must inherit from
 * `SharedResource` and override `operator==` and `hash()`.
 *
 * The nested `Hash` functor is used by `SharedResources<Derived>`'s internal
 * content-dedup lookup for O(1) interning.  Virtual dispatch ensures that
 * the concrete derived type's hash and equality implementations are called
 * through the base pointer.
 *
 * ### Two-type design
 * - `SharedResource`          — entry base (singular): virtual identity contract.
 * - `SharedResources<Derived>` — container (plural): CRTP interning table.
 */
struct SharedResource
{
    virtual ~SharedResource() = default;

    /**
     * @brief Content equality — must compare to the same concrete derived type.
     *
     * Implementations cast `other` to the concrete type via `static_cast` and
     * compare fields.  `SharedResources<Derived>`'s dedup path guarantees that
     * only entries of the same concrete type are compared.
     *
     * @param other  Reference to the other entry (concrete type must match).
     * @return       `true` if the entries represent identical content.
     */
    virtual bool operator== (const SharedResource& other) const noexcept = 0;

    /**
     * @brief Content hash — must be consistent with `operator==`.
     *
     * The `SharedResource::Hash` functor calls this via virtual dispatch so
     * `SharedResources<Derived>` uses the correct per-type hash without
     * knowing the concrete type.
     *
     * @return Polynomial rolling hash of the entry's content.
     */
    virtual size_t hash() const noexcept = 0;

    /**
     * @struct Hash
     * @brief Hash functor for `SharedResources<Derived>`'s dedup lookup.
     *
     * Delegates to the virtual `hash()` method so the concrete derived
     * type's computation is used without the lookup knowing that type.
     */
    struct Hash
    {
        size_t operator() (const SharedResource& e) const noexcept
        {
            return e.hash();
        }
    };
};

} // namespace jam
