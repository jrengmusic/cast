#pragma once
namespace jam
{
/*____________________________________________________________________________*/

/**
   * @file jam_Array.h
   * @brief Lightweight dynamic array — juce::Array ergonomics with STL compatibility.
   */

/**
   * @class Array
   * @brief A dynamic-sized array combining juce::Array's int-based API with
   *        std::vector's STL compatibility, at a smaller footprint than either
   *        std::vector.
   *
   * Design goals:
   * - **int-indexed:** eliminates the size_t cast noise endemic to std::vector
   *   usage in JUCE codebases. int counters also shrink the footprint.
   * - **STL-compatible:** exposes pointer-based iterators — works with range-for,
   *   std::algorithm, and structured bindings.
   * - **juce::Array API:** add, addIfNotAlreadyThere, insert, insertMultiple,
   *   insertArray, set, remove, removeRange, indexOf, contains, ensureStorageAllocated,
   *   sort.
   * - **Amortized O(1) append:** capacity doubles on exhaustion.
   * - **Move-only:** unique ownership of the backing store — no accidental deep copies.
   * - **Uninitialized backing store:** storage is a raw ElementType* allocated with
   *   std::malloc/std::realloc/std::free. Only the slots in [0, size()) are live,
   *   constructed objects — capacity beyond size() is untyped memory. Elements are
   *   placement-constructed on add/insert and explicitly destroyed on vacate
   *   (remove(), removeRange(), resize() shrink, clear(), destruction), matching
   *   std::vector's construct-on-insert/destroy-on-erase semantics.
   *
   * Footprint (64-bit): raw pointer (8) + numElements (4) + numAllocated (4) = 16 bytes,
   * versus std::vector's 24. Matches juce::Array's footprint while adding STL iterators.
   *
   * indexOf / contains / addIfNotAlreadyThere are O(n) linear scans — identical to
   * juce::Array. ElementType must be destructible, copy- or move-constructible for
   * add/insert, and provide operator== (for search) and operator< (for sort).
   * resize() growth additionally requires ElementType to be default-constructible.
   * ElementType's alignment must not exceed std::max_align_t (malloc's guarantee).
   *
   * @tparam ElementType  The value type stored in the array.
   */
template<typename ElementType, typename TypeOfCriticalSectionToUse = juce::DummyCriticalSection>
class Array : public TypeOfCriticalSectionToUse
{
public:
    //==============================================================================
    // Construction / Destruction
    //==============================================================================

    /** @brief Creates an empty array with no allocation. */
    Array() noexcept = default;

    /** @brief Destructor. Destroys live elements and releases the backing store. */
    ~Array()
    {
        const ScopedLockType lock (getLock());
        destroyRange (storage, storage + numElements);
        std::free (storage);
    }

    /**
       * @brief Move constructor — transfers ownership of the backing store,
       *        leaving the source empty (size and capacity both zero).
       *
       * Explicit (not defaulted): a defaulted move leaves the source's
       * numElements/numAllocated at their pre-move values even though storage
       * itself is now null — a subsequent add() on the source would then skip
       * reallocation (capacity looks sufficient) and write through a null
       * pointer. Zeroing both counters keeps the source in the same empty,
       * safely-reusable state std::vector guarantees after a move.
       */
    Array (Array&& other) noexcept
        : TypeOfCriticalSectionToUse()
    {
        const ScopedLockType lock (other.getLock());
        storage = other.storage;
        numElements = other.numElements;
        numAllocated = other.numAllocated;
        other.storage = nullptr;
        other.numElements = 0;
        other.numAllocated = 0;
    }

    /**
       * @brief Move assignment — transfers ownership of the backing store,
       *        leaving the source empty (size and capacity both zero).
       *
       * Explicit for the same reason as the move constructor above.
       */
    Array& operator= (Array&& other) noexcept
    {
        if (this != &other)
        {
            const ScopedLockType lock (getLock());
            const ScopedLockType otherLock (other.getLock());

            destroyRange (storage, storage + numElements);
            std::free (storage);

            storage = other.storage;
            numElements = other.numElements;
            numAllocated = other.numAllocated;

            other.storage = nullptr;
            other.numElements = 0;
            other.numAllocated = 0;
        }

        return *this;
    }

    /**
       * @brief Creates an array with pre-allocated capacity but zero elements.
       *
       * @param initialCapacity  Number of element slots to pre-allocate.
       */
    explicit Array (int initialCapacity)
        : storage (allocateRaw (std::max (0, initialCapacity)))
        , numElements (0)
        , numAllocated (std::max (0, initialCapacity))
    {
    }

