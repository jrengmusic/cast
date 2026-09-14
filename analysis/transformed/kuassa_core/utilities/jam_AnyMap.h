/**
 * @file jam_AnyMap.h
 * @brief Type-erased, identifier-keyed heterogeneous object owner.
 */

namespace jam
{
/*____________________________________________________________________________*/

/**
 * @class AnyMap
 * @brief Type-erased heterogeneous owner keyed by juce::Identifier.
 *
 * AnyMap stores objects of arbitrary types in an identifier-keyed container,
 * ensuring correct destruction via type-erased deleters.  It is the keyed
 * analog of AnyOwner — same Deleter pattern, same non-copyable but movable
 * semantics, same type-check asserts on retrieval.
 *
 * Typical usage:
 * @code
 * AnyMap map;
 * map.add<float> (juce::Identifier { "windowOpacity" }, 0.85f);
 * map.add<int>   (juce::Identifier { "blurRadius" }, 20);
 *
 * float opacity = *map.get<float> (juce::Identifier { "windowOpacity" });
 * int   blur    = *map.get<int>   (juce::Identifier { "blurRadius" });
 * @endcode
 *
 * @note Prefer AnyOwner (index-keyed) when order matters and identifier keys
 *       add no value.  Use AnyMap when callers look up entries by name.
 *       String overloads are provided for backward compatibility; they
 *       auto-intern the string to a juce::Identifier.
 *       Derived containers (e.g. ColourScheme) may access the protected storage
 *       members directly for typed recursion — entries and types are
 *       intentionally protected rather than private to enable this pattern.
 */
template <typename TypeOfCriticalSectionToUse = juce::DummyCriticalSection>
class AnyMap : public TypeOfCriticalSectionToUse
{
public:
    using ScopedLockType = typename TypeOfCriticalSectionToUse::ScopedLockType;

    /**
     * @brief Access the lock used to synchronise this map's storage.
     * @return A reference to the critical section instance.
     */
    const TypeOfCriticalSectionToUse& getLock() const noexcept
    {
        return *this;
    }

    /** @brief Default constructor. */
    AnyMap() = default;

    /** @brief Deleted copy constructor. */
    AnyMap (const AnyMap&) = delete;

    /** @brief Deleted copy assignment. */
    AnyMap& operator= (const AnyMap&) = delete;

    /** @brief Move constructor. */
    AnyMap (AnyMap&& other) noexcept
        : TypeOfCriticalSectionToUse()
    {
        const ScopedLockType lock (other.getLock());
        entries = std::move (other.entries);
        types   = std::move (other.types);
    }

    /** @brief Move assignment. */
    AnyMap& operator= (AnyMap&& other) noexcept
    {
        if (this != &other)
        {
            const ScopedLockType lock (getLock());
            const ScopedLockType otherLock (other.getLock());
            entries = std::move (other.entries);
            types   = std::move (other.types);
        }

        return *this;
    }

    /**
     * @brief Construct and store an object of type AnyType under @p key.
     *
     * @tparam AnyType The type of object to construct.
     * @tparam Args    Constructor argument types.
     * @param key      The identifier key to associate with the new object.
     * @param args     Arguments forwarded to AnyType's constructor.
     * @return A pointer to the object stored under @p key: the newly
     *         constructed object if @p key was absent, or the existing
     *         entry's pointer if @p key was already present. On a duplicate
     *         key the newly constructed object is destroyed and the
     *         existing entry is left unchanged; a debug build logs the
     *         duplicate via debug::Log::write and asserts the requested
     *         type matches the stored type.
     */
    template <typename AnyType, typename... Args>
    AnyType* add (const juce::Identifier& key, Args&&... args)
    {
        const ScopedLockType lock (getLock());

        auto pointer { std::make_unique<AnyType> (std::forward<Args> (args)...) };

        auto [entry, inserted] { entries.try_emplace (key, nullptr, &AnyMap::deleter<AnyType>) };
        auto& [storedKey, storedPointer] { *entry };

        if (inserted)
        {
            auto* raw { pointer.release() };
            storedPointer.reset (raw);

            AnyMap* group { nullptr };

            if constexpr (std::is_base_of_v<AnyMap, AnyType>)
            {
                group = static_cast<AnyMap*> (raw);
            }

            types.emplace (key, Trait { std::type_index (typeid (AnyType)), group });
        }
        else
        {
#if JUCE_DEBUG
            debug::Log::write ("AnyMap::add: duplicate key \"" + key.toString() + "\"");
#endif
            assert (types.at (key).type == std::type_index (typeid (AnyType)));
        }

        return static_cast<AnyType*> (storedPointer.get());
    }

