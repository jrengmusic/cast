#pragma once

namespace jam
{

/**
   * @file jam_Strings.h
   * @brief Arena-backed string container with a juce::StringArray-compatible API.
   */

/**
   * @class Strings
   * @brief A dynamic string collection offering juce::StringArray's method surface
   *        while storing every element's bytes contiguously in one growable arena.
   *
   * Storage is two-tier:
   * - **Materialised tier:** every added string's UTF-8 bytes live in a single
   *   append-only jam::Array\<char\> arena, addressed by a packed {offset, length}
   *   Span per element in the spans table. Once appended, bytes are never
   *   overwritten, shifted, or freed in place — mutation always appends fresh
   *   bytes and repoints the affected Span, so existing Spans stay valid for as
   *   long as the arena itself lives.
   * - **Staging tier:** add(juce::String) hands the caller's String handle
   *   straight into a juce::Array\<juce::String\> intake buffer with a single
   *   move — zero refcount churn, zero arena bytes touched. Staged handles are
   *   transcribed into the arena (flushed) the moment any read path needs a
   *   consistent view of contents: search, iteration order, sort, join, and so
   *   on. Native ingest via add(std::string_view) writes straight to the arena
   *   when the staging tier is already empty, skipping juce::String entirely.
   *
   * Two auxiliary indexes accelerate repeated queries: exactOccurrences (a
   * span → count map, watermark-deferred so appends stay O(1) amortised and the
   * map is only caught up to the current tail on the next read) and the pair
   * exactPositions / foldedPositions (span → first-index maps, rebuilt in full
   * whenever the underlying spans table changes — spans is always the single
   * source of truth these are lazily re-derived from). Case-insensitive lookups
   * fold ASCII bytes on the fly inside the hash/equality functors; no folded
   * copy of any string is ever materialised.
   *
   * Move and swap are O(1): every index functor holds a pointer to the boxed
   * arena object rather than to the owning Strings instance, so the arena's
   * heap address — stable across move/swap — keeps every functor valid without
   * a rebuild.
   *
   * Case folding is ASCII-range only, matching this codebase's existing
   * lower/upper-case trade-off elsewhere: no UTF-8-aware fold primitive is
   * available that would avoid materialising a folded copy, so the ASCII fast
   * path is the deliberate choice over full Unicode case folding. Arena growth
   * is monotonic — repeated remove/replace churn grows the arena's high-water
   * mark but never shrinks it, trading memory for the append-only invariant
   * that keeps every Span valid. Allocation failure is fatal, not
   * exception-safe — this class's own backing store, jam::Array, aborts on
   * out-of-memory via its std::malloc/std::realloc convention, and this class
   * makes no attempt to recover from that.
   */
class Strings
{
public:
    /** @brief Creates an empty string container with no allocation. */
    Strings() noexcept = default;

    /** @brief Destructor. Releases the arena and all staged and materialised strings. */
    ~Strings() = default;

    /**
       * @brief Copy constructor.
       *
       * Bulk-copies the source arena bytes and span table verbatim (both are
       * trivially copyable). exactOccurrences stays empty and
       * occurrenceWatermark stays 0 — the copy itself is a pure memcpy, and
       * the occurrence index re-derives lazily from the watermark on the
       * next exact-path query. Position maps stay dirty and are rederived
       * lazily on the next read, never copied.
       *
       * @param other  The container to copy.
       */
    Strings (const Strings& other);

    /**
       * @brief Move constructor. O(1).
       *
       * Every index map's functors hold a pointer to the boxed arena object,
       * not to *this — the box's address is stable across this move, so this
       * object's maps travel with their functors unchanged and need no
       * rebuild. other is left in a valid, reusable empty state: its maps
       * are rebuilt against the fresh arena box it receives, so a later
       * mutation on other never dereferences a functor pointing at the
       * moved-from arena.
       *
       * @param other  The container to move from. Left empty.
       */
    Strings (Strings&& other) noexcept;

    /**
       * @brief Creates a container holding a single string.
       * @param firstValue  The string to add.
       */
    explicit Strings (const juce::String& firstValue) { addToStorage (getView (firstValue)); }