    /**
       * @brief Creates an array from an initializer list.
       *
       * @param list  The values to copy into this array.
       */
    Array (std::initializer_list<ElementType> list)
        : storage (allocateRaw (static_cast<int> (list.size())))
        , numElements (static_cast<int> (list.size()))
        , numAllocated (static_cast<int> (list.size()))
    {
        int i { 0 };

        for (const auto& element : list)
            new (storage + i++) ElementType (element);
    }

    using ScopedLockType = typename TypeOfCriticalSectionToUse::ScopedLockType;

    /** @brief Returns the critical section used to lock this array. */
    const TypeOfCriticalSectionToUse& getLock() const noexcept
    {
        return *this;
    }

    //==============================================================================
    // Size / Capacity
    //==============================================================================

    /** @brief Returns the current number of elements. */
    int size() const noexcept
    {
        const ScopedLockType lock (getLock());
        return numElements;
    }

    /** @brief Returns true when the array contains no elements. */
    bool isEmpty() const noexcept
    {
        const ScopedLockType lock (getLock());
        return numElements == 0;
    }

    /** @brief Returns the allocated capacity (elements before the next realloc). */
    int getCapacity() const noexcept
    {
        const ScopedLockType lock (getLock());
        return numAllocated;
    }

    /**
       * @brief Grows internal storage to hold at least the given number of elements.
       *
       * Grows to exactly minNumElements (no doubling) — the caller knows the target.
       * No-op when the capacity already suffices. Does not change size().
       *
       * @param minNumElements  Minimum element slots to guarantee.
       */
    void ensureStorageAllocated (int minNumElements)
    {
        const ScopedLockType lock (getLock());

        if (minNumElements > numAllocated)
            reallocateStorage (minNumElements);
    }

    /**
       * @brief Changes the live element count to newSize.
       *
       * Growing ensures capacity for newSize, then placement-constructs
       * ElementType{} into the newly exposed slots [size(), newSize). Shrinking
       * destroys the vacated tail [newSize, size()) to release any RAII resource
       * those elements held, then reduces the live count. Capacity is retained
       * either way, mirroring clear()'s own contract.
       *
       * @param newSize  The desired live element count.
       */
    void resize (int newSize)
    {
        jassert (newSize >= 0);

        const ScopedLockType lock (getLock());

        if (newSize > numElements)
        {
            ensureStorageAllocated (newSize);

            for (int i { numElements }; i < newSize; ++i)
                new (storage + i) ElementType{};
        }
        else
        {
            destroyRange (storage + newSize, storage + numElements);
        }

        numElements = newSize;
    }

    //==============================================================================
    // Iterators
    //==============================================================================

    /** @brief Returns a pointer to the first element. */
    ElementType* begin() noexcept
    {
        const ScopedLockType lock (getLock());
        return storage;
    }

    /** @brief Returns a const pointer to the first element. */
    const ElementType* begin() const noexcept
    {
        const ScopedLockType lock (getLock());
        return storage;
    }

    /** @brief Returns a pointer past the last element. */
    ElementType* end() noexcept
    {
        const ScopedLockType lock (getLock());
        return storage + numElements;
    }

    /** @brief Returns a const pointer past the last element. */
    const ElementType* end() const noexcept
    {
        const ScopedLockType lock (getLock());
        return storage + numElements;
    }

    /** @brief Returns a raw pointer to the contiguous element storage. */
    ElementType* data() noexcept
    {
        const ScopedLockType lock (getLock());
        return storage;
    }

    /** @brief Returns a const raw pointer to the contiguous element storage. */
    const ElementType* data() const noexcept
    {
        const ScopedLockType lock (getLock());
        return storage;
    }

    //==============================================================================
    // Element Access
    //==============================================================================

    /**
       * @brief Returns a reference to the first element.
       * @pre Array must not be empty (asserted).
       */
    ElementType& first() noexcept
    {
        const ScopedLockType lock (getLock());
        jassert (numElements > 0);
        return storage[0];
    }

    /** @brief Returns a const reference to the first element. */
    const ElementType& first() const noexcept
    {
        const ScopedLockType lock (getLock());
        jassert (numElements > 0);
        return storage[0];
    }