    /**
     * @brief Retrieve a stored object by key with type checking (const).
     *
     * @tparam AnyType The expected type of the stored object.
     * @param key      The identifier key to look up.
     * @return A const pointer to the stored object of type AnyType.
     *
     * @warning Asserts if the key is absent or the type does not match.
     */
    template <typename AnyType>
    const AnyType* get (const juce::Identifier& key) const
    {
        const ScopedLockType lock (getLock());

        assert (entries.count (key) > 0);
        assert (types.at (key).type == std::type_index (typeid (AnyType)));
        return static_cast<const AnyType*> (entries.at (key).get());
    }

    /**
     * @brief Retrieve a stored object by key with type checking (non-const).
     *
     * @tparam AnyType The expected type of the stored object.
     * @param key      The identifier key to look up.
     * @return A pointer to the stored object of type AnyType.
     *
     * @warning Asserts if the key is absent or the type does not match.
     */
    template <typename AnyType>
    AnyType* get (const juce::Identifier& key)
    {
        const ScopedLockType lock (getLock());

        assert (entries.count (key) > 0);
        assert (types.at (key).type == std::type_index (typeid (AnyType)));
        return static_cast<AnyType*> (entries.at (key).get());
    }

    /**
     * @brief Test whether a key is present in the map.
     * @param key  The identifier key to test.
     * @return True if the key exists, false otherwise.
     */
    bool contains (const juce::Identifier& key) const noexcept
    {
        const ScopedLockType lock (getLock());
        return entries.count (key) > 0;
    }

    /**
     * @brief Test whether the stored type for @p key matches AnyType.
     * @tparam AnyType  The type to check against.
     * @param key       The identifier key to test.
     * @return True if the key exists and the stored type matches AnyType.
     */
    template <typename AnyType>
    bool isType (const juce::Identifier& key) const noexcept
    {
        const ScopedLockType lock (getLock());

        const auto typeEntry { types.find (key) };

        if (typeEntry != types.end())
        {
            const auto& [storedKey, storedTrait] { *typeEntry };
            return storedTrait.type == std::type_index (typeid (AnyType));
        }

        return false;
    }

    /**
     * @brief Construct and store an object of type AnyType under @p key (String overload).
     *
     * Auto-interns @p key to a juce::Identifier.  Provided for backward
     * compatibility with callers that hold juce::String keys.
     *
     * @tparam AnyType The type of object to construct.
     * @tparam Args    Constructor argument types.
     * @param key      The string key; auto-interned to juce::Identifier.
     * @param args     Arguments forwarded to AnyType's constructor.
     * @return A pointer to the object stored under @p key: the newly
     *         constructed object if @p key was absent, or the existing
     *         entry's pointer if @p key was already present. On a duplicate
     *         key the newly constructed object is destroyed and the
     *         existing entry is left unchanged; a debug build logs the
     *         duplicate via debug::Log::write and asserts the requested
     *         type matches the stored type.
     */
    template <typename AnyType, typename... Args>
    AnyType* add (const juce::String& key, Args&&... args)
    {
        const ScopedLockType lock (getLock());
        return add<AnyType> (juce::Identifier { key }, std::forward<Args> (args)...);
    }

    /**
     * @brief Retrieve a stored object by string key with type checking (const).
     *
     * Auto-interns @p key to a juce::Identifier.  Provided for backward
     * compatibility with callers that hold juce::String keys.
     *
     * @tparam AnyType The expected type of the stored object.
     * @param key      The string key; auto-interned to juce::Identifier.
     * @return A const pointer to the stored object of type AnyType.
     */
    template <typename AnyType>
    const AnyType* get (const juce::String& key) const
    {
        const ScopedLockType lock (getLock());
        return get<AnyType> (juce::Identifier { key });
    }

    /**
     * @brief Retrieve a stored object by string key with type checking (non-const).
     *
     * Auto-interns @p key to a juce::Identifier.  Provided for backward
     * compatibility with callers that hold juce::String keys.
     *
     * @tparam AnyType The expected type of the stored object.
     * @param key      The string key; auto-interned to juce::Identifier.
     * @return A pointer to the stored object of type AnyType.
     */
    template <typename AnyType>
    AnyType* get (const juce::String& key)
    {
        const ScopedLockType lock (getLock());
        return get<AnyType> (juce::Identifier { key });
    }

    /**
     * @brief Test whether a string key is present in the map.
     *
     * Auto-interns @p key to a juce::Identifier.  Provided for backward
     * compatibility with callers that hold juce::String keys.
     *
     * @param key  The string key; auto-interned to juce::Identifier.
     * @return True if the key exists, false otherwise.
     */
    bool contains (const juce::String& key) const noexcept
    {
        const ScopedLockType lock (getLock());
        return contains (juce::Identifier { key });
    }

