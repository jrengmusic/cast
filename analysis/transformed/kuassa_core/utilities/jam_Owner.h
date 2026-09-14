/**
 * @file jam_Owner.h
 * @brief Owning jam::Array of unique_ptr<ObjectClass> with juce::OwnedArray-style API.
 */

#pragma once

namespace jam
{
/*____________________________________________________________________________*/

//==============================================================================
/**
 * @brief A wrapper around jam::Array specifically designed for holding objects.
 *
 * This class gives jam::Array interface similar to juce::OwnedArray, but you
 * still can use as jam::Array, i.e with std::algorithm.
 *
 * std::for_each (myOwner.begin(), myOwner.end(), [] (auto& item) { // do something for each item });
 *
 * This holds a list of pointers to objects, and will automatically
 * delete the objects when they are removed from the array, or when the
 * array is itself deleted.
 *
 * When std::hash<ObjectClass> is specialized, Owner provides O(1) content-based
 * dedup via an internal unordered_set. Otherwise, addIfNotAlreadyThere falls back
 * to O(n) linear scan with operator==. sort() additionally requires
 * ObjectClass::operator<.
 *
 * Declare it in the form:
 *
 *     Owner<ObjectClass> myOwner;
 *
 * ..and then add new objects, either one of the following will work just fine :
 *
 *     myOwner.add (std::make_unique<MyObjectClass>());
 *
 *     auto item = std::make_unique<MyObjectClass> {};
 *     myOwner.add (std::move (item));
 *
 * After adding objects, they are owned by the Owner and will be deleted when
 * removed or replaced.
 */
template<typename ObjectClass>
class Owner
{
public:
    using const_iterator = const std::unique_ptr<ObjectClass>*;
    using iterator = std::unique_ptr<ObjectClass>*;

    //==============================================================================
    Owner() = default;
    ~Owner() = default;

    /** @brief Move constructor. */
    Owner (Owner&&) noexcept = default;

    /** @brief Move assignment. */
    Owner& operator= (Owner&&) noexcept = default;

    //==============================================================================
    /**
     * @brief Constructor using std::initializer_list with std::move.
     *
     * @param list The initializer list to use for constructing the Owner.
     */
    Owner (std::initializer_list<std::unique_ptr<ObjectClass>> list)
    {
        for (auto& item : list)
            add (std::move (const_cast<std::unique_ptr<ObjectClass>&> (item)));
    }

    /**
     * @brief Constructor to create an Owner with a specified size and arguments.
     *
     * @tparam Args The types of the arguments to pass to the object's constructor.
     * @param size The number of objects to create.
     * @param args The arguments to pass to the object's constructor.
     */
    template<typename... Args>
    Owner (size_t size, Args&&... args)
    {
        for (size_t i { 0 }; i < size; ++i)
            add (std::make_unique<ObjectClass> (args...));
    }

    //==============================================================================
    /**
     * @brief Adds a new object to the end of the array.
     *
     * @param newObject The unique_ptr object to add.
     * @returns Reference to the added unique_ptr.
     */
    std::unique_ptr<ObjectClass>& add (std::unique_ptr<ObjectClass>&& newObject)
    {
        if constexpr (IsHashable<ObjectClass>::value)
        {
            lookup.emplace (newObject.get(), entries.size());
        }

        entries.add (std::move (newObject));
        return entries.last();
    }

    //==============================================================================
    /**
     * @brief Adds only if content is not already present.
     *        O(1) if std::hash<ObjectClass> is specialized — no linear scan.
     *        O(n) linear fallback otherwise. Requires operator== in both cases.
     *
     * @param newObject The unique_ptr object to add if not already present.
     * @returns Index of the existing or newly inserted entry.
     */
    int addIfNotAlreadyThere (std::unique_ptr<ObjectClass>&& newObject)
    {
        int result { -1 };

        if constexpr (IsHashable<ObjectClass>::value)
        {
            const auto found { lookup.find (newObject.get()) };

            if (found != lookup.end())
            {
                result = found->second;
            }
            else
            {
                result = entries.size();
                lookup.emplace (newObject.get(), result);
                entries.add (std::move (newObject));
            }
        }
        else
        {
            bool found { false };

            for (int i { 0 }; i < entries.size() and not found; ++i)
            {
                if (*entries.at (i) == *newObject)
                {
                    result = i;
                    found = true;
                }
            }

            if (not found)
            {
                result = entries.size();
                entries.add (std::move (newObject));
            }
        }

        return result;
    }