    /**
       * @brief Returns a reference to the last element.
       * @pre Array must not be empty (asserted).
       */
    ElementType& last() noexcept
    {
        const ScopedLockType lock (getLock());
        jassert (numElements > 0);
        return storage[static_cast<size_t> (numElements - 1)];
    }

    /** @brief Returns a const reference to the last element. */
    const ElementType& last() const noexcept
    {
        const ScopedLockType lock (getLock());
        jassert (numElements > 0);
        return storage[static_cast<size_t> (numElements - 1)];
    }

    /**
       * @brief Fast access to an element by index without bounds checking.
       * @param index  Zero-based index (0 to size() - 1).
       */
    ElementType& operator[] (int index) noexcept
    {
        const ScopedLockType lock (getLock());
        return storage[static_cast<size_t> (index)];
    }

    /** @brief Fast const access to an element by index without bounds checking. */
    const ElementType& operator[] (int index) const noexcept
    {
        const ScopedLockType lock (getLock());
        return storage[static_cast<size_t> (index)];
    }

    /**
       * @brief Bounds-checked access to an element by index.
       *
       * @param index  Zero-based index.
       * @returns      Reference to the element.
       * @throws       std::out_of_range if index is invalid.
       */
    ElementType& at (int index)
    {
        const ScopedLockType lock (getLock());

        if (index >= 0 and index < numElements)
            return storage[static_cast<size_t> (index)];

        throw std::out_of_range ("Array index out of bounds");
    }

    /** @brief Bounds-checked const access to an element by index. */
    const ElementType& at (int index) const
    {
        const ScopedLockType lock (getLock());

        if (index >= 0 and index < numElements)
            return storage[static_cast<size_t> (index)];

        throw std::out_of_range ("Array index out of bounds");
    }

    /**
       * @brief Replaces the element at a given index with a new value.
       *
       * Replaces in place when the index is within range; appends when the index
       * equals the current size.
       *
       * @param indexToChange  Index of the element to replace (0 to size()).
       * @param newValue       The value to store.
       */
    void set (int indexToChange, const ElementType& newValue)
    {
        jassert (indexToChange >= 0 and indexToChange <= numElements);

        const ScopedLockType lock (getLock());

        if (indexToChange >= 0 and indexToChange < numElements)
            storage[static_cast<size_t> (indexToChange)] = newValue;
        else if (indexToChange == numElements)
            add (newValue);
    }

    //==============================================================================
    // Add
    //==============================================================================

    /**
       * @brief Appends a copy of the element. Amortized O(1).
       * @param element  The element to append.
       */
    void add (const ElementType& element)
    {
        const ScopedLockType lock (getLock());
        ensureCapacity (numElements + 1);
        new (storage + numElements) ElementType (element);
        ++numElements;
    }

    /**
       * @brief Appends an element by move. Amortized O(1).
       * @param element  The element to move-append.
       */
    void add (ElementType&& element)
    {
        const ScopedLockType lock (getLock());
        ensureCapacity (numElements + 1);
        new (storage + numElements) ElementType (std::move (element));
        ++numElements;
    }

    /**
       * @brief Appends the element only if no equal element already exists.
       *
       * O(n) linear scan via operator==.
       *
       * @param newElement  The element to add if not already present.
       * @returns           true if the element was added; false if already present.
       */
    bool addIfNotAlreadyThere (const ElementType& newElement)
    {
        const ScopedLockType lock (getLock());

        if (contains (newElement))
            return false;

        add (newElement);
        return true;
    }

    //==============================================================================
    // Insert
    //==============================================================================

    /**
       * @brief Inserts a new element at a given position, shifting later elements up.
       *
       * An index outside [0, size()) appends at the end.
       *
       * @param indexToInsertAt  Position at which to insert.
       * @param newElement       The element to insert.
       */
    void insert (int indexToInsertAt, const ElementType& newElement)
    {
        const ScopedLockType lock (getLock());
        const int target { clampInsertIndex (indexToInsertAt) };

        insertShifted (target, 1, [&] (int) -> const ElementType& { return newElement; });
    }

    /**
       * @brief Inserts a new element by move at a given position, shifting
       *        later elements up.
       *
       * An index outside [0, size()) appends at the end.
       *
       * @param indexToInsertAt  Position at which to insert.
       * @param newElement       The element to move-insert.
       */
    void insert (int indexToInsertAt, ElementType&& newElement)
    {
        const ScopedLockType lock (getLock());
        const int target { clampInsertIndex (indexToInsertAt) };

        insertShifted (target, 1, [&] (int) -> ElementType&& { return std::move (newElement); });
    }