    /**
       * @brief Creates a container from a variadic list of string-convertible values.
       * @tparam OtherElements  Types convertible to juce::String via its constructor.
       * @param firstValue   The first string to add.
       * @param otherValues  The remaining strings to add, in order.
       */
    template<typename... OtherElements>
    Strings (juce::StringRef firstValue, OtherElements&&... otherValues)
    {
        const auto first = juce::String (firstValue);
        addToStorage (getView (first));

        (
            [this] (auto&& element)
            {
                const auto text = juce::String (std::forward<decltype (element)> (element));
                addToStorage (getView (text));
            }(std::forward<OtherElements> (otherValues)),
            ...);
    }

    /**
       * @brief Creates a container from a brace-enclosed list of C strings.
       * @param stringList  Null-terminated C strings to add, in order.
       */
    Strings (const std::initializer_list<const char*>& stringList);

    /**
       * @brief Creates a container from a juce::Array of strings.
       * @param stringArray  The strings to add, in order.
       */
    explicit Strings (const juce::Array<juce::String>& stringArray);

    /**
       * @brief Creates a container from a juce::StringArray.
       * @param stringArray  The strings to add, in order.
       */
    explicit Strings (const juce::StringArray& stringArray);

    /**
       * @brief Creates a container from a raw array of juce::String.
       * @param stringsToAdd     Pointer to the first string.
       * @param numberOfStrings  Number of strings to add.
       * @pre numberOfStrings is not negative, and stringsToAdd is non-null
       *      whenever numberOfStrings is greater than zero.
       */
    Strings (const juce::String* stringsToAdd, int numberOfStrings);

    /**
       * @brief Creates a container from a raw array of C strings, with an
       *        explicit count. Entries may be null and contribute an empty string.
       * @param strings          Pointer to the first C string.
       * @param numberOfStrings  Number of entries to add.
       * @pre numberOfStrings is not negative, and strings is non-null
       *      whenever numberOfStrings is greater than zero.
       */
    Strings (const char* const* strings, int numberOfStrings);

    /**
       * @brief Creates a container from a null-terminated array of C strings.
       * @param strings  Pointer to the first entry of a null-terminated array.
       * @pre strings is non-null and the array is terminated by a null entry.
       */
    explicit Strings (const char* const* strings);

    /**
       * @brief Creates a container from a raw array of wide C strings, with an
       *        explicit count. Entries may be null and contribute an empty string.
       * @param strings          Pointer to the first wide C string.
       * @param numberOfStrings  Number of entries to add.
       * @pre numberOfStrings is not negative, and strings is non-null
       *      whenever numberOfStrings is greater than zero.
       */
    Strings (const wchar_t* const* strings, int numberOfStrings);

    /**
       * @brief Creates a container from a null-terminated array of wide C strings.
       * @param strings  Pointer to the first entry of a null-terminated array.
       * @pre strings is non-null and the array is terminated by a null entry.
       */
    explicit Strings (const wchar_t* const* strings);

    /**
       * @brief Copy assignment. Strong exception-neutral via copy-then-swap.
       * @param other  The container to copy.
       * @returns      Reference to this container.
       */
    Strings& operator= (const Strings& other);

    /**
       * @brief Move assignment. O(1) via move-then-swap.
       * @param other  The container to move from. Left empty.
       * @returns      Reference to this container.
       */
    Strings& operator= (Strings&& other) noexcept;

    /**
       * @brief Exchanges the entire contents of this container with other. O(1).
       * @param other  The container to swap with.
       */
    void swapWith (Strings& other) noexcept;

    /**
       * @brief Compares contents element-by-element, in order.
       * @param other  The container to compare against.
       * @returns      true when both containers hold the same strings in the same order.
       */
    bool operator== (const Strings& other) const noexcept;

    /**
       * @brief Compares contents element-by-element, in order.
       * @param other  The container to compare against.
       * @returns      true when the containers differ in size or in any element.
       */
    bool operator!= (const Strings& other) const noexcept { return not(*this == other); }

    //==============================================================================
    // Add / insert / set
    //==============================================================================

    /**
       * @brief Appends a string, taking ownership of its handle.
       *
       * Byte-for-byte juce's own add: one handle move into the staging tier,
       * zero refcount atomics on the temporary, zero arena bytes touched.
       *
       * @param stringToAdd  The string to append.
       */
    void add (juce::String stringToAdd) { intake.add (std::move (stringToAdd)); }