    //==============================================================================
    /**
     * @brief Inserts an object at the given position, shifting subsequent entries.
     *
     * @param pos        Position to insert before.
     * @param newObject  The unique_ptr object to insert.
     * @returns Iterator to the newly inserted entry.
     */
    iterator insert (const_iterator pos, std::unique_ptr<ObjectClass>&& newObject)
    {
        const auto insertIndex { static_cast<int> (pos - entries.begin()) };

        if constexpr (IsHashable<ObjectClass>::value)
        {
            for (auto& [ptr, idx] : lookup)
            {
                if (idx >= insertIndex)
                    ++idx;
            }

            lookup.emplace (newObject.get(), insertIndex);
        }

        entries.insert (insertIndex, std::move (newObject));
        return entries.begin() + insertIndex;
    }

    //==============================================================================
    /**
     * @brief Checks if the array contains a specific object.
     *
     * @param objectToLookFor The object to look for in the array.
     * @returns true if the object is found, false otherwise.
     */
    bool contains (const std::unique_ptr<ObjectClass>& objectToLookFor) const noexcept
    {
        return getConstIterator (objectToLookFor) != entries.end();
    }

    //==============================================================================
    /**
     * @brief Finds the index of an object which might be in the array.
     *
     * @param objectToLookFor The object to look for in the array.
     * @returns The index of the object if found, -1 otherwise.
     */
    int indexOf (const std::unique_ptr<ObjectClass>& objectToLookFor) const noexcept
    {
        const auto found { getConstIterator (objectToLookFor) };
        int result { -1 };

        if (found != entries.end())
            result = static_cast<int> (std::distance (entries.begin(), found));

        return result;
    }

    /**
     * @brief Finds the index of an object by raw pointer.
     *
     * @param objectToLookFor The raw pointer to search for.
     * @returns The index of the object if found, -1 otherwise.
     */
    int indexOf (const ObjectClass* objectToLookFor) const noexcept
    {
        const auto found { std::find_if (entries.begin(),
                                         entries.end(),
                                         [objectToLookFor] (const auto& ptr)
                                         {
                                             return ptr.get() == objectToLookFor;
                                         }) };

        int result { -1 };

        if (found != entries.end())
            result = static_cast<int> (std::distance (entries.begin(), found));

        return result;
    }

    //==============================================================================
    /** @brief Finds the index of an entry by value comparison.
     *
     *  O(1) if ObjectClass defines a Hash functor (uses the internal lookup table).
     *  O(n) linear fallback with operator== otherwise.
     *
     *  @param value  The value to search for.
     *  @return Index of the matching entry, or -1 if not found.
     */
    int find (const ObjectClass& value) const noexcept
    {
        int result { -1 };

        if constexpr (IsHashable<ObjectClass>::value)
        {
            const auto found { lookup.find (const_cast<ObjectClass*> (&value)) };

            if (found != lookup.end())
                result = found->second;
        }
        else
        {
            bool found { false };

            for (int i { 0 }; i < entries.size() and not found; ++i)
            {
                if (*entries.at (i) == value)
                {
                    result = i;
                    found = true;
                }
            }
        }

        return result;
    }

    //==============================================================================
    /**
     * @brief Removes an object from the array.
     *
     * @param indexToRemove The index of the object to remove.
     */
    void remove (int indexToRemove)
    {
        if constexpr (IsHashable<ObjectClass>::value)
        {
            lookup.erase (entries.at (indexToRemove).get());

            for (auto& [ptr, idx] : lookup)
            {
                if (idx > indexToRemove)
                    --idx;
            }
        }

        entries.remove (indexToRemove);
    }

    void remove (size_t indexToRemove) { remove (static_cast<int> (indexToRemove)); }

    //==============================================================================
    /**
     * @brief Replaces the entry at index with a new object, keeping lookup in sync.
     *
     * @param index      The index to replace.
     * @param newObject  The new object to store at index.
     */
    void set (size_t index, std::unique_ptr<ObjectClass>&& newObject)
    {
        if constexpr (IsHashable<ObjectClass>::value)
        {
            lookup.erase (entries.at (static_cast<int> (index)).get());
            lookup.emplace (newObject.get(), static_cast<int> (index));
        }

        entries.at (static_cast<int> (index)) = std::move (newObject);
    }

    //==============================================================================
    /**
     * @brief Removes a range of objects from the array.
     *
     * @param startIndex The starting index of the range to remove.
     * @param numberToRemove The number of objects to remove.
     */
    void removeRange (int startIndex, int numberToRemove)
    {
        if constexpr (IsHashable<ObjectClass>::value)
        {
            const auto first { entries.begin() + startIndex };
            const auto last { first + numberToRemove };

            for (auto pos { first }; pos != last; ++pos)
                lookup.erase (pos->get());

            for (auto& [ptr, idx] : lookup)
            {
                if (idx >= startIndex)
                    idx -= numberToRemove;
            }
        }

        entries.removeRange (startIndex, numberToRemove);
    }

    //==============================================================================
    /**
     * @brief Removes every entry, resetting the array to empty.
     */
    void clear() noexcept
    {
        if constexpr (IsHashable<ObjectClass>::value)
            lookup.clear();

        entries.clear();
    }

