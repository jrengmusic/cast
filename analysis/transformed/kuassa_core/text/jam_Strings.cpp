namespace jam
{
/*____________________________________________________________________________*/

Strings::Strings (const Strings& other)
{
    arena->insertArray (0, other.arena->data(), other.arena->size());
    spans.insertArray (0, other.spans.data(), other.spans.size());
    intake = other.intake;

    positionsDirty = true;
}

Strings::Strings (Strings&& other) noexcept
    : arena (std::move (other.arena))
    , spans (std::move (other.spans))
    , intake (std::move (other.intake))
    , exactPositions (std::move (other.exactPositions))
    , foldedPositions (std::move (other.foldedPositions))
    , exactOccurrences (std::move (other.exactOccurrences))
    , positionsDirty (other.positionsDirty)
    , intakeQueried (other.intakeQueried)
    , occurrenceWatermark (other.occurrenceWatermark)
{
    other.arena = std::make_unique<jam::Array<char>>();
    other.exactPositions = decltype (exactPositions) { 0,
                                                       TransparentHash { other.arena.get() },
                                                       TransparentEqual { other.arena.get() } };
    other.foldedPositions = decltype (foldedPositions) { 0,
                                                         CaseFoldHash { other.arena.get() },
                                                         CaseFoldEqual { other.arena.get() } };
    other.exactOccurrences = decltype (exactOccurrences) {
        0, TransparentHash { other.arena.get() }, TransparentEqual { other.arena.get() }
    };
    other.positionsDirty = true;
    other.intakeQueried = false;
    other.occurrenceWatermark = 0;
}

Strings::Strings (const std::initializer_list<const char*>& stringList)
{
    std::size_t totalBytes { 0 };

    for (const auto* value : stringList)
        totalBytes += std::string_view { value }.size();

    ensureStorageAllocated (static_cast<int> (stringList.size()));
    arena->ensureStorageAllocated (static_cast<int> (totalBytes));

    for (const auto* value : stringList)
        addToStorage (std::string_view { value });
}

Strings::Strings (const juce::Array<juce::String>& stringArray)
{
    std::size_t totalBytes { 0 };

    for (const auto& value : stringArray)
        totalBytes += getView (value).size();

    ensureStorageAllocated (stringArray.size());
    arena->ensureStorageAllocated (static_cast<int> (totalBytes));

    for (const auto& value : stringArray)
        addToStorage (getView (value));
}

Strings::Strings (const juce::StringArray& stringArray)
{
    std::size_t totalBytes { 0 };

    for (const auto& value : stringArray)
        totalBytes += getView (value).size();

    ensureStorageAllocated (stringArray.size());
    arena->ensureStorageAllocated (static_cast<int> (totalBytes));

    for (const auto& value : stringArray)
        addToStorage (getView (value));
}

Strings::Strings (const juce::String* stringsToAdd, int numberOfStrings)
{
    jassert (numberOfStrings >= 0);
    jassert (numberOfStrings == 0 or stringsToAdd != nullptr);

    std::size_t totalBytes { 0 };

    for (int index = 0; index < numberOfStrings; ++index)
        totalBytes += getView (stringsToAdd[index]).size();

    ensureStorageAllocated (numberOfStrings);
    arena->ensureStorageAllocated (static_cast<int> (totalBytes));

    for (int index = 0; index < numberOfStrings; ++index)
    {
        const auto& value { stringsToAdd[index] };
        addToStorage (getView (value));
    }
}

Strings::Strings (const char* const* strings, int numberOfStrings)
{
    jassert (numberOfStrings >= 0);
    jassert (numberOfStrings == 0 or strings != nullptr);

    std::size_t totalBytes { 0 };

    for (int index = 0; index < numberOfStrings; ++index)
        totalBytes +=
            strings[index] != nullptr ? std::string_view { strings[index] }.size() : 0;

    ensureStorageAllocated (numberOfStrings);
    arena->ensureStorageAllocated (static_cast<int> (totalBytes));

    for (int index = 0; index < numberOfStrings; ++index)
        addToStorage (strings[index] != nullptr ? std::string_view { strings[index] }
                                                : std::string_view {});
}

Strings::Strings (const char* const* strings)
{
    jassert (strings != nullptr);

    for (auto* cursor = strings; *cursor != nullptr; ++cursor)
        addToStorage (std::string_view { *cursor });
}

Strings::Strings (const wchar_t* const* strings, int numberOfStrings)
{
    jassert (numberOfStrings >= 0);
    jassert (numberOfStrings == 0 or strings != nullptr);

    for (int index = 0; index < numberOfStrings; ++index)
    {
        const auto value { juce::String (strings[index] != nullptr ? strings[index] : L"") };
        addToStorage (getView (value));
    }
}

Strings::Strings (const wchar_t* const* strings)
{
    jassert (strings != nullptr);

    for (auto* cursor = strings; *cursor != nullptr; ++cursor)
    {
        const auto value { juce::String (*cursor) };
        addToStorage (getView (value));
    }
}

Strings& Strings::operator= (const Strings& other)
{
    if (this != &other)
    {
        Strings copy (other);
        swapWith (copy);
    }

    return *this;
}

Strings& Strings::operator= (Strings&& other) noexcept
{
    if (this != &other)
    {
        Strings moved (std::move (other));
        swapWith (moved);
    }

    return *this;
}

void Strings::swapWith (Strings& other) noexcept
{
    std::swap (arena, other.arena);
    std::swap (spans, other.spans);
    intake.swapWith (other.intake);
    std::swap (exactPositions, other.exactPositions);
    std::swap (foldedPositions, other.foldedPositions);
    std::swap (exactOccurrences, other.exactOccurrences);
    std::swap (positionsDirty, other.positionsDirty);
    std::swap (intakeQueried, other.intakeQueried);
    std::swap (occurrenceWatermark, other.occurrenceWatermark);
}

bool Strings::operator== (const Strings& other) const noexcept
{
    flushIntake();
    other.flushIntake();

    return spans.size() == other.spans.size()
           and std::equal (spans.begin(),
                           spans.end(),
                           other.spans.begin(),
                           [this, &other] (Span left, Span right)
                           {
                               return getView (left) == other.getView (right);
                           });
}

void Strings::add (std::string_view textToAdd)
{
    if (not intake.isEmpty())
        flushIntake();

    addToStorage (textToAdd);
}

void Strings::add (juce::StringRef stringToAdd)
{
    add (std::string_view { stringToAdd.text.getAddress() });
}

void Strings::insert (int index, const juce::String& stringToAdd)
{
    flushIntake();
    addToStorage (getView (stringToAdd));

    const auto tailIndex { spans.size() - 1 };
    const auto insertIndex { (index >= 0 and index < tailIndex) ? index : tailIndex };

    if (insertIndex != tailIndex)
    {
        const auto insertedSpan { spans.at (tailIndex) };
        spans.remove (tailIndex);
        spans.insert (insertIndex, insertedSpan);

        if (insertIndex < occurrenceWatermark)
        {
            incrementExactOccurrence (insertedSpan);
            ++occurrenceWatermark;
        }
    }
}

bool Strings::addIfNotAlreadyThere (const juce::String& stringToAdd, bool ignoreCase)
{
    const auto query { getView (stringToAdd) };

    if (ignoreCase)
    {
        flushIntake();
        cachePositions();
        if (foldedPositions.contains (query))
            return false;

        addToStorage (query);
        return true;
    }

    if (intakeQueried)
        flushIntake();

    intakeQueried = not intake.isEmpty();
    cacheOccurrences();

    if (exactOccurrences.contains (query) or intakeContains (query))
        return false;

    if (intake.isEmpty())
    {
        addToStorage (query);
        incrementExactOccurrence (spans.at (spans.size() - 1));
        ++occurrenceWatermark;
        return true;
    }

    intake.add (stringToAdd);
    return true;
}

void Strings::set (int index, const juce::String& newString)
{
    jassert (index >= 0);
    flushIntake();

    if (index >= 0)
    {
        if (index < spans.size())
            replaceSpan (index, getView (newString));
        else
            addToStorage (getView (newString));
    }
}

void Strings::addArray (const Strings& other, int startIndex, int numElementsToAdd)
{
    flushIntake();
    other.flushIntake();

    jassert (startIndex >= 0);
    const auto clampedStart { std::max (0, startIndex) };

    const auto endIndex { numElementsToAdd < 0
                              ? other.spans.size()
                              : std::min (other.spans.size(), clampedStart + numElementsToAdd) };

    for (int index = clampedStart; index < endIndex; ++index)
        addToStorage (other.getView (other.spans.at (index)));
}

void Strings::mergeArray (const Strings& other, bool ignoreCase)
{
    other.flushIntake();

    for (const auto span : other.spans)
        addIfNotAlreadyThere (other.getString (span), ignoreCase);
}

bool Strings::contains (juce::StringRef stringToLookFor, bool ignoreCase) const
{
    const auto query { std::string_view { stringToLookFor.text.getAddress() } };

    if (ignoreCase or intakeQueried)
        flushIntake();

    if (not intake.isEmpty())
    {
        intakeQueried = true;
        cacheOccurrences();

        return exactOccurrences.contains (query) or intakeContains (query);
    }

    if (ignoreCase)
    {
        cachePositions();
        return foldedPositions.contains (query);
    }

    cacheOccurrences();
    return exactOccurrences.contains (query);
}

int Strings::indexOf (juce::StringRef stringToLookFor, bool ignoreCase, int startIndex) const
{
    flushIntake();

    if (startIndex <= 0)
        return findPosition (
            std::string_view { stringToLookFor.text.getAddress() }, ignoreCase);

    const auto candidateString { juce::String (stringToLookFor) };
    const auto candidateView { getView (candidateString) };

    for (int index = startIndex; index < spans.size(); ++index)
    {
        const auto elementView { getView (spans.at (index)) };
        const auto matches { ignoreCase
                                 ? CaseFoldEqual::foldedEqual (elementView, candidateView)
                                 : elementView == candidateView };

        if (matches)
            return index;
    }

    return -1;
}

void Strings::clear()
{
    arena->clear();
    spans.clear();
    intake.clear();
    exactPositions.clear();
    foldedPositions.clear();
    exactOccurrences.clear();
    positionsDirty = false;
    intakeQueried = false;
    occurrenceWatermark = 0;
}

void Strings::remove (int index)
{
    flushIntake();

    if (index >= 0 and index < spans.size())
    {
        if (index < occurrenceWatermark)
        {
            decrementExactOccurrence (spans.at (index));
            --occurrenceWatermark;
        }

        spans.remove (index);
        positionsDirty = true;
    }
}

void Strings::removeString (juce::StringRef stringToRemove, bool ignoreCase)
{
    flushIntake();

    for (auto index = indexOf (stringToRemove, ignoreCase, 0); index >= 0;
         index = indexOf (stringToRemove, ignoreCase, 0))
        remove (index);
}

void Strings::removeRange (int startIndex, int numberToRemove)
{
    flushIntake();

    const auto clampedStart { juce::jlimit (0, spans.size(), startIndex) };
    const auto clampedCount { juce::jlimit (0, spans.size() - clampedStart, numberToRemove) };

    if (clampedCount > 0)
    {
        const auto indexedEnd { std::min (clampedStart + clampedCount, occurrenceWatermark) };

        for (int index = clampedStart; index < indexedEnd; ++index)
            decrementExactOccurrence (spans.at (index));

        const auto indexedCount { std::max (0, indexedEnd - clampedStart) };
        occurrenceWatermark -= indexedCount;

        spans.removeRange (clampedStart, clampedCount);
        positionsDirty = true;
    }
}

void Strings::removeDuplicates (bool ignoreCase)
{
    flushIntake();
    cacheOccurrences();

    jam::Array<Span> kept;
    jam::HashSet<Span, TransparentHash, TransparentEqual> seenExact {
        0, TransparentHash { arena.get() }, TransparentEqual { arena.get() }
    };
    jam::HashSet<Span, CaseFoldHash, CaseFoldEqual> seenFolded {
        0, CaseFoldHash { arena.get() }, CaseFoldEqual { arena.get() }
    };

    const auto keepFirstOccurrences = [this, &kept] (auto& seen)
    {
        for (const auto span : spans)
        {
            auto [entry, inserted] = seen.insert (span);

            if (inserted)
                kept.add (span);
            else
                decrementExactOccurrence (span);
        }
    };

    if (ignoreCase)
        keepFirstOccurrences (seenFolded);
    else
        keepFirstOccurrences (seenExact);

    spans = std::move (kept);
    occurrenceWatermark = spans.size();
    positionsDirty = true;
}

void Strings::removeEmptyStrings (bool removeWhitespaceStrings)
{
    flushIntake();

    for (int index = spans.size(); --index >= 0;)
    {
        const auto value { getString (spans.at (index)) };
        const auto isEmptyMatch {
            removeWhitespaceStrings ? not value.containsNonWhitespaceChars() : value.isEmpty()
        };

        if (isEmptyMatch)
            remove (index);
    }
}

void Strings::move (int currentIndex, int newIndex) noexcept
{
    flushIntake();

    if (occurrenceWatermark > 0)
        cacheOccurrences();

    if (currentIndex >= 0 and currentIndex < spans.size())
    {
        const auto target {
            (newIndex < 0 or newIndex >= spans.size()) ? spans.size() - 1 : newIndex
        };

        if (target != currentIndex)
        {
            const auto span { spans.at (currentIndex) };
            spans.remove (currentIndex);
            spans.insert (target, span);
        }

        positionsDirty = true;
    }
}

void Strings::trim()
{
    flushIntake();

    for (int index = 0; index < spans.size(); ++index)
    {
        const auto oldView { getView (spans.at (index)) };
        const auto trimmedString { getString (spans.at (index)).trim() };
        const auto trimmedView { getView (trimmedString) };

        if (trimmedView != oldView)
            replaceSpan (index, trimmedView);
    }

    positionsDirty = true;
}

int Strings::addTokens (juce::StringRef stringToTokenise, bool preserveQuotedStrings)
{
    auto tokens { juce::StringArray::fromTokens (stringToTokenise, preserveQuotedStrings) };
    const auto tokenCount { tokens.size() };

    for (auto& token : tokens)
        intake.add (std::move (token));

    return tokenCount;
}

int Strings::addTokens (juce::StringRef stringToTokenise,
                        juce::StringRef breakCharacters,
                        juce::StringRef quoteCharacters)
{
    auto tokens {
        juce::StringArray::fromTokens (stringToTokenise, breakCharacters, quoteCharacters)
    };
    const auto tokenCount { tokens.size() };

    for (auto& token : tokens)
        intake.add (std::move (token));

    return tokenCount;
}

int Strings::addLines (juce::StringRef stringToBreakUp)
{
    auto lines { juce::StringArray::fromLines (stringToBreakUp) };
    const auto lineCount { lines.size() };

    for (auto& line : lines)
        intake.add (std::move (line));

    return lineCount;
}

jam::Strings Strings::fromTokens (juce::StringRef stringToTokenise, bool preserveQuotedStrings)
{
    jam::Strings result;
    result.addTokens (stringToTokenise, preserveQuotedStrings);
    return result;
}

jam::Strings Strings::fromTokens (juce::StringRef stringToTokenise,
                                  juce::StringRef breakCharacters,
                                  juce::StringRef quoteCharacters)
{
    jam::Strings result;
    result.addTokens (stringToTokenise, breakCharacters, quoteCharacters);
    return result;
}

jam::Strings Strings::fromLines (juce::StringRef stringToBreakUp)
{
    jam::Strings result;
    result.addLines (stringToBreakUp);
    return result;
}

juce::String Strings::joinIntoString (juce::StringRef separatorString,
                                      int startIndex,
                                      int numberOfElements) const
{
    flushIntake();

    const auto clampedStart { juce::jlimit (0, spans.size(), startIndex) };
    const auto clampedCount {
        numberOfElements < 0 ? spans.size() - clampedStart
                             : juce::jlimit (0, spans.size() - clampedStart, numberOfElements)
    };

    const auto separatorView { std::string_view { separatorString.text.getAddress() } };
    std::size_t totalBytes { 0 };

    for (int index = clampedStart; index < clampedStart + clampedCount; ++index)
        totalBytes += getView (spans.at (index)).size();

    if (clampedCount > 1)
        totalBytes += separatorView.size() * static_cast<std::size_t> (clampedCount - 1);

    std::string joined;
    joined.reserve (totalBytes);

    for (int index = clampedStart; index < clampedStart + clampedCount; ++index)
    {
        if (index > clampedStart)
            joined.append (separatorView);

        joined.append (getView (spans.at (index)));
    }

    return juce::String::fromUTF8 (joined.data(), static_cast<int> (joined.size()));
}

void Strings::appendNumbersToDuplicates (
    bool ignoreCaseWhenComparing,
    bool appendNumberToFirstInstance,
    juce::CharPointer_UTF8 preNumberString,
    juce::CharPointer_UTF8 postNumberString)
{
    flushIntake();

    const auto prefix { preNumberString.getAddress() != nullptr ? juce::String (preNumberString)
                                                                : juce::String (" (") };
    const auto suffix { postNumberString.getAddress() != nullptr
                            ? juce::String (postNumberString)
                            : juce::String (")") };

    for (int index = 0; index < spans.size() - 1; ++index)
    {
        const auto original { getString (spans.at (index)) };
        auto nextIndex { indexOf (original, ignoreCaseWhenComparing, index + 1) };

        if (nextIndex >= 0)
        {
            int number { 0 };

            if (appendNumberToFirstInstance)
                set (index, original + prefix + juce::String (++number) + suffix);
            else
                ++number;

            while (nextIndex >= 0)
            {
                const auto nextValue { getString (spans.at (nextIndex)) };
                set (nextIndex, nextValue + prefix + juce::String (++number) + suffix);
                nextIndex = indexOf (original, ignoreCaseWhenComparing, nextIndex + 1);
            }
        }
    }
}

void Strings::sort (bool ignoreCase)
{
    flushIntake();

    if (occurrenceWatermark > 0)
        cacheOccurrences();

    if (ignoreCase)
    {
        std::sort (spans.begin(),
                   spans.end(),
                   [this] (Span left, Span right)
                   {
                       const auto leftView { getView (left) };
                       const auto rightView { getView (right) };

                       return std::lexicographical_compare (
                           leftView.begin(),
                           leftView.end(),
                           rightView.begin(),
                           rightView.end(),
                           [] (unsigned char leftCharacter, unsigned char rightCharacter)
                           {
                               return std::tolower (leftCharacter)
                                      < std::tolower (rightCharacter);
                           });
                   });
    }
    else
    {
        std::sort (spans.begin(),
                   spans.end(),
                   [this] (Span left, Span right)
                   {
                       return getView (left) < getView (right);
                   });
    }

    positionsDirty = true;
}

void Strings::sortNatural()
{
    flushIntake();

    if (occurrenceWatermark > 0)
        cacheOccurrences();

    jam::Array<NaturalSortEntry> entries;
    entries.ensureStorageAllocated (spans.size());

    for (const auto span : spans)
        entries.add (NaturalSortEntry { getString (span), span });

    std::sort (entries.begin(),
               entries.end(),
               [] (const NaturalSortEntry& left, const NaturalSortEntry& right)
               {
                   return left.text.compareNatural (right.text) < 0;
               });

    for (int index = 0; index < entries.size(); ++index)
        spans.set (index, entries.at (index).span);

    positionsDirty = true;
}

void Strings::ensureStorageAllocated (int minNumElements)
{
    spans.ensureStorageAllocated (minNumElements);
    intake.ensureStorageAllocated (minNumElements);
    exactOccurrences.reserve (static_cast<std::size_t> (minNumElements));
}

void Strings::replaceSpan (int index, std::string_view text)
{
    const auto oldSpan { spans.at (index) };
    addToStorage (text);

    const auto newSpan { spans.at (spans.size() - 1) };
    spans.remove (spans.size() - 1);

    if (index < occurrenceWatermark)
    {
        decrementExactOccurrence (oldSpan);
        incrementExactOccurrence (newSpan);
    }

    spans.set (index, newSpan);
}

int Strings::findPosition (std::string_view query, bool ignoreCase) const
{
    cachePositions();

    if (ignoreCase)
    {
        if (auto found = foldedPositions.find (query); found != foldedPositions.end())
        {
            const auto& [foundSpan, foundIndex] = *found;
            return foundIndex;
        }

        return -1;
    }

    if (auto found = exactPositions.find (query); found != exactPositions.end())
    {
        const auto& [foundSpan, foundIndex] = *found;
        return foundIndex;
    }

    return -1;
}

void Strings::flushIntake() const
{
    for (const auto& stagedString : intake)
        addToStorage (getView (stagedString));

    intake.clearQuick();
    intakeQueried = false;
}

bool Strings::intakeContains (std::string_view query) const
{
    for (const auto& stagedString : intake)
        if (getView (stagedString) == query)
            return true;

    return false;
}

void Strings::cacheOccurrences() const
{
    jassert (occurrenceWatermark >= 0 and occurrenceWatermark <= spans.size());

    if (occurrenceWatermark < spans.size())
    {
        for (int index = occurrenceWatermark; index < spans.size(); ++index)
            incrementExactOccurrence (spans.at (index));

        occurrenceWatermark = spans.size();
    }
}

void Strings::incrementExactOccurrence (Span span) const
{
    auto [occurrenceEntry, inserted] = exactOccurrences.try_emplace (span, 1);

    if (not inserted)
    {
        auto& [occurrenceSpan, occurrenceCount] = *occurrenceEntry;
        ++occurrenceCount;
    }
}

void Strings::decrementExactOccurrence (Span span)
{
    if (auto found = exactOccurrences.find (span); found != exactOccurrences.end())
    {
        auto& [foundSpan, foundCount] = *found;
        --foundCount;

        if (foundCount == 0)
            exactOccurrences.erase (span);
    }
}

void Strings::cachePositions() const
{
    if (positionsDirty)
        rebuildPositions();
}

void Strings::rebuildPositions() const
{
    exactPositions.clear();
    foldedPositions.clear();

    for (int index = 0; index < spans.size(); ++index)
    {
        const auto span { spans.at (index) };
        exactPositions.try_emplace (span, index);
        foldedPositions.try_emplace (span, index);
    }

    positionsDirty = false;
}

std::string_view Strings::getView (const jam::Array<char>* arena, Span span)
{
    const auto [offset, length] = span;
    return std::string_view { arena->data() + offset, static_cast<std::size_t> (length) };
}

std::string_view Strings::getView (const juce::String& text)
{
    return std::string_view { text.toRawUTF8() };
}

std::uint64_t Strings::TransparentHash::operator() (Span key) const noexcept
{
    return jam::Hash<std::string_view> {}(getView (arena, key));
}

std::uint64_t Strings::TransparentHash::operator() (std::string_view key) const noexcept
{
    return jam::Hash<std::string_view> {}(key);
}

bool Strings::TransparentEqual::operator() (Span left, Span right) const noexcept
{
    return getView (arena, left) == getView (arena, right);
}

bool Strings::TransparentEqual::operator() (Span left, std::string_view right) const noexcept
{
    return getView (arena, left) == right;
}

bool Strings::TransparentEqual::operator() (std::string_view left, Span right) const noexcept
{
    return left == getView (arena, right);
}

bool Strings::TransparentEqual::operator() (std::string_view left,
                                            std::string_view right) const noexcept
{
    return left == right;
}

std::uint64_t Strings::CaseFoldHash::operator() (std::string_view key) const noexcept
{
    auto hash { fnvOffsetBasis };

    for (const auto character : key)
    {
        hash ^= static_cast<unsigned char> (
            std::tolower (static_cast<unsigned char> (character)));
        hash *= fnvPrime;
    }

    return hash;
}

std::uint64_t Strings::CaseFoldHash::operator() (Span key) const noexcept
{
    return operator() (getView (arena, key));
}

bool Strings::CaseFoldEqual::foldedEqual (std::string_view left,
                                          std::string_view right) noexcept
{
    if (left.size() != right.size())
        return false;

    return std::equal (left.begin(),
                       left.end(),
                       right.begin(),
                       [] (unsigned char leftCharacter, unsigned char rightCharacter)
                       {
                           return std::tolower (leftCharacter)
                                  == std::tolower (rightCharacter);
                       });
}

bool Strings::CaseFoldEqual::operator() (Span left, Span right) const noexcept
{
    return foldedEqual (getView (arena, left), getView (arena, right));
}

bool Strings::CaseFoldEqual::operator() (Span left, std::string_view right) const noexcept
{
    return foldedEqual (getView (arena, left), right);
}

bool Strings::CaseFoldEqual::operator() (std::string_view left, Span right) const noexcept
{
    return foldedEqual (left, getView (arena, right));
}

bool Strings::CaseFoldEqual::operator() (std::string_view left,
                                         std::string_view right) const noexcept
{
    return foldedEqual (left, right);
}

juce::String Strings::getString (Span span) const
{
    const auto view { getView (span) };
    return juce::String::fromUTF8 (view.data(), static_cast<int> (view.size()));
}

void Strings::addToStorage (std::string_view text) const
{
    const auto offset { arena->size() };
    const auto length { static_cast<int32_t> (text.size()) };

    jassert (text.size() <= static_cast<std::size_t> (std::numeric_limits<int32_t>::max()));
    jassert (static_cast<std::int64_t> (offset) + static_cast<std::int64_t> (length)
             <= std::numeric_limits<int32_t>::max());

    arena->insertArray (arena->size(), text.data(), static_cast<int> (text.size()));

    const auto span { Span::pack (static_cast<int32_t> (offset), length) };
    spans.add (span);
    positionsDirty = true;
}

Strings::ConstIterator::ConstIterator (const Strings* containingStrings,
                                       int elementIndex) noexcept
    : owner (containingStrings)
    , index (elementIndex)
{
}

Strings::ConstIterator& Strings::ConstIterator::operator++() noexcept
{
    ++index;
    return *this;
}

}// namespace jam