    /**
     * @brief Iterate all entries with type-erased access via Base.
     *
     * Calls @p callback for every stored entry, casting each entry to Base*.
     * Caller is responsible for ensuring all stored objects are derived from
     * (or identical to) Base.
     *
     * @tparam Base    The base type to cast stored objects to.
     * @param callback Invoked with the key and a Base reference per entry.
     */
    template <typename Base>
    void forEach (const std::function<void (const juce::Identifier&, Base&)>& callback)
    {
        const ScopedLockType lock (getLock());

        for (auto& [key, ptr] : entries)
        {
            callback (key, *static_cast<Base*> (ptr.get()));
        }
    }

    /**
     * @brief Recursively visits every leaf entry in this map and all nested AnyMaps.
     *
     * Groups are entries whose stored type is AnyMap or derives from it; they are
     * recursed into.  All other entries are passed to @p function as void*.
     *
     * @param function  Called for each leaf entry with its key and raw pointer.
     */
    void applyFunctionRecursively (const std::function<void (const juce::Identifier&, void*)>& function)
    {
        const ScopedLockType lock (getLock());

        for (auto& [key, ptr] : entries)
        {
            const auto& trait { types.at (key) };

            if (trait.group != nullptr)
                trait.group->applyFunctionRecursively (function);
            else
                function (key, ptr.get());
        }
    }

    /**
     * @brief Recursively visits every leaf entry in this map and all nested AnyMaps (const).
     *
     * Groups are entries whose stored type is AnyMap or derives from it; they are
     * recursed into.  All other entries are passed to @p function as const void*.
     *
     * @param function  Called for each leaf entry with its key and raw pointer.
     */
    void applyFunctionRecursively (const std::function<void (const juce::Identifier&, const void*)>& function) const
    {
        const ScopedLockType lock (getLock());

        for (const auto& [key, ptr] : entries)
        {
            const auto& trait { types.at (key) };

            if (trait.group != nullptr)
            {
                const AnyMap& group { *trait.group };
                group.applyFunctionRecursively (function);
            }
            else
            {
                function (key, ptr.get());
            }
        }
    }

    /**
     * @brief Recursively searches leaf entries. Short-circuits on first true return.
     *
     * Groups are entries whose stored type is AnyMap or derives from it; they are
     * recursed into.  All other entries are passed to @p function. Returns true
     * immediately when @p function returns true.
     *
     * @param function  Called for each leaf entry. Return true to stop traversal.
     * @return true if any function call returned true.
     */
    bool applyFunctionRecursively (const std::function<bool (const juce::Identifier&, void*)>& function)
    {
        const ScopedLockType lock (getLock());

        for (auto& [key, ptr] : entries)
        {
            const auto& trait { types.at (key) };

            if (trait.group != nullptr)
            {
                if (trait.group->applyFunctionRecursively (function))
                    return true;
            }
            else
            {
                if (function (key, ptr.get()))
                    return true;
            }
        }

        return false;
    }

    /**
     * @brief Query the number of stored objects.
     * @return The number of entries in the map.
     */
    size_t size() const noexcept
    {
        const ScopedLockType lock (getLock());
        return entries.size();
    }

    /**
     * @brief Remove all stored objects.
     */
    void clear() noexcept
    {
        const ScopedLockType lock (getLock());
        entries.clear();
        types.clear();
    }

    /**
     * @brief Remove a single entry by key.
     * @param key  The key to remove.
     * @return True if the key existed and was removed, false if not found.
     */
    bool remove (const juce::Identifier& key) noexcept
    {
        const ScopedLockType lock (getLock());
        types.erase (key);
        return entries.erase (key) > 0;
    }

    /**
     * @brief Remove a single entry by string key.
     * @param key  The string key; auto-interned to juce::Identifier.
     * @return True if the key existed and was removed, false if not found.
     */
    bool remove (const juce::String& key) noexcept
    {
        const ScopedLockType lock (getLock());
        return remove (juce::Identifier { key });
    }

protected:
    using Deleter = void (*) (void*);

    /**
     * @struct Trait
     * @brief Per-key metadata: the stored dynamic type and, for entries that are
     *        an AnyMap (derived types included), a base-adjusted pointer to it.
     */
    struct Trait
    {
        std::type_index type;         ///< Exact stored type, checked by get\<AnyType\>()'s typed-read gate.
        AnyMap* group { nullptr };    ///< Base-adjusted pointer aliasing the entry's own storage; non-null only when the stored type is AnyMap or derives from it.
    };

    /**
     * @brief Type-specific deleter for stored objects.
     * @tparam T The type of object to delete.
     * @param p  Pointer to the object.
     */
    template <typename T>
    static void deleter (void* p)
    {
        delete static_cast<T*> (p);
    }

    jam::HashMap<juce::Identifier, std::unique_ptr<void, Deleter>> entries; ///< Stored objects.
    jam::HashMap<juce::Identifier, Trait>                           types;  ///< Type info per key.
};

/**_____________________________END OF NAMESPACE______________________________*/
}// namespace jam