    /**
       * @brief Appends raw UTF-8 bytes directly, bypassing juce::String entirely.
       *
       * Direct-to-arena when the staging tier is empty (one memcpy, zero
       * mallocs); otherwise the staged elements are flushed first to
       * preserve logical order.
       *
       * @param textToAdd  The UTF-8 bytes to append.
       */
    void add (std::string_view textToAdd);

    /**
       * @brief Appends raw UTF-8 bytes directly, bypassing juce::String entirely.
       * @param textToAdd  The UTF-8 bytes to append.
       */
    void add (const std::string& textToAdd) { add (std::string_view { textToAdd }); }

    /**
       * @brief Appends raw UTF-8 bytes directly, bypassing juce::String entirely.
       * @param stringToAdd  The string to append.
       */
    void add (juce::StringRef stringToAdd);

    /**
       * @brief Inserts a string at a given position, shifting later elements up.
       *
       * Appends the new bytes via addToStorage first (the single append+pack
       * sequence shared by every mutator), then relocates the freshly
       * created tail span into position — spans.remove/insert ride
       * jam::Array's memmove paths for trivially copyable elements.
       *
       * @param index         Position at which to insert. An index outside
       *                      [0, size()) appends at the end.
       * @param stringToAdd   The string to insert.
       */
    void insert (int index, const juce::String& stringToAdd);

    /**
       * @brief Appends stringToAdd only when no equal string is already present.
       *
       * The non-ignoreCase path stages a one-query grace before flushing:
       * the first query against a non-empty staging tier answers from the
       * materialised index plus a linear walk of the staged handles and
       * marks the tier queried; a second query while queried flushes first
       * and falls through to the plain indexed path. Absent values found
       * only once the tier is non-empty are staged (not materialised) — no
       * flush is paid just to append.
       *
       * @param stringToAdd  The string to add if not already present.
       * @param ignoreCase   When true, presence is checked case-insensitively.
       * @returns            true when the string was added; false when an
       *                      equal string was already present.
       */
    bool addIfNotAlreadyThere (const juce::String& stringToAdd, bool ignoreCase);

    /**
       * @brief Replaces the string at a given index.
       *
       * Appends replacement bytes to the arena (preserving the append-only
       * invariant), then decrements the old span's occurrence count and
       * overwrites the slot in place.
       *
       * @param index      Index of the element to replace. An index equal to
       *                    size() appends instead.
       * @param newString  The replacement string.
       * @pre index is not negative.
       */
    void set (int index, const juce::String& newString);

    /**
       * @brief Appends a range of elements copied from another container.
       * @param other               The source container.
       * @param startIndex          Index of the first element to copy.
       * @param numElementsToAdd    Number of elements to copy; a negative
       *                            value copies through the end of other.
       */
    void addArray (const Strings& other, int startIndex, int numElementsToAdd);

    /**
       * @brief Appends every element in [start, end), converting each to
       *        juce::String on the way in.
       * @tparam Iterator  An input iterator whose value type is convertible
       *                   to juce::String.
       * @param start  Iterator to the first element to append.
       * @param end    Iterator past the last element to append.
       */
    template<typename Iterator>
    void addArray (Iterator&& start, Iterator&& end)
    {
        flushIntake();

        for (auto cursor = start; cursor != end; ++cursor)
        {
            const auto value = juce::String (*cursor);
            addToStorage (getView (value));
        }
    }

    /**
       * @brief Appends every element of other that is not already present.
       * @param other       The source container.
       * @param ignoreCase  When true, presence is checked case-insensitively.
       */
    void mergeArray (const Strings& other, bool ignoreCase);

    //==============================================================================
    // Search
    //==============================================================================

    /**
       * @brief Returns true when an equal string is present.
       *
       * The case-sensitive path queries exactOccurrences directly against
       * StringRef's own zero-copy UTF-8 pointer — no arena bytes are ever
       * materialised for a query. The ignoreCase path hashes and compares
       * the raw query through CaseFoldHash/CaseFoldEqual — no folded copy
       * is allocated. The non-ignoreCase path mirrors
       * addIfNotAlreadyThere's one-query grace: while the staging tier
       * holds unflushed handles, the first query answers from the
       * materialised index plus a linear walk of the staged handles; a
       * second query flushes first and falls through to the plain indexed
       * path.
       *
       * @param stringToLookFor  The string to search for.
       * @param ignoreCase       When true, the search is case-insensitive.
       * @returns                true when an equal string is present.
       */
    bool contains (juce::StringRef stringToLookFor, bool ignoreCase) const;