    /**
       * @brief Inserts multiple copies of an element at a given position.
       *
       * @param indexToInsertAt          Position at which to insert.
       * @param newElement               The element to copy in.
       * @param numberOfTimesToInsertIt  Number of copies to insert.
       */
    void
    insertMultiple (int indexToInsertAt, const ElementType& newElement, int numberOfTimesToInsertIt)
    {
        const ScopedLockType lock (getLock());

        if (numberOfTimesToInsertIt > 0)
        {
            const int target { clampInsertIndex (indexToInsertAt) };

            insertShifted (target, numberOfTimesToInsertIt, [&] (int) -> const ElementType&
            { return newElement; });
        }
    }

    /**
       * @brief Inserts an array of values at a given position.
       *
       * @param indexToInsertAt   Position at which to insert.
       * @param newElements       Pointer to the source values.
       * @param numberOfElements  Number of values to insert.
       */
    void insertArray (int indexToInsertAt, const ElementType* newElements, int numberOfElements)
    {
        const ScopedLockType lock (getLock());

        if (numberOfElements > 0)
        {
            const int target { clampInsertIndex (indexToInsertAt) };

            insertShifted (target, numberOfElements, [&] (int offset) -> const ElementType&
            { return newElements[offset]; });
        }
    }

    //==============================================================================
    // Remove
    //==============================================================================

    /**
       * @brief Removes the element at a given index, shifting later elements left.
       * @param indexToRemove  Zero-based index of the element to remove.
       * @pre indexToRemove must be in [0, size()).
       */
    void remove (int indexToRemove)
    {
        jassert (indexToRemove >= 0 and indexToRemove < numElements);

        const ScopedLockType lock (getLock());

        if constexpr (std::is_trivially_copyable_v<ElementType>)
        {
            std::memmove (storage + indexToRemove,
                storage + indexToRemove + 1,
                static_cast<size_t> (numElements - indexToRemove - 1) * sizeof (ElementType));
        }
        else
        {
            for (int i { indexToRemove }; i < numElements - 1; ++i)
                storage[static_cast<size_t> (i)] = std::move (storage[static_cast<size_t> (i + 1)]);

            destroyRange (storage + numElements - 1, storage + numElements);
        }

        --numElements;
    }

    /**
       * @brief Removes a contiguous range of elements, shifting later elements left.
       * @param startIndex      Index of the first element to remove.
       * @param numberToRemove  Number of elements to remove.
       * @pre The range must lie within [0, size()).
       */
    void removeRange (int startIndex, int numberToRemove)
    {
        jassert (startIndex >= 0);
        jassert (numberToRemove >= 0);
        jassert (startIndex + numberToRemove <= numElements);

        const ScopedLockType lock (getLock());

        if constexpr (std::is_trivially_copyable_v<ElementType>)
        {
            std::memmove (storage + startIndex,
                storage + startIndex + numberToRemove,
                static_cast<size_t> (numElements - startIndex - numberToRemove)
                    * sizeof (ElementType));
        }
        else
        {
            for (int i { startIndex }; i < numElements - numberToRemove; ++i)
                storage[static_cast<size_t> (i)] =
                    std::move (storage[static_cast<size_t> (i + numberToRemove)]);

            destroyRange (storage + numElements - numberToRemove, storage + numElements);
        }

        numElements -= numberToRemove;
    }

    /**
       * @brief Removes all elements, resetting size to zero. Capacity is retained.
       *
       * Destroys every live slot [0, size()) — this is where RAII resources held
       * by elements are deterministically released, not at the point a slot is
       * later overwritten by add()/set(). No-op for trivially destructible types.
       */
    void clear()
    {
        const ScopedLockType lock (getLock());
        destroyRange (storage, storage + numElements);
        numElements = 0;
    }

    //==============================================================================
    // Search / Sort
    //==============================================================================

    /**
       * @brief Searches for an element by value and returns its index. O(n).
       * @param elementToFind  The value to search for.
       * @returns              Index of the first match, or -1 if not found.
       */
    int indexOf (const ElementType& elementToFind) const noexcept
    {
        const ScopedLockType lock (getLock());

        for (int i { 0 }; i < numElements; ++i)
            if (storage[static_cast<size_t> (i)] == elementToFind)
                return i;

        return -1;
    }

