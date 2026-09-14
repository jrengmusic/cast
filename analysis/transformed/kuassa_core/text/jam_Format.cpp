namespace jam
{
/*____________________________________________________________________________*/

juce::String Format::getFft() noexcept { return text::English::fft; }
juce::String Format::getImportLicense() noexcept { return text::English::importLicensePrompt; }
juce::String Format::getSelectDirectory() noexcept { return text::English::selectDirectory; }
juce::String Format::getSelectFile() noexcept { return text::English::selectFile; }
juce::String Format::getDefaultForNewInstance() noexcept
{
    return text::English::defaultForNewInstance;
}

juce::String Format::getAlertNewerVersionPreset (juce::StringRef projectName) noexcept
{
    juce::String result;
    result << text::English::alertNewerVersionPresetPrefix << Chars::space << projectName
           << text::English::alertNewerVersionPresetSuffix << Id::downloadsUrl;
    return result;
}

juce::String Format::getPresetAlreadyExists (juce::StringRef presetFileName) noexcept
{
    juce::String result;
    result << text::English::presetAlreadyExistsPrefix << Chars::space << presetFileName
           << Chars::space << text::English::fileAlreadyExistsSuffix;
    return result;
}

juce::String Format::getFileAlreadyExists (juce::StringRef fileName) noexcept
{
    juce::String result;
    result << text::English::fileAlreadyExistsPrefix << Chars::space << fileName << Chars::space
           << text::English::fileAlreadyExistsSuffix;
    return result;
}

juce::String Format::getAskReplace() noexcept { return text::English::askReplace; }

juce::String Format::getAlertUserManualNotFound() noexcept
{
    return text::English::alertUserManualNotFound;
}

juce::String Format::getAlertIRNameTooLong() noexcept { return text::English::alertIRNameTooLong; }

juce::String Format::getChooseShorterNameOrLocation() noexcept
{
    return text::English::chooseShorterNameOrLocation;
}

juce::String Format::getAlertIRNotRecognized() noexcept
{
    return text::English::alertIRNotRecognized;
}

juce::String Format::getChooseAnotherFile() noexcept { return text::English::chooseAnotherFile; }

juce::String Format::getPleaseTryAgain() noexcept { return text::English::pleaseTryAgain; }

juce::String Format::getNoCigar() noexcept { return text::English::noCigar; }

juce::String Format::getAlertAuthorizationSuccesful() noexcept
{
    return text::English::alertAuthorizationSuccesful;
}

juce::String Format::getRockNRoll (juce::StringRef productName) noexcept
{
    juce::String result;
    result << text::English::rockNRollPrefix << Chars::space << productName
           << text::English::rockNRollSuffix;
    return result;
}

juce::String Format::getAuthorizationFailed() noexcept
{
    return text::English::authorizationFailed;
}

juce::String Format::getTryAgainWithCorrectLicense (juce::StringRef fileName) noexcept
{
    juce::String result;
    result << text::English::tryAgainWithCorrectLicensePrefix << fileName
           << text::English::tryAgainWithCorrectLicenseSuffix << Chars::space << Id::supportEmail;
    return result;
}

juce::String Format::getProductInDemo (juce::StringRef productName) noexcept
{
    juce::String result;
    result << productName << text::English::productInDemoSuffix;
    return result;
}

juce::String Format::getAlertNoise() noexcept { return text::English::alertNoise; }

//==============================================================================
static constexpr int preallocateSlackBytes { 4 };

static juce::String removeExclamation (const juce::String& text) noexcept
{
    return text.removeCharacters (juce::String::charToString (Chars::exclamation));
}

static juce::String prependUnderscoreIfLeadingDigit (const juce::String& text) noexcept
{
    if (text.isNotEmpty() and Chars::isNumeric (text[0]))
        return Chars::underscore + text;

    return text;
}

static juce::String foldDiacritic (juce::juce_wchar character) noexcept
{
    for (const auto& [asciiLetter, diacriticVariants] : map::diacritics)
        if (diacriticVariants.containsChar (character))
            return asciiLetter;

    return juce::String::charToString (character);
}

static juce::String foldDiacritics (const juce::String& text) noexcept
{
    juce::String folded;
    folded.preallocateBytes (text.length() + preallocateSlackBytes);

    for (auto c : text)
    {
        if (c >= Chars::nonAsciiStart)
            folded << foldDiacritic (c);
        else
            folded << c;
    }

    return folded;
}

static juce::String collapseIllegalCharacters (const juce::String& text) noexcept
{
    juce::String cleaned;
    cleaned.preallocateBytes (text.length() + preallocateSlackBytes);

    bool lastWasUnderscore { false };
    for (auto c : text)
    {
        if (juce::String (Chars::special).containsChar (c) or c >= Chars::nonAsciiStart)
        {
            if (not lastWasUnderscore)
            {
                cleaned << Chars::underscore;
                lastWasUnderscore = true;
            }
        }
        else
        {
            cleaned << c;
            lastWasUnderscore = false;
        }
    }

    return cleaned;
}

juce::String Format::toValidID (juce::StringRef textToFormat, bool shouldBeUpperCase) noexcept
{
    const std::string source { textToFormat };
    std::string escaped;
    escape (source.begin(), source.end(), std::back_inserter (escaped));

    juce::String result { removeExclamation (juce::String (escaped)) };
    result = prependUnderscoreIfLeadingDigit (result);
    result = collapseIllegalCharacters (foldDiacritics (result));

    return shouldBeUpperCase ? result.toUpperCase() : result;
}

juce::String Format::toFileExtension (juce::StringRef ext) noexcept
{
    juce::String formatted { ext };

    if (formatted.contains (Id::asteriskDot))
        return formatted;

    juce::String tmp;
    tmp << Id::asteriskDot << formatted;
    return tmp;
}

juce::String Format::toFileName (juce::StringRef name, juce::StringRef extension) noexcept
{
    juce::String tmp;
    tmp << name << Chars::dot << juce::String (extension).removeCharacters (Id::asteriskDot);
    return tmp;
}

juce::String Format::toFileName (const juce::String& path) noexcept
{
    return juce::File::createFileWithoutCheckingPath (path).getFileName();
}

juce::String
Format::toFullPathFileName (juce::StringRef parentDirectory, juce::StringRef filename) noexcept
{
    juce::String tmp;
    tmp << parentDirectory << juce::File::getSeparatorString() << filename;
    return tmp;
}

juce::String Format::toPathName (juce::StringRef name) noexcept
{
    return toValidID (juce::String (name).trim())
        .replace (juce::String::charToString (Chars::underscore),
                  juce::String::charToString (Chars::dash))
        .toLowerCase();
}

juce::String Format::upperFirstChar (juce::StringRef textToFormat) noexcept
{
    juce::String formatted { textToFormat };
    return formatted.replaceSection (0, 1, formatted.substring (0, 1).toUpperCase());
}

juce::String Format::lowerFirstChar (juce::StringRef textToFormat) noexcept
{
    juce::String formatted { textToFormat };
    return formatted.replaceSection (0, 1, formatted.substring (0, 1).toLowerCase());
}

juce::String Format::toTitleCase (juce::StringRef textToFormat) noexcept
{
    return forEachWord (textToFormat,
                        [&] (juce::StringRef word, int)
                        {
                            return normalizeWord (word);
                        });
}

juce::String Format::toPascalCase (juce::StringRef textToFormat) noexcept
{
    return toValidID (forEachWord (
        textToFormat,
        [&] (juce::StringRef word, int)
        {
            return normalizeWord (word);
        },
        juce::String()));
}

juce::String Format::toScreamingSnakeCase (juce::StringRef textToFormat) noexcept
{
    return forEachWord (
        textToFormat,
        [&] (juce::StringRef word, int)
        {
            return juce::String (word).toUpperCase();
        },
        juce::String::charToString (Chars::underscore));
}

juce::String Format::toSnakeCase (juce::StringRef textToFormat) noexcept
{
    return forEachWord (
        textToFormat,
        [&] (juce::StringRef word, int)
        {
            return isAbbreviation (word) ? juce::String (word) : juce::String (word).toLowerCase();
        },
        juce::String::charToString (Chars::underscore));
}

juce::String Format::toCamelCase (juce::StringRef textToFormat) noexcept
{
    return toValidID (forEachWord (
        textToFormat,
        [&] (juce::StringRef word, int index)
        {
            if (index == 0)
                return isAbbreviation (word) ? juce::String (word)
                                             : juce::String (word).toLowerCase();

            return normalizeWord (word);
        },
        juce::String()));
}

juce::String Format::toUnit (juce::StringRef textToFormat) noexcept
{
    return juce::String (textToFormat)
        .replace (Id::dB.toString(), Id::dB.toString(), true)
        .replace (Id::hz.toString(), Id::hertz.toString(), true);
}

juce::String Format::correctKeywordCase (const juce::String& input) noexcept
{
    static const juce::StringArray keywords { "hf", "mf", "lmf", "hmf", "lf" };
    juce::String result { input };

    for (const auto& keyword : keywords)
    {
        int index { 0 };

        while ((index = result.indexOfIgnoreCase (index, keyword)) != -1)
        {
            const bool isStartOk { (index == 0) or not juce::CharacterFunctions::isLetterOrDigit (result[index - 1]) };
            const bool isEndOk { (index + keyword.length() >= result.length()) or not juce::CharacterFunctions::isLetterOrDigit (result[index + keyword.length()]) };

            if (isStartOk and isEndOk)
                result = result.replaceSection (index, keyword.length(), keyword.toUpperCase());

            index += keyword.length();
        }
    }

    return result;
}

juce::String Format::toNoteName (juce::StringRef textToFormat) noexcept
{
    if (not isNumber (textToFormat)
        and not juce::String (textToFormat)
                    .containsIgnoreCase (juce::String::charToString (Chars::lowerK)))
    {
        juce::String formatted { upperFirstChar (textToFormat) };

        int index { -1 };
        int accidental { 0 };

        switch (formatted.length())
        {
            case 2:
                break;
            case 3:
            case 4:
            {
                if (const auto suffix { formatted.substring (2) };
                    std::all_of (suffix.begin(),
                                 suffix.end(),
                                 [] (auto c)
                                 {
                                     return Chars::isNumeric (c);
                                 }))
                {
                    formatted =
                        formatted.replaceSection (1, 1, formatted.substring (1, 2).toLowerCase());
                    juce::String octave { formatted.substring (2) };

                    if (isNoteNameFlat (formatted))
                        accidental = -1;
                    else if (isNoteNameSharp (formatted))
                        accidental = 1;

                    if (accidental)
                    {
                        juce::String letter { formatted.substring (0, 1) };
                        index = 12
                                + ((accidental < 0
                                        ? (map::Flats::getInstance()->contains (letter)
                                               ? map::Flats::getInstance()->get (letter)
                                               : -1)
                                        : (map::Sharps::getInstance()->contains (letter)
                                               ? map::Sharps::getInstance()->get (letter)
                                               : -1))
                                   + accidental);
                        formatted = (accidental < 0 ? map::Flats::getInstance()->get (index % 12)
                                                    : map::Sharps::getInstance()->get (index % 12))
                                    + octave;
                    }
                }

                if (not accidental)
                    return juce::String {};
            }
            break;

            default:
                return juce::String {};
        }

        return formatted;
    }

    return juce::String {};
}

juce::String Format::appendWithSpace (juce::StringRef text, juce::StringRef textToAppend) noexcept
{
    juce::String result;
    result << text << Chars::space << textToAppend;
    return result;
}

juce::String Format::appendNewline (juce::StringRef text, juce::StringRef textToAppend) noexcept
{
    juce::String result;
    result << text << Chars::newline << textToAppend;
    return result;
}

juce::String Format::prependNewLine (juce::StringRef text, juce::StringRef textToPrepend) noexcept
{
    juce::String result;
    result << textToPrepend << Chars::newline << text;
    return result;
}

juce::String
Format::appendWithUnderscore (juce::StringRef text, juce::StringRef textToAppend) noexcept
{
    juce::String result;
    result << text << Chars::underscore << textToAppend;
    return result;
}

juce::String Format::appendWithDash (juce::StringRef text, juce::StringRef textToAppend) noexcept
{
    juce::String result;
    result << text << Chars::dash << textToAppend;
    return result;
}

juce::String
Format::prependWithUnderscore (juce::StringRef text, juce::StringRef textToPrepend) noexcept
{
    juce::String result;
    result << textToPrepend << Chars::underscore << text;
    return result;
}

juce::String Format::withEnclosure (juce::StringRef text, juce::juce_wchar open) noexcept
{
    jassert (Chars::enclosure.contains (open));

    juce::String result;
    result << open << text << Chars::enclosure.get (open);
    return result;
}

juce::String Format::withoutEnclosure (juce::StringRef text, juce::juce_wchar open) noexcept
{
    jassert (Chars::enclosure.contains (open));

    const auto close { Chars::enclosure.get (open) };

    jassert (juce::String (text).startsWithChar (open) and juce::String (text).endsWithChar (close));

    return upTo (from (text, juce::String::charToString (open), false),
                 juce::String::charToString (close), false);
}

juce::String Format::removeUnderscore (juce::StringRef textWithUnderscore) noexcept
{
    return juce::String (textWithUnderscore)
        .replace (juce::String::charToString (Chars::underscore),
                  juce::String::charToString (Chars::space));
}

juce::String Format::join (juce::StringRef textToJoin) noexcept
{
    return juce::String (textToJoin)
        .removeCharacters (juce::String::charToString (Chars::space))
        .trim();
}

juce::String Format::getPreColon (juce::StringRef textWithColon) noexcept
{
    return upTo (textWithColon, juce::String::charToString (Chars::colon), false).trim();
}

juce::String Format::getPostColon (juce::StringRef textWithColon) noexcept
{
    return from (textWithColon, juce::String::charToString (Chars::colon), false).trim();
}

juce::String Format::getYearInRoman() noexcept
{
    std::string roman;

    auto year { juce::String (__DATE__).fromLastOccurrenceOf (" ", false, true).getIntValue() };

    for (const auto& [rom, num] : map::romanNumerals)
    {
        while (year - num >= 0)
        {
            roman += rom.toRawUTF8();
            year -= num;
        }
    }

    return juce::String::fromUTF8 (roman.data(), static_cast<int> (roman.size()));
}

bool Format::hasPlaceholder (juce::StringRef code,
                             juce::StringRef placeholder,
                             juce::StringRef delimiter) noexcept
{
    const std::string_view whole { code.text.getAddress() };
    const std::string_view delim { delimiter.text.getAddress() };
    const std::string_view place { placeholder.text.getAddress() };

    std::string needle;
    needle.append (delim);
    needle.append (place);
    needle.append (delim);

    return whole.find (needle) != std::string_view::npos;
}

juce::String Format::replaceholder (juce::StringRef wholeString,
                                    juce::StringRef placeholder,
                                    juce::StringRef stringReplacement,
                                    juce::StringRef delimiter) noexcept
{
    const std::string_view whole { wholeString.text.getAddress() };
    const std::string_view delim { delimiter.text.getAddress() };
    const std::string_view place { placeholder.text.getAddress() };
    const std::string_view repl { stringReplacement.text.getAddress() };

    std::string needle;
    needle.append (delim);
    needle.append (place);
    needle.append (delim);

    const auto first { whole.find (needle) };

    if (first == std::string_view::npos)
        return wholeString;

    std::string result;
    result.reserve (whole.size());

    size_t pos { 0 };
    size_t found { first };

    do
    {
        result.append (whole, pos, found - pos);
        result.append (repl);
        pos = found + needle.size();
    } while ((found = whole.find (needle, pos)) != std::string_view::npos);

    result.append (whole, pos);

    return juce::String::fromUTF8 (result.data(), static_cast<int> (result.size()));
}

juce::String Format::from (juce::StringRef wholeString,
                           juce::StringRef substring,
                           bool includeSubstring) noexcept
{
    const std::string_view whole { wholeString.text.getAddress() };
    const std::string_view part { substring.text.getAddress() };

    const auto found { whole.find (part) };

    if (found == std::string_view::npos)
        return {};

    const auto start { includeSubstring ? found : found + part.size() };

    return juce::String::fromUTF8 (whole.data() + start, static_cast<int> (whole.size() - start));
}

juce::String Format::upTo (juce::StringRef wholeString,
                           juce::StringRef substring,
                           bool includeSubstring) noexcept
{
    const std::string_view whole { wholeString.text.getAddress() };
    const std::string_view part { substring.text.getAddress() };

    const auto found { whole.find (part) };

    if (found == std::string_view::npos)
        return wholeString;

    const auto end { includeSubstring ? found + part.size() : found };

    return juce::String::fromUTF8 (whole.data(), static_cast<int> (end));
}

std::string_view Format::trim (std::string_view text) noexcept
{
    static constexpr char whitespaceChars[] { static_cast<char> (Chars::space),
                                              static_cast<char> (Chars::tab),
                                              static_cast<char> (Chars::carriageReturn),
                                              static_cast<char> (Chars::newline) };
    static constexpr std::string_view whitespace { whitespaceChars, sizeof (whitespaceChars) };

    const auto first { text.find_first_not_of (whitespace) };

    if (first == std::string_view::npos)
        return {};

    const auto last { text.find_last_not_of (whitespace) };

    return text.substr (first, last - first + 1);
}

static const juce::String escapedNewline { "\\n" };

juce::String
Format::toTrademark (juce::StringRef textToBeFormatted, juce::StringRef productName) noexcept
{
    const auto delimiter { juce::String::charToString (Chars::at) };

#if JUCE_MODULE_AVAILABLE_juce_audio_processors
    juce::String formatted { replaceholder (textToBeFormatted, Id::productName, productName, delimiter) };
#else
    juce::String formatted { replaceholder (
        textToBeFormatted, Id::productName, ProjectInfo::projectName, delimiter) };
#endif

    formatted = replaceholder (formatted, Id::companyName, ProjectInfo::companyName, delimiter);
    formatted = replaceholder (formatted, Id::legalCompanyName, ProjectInfo::legalCompanyName, delimiter);

    return formatted.replace (escapedNewline, juce::String::charToString (Chars::newline));
}

juce::String Format::getBuildYear() noexcept
{
    return juce::String (__DATE__).fromLastOccurrenceOf (
        juce::String::charToString (Chars::space), false, true);
}

juce::String Format::abbreviate (juce::StringRef textToBeFormatted) noexcept
{
    juce::String formatted { removeUnderscore (textToBeFormatted) };

    jam::Strings words;
    words.addTokens (formatted, false);

    if (words.size() > 1)
    {
        std::string initials;

        for (const auto& w : words)
            initials += w.substring (0, 1).toRawUTF8();

        formatted = juce::String::fromUTF8 (initials.data(), static_cast<int> (initials.size()));
    }

    return formatted.toUpperCase();
}

juce::String Format::toSVGid (juce::StringRef preColon, juce::StringRef postColon) noexcept
{
    auto post { juce::String (postColon).replace (
        juce::String::charToString (Chars::space), juce::String::charToString (Chars::dash)) };
    juce::String formatted { preColon + Chars::colon + Chars::dash + post };

    return formatted;
}

juce::String Format::toKebabCase (juce::StringRef textToFormat) noexcept
{
    return forEachWord (
        textToFormat,
        [&] (juce::StringRef word, int)
        {
            return isAbbreviation (word) ? juce::String (word) : juce::String (word).toLowerCase();
        },
        juce::String::charToString (Chars::dash));
}

bool Format::isUsingStandardChars (std::string stringToCheck) noexcept
{
    for (size_t i { 0 }; i < stringToCheck.length(); ++i)
    {
        if (static_cast<unsigned char> (stringToCheck.at (i)) >= Chars::nonAsciiStart)
            return false;
    }

    return true;
}

juce::String Format::createProductPageURL (const juce::String& productName) noexcept
{
    juce::String website { Id::productsUrl };

    jam::Strings words;
    words.addTokens (productName.toLowerCase(), false);

    for (int i { 0 }; i < words.size(); ++i)
        words.set (i, words.at (i).removeCharacters (Chars::special));

    juce::String url;
    url << website << words.joinIntoString (juce::String::charToString (Chars::dash), 0, -1);
    return url;
}

juce::String Format::toEmail (juce::StringRef mail, juce::StringRef domain) noexcept
{
    return { mail + juce::String::charToString (Chars::at) + domain };
}

juce::String Format::toDefaultPresetVersion (juce::StringRef versionString) noexcept
{
    return Id::versionPrefix.toString() + versionString;
}

juce::String Format::fromBoolean (bool trueOrFalse) noexcept
{
    return trueOrFalse ? Id::tokenTrue.toString() : Id::tokenFalse.toString();
}

juce::String Format::dashToWhitespace (juce::StringRef text) noexcept
{
    return juce::String (text).replace (
        juce::String::charToString (Chars::dash), juce::String::charToString (Chars::space));
}

//==============================================================================
bool Format::isNumber (juce::StringRef text) noexcept
{
    const juce::String value { text };
    return std::all_of (value.begin(),
                        value.end(),
                        [] (auto c)
                        {
                            return Chars::isNumeric (c);
                        });
}

int Format::getMIDINoteNumberFromName (juce::StringRef noteName) noexcept
{
    int number { -1 };

    if (juce::String name { toNoteName (noteName) }; name.isNotEmpty())
    {
        int noteLength { 0 };
        for (auto c : name)
        {
            if (Chars::isNumeric (c))
                break;

            ++noteLength;
        }

        juce::String note { name.substring (0, noteLength) };

        int octave { name.substring (note.length()).getIntValue() + 1 };

        number =
            (isNoteNameFlat (name)
                 ? (map::Flats::getInstance()->contains (note) ? map::Flats::getInstance()->get (note) : -1)
                 : (map::Sharps::getInstance()->contains (note) ? map::Sharps::getInstance()->get (note) : -1))
            + (12 * octave);
    }

    return number;
}

bool Format::isValidNoteName (juce::StringRef noteName) noexcept
{
    return toNoteName (noteName).isNotEmpty();
}

bool Format::isNoteNameSharp (juce::StringRef noteName) noexcept
{
    if (juce::String name { noteName }; name.length() >= 3)
        return name.substring (1, 2).compare (juce::String::charToString (Chars::hash)) == 0;

    return false;
}

bool Format::isNoteNameFlat (juce::StringRef noteName) noexcept
{
    if (juce::String name { noteName }; name.length() >= 3)
        return name.substring (1, 2).compare (Id::b.toString()) == 0;

    return false;
}

bool Format::isKilo (juce::StringRef textNumber) noexcept
{
    if (juce::String number { textNumber };
        number.endsWithIgnoreCase (juce::String::charToString (Chars::lowerK)))
    {
        const juce::String value { number.upToFirstOccurrenceOf (
            juce::String::charToString (Chars::lowerK), false, true) };
        return std::any_of (value.begin(),
                            value.end(),
                            [] (auto c)
                            {
                                return Chars::isNumeric (c);
                            });
    }

    return false;
}

bool Format::isVersionOld (const juce::String& versionToCheck,
                           const juce::String& versionToCompare) noexcept
{
    if (versionToCheck.isEmpty())
        return true;
    if (versionToCompare.isEmpty())
        return false;

    jam::Strings toCheckParts, toCompareParts;

    toCheckParts.addTokens (versionToCheck, juce::String::charToString (Chars::dot), "");
    toCompareParts.addTokens (versionToCompare, juce::String::charToString (Chars::dot), "");

    for (int i { 0 }; i < juce::jmax (toCheckParts.size(), toCompareParts.size()); ++i)
    {
        int partToCheck { i < toCheckParts.size() ? toCheckParts.at (i).getIntValue() : 0 };
        int partToCompare { i < toCompareParts.size() ? toCompareParts.at (i).getIntValue() : 0 };

        if (partToCheck < partToCompare)
            return true;
        if (partToCheck > partToCompare)
            return false;
    }

    return false;
}

juce::String Format::fromBounds (const juce::var& v) noexcept
{
    const auto [x, y, w, h] = jam::Bounds { v };
    return juce::String (x) + juce::String::charToString (Chars::comma) + juce::String (y)
           + juce::String::charToString (Chars::space) + juce::String (w)
           + juce::String::charToString (Chars::lowerX) + juce::String (h);
}

//==============================================================================
static constexpr int byteHexDigits { 2 };
static constexpr int codepointHexDigits { 4 };
static constexpr size_t hexEscapeCharsPerByte { 4 };
static constexpr size_t literalDelimiterCount { 2 };
static constexpr size_t utf8BytesPerCodepoint { 4 };

static juce::juce_wchar parseCodepoint (const juce::String& input) noexcept
{
    const auto isHex { input.startsWithIgnoreCase (Id::hexPrefix) };
    return isHex ? static_cast<juce::juce_wchar> (
                       input.substring (Id::hexPrefix.toString().length()).getHexValue32())
                 : input.getCharPointer().getAndAdvance();
}

static juce::String encodeCodepointAsEscapedUtf8 (juce::juce_wchar codepoint) noexcept
{
    const auto encoded { juce::String::charToString (codepoint) };
    const auto* raw { encoded.getCharPointer().getAddress() };

    std::string result;
    result.reserve (encoded.getNumBytesAsUTF8() * hexEscapeCharsPerByte);

    for (int i { 0 }; raw[i] != 0; ++i)
    {
        const auto byte { static_cast<unsigned char> (raw[i]) };
        const auto escaped { Id::hexEscapePrefix
                             + juce::String::toHexString (byte).paddedLeft (
                                 Chars::zero, byteHexDigits) };
        result += escaped.toRawUTF8();
    }

    return juce::String::fromUTF8 (result.data(), static_cast<int> (result.size()));
}

static std::string encodeCodepointAsUtf8 (juce::juce_wchar codepoint) noexcept
{
    const juce::juce_wchar source[] { codepoint, 0 };
    const auto byteCount { juce::CharPointer_UTF8::getBytesRequiredFor (codepoint) };
    char encoded[utf8BytesPerCodepoint + 1] {};

    juce::CharPointer_UTF8 (encoded).writeWithDestByteLimit (
        juce::CharPointer_UTF32 (source), byteCount + 1);

    return std::string (encoded, byteCount);
}

static juce::String encodeUPlusToken (const juce::String& token) noexcept
{
    jassert (token.startsWith (Id::codepointPrefix));

    const auto cp { static_cast<juce::juce_wchar> (
        token.substring (Id::codepointPrefix.toString().length()).getHexValue32()) };
    return encodeCodepointAsEscapedUtf8 (cp);
}

static std::pair<juce::juce_wchar, size_t> decodeUPlusToken (std::string_view input,
                                                              size_t position) noexcept
{
    juce::uint32 codepoint { 0 };
    auto digitEnd { position };

    while (digitEnd < input.size()
           and digitEnd - position < static_cast<size_t> (codepointHexDigits))
    {
        const auto value { juce::CharacterFunctions::getHexDigitValue (
            static_cast<juce::juce_wchar> (input.at (digitEnd))) };

        if (value < 0)
            break;

        codepoint = (codepoint << 4) | static_cast<juce::uint32> (value);
        ++digitEnd;
    }

    return { static_cast<juce::juce_wchar> (codepoint), digitEnd };
}

juce::String Format::toLiteral (const juce::String& input) noexcept
{
    const auto* raw { input.getCharPointer().getAddress() };

    std::string result;
    result.reserve (input.getNumBytesAsUTF8() * hexEscapeCharsPerByte + literalDelimiterCount);

    result += static_cast<char> (Chars::doubleQuote);

    for (int i { 0 }; raw[i] != 0; ++i)
    {
        const auto byte { static_cast<unsigned char> (raw[i]) };

        if (byte == Chars::backslash)
        {
            const auto next { static_cast<unsigned char> (raw[i + 1]) };
            const auto isEscapeCharacter { jam::Map::containsValue (Chars::escape, next) };

            if (isEscapeCharacter)
            {
                result += static_cast<char> (byte);
                result += static_cast<char> (next);
                ++i;
            }
            else
            {
                result += static_cast<char> (byte);
                result += static_cast<char> (byte);

                if (next == Chars::backslash)
                    ++i;
            }
        }
        else if (byte == Chars::doubleQuote)
        {
            result += static_cast<char> (Chars::backslash);
            result += static_cast<char> (Chars::doubleQuote);
        }
        else if (auto entry { Chars::escape.find (byte) }; entry != Chars::escape.end())
        {
            const auto& [controlByte, escapeCharacter] { *entry };
            result += static_cast<char> (Chars::backslash);
            result += static_cast<char> (escapeCharacter);
        }
        else if (byte >= Chars::nonAsciiStart or byte < Chars::space)
        {
            const auto escaped { Id::hexEscapePrefix
                                 + juce::String::toHexString (byte).paddedLeft (
                                     Chars::zero, byteHexDigits) };
            result += escaped.toRawUTF8();
        }
        else
            result += static_cast<char> (byte);
    }

    result += static_cast<char> (Chars::enclosure.get (Chars::doubleQuote));

    return juce::String::fromUTF8 (result.data(), static_cast<int> (result.size()));
}

juce::String Format::toUTF8 (const juce::String& input) noexcept
{
    const auto tokens { jam::Strings::fromTokens (input, false) };

    std::string result;

    for (const auto& token : tokens)
        result += encodeUPlusToken (token).toRawUTF8();

    return juce::String::fromUTF8 (result.data(), static_cast<int> (result.size()));
}

std::string Format::fromUTF8 (std::string_view input) noexcept
{
    static const juce::String& codepointPrefix { Id::codepointPrefix.toString() };
    const std::string_view prefix { codepointPrefix.getCharPointer().getAddress() };
    std::string result;
    result.reserve (input.size());

    size_t position { 0 };
    auto found { input.find (prefix) };

    while (found != std::string_view::npos)
    {
        result.append (input, position, found - position);
        const auto tokenStart { found + prefix.size() };
        const auto [codepoint, digitEnd] { decodeUPlusToken (input, tokenStart) };

        if (digitEnd > tokenStart)
        {
            result.append (encodeCodepointAsUtf8 (codepoint));
            position = digitEnd;
        }
        else
        {
            result.append (input, found, prefix.size());
            position = tokenStart;
        }

        found = input.find (prefix, position);
    }

    result.append (input, position);
    return result;
}

juce::String Format::toHex (const juce::String& input) noexcept
{
    const auto cp { parseCodepoint (input) };
    return Id::hexPrefix + juce::String::toHexString (cp);
}

juce::String Format::toCodepoint (const juce::String& input) noexcept
{
    const auto cp { parseCodepoint (input) };
    return Id::codepointPrefix
           + juce::String::toHexString (cp)
                 .paddedLeft (Chars::zero, codepointHexDigits)
                 .toUpperCase();
}

juce::String Format::toSymbol (const juce::String& input) noexcept
{
    const auto count { (input.length() - input.replace (Id::doubleColon, juce::String()).length())
                       / Id::doubleColon.toString().length() };
    return count == 1 ? Id::juceNamespace + input : input;
}

juce::String Format::toUnicode (const juce::String& input) noexcept
{
    auto hex { input };

    if (hex.startsWithIgnoreCase (Id::hexPrefix))
        hex = hex.substring (Id::hexPrefix.toString().length());

    return Id::codepointPrefix + hex.paddedLeft (Chars::zero, codepointHexDigits).toUpperCase();
}

juce::String Format::fromId (const juce::String& input) noexcept
{
    return Id::idNamespace + toCamelCase (input) + Id::toStringSuffix;
}

juce::String Format::fromMap (const juce::String& input) noexcept
{
    return Id::mapNamespace + input;
}

juce::String Format::fromIdentifier (const juce::String& input) noexcept
{
    return Id::idNamespace + toCamelCase (input);
}

juce::String Format::fromCodepoint (const juce::String& input) noexcept
{
    return Id::fromUtF8Prefix + toUTF8 (input) + Id::fromUtF8Suffix;
}

/**_____________________________END OF NAMESPACE______________________________*/
} /** namespace jam */