    /**
       * @brief Searches for a string starting from a given index.
       *
       * startIndex == 0 (the common case) answers via a single O(1) hash
       * lookup — zero allocation on both the exact and folded paths, since
       * folding happens inside the functors rather than on a materialised
       * copy. startIndex > 0 cannot be answered from a single
       * first-occurrence record when duplicates exist before startIndex, so
       * it falls back to an O(n) scan of spans directly — the index maps
       * are never consulted there.
       *
       * @param stringToLookFor  The string to search for.
       * @param ignoreCase       When true, the search is case-insensitive.
       * @param startIndex       Index to begin searching from.
       * @returns                Index of the first match at or after
       *                         startIndex, or -1 if not found.
       */
    int indexOf (juce::StringRef stringToLookFor, bool ignoreCase, int startIndex) const;

    //==============================================================================
    // Remove
    //==============================================================================

    /** @brief Removes every element and releases the arena's allocated bytes. */
    void clear();

    /**
       * @brief Removes every element.
       *
       * jam::Array::clear() already retains allocated capacity as its own
       * documented contract — there is no separate non-deallocating variant
       * to delegate to, so clearQuick reuses clear() rather than
       * duplicating identical behaviour.
       */
    void clearQuick() { clear(); }

    /**
       * @brief Removes the element at a given index.
       * @param index  Index of the element to remove. Out-of-range indices are ignored.
       */
    void remove (int index);

    /**
       * @brief Removes every occurrence of a string.
       * @param stringToRemove  The string to remove.
       * @param ignoreCase      When true, matching is case-insensitive.
       */
    void removeString (juce::StringRef stringToRemove, bool ignoreCase);

    /**
       * @brief Removes a contiguous range of elements.
       * @param startIndex      Index of the first element to remove, clamped
       *                        to [0, size()].
       * @param numberToRemove  Number of elements to remove, clamped to the
       *                        remaining size from startIndex.
       */
    void removeRange (int startIndex, int numberToRemove);

    /**
       * @brief Removes every element after its first occurrence, keeping the first.
       * @param ignoreCase  When true, duplicates are detected case-insensitively.
       */
    void removeDuplicates (bool ignoreCase);

    /**
       * @brief Removes every empty (or, optionally, whitespace-only) string.
       * @param removeWhitespaceStrings  When true, strings containing only
       *                                 whitespace are also removed.
       */
    void removeEmptyStrings (bool removeWhitespaceStrings);

    /**
       * @brief Moves an element from one index to another, shifting the
       *        elements in between.
       * @param currentIndex  Index of the element to move. Out-of-range
       *                      indices are ignored.
       * @param newIndex      Destination index. An out-of-range value moves
       *                      the element to the end.
       */
    void move (int currentIndex, int newIndex) noexcept;

    /** @brief Trims leading and trailing whitespace from every element in place. */
    void trim();

    //==============================================================================
    // Tokenization, join, transform
    //==============================================================================

    /**
       * @brief Appends every token of a string, split on whitespace.
       * @param stringToTokenise      The string to tokenise.
       * @param preserveQuotedStrings When true, quoted substrings are kept intact.
       * @returns                     Number of tokens appended.
       */
    int addTokens (juce::StringRef stringToTokenise, bool preserveQuotedStrings);

    /**
       * @brief Appends every token of a string, split on the given break characters.
       * @param stringToTokenise  The string to tokenise.
       * @param breakCharacters   Characters that separate tokens.
       * @param quoteCharacters   Characters that delimit quoted substrings kept intact.
       * @returns                 Number of tokens appended.
       */
    int addTokens (juce::StringRef stringToTokenise,
                   juce::StringRef breakCharacters,
                   juce::StringRef quoteCharacters);

    /**
       * @brief Appends every line of a string, split on line breaks.
       * @param stringToBreakUp  The string to split.
       * @returns                Number of lines appended.
       */
    int addLines (juce::StringRef stringToBreakUp);