    /**
       * @brief Alias for indexOf. O(n).
       * @param elementToFind  The value to search for.
       * @returns              Index of the first match, or -1 if not found.
       */
    int find (const ElementType& elementToFind) const noexcept { return indexOf (elementToFind); }

    /**
       * @brief Returns true if an element equal to the given value is present. O(n).
       * @param elementToFind  The value to search for.
       */
    bool contains (const ElementType& elementToFind) const noexcept
    {
        const ScopedLockType lock (getLock());
        return indexOf (elementToFind) != -1;
    }

    /** @brief Sorts the elements in ascending order via operator<. */
    void sort()
    {
        const ScopedLockType lock (getLock());
        std::sort (begin(), end());
    }

    //==============================================================================
    // Raw storage API (HeapBlock-mirrored, trivially-copyable ElementType only)
    //==============================================================================

    void malloc (int n)
    {
        static_assert (std::is_trivially_copyable_v<ElementType>,
            "jam::Array::malloc requires a trivially copyable ElementType.");

        std::free (storage);
        storage = static_cast<ElementType*> (
            std::malloc (static_cast<size_t> (n) * sizeof (ElementType)));
        numElements = n;
        numAllocated = n;
    }

    void calloc (int n)
    {
        static_assert (std::is_trivially_copyable_v<ElementType>,
            "jam::Array::calloc requires a trivially copyable ElementType.");

        std::free (storage);
        storage = static_cast<ElementType*> (std::calloc (static_cast<size_t> (n), sizeof (ElementType)));
        numElements = n;
        numAllocated = n;
    }

    void allocate (int n, bool initialiseToZero)
    {
        static_assert (std::is_trivially_copyable_v<ElementType>,
            "jam::Array::allocate requires a trivially copyable ElementType.");

        if (initialiseToZero)
            calloc (n);
        else
            malloc (n);
    }

    void realloc (int n)
    {
        static_assert (std::is_trivially_copyable_v<ElementType>,
            "jam::Array::realloc requires a trivially copyable ElementType.");

        storage = static_cast<ElementType*> (
            std::realloc (storage, static_cast<size_t> (n) * sizeof (ElementType)));
        numElements = n;
        numAllocated = n;
    }

    void free()
    {
        static_assert (std::is_trivially_copyable_v<ElementType>,
            "jam::Array::free requires a trivially copyable ElementType.");

        std::free (storage);
        storage = nullptr;
        numElements = 0;
        numAllocated = 0;
    }

    void clear (int numElementsToClear)
    {
        static_assert (std::is_trivially_copyable_v<ElementType>,
            "jam::Array::clear (int) requires a trivially copyable ElementType.");

        std::memset (storage, 0, static_cast<size_t> (numElementsToClear) * sizeof (ElementType));
    }

    ElementType* getData() noexcept
    {
        static_assert (std::is_trivially_copyable_v<ElementType>,
            "jam::Array::getData requires a trivially copyable ElementType.");

        return storage;
    }

    const ElementType* getData() const noexcept
    {
        static_assert (std::is_trivially_copyable_v<ElementType>,
            "jam::Array::getData requires a trivially copyable ElementType.");

        return storage;
    }

    ElementType* get() noexcept
    {
        static_assert (std::is_trivially_copyable_v<ElementType>,
            "jam::Array::get requires a trivially copyable ElementType.");

        return storage;
    }

    const ElementType* get() const noexcept
    {
        static_assert (std::is_trivially_copyable_v<ElementType>,
            "jam::Array::get requires a trivially copyable ElementType.");

        return storage;
    }

private:
    //==============================================================================
    // Storage primitives
    //==============================================================================

    /**
       * @brief Allocates numSlots untyped element slots via std::malloc.
       * @param numSlots  Number of element-sized slots to allocate.
       * @returns         Pointer to the raw block, or nullptr when numSlots is 0.
       */
    static ElementType* allocateRaw (int numSlots)
    {
        if (numSlots == 0)
            return nullptr;

        return static_cast<ElementType*> (
            std::malloc (static_cast<size_t> (numSlots) * sizeof (ElementType)));
    }

    /**
       * @brief Destroys the live elements in [first, last). No-op when
       *        ElementType is trivially destructible.
       */
    static void destroyRange (ElementType* first, ElementType* last) noexcept
    {
        if constexpr (not std::is_trivially_destructible_v<ElementType>)
            for (auto* p { first }; p != last; ++p)
                p->~ElementType();
    }