    //==============================================================================
    /**
     * @brief Sorts entries in ascending order via ObjectClass::operator<.
     *
     * @note ObjectClass must provide operator< for this to compile.
     */
    void sort()
    {
        std::sort (entries.begin(), entries.end(),
                   [] (const std::unique_ptr<ObjectClass>& a, const std::unique_ptr<ObjectClass>& b)
                   {
                       return *a < *b;
                   });

        if constexpr (IsHashable<ObjectClass>::value)
        {
            lookup.clear();

            for (int i { 0 }; i < entries.size(); ++i)
                lookup.emplace (entries.at (i).get(), i);
        }
    }

    //==============================================================================
    /** @brief Get a constant iterator to an object. */
    const_iterator cit (const std::unique_ptr<ObjectClass>& obj) const noexcept
    {
        return getConstIterator (obj);
    }

    /** @brief Get a constant iterator to an index. */
    const_iterator cit (int index) const noexcept { return entries.begin() + index; }

    /** @brief Get an iterator to an object. */
    iterator it (const std::unique_ptr<ObjectClass>& obj) noexcept { return getIterator (obj); }

    /** @brief Get an iterator to an index. */
    iterator it (int index) noexcept { return entries.begin() + index; }

    /**
     * @brief Check if an object is at a specific index.
     *
     * @param objectToLookFor The object to look for in the array.
     * @param index The index to check.
     * @returns true if the object is at the index, false otherwise.
     */
    template<typename PointerRefOrInt>
    bool is (const PointerRefOrInt& objectToLookFor, int index) noexcept
    {
        return cit (objectToLookFor) == cit (index);
    }

    //==============================================================================
    /**
     * @brief Check if the array is empty.
     *
     * @returns true if the array is empty, false otherwise.
     */
    bool isEmpty() const noexcept { return entries.isEmpty(); }

    //==============================================================================
    /** @brief Iterator to the first entry. */
    iterator begin() noexcept { return entries.begin(); }

    /** @brief Iterator past the last entry. */
    iterator end() noexcept { return entries.end(); }

    /** @brief Const iterator to the first entry. */
    const_iterator begin() const noexcept { return entries.begin(); }

    /** @brief Const iterator past the last entry. */
    const_iterator end() const noexcept { return entries.end(); }

    /** @brief Const iterator to the first entry. */
    const_iterator cbegin() const noexcept { return entries.begin(); }

    /** @brief Const iterator past the last entry. */
    const_iterator cend() const noexcept { return entries.end(); }

    /** @brief Bounds-checked access to the entry at index. */
    std::unique_ptr<ObjectClass>& at (size_t index) { return entries.at (static_cast<int> (index)); }

    /** @brief Bounds-checked const access to the entry at index. */
    const std::unique_ptr<ObjectClass>& at (size_t index) const
    {
        return entries.at (static_cast<int> (index));
    }

    /** @brief Reference to the last entry. */
    std::unique_ptr<ObjectClass>& back() { return entries.last(); }

    /** @brief Const reference to the last entry. */
    const std::unique_ptr<ObjectClass>& back() const { return entries.last(); }

    /** @brief Number of entries held. */
    int size() const noexcept { return entries.size(); }

private:
    //==============================================================================
    /** @brief Content-based hash functor — dereferences pointer, hashes object via T::Hash. */
    struct ContentHash
    {
        size_t operator() (const ObjectClass* p) const noexcept
        {
            return typename ObjectClass::Hash {}(*p);
        }
    };

    /** @brief Content-based equality functor — dereferences pointers, compares objects. */
    struct ContentEqual
    {
        bool operator() (const ObjectClass* a, const ObjectClass* b) const noexcept
        {
            return *a == *b;
        }
    };

    /** @brief No-op lookup for types without std::hash specialization. */
    struct EmptyLookup
    {
        void erase (ObjectClass*) noexcept {}
    };

    using LookupTable =
        std::conditional_t<IsHashable<ObjectClass>::value,
                           jam::HashMap<ObjectClass*, int, ContentHash, ContentEqual>,
                           EmptyLookup>;
    LookupTable lookup;
    jam::Array<std::unique_ptr<ObjectClass>> entries;

    //==============================================================================
    /** @brief Get a constant iterator to an object by unique_ptr identity. */
    const_iterator
    getConstIterator (const std::unique_ptr<ObjectClass>& objectToLookFor) const noexcept
    {
        return std::find (entries.begin(), entries.end(), objectToLookFor);
    }

    /** @brief Get an iterator to an object by unique_ptr identity. */
    iterator getIterator (const std::unique_ptr<ObjectClass>& objectToLookFor) noexcept
    {
        return std::find (entries.begin(), entries.end(), objectToLookFor);
    }

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Owner)
};

/**_____________________________END OF NAMESPACE______________________________*/
}// namespace jam