    /**
       * @brief Creates a container from the tokens of a string, split on whitespace.
       * @param stringToTokenise      The string to tokenise.
       * @param preserveQuotedStrings When true, quoted substrings are kept intact.
       * @returns                     A new container holding the tokens.
       */
    [[nodiscard]] static jam::Strings
    fromTokens (juce::StringRef stringToTokenise, bool preserveQuotedStrings);

    /**
       * @brief Creates a container from the tokens of a string, split on the
       *        given break characters.
       * @param stringToTokenise  The string to tokenise.
       * @param breakCharacters   Characters that separate tokens.
       * @param quoteCharacters   Characters that delimit quoted substrings kept intact.
       * @returns                 A new container holding the tokens.
       */
    [[nodiscard]] static jam::Strings fromTokens (juce::StringRef stringToTokenise,
                                                  juce::StringRef breakCharacters,
                                                  juce::StringRef quoteCharacters);

    /**
       * @brief Creates a container from the lines of a string, split on line breaks.
       * @param stringToBreakUp  The string to split.
       * @returns                A new container holding the lines.
       */
    [[nodiscard]] static jam::Strings fromLines (juce::StringRef stringToBreakUp);

    /**
       * @brief Joins a range of elements into a single string, separated by
       *        separatorString.
       *
       * A single pass over the requested range: the exact byte count is
       * known up front from the span lengths, so the accumulator reserves
       * once and every element/separator is appended straight from its
       * arena view — no temporary juce::StringArray, no repeated
       * reallocation.
       *
       * @param separatorString    String inserted between consecutive elements.
       * @param startIndex         Index of the first element to join, clamped to [0, size()].
       * @param numberOfElements   Number of elements to join; a negative
       *                           value joins through the end.
       * @returns                  The joined string.
       */
    juce::String joinIntoString (juce::StringRef separatorString,
                                 int startIndex = 0,
                                 int numberOfElements = -1) const;

    /**
       * @brief Appends a distinguishing number to every duplicate string.
       * @param ignoreCaseWhenComparing  When true, duplicates are detected case-insensitively.
       * @param appendNumberToFirstInstance  When true, the first occurrence
       *                                     of a duplicated string is also numbered.
       * @param preNumberString   Text inserted before the number; defaults to " (" when null.
       * @param postNumberString  Text inserted after the number; defaults to ")" when null.
       */
    void appendNumbersToDuplicates (
        bool ignoreCaseWhenComparing,
        bool appendNumberToFirstInstance,
        juce::CharPointer_UTF8 preNumberString = juce::CharPointer_UTF8 (nullptr),
        juce::CharPointer_UTF8 postNumberString = juce::CharPointer_UTF8 (nullptr));

    //==============================================================================
    // Sort
    //==============================================================================

    /**
       * @brief Sorts elements lexicographically.
       * @param ignoreCase  When true, comparison is case-insensitive.
       */
    void sort (bool ignoreCase);

    /**
       * @brief Sorts elements using juce::String's natural (numeric-aware) comparison.
       *
       * Decorate-sort-undecorate: each element is materialised into a
       * juce::String exactly once (compareNatural needs it), sorted by that
       * decoration, then the spans are written back in sorted order — the
       * arena itself is never touched.
       */
    void sortNatural();

    //==============================================================================
    // Storage management
    //==============================================================================

    /**
       * @brief Reserves capacity for at least minNumElements further insertions.
       *
       * Reserves both the span table and the occurrence map — after this
       * call, neither spans growth nor an exactOccurrences rehash can fire
       * below minNumElements insertions.
       *
       * @param minNumElements  Minimum number of further insertions to guarantee.
       */
    void ensureStorageAllocated (int minNumElements);

    /**
       * @brief Reserved for reducing storage overhead. Currently a no-op.
       *
       * jam::Array exposes no shrink-to-fit primitive, only
       * ensureStorageAllocated for growth — there is nothing to delegate
       * to, and hand-rolling a manual reallocation would hand-roll
       * functionality the framework does not provide. Remains a no-op
       * until jam::Array gains a shrink-to-fit method.
       */
    void minimiseStorageOverheads() {}