    /**
       * @brief Writes value into slot index, choosing assignment when the slot
       *        was already live before this operation began, or placement
       *        construction when the slot is untyped memory.
       *
       * @param index               Destination slot index.
       * @param liveCountBeforeOp    Live element count at the start of the
       *                             calling operation — slots below this were
       *                             constructed objects, slots at or above it
       *                             are untyped memory.
       * @param value               The value to write.
       */
    template<typename U>
    void placeElement (int index, int liveCountBeforeOp, U&& value)
    {
        jassert (index >= 0 and index < numAllocated);
        jassert (liveCountBeforeOp >= 0 and liveCountBeforeOp <= numAllocated);

        if (index < liveCountBeforeOp)
            storage[static_cast<size_t> (index)] = std::forward<U> (value);
        else
            new (storage + index) ElementType (std::forward<U> (value));
    }

    //==============================================================================
    // Growth
    //==============================================================================

    /**
       * @brief Reallocates storage to newCapacity, transferring existing elements.
       *        Single source of truth for all growth.
       *
       * Trivially copyable ElementType grows via std::realloc. Otherwise a new
       * block is allocated, live elements are relocated with
       * std::uninitialized_move, the old elements are destroyed, and the old
       * block is freed.
       *
       * @param newCapacity  New allocation size in element slots.
       */
    void reallocateStorage (int newCapacity)
    {
        static_assert (alignof (ElementType) <= alignof (std::max_align_t),
            "jam::Array allocates via std::malloc/realloc, which guarantees only "
            "std::max_align_t alignment.");

        if constexpr (std::is_trivially_copyable_v<ElementType>)
        {
            storage = static_cast<ElementType*> (
                std::realloc (storage, static_cast<size_t> (newCapacity) * sizeof (ElementType)));
        }
        else
        {
            auto* newStorage { allocateRaw (newCapacity) };

            std::uninitialized_move (storage, storage + numElements, newStorage);
            destroyRange (storage, storage + numElements);
            std::free (storage);

            storage = newStorage;
        }

        numAllocated = newCapacity;
    }

    /**
       * @brief Ensures capacity for at least the required count, doubling on growth.
       * @param required  Minimum element slots needed.
       */
    void ensureCapacity (int required)
    {
        if (required > numAllocated)
            reallocateStorage (std::max (required, numAllocated * 2));
    }

    /**
       * @brief Clamps an insert index to a valid target; out-of-range appends at end.
       * @param index  Requested insert index.
       * @returns      A target in [0, size()].
       */
    int clampInsertIndex (int index) const noexcept
    {
        if (index >= 0 and index < numElements)
            return index;

        return numElements;
    }

    /**
       * @brief Shared shift-and-fill core for insert / insertMultiple / insertArray.
       *
       * Ensures capacity for count additional elements, shifts the tail
       * [target, size()) up by count slots, then fills [target, target + count)
       * via valueAt(0..count-1). Trivially copyable ElementType shifts via
       * std::memmove and fills via plain assignment. Otherwise the shift walks
       * back-to-front so every read happens before its slot is overwritten, and
       * placeElement() chooses assignment versus placement construction per
       * slot depending on whether that slot held a live object before this call.
       *
       * @param target   Clamped insert position.
       * @param count    Number of new elements being inserted.
       * @param valueAt  Callable taking an int offset in [0, count) and
       *                 returning the value for that new slot.
       */
    template<typename ValueAt>
    void insertShifted (int target, int count, ValueAt&& valueAt)
    {
        const int oldCount { numElements };
        ensureCapacity (oldCount + count);

        if constexpr (std::is_trivially_copyable_v<ElementType>)
        {
            std::memmove (storage + target + count,
                storage + target,
                static_cast<size_t> (oldCount - target) * sizeof (ElementType));

            for (int k { 0 }; k < count; ++k)
                storage[static_cast<size_t> (target + k)] = valueAt (k);
        }
        else
        {
            for (int i { oldCount - 1 }; i >= target; --i)
                placeElement (i + count, oldCount, std::move (storage[static_cast<size_t> (i)]));

            for (int k { 0 }; k < count; ++k)
                placeElement (target + k, oldCount, valueAt (k));
        }

        numElements = oldCount + count;
    }

    //==============================================================================
    // Storage
    //==============================================================================

    ElementType* storage { nullptr };///< Raw, malloc-owned element storage.
    int numElements { 0 };///< Number of live elements.
    int numAllocated { 0 };///< Allocated capacity (slots).

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Array)
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
