/**
 * @file jam_SharedResources.h
 * @brief SharedResources (CRTP interning container).
 */
#pragma once

namespace jam
{

/*____________________________________________________________________________*/

/**
 * @struct SharedResources
 * @brief CRTP interning table — deduplicates `Derived::Entry` objects.
 *
 * Stores entries polymorphically in fixed-size chunks of `SharedResource`
 * pointers, indexed by a chunk directory that never relocates.  The
 * concrete entry type is `Derived::Entry`; it inherits `SharedResource`
 * and provides `operator==` and `hash()` overrides so the container's
 * internal lookup can dedup by content.
 *
 * ### Usage
 * ```cpp
 * struct MyTable : SharedResources<MyTable>
 * {
 *     struct Entry : SharedResource { ... };
 * };
 * ```
 *
 * Inherits `addIfNotAlreadyThere(entry) -> int` and `get(index)` accessors.
 * The returned index is stable for the lifetime of the container.
 *
 * @tparam Derived  The concrete CRTP subclass that defines `Entry`.
 */
template <typename Derived>
struct SharedResources : Instance<Derived>
{
protected:
    SharedResources() noexcept = default;

public:
    /**
     * @brief Interns an entry — returns the index of the existing or new entry.
     *
     * Looks up `entry` by value in the internal content-dedup lookup (O(1)
     * hash path via `SharedResource::Hash` → virtual `hash()`).  If not found,
     * a copy is heap-allocated as `Derived::Entry` and stored.
     *
     * The parameter type is `const SharedResource&` (base) so that `Derived::Entry`
     * is not referenced in the method signature — signatures are resolved at class
     * instantiation time when `Derived` is still incomplete.  `Derived::Entry` is
     * referenced only in the method body, which is deferred to the point of call.
     *
     * Callers construct the entry explicitly:
     * `addIfNotAlreadyThere (Stamp::Entry { fg, bg, underline, flags })`
     *
     * @param entry  Entry to intern (concrete type must be `Derived::Entry`).
     * @return       Index of the matching entry (existing or newly inserted).
     */
    int addIfNotAlreadyThere (const SharedResource& entry) noexcept
    {
        const std::scoped_lock writeLock { writeMutex };

        const auto found { lookup.find (const_cast<SharedResource*> (&entry)) };

        if (found != lookup.end())
            return found->second;

        return installLocked (std::make_unique<typename Derived::Entry> (
            static_cast<const typename Derived::Entry&> (entry)));
    }

    /**
     * @brief Interns a heap-allocated entry (move-only path, e.g. Typeface).
     *
     * Takes ownership of the supplied `unique_ptr<SharedResource>` directly.
     * `unique_ptr<Derived::Entry>` converts implicitly because `Derived::Entry`
     * inherits `SharedResource`.
     *
     * @param entry  Heap-allocated entry to intern (ownership transferred).
     * @return       Index of the matching entry (existing or newly inserted).
     */
    int addIfNotAlreadyThere (std::unique_ptr<SharedResource> entry) noexcept
    {
        const std::scoped_lock writeLock { writeMutex };

        const auto found { lookup.find (entry.get()) };

        if (found != lookup.end())
            return found->second;

        return installLocked (std::move (entry));
    }

    /**
     * @brief Returns a const reference to the entry at `index`.
     *
     * Return type is deduced (`auto&`) — the body casts from `SharedResource&`
     * to `Derived::Entry&`.  Return-type deduction is deferred to the point of
     * call, so `Derived::Entry` is visible by then.
     *
     * A cross-thread reader must bound `index` by a preceding `size()` call
     * (acquire) or equivalent synchronization — the debug-only bounds assert
     * is not a fence.
     *
     * @param index  Zero-based entry index.
     * @return       Const reference to the concrete entry.
     */
    auto& get (int index) const noexcept
    {
        jassert (index >= 0 and index < size());

        const int chunkIndex { index / chunkSize };
        const int slotIndex { index % chunkSize };

        return static_cast<const typename Derived::Entry&> (
            *chunks.at (static_cast<size_t> (chunkIndex))->at (static_cast<size_t> (slotIndex)));
    }

    /**
     * @brief Returns a mutable reference to the entry at `index`.
     *
     * @param index  Zero-based entry index.
     * @return       Mutable reference to the concrete entry.
     */
    auto& get (int index) noexcept
    {
        jassert (index >= 0 and index < size());

        const int chunkIndex { index / chunkSize };
        const int slotIndex { index % chunkSize };

        return static_cast<typename Derived::Entry&> (
            *chunks.at (static_cast<size_t> (chunkIndex))->at (static_cast<size_t> (slotIndex)));
    }

    /**
     * @brief Returns the number of interned entries.
     * @return Count of entries currently stored.
     */
    int size() const noexcept
    {
        return count.load (std::memory_order_acquire);
    }

private:
    static constexpr int chunkSize { 256 };
    static constexpr int maxChunks { 256 };
    static constexpr int maxEntries { chunkSize * maxChunks };

    using Chunk = std::array<std::unique_ptr<SharedResource>, chunkSize>;

    /** @brief Content-based hash functor — dereferences pointer, hashes entry via SharedResource::Hash. */
    struct ContentHash
    {
        size_t operator() (const SharedResource* p) const noexcept
        {
            return SharedResource::Hash {}(*p);
        }
    };

    /** @brief Content-based equality functor — dereferences pointers, compares entries. */
    struct ContentEqual
    {
        bool operator() (const SharedResource* a, const SharedResource* b) const noexcept
        {
            return *a == *b;
        }
    };

    /**
     * @brief Installs a new entry under `writeMutex` — allocates its chunk if
     *        needed, then publishes the entry via a release-store to `count`.
     */
    int installLocked (std::unique_ptr<SharedResource> entry)
    {
        const int index { count.load (std::memory_order_relaxed) };

        jassert (index < maxEntries);

        const int chunkIndex { index / chunkSize };
        const int slotIndex { index % chunkSize };

        if (chunks.at (static_cast<size_t> (chunkIndex)) == nullptr)
            chunks.at (static_cast<size_t> (chunkIndex)) = std::make_unique<Chunk>();

        lookup.emplace (entry.get(), index);
        chunks.at (static_cast<size_t> (chunkIndex))->at (static_cast<size_t> (slotIndex)) = std::move (entry);

        count.store (index + 1, std::memory_order_release);

        return index;
    }

    /** @brief Chunk directory — fixed array of lazily-allocated chunks, never relocates. */
    std::array<std::unique_ptr<Chunk>, maxChunks> chunks;

    /** @brief Published entry count — readers acquire-load, writers release-store. */
    std::atomic<int> count { 0 };

    /** @brief Serializes addIfNotAlreadyThere across threads. */
    std::mutex writeMutex;

    /** @brief O(1) content-dedup lookup, accessed only under writeMutex. */
    jam::HashMap<SharedResource*, int, ContentHash, ContentEqual> lookup;
};

} // namespace jam