    //==============================================================================
    // Size, element access, iteration
    //==============================================================================

    /** @brief Returns the number of elements. */
    int size() const
    {
        flushIntake();
        return spans.size();
    }

    /**
       * @brief Returns a copy of the element at a given index.
       * @param index  Index of the element to read.
       * @pre index is within [0, size()).
       */
    juce::String at (int index) const
    {
        flushIntake();
        jassert (index >= 0 and index < spans.size());
        return getString (spans.at (index));
    }

    /**
       * @class ConstIterator
       * @brief Read-only forward iterator over a Strings container's elements.
       *
       * Materialises each element on dereference via owner->at() — no
       * separate iteration-only view is cached, so the iterator stays valid
       * across everything at() itself tolerates.
       *
       * @pre The owning Strings container must outlive every iterator
       * derived from it — the standard iterator-invalidation contract, not
       * stored ownership.
       */
    class ConstIterator
    {
    public:
        /**
           * @brief Creates an iterator positioned at a given element.
           * @param containingStrings  The container this iterator walks.
           * @param elementIndex       The initial element index.
           */
        ConstIterator (const Strings* containingStrings, int elementIndex) noexcept;

        /** @brief Returns a copy of the element at this iterator's current position. */
        juce::String operator*() const { return owner->at (index); }

        /** @brief Advances to the next element. */
        ConstIterator& operator++() noexcept;

        /**
           * @brief Compares two iterators by position.
           * @param other  The iterator to compare against.
           * @returns      true when the two iterators point at different indices.
           */
        bool operator!= (const ConstIterator& other) const noexcept { return index != other.index; }

        /**
           * @brief Compares two iterators by position.
           * @param other  The iterator to compare against.
           * @returns      true when the two iterators point at the same index.
           */
        bool operator== (const ConstIterator& other) const noexcept { return index == other.index; }

    private:
        const Strings* owner;///< The container this iterator walks.
        int index;///< This iterator's current element index.
    };

    /** @brief Returns an iterator to the first element. */
    ConstIterator begin() const noexcept { return ConstIterator { this, 0 }; }

    /** @brief Returns an iterator past the last element. */
    ConstIterator end() const { return ConstIterator { this, size() }; }

private:
    /**
       * @brief Packed {offset, length} address into the arena.
       *
       * Transport only — unpacked via getView() at every read.
       */
    using Span = jam::Union<int32_t, int32_t>;

    /** @brief A materialised element paired with its span, used only during sortNatural(). */
    struct NaturalSortEntry
    {
        juce::String text;///< The element's materialised text, used for natural comparison.
        Span span;///< The element's arena address, written back after sorting.
    };

    /**
       * @brief The single materialisation point for a packed Span.
       *
       * Every functor and every read path unpacks offset/length through this.
       *
       * @param arena  The arena the span addresses into.
       * @param span   The packed {offset, length} to unpack.
       * @returns      A view over the span's bytes.
       */
    static std::string_view getView (const jam::Array<char>* arena, Span span);

    /**
       * @brief Returns a zero-copy view over a juce::String's UTF-8 bytes.
       *
       * Valid only while text lives — every call site keeps the String
       * alive in scope for the duration the returned view is used. juce
       * String's internal encoding is UTF-8, so the byte length is exactly
       * the distance to the terminator — the same strlen-class walk
       * std::string_view's const char* constructor already performs;
       * decoding per codepoint is redundant here.
       *
       * @param text  The string to view.
       * @returns     A view over text's UTF-8 bytes.
       */
    static std::string_view getView (const juce::String& text);

    /**
       * @brief Transparent hash functor over Span/std::string_view arena content.
       *
       * Each functor holds a pointer to the boxed arena object (the
       * unique_ptr target), never to the owning Strings — this pointer
       * stays valid across arena growth and across Strings move/swap,
       * since the box's heap address never changes for either operation.
       */
    struct TransparentHash
    {
        using is_transparent = void;

        const jam::Array<char>* arena;///< The arena every Span key addresses into.

        std::uint64_t operator() (Span key) const noexcept;
        std::uint64_t operator() (std::string_view key) const noexcept;
    };

    /** @brief Transparent byte-exact equality functor over Span/std::string_view arena content. */
    struct TransparentEqual
    {
        using is_transparent = void;

        const jam::Array<char>* arena;///< The arena every Span key addresses into.

        bool operator() (Span left, Span right) const noexcept;
        bool operator() (Span left, std::string_view right) const noexcept;
        bool operator() (std::string_view left, Span right) const noexcept;
        bool operator() (std::string_view left, std::string_view right) const noexcept;
    };

    /**
       * @brief Transparent case-folding hash functor over Span/std::string_view arena content.
       *
       * Case-folds each byte on the fly via std::tolower while accumulating
       * an FNV-1a hash — no folded copy is ever allocated. ASCII-range fold
       * only, matching this codebase's existing toAsciiLowerCase() trade-off:
       * no UTF-8-aware fold primitive is available that avoids the same
       * allocation cost, so the ASCII fast path is the deliberate choice.
       */
    struct CaseFoldHash
    {
        using is_transparent = void;

        static constexpr std::uint64_t fnvOffsetBasis {
            0xcbf29ce484222325ULL
        };///< FNV-1a 64-bit offset basis.
        static constexpr std::uint64_t fnvPrime { 0x100000001b3ULL };///< FNV-1a 64-bit prime.

        const jam::Array<char>* arena;///< The arena every Span key addresses into.

        std::uint64_t operator() (std::string_view key) const noexcept;
        std::uint64_t operator() (Span key) const noexcept;
    };

    /** @brief Transparent ASCII case-insensitive equality functor over Span/std::string_view arena content. */
    struct CaseFoldEqual
    {
        using is_transparent = void;

        const jam::Array<char>* arena;///< The arena every Span key addresses into.

        /**
           * @brief Compares two byte views for ASCII case-insensitive equality.
           * @param left   The first view.
           * @param right  The second view.
           * @returns      true when both views are equal, ignoring ASCII letter case.
           */
        static bool foldedEqual (std::string_view left, std::string_view right) noexcept;

        bool operator() (Span left, Span right) const noexcept;
        bool operator() (Span left, std::string_view right) const noexcept;
        bool operator() (std::string_view left, Span right) const noexcept;
        bool operator() (std::string_view left, std::string_view right) const noexcept;
    };

    /**
       * @brief The single materialisation point for reading this container's own spans.
       *
       * Every read path (comparisons, conversions, hashing outside the
       * functors, joinIntoString) goes through this to view arena bytes as
       * a std::string_view.
       *
       * @param span  The span to view.
       * @returns     A view over span's bytes in this container's arena.
       */
    std::string_view getView (Span span) const { return getView (arena.get(), span); }

    /**
       * @brief Materialises a span's bytes into an owning juce::String.
       * @param span  The span to materialise.
       * @returns     A copy of span's bytes as a juce::String.
       */
    juce::String getString (Span span) const;

    /**
       * @brief Replaces the bytes addressed by the span at a given index.
       *
       * Appends replacement bytes to the arena, retires the stale slot's
       * occurrence when indexed, and installs the fresh tail span in the
       * given position — the shared sequence behind set() and trim(). The
       * appended-then-popped tail span is above the watermark by
       * construction, so addToStorage's non-indexing is already correct
       * when index is at or above the watermark.
       *
       * @param index  Index of the element to replace.
       * @param text   The replacement bytes.
       */
    void replaceSpan (int index, std::string_view text);

    /**
       * @brief Finds the first index of a query string via the position maps.
       *
       * A single O(1) hash lookup against the appropriate position map —
       * zero allocation on both the exact and folded paths, since folding
       * happens inside the functors rather than on a materialised copy.
       *
       * @param query       The bytes to search for.
       * @param ignoreCase  When true, the search is case-insensitive.
       * @returns           Index of the first match, or -1 if not found.
       */
    int findPosition (std::string_view query, bool ignoreCase) const;

    /**
       * @brief Transcribes every staged handle into the arena and empties the staging tier.
       *
       * The sole transition point between staged and materialised
       * elements. Preserves logical order: intake elements are always
       * logically after every existing span.
       */
    void flushIntake() const;

    /**
       * @brief Walks the staging tier for a byte-exact match.
       *
       * The shared linear scan behind contains() and
       * addIfNotAlreadyThere()'s one-query grace path.
       *
       * @param query  The bytes to search for.
       * @returns      true when a staged element's bytes exactly match query.
       */
    bool intakeContains (std::string_view query) const;

    /**
       * @brief Appends bytes to the arena and packs the resulting span.
       *
       * The single canonical insertion primitive — every constructor and
       * mutator delegates here. Appends bytes to the arena (append-only
       * invariant — never overwrites, shifts, or frees existing bytes) and
       * packs the resulting Span. exactOccurrences indexing is deferred to
       * cacheOccurrences — this call touches neither the map nor the watermark.
       *
       * @param text  The UTF-8 bytes to append.
       */
    void addToStorage (std::string_view text) const;

    /**
       * @brief Catches exactOccurrences up to the current tail of spans.
       *
       * Drains the unindexed tail [occurrenceWatermark, spans.size()) into
       * exactOccurrences and advances the watermark to spans.size() — the
       * single site where the append-only backlog is caught up. Called
       * before every read of exactOccurrences.
       */
    void cacheOccurrences() const;

    /**
       * @brief Records one more occurrence of span in exactOccurrences.
       *
       * exactOccurrences is maintained O(1) per call — try_emplace either
       * seeds a new entry at 1 or bumps the existing count in place.
       * Heterogeneous content equality means a new span with identical
       * bytes finds the existing entry. exactOccurrences is mutable so
       * this can be driven from cacheOccurrences (const) as well as
       * mutating call sites.
       *
       * @param span  The span whose occurrence count is incremented.
       */
    void incrementExactOccurrence (Span span) const;

    /**
       * @brief Records one fewer occurrence of span in exactOccurrences.
       *
       * Mirrors the erase-at-zero contract of the prior eager design — an
       * occurrence count reaching zero means the value is no longer present.
       *
       * @param span  The span whose occurrence count is decremented.
       */
    void decrementExactOccurrence (Span span);

    /**
       * @brief Rebuilds the position maps when stale.
       *
       * Private, internal cache-validity gate — nothing outside this class
       * ever reads or sets positionsDirty; correctness is always fully and
       * deterministically re-derivable from spans, the true source of truth.
       */
    void cachePositions() const;

    /**
       * @brief Fully rebuilds exactPositions and foldedPositions from spans.
       *
       * The only place either map is written. Keys are Spans already
       * owned by spans, so no arena byte is ever copied; these keys are
       * safe by the append-only invariant, since every mutating method
       * dirties positionsDirty and the maps are only ever read again after
       * this rebuild re-establishes them. Iterating spans by ascending
       * index and using try_emplace's no-overwrite semantics gives
       * first-occurrence-wins for free.
       */
    void rebuildPositions() const;

    std::unique_ptr<jam::Array<char>> arena {
        std::make_unique<jam::Array<char>>()
    };///< Append-only backing store for every element's UTF-8 bytes.
    mutable jam::Array<Span>
        spans;///< Ordered {offset, length} addresses into arena — the single source of truth for element order and content.
    mutable juce::Array<juce::String>
        intake;///< Staged element handles awaiting transcription into the arena.
    mutable jam::HashMap<Span, int, TransparentHash, TransparentEqual> exactPositions {
        0,
        TransparentHash { arena.get() },
        TransparentEqual { arena.get() }
    };///< Span → first-index map for exact lookups; lazily rebuilt when positionsDirty.
    mutable jam::HashMap<Span, int, CaseFoldHash, CaseFoldEqual> foldedPositions {
        0,
        CaseFoldHash { arena.get() },
        CaseFoldEqual { arena.get() }
    };///< Span → first-index map for case-insensitive lookups; lazily rebuilt when positionsDirty.
    mutable jam::HashMap<Span, int, TransparentHash, TransparentEqual> exactOccurrences {
        0,
        TransparentHash { arena.get() },
        TransparentEqual { arena.get() }
    };///< Span → occurrence count, caught up to occurrenceWatermark on read.
    mutable bool positionsDirty {
        true
    };///< True when exactPositions/foldedPositions no longer reflect spans.
    mutable bool intakeQueried {
        false
    };///< True once a query has answered against the staging tier without flushing it.
    mutable int occurrenceWatermark {
        0
    };///< Index below which exactOccurrences already reflects spans.
};

}// namespace jam
