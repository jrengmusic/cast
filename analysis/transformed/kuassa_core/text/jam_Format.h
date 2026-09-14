/**
 * @file jam_Format.h
 * @brief Localised UI strings and string/note/version formatting helpers.
 */

#pragma once
namespace jam
{
/*____________________________________________________________________________*/

struct Format
{
    /*____________________________________________________________________________*/
    // Localised UI strings — each returns a fixed, user-facing message.

    /** @brief Localised label for an FFT-related control. */
    static juce::String getFft() noexcept;
    /** @brief Localised prompt for importing a license. */
    static juce::String getImportLicense() noexcept;
    /** @brief Localised prompt to select a directory. */
    static juce::String getSelectDirectory() noexcept;
    /** @brief Localised prompt to select a file. */
    static juce::String getSelectFile() noexcept;
    /** @brief Localised default name for a new instance. */
    static juce::String getDefaultForNewInstance() noexcept;
    /**
     * @brief Localised alert that a newer preset version was found.
     * @param projectName Name of the consuming project.
     */
    static juce::String getAlertNewerVersionPreset (juce::StringRef projectName) noexcept;
    /**
     * @brief Localised alert that a preset file already exists.
     * @param presetFileName Name of the conflicting preset file.
     */
    static juce::String getPresetAlreadyExists (juce::StringRef presetFileName) noexcept;
    /**
     * @brief Localised alert that a file already exists.
     * @param fileName Name of the conflicting file.
     */
    static juce::String getFileAlreadyExists (juce::StringRef fileName) noexcept;
    /** @brief Localised prompt asking whether to replace an existing file. */
    static juce::String getAskReplace() noexcept;
    /** @brief Localised alert that the user manual could not be found. */
    static juce::String getAlertUserManualNotFound() noexcept;
    /** @brief Localised alert that an impulse-response name is too long. */
    static juce::String getAlertIRNameTooLong() noexcept;
    /** @brief Localised prompt to choose a shorter name or location. */
    static juce::String getChooseShorterNameOrLocation() noexcept;
    /** @brief Localised alert that an impulse-response file was not recognised. */
    static juce::String getAlertIRNotRecognized() noexcept;
    /** @brief Localised prompt to choose another file. */
    static juce::String getChooseAnotherFile() noexcept;
    /** @brief Localised prompt to try again. */
    static juce::String getPleaseTryAgain() noexcept;
    /** @brief Localised dismissive failure message. */
    static juce::String getNoCigar() noexcept;
    /** @brief Localised confirmation that authorization succeeded. */
    static juce::String getAlertAuthorizationSuccesful() noexcept;
    /**
     * @brief Localised success message naming the authorised product.
     * @param productName Name of the authorised product.
     */
    static juce::String getRockNRoll (juce::StringRef productName) noexcept;
    /** @brief Localised alert that authorization failed. */
    static juce::String getAuthorizationFailed() noexcept;
    /**
     * @brief Localised prompt to retry with the correct license file.
     * @param fileName Name of the license file expected.
     */
    static juce::String getTryAgainWithCorrectLicense (juce::StringRef fileName) noexcept;
    /**
     * @brief Localised notice that the named product is running in demo mode.
     * @param productName Name of the product.
     */
    static juce::String getProductInDemo (juce::StringRef productName) noexcept;
    /** @brief Localised generic noise/error alert. */
    static juce::String getAlertNoise() noexcept;

    //==============================================================================
private:
    /**
     * @brief Splits text on spaces and lower→upper camel transitions, then applies
     *        function to each word (with its zero-based index) and joins the results.
     */
    template<typename Function>
    static juce::String forEachWord (juce::StringRef text,
                                     const Function& function,
                                     juce::StringRef separator = " ") noexcept
    {
        juce::String source { text };
        jam::Strings words;
        juce::String current;

        for (auto c : source)
        {
            if (juce::CharacterFunctions::isWhitespace (c))
            {
                if (current.isNotEmpty())
                {
                    words.add (current);
                    current.clear();
                }

                continue;
            }

            const auto lastChar { current.getLastCharacter() };
            const auto lastIsLower { juce::CharacterFunctions::isLowerCase (lastChar) };
            const auto lastIsDigit { juce::CharacterFunctions::isDigit (lastChar) };
            const auto lastIsUpper { juce::CharacterFunctions::isUpperCase (lastChar) };
            const auto cIsUpper { juce::CharacterFunctions::isUpperCase (c) };
            const auto cIsLower { juce::CharacterFunctions::isLowerCase (c) };

            const auto isUpperRun { lastIsUpper and cIsLower and current.length() > 1
                                    and juce::CharacterFunctions::isUpperCase (
                                        current[current.length() - 2]) };

            if (current.isNotEmpty()
                and (((lastIsLower or lastIsDigit) and cIsUpper) or isUpperRun))
            {
                if (isUpperRun)
                {
                    words.add (current.substring (0, current.length() - 1));
                    current = current.substring (current.length() - 1);
                }
                else
                {
                    words.add (current);
                    current.clear();
                }
            }

            current += juce::String::charToString (c);
        }

        if (current.isNotEmpty())
            words.add (current);

        jam::Strings transformed;
        int index { 0 };

        for (const auto& word : words)
        {
            transformed.add (function (word, index));
            ++index;
        }

        return transformed.joinIntoString (separator, 0, -1);
    }

    /** @brief Checks whether a word is an all-uppercase abbreviation (digits neutral). */
    static bool isAbbreviation (juce::StringRef word) noexcept
    {
        juce::String w { word };
        return w.compare (w.toUpperCase()) == 0;
    }

    /** @brief Normalizes a word: abbreviations pass through intact, others lowercase-then-upperFirst. */
    static juce::String normalizeWord (juce::StringRef word) noexcept
    {
        if (isAbbreviation (word))
            return juce::String { word };

        return upperFirstChar (juce::String (word).toLowerCase());
    }

    /**
     * @brief Replaces XML-reserved characters with their entity forms.
     *
     * Scans the range from begin to end one character at a time; a character
     * present in the escape table is replaced by its XML entity, and every
     * other character passes through unchanged. \c out is expected to be an
     * unbounded output iterator.
     *
     * @tparam InIter Input iterator type over the characters to escape.
     * @tparam OutIter Output iterator type receiving the escaped result.
     * @param begin Start of the range to scan.
     * @param end End of the range to scan.
     * @param out Output iterator that receives each emitted character.
     * @return The output iterator position after the last emitted character.
     */
    template<typename InIter, typename OutIter>
    static OutIter escape (InIter begin, InIter end, OutIter out)
    {
        for (; begin != end; ++begin)
        {
            if (auto entry { map::xmlEscapes.find (*begin) }; entry != map::xmlEscapes.end())
            {
                const auto& [character, replacement] { *entry };

                for (auto replacementChar : replacement)
                    *out++ = replacementChar;
            }
            else
            {
                *out++ = *begin;
            }
        }

        return out;
    }

    //==============================================================================

public:
    /**
     * @brief Converts a string into a valid identifier.
     *
     * Removes invalid characters, replaces special characters with underscores,
     * and optionally uppercases the result.
     *
     * @param textToFormat Input text to convert.
     * @param shouldBeUpperCase If true, result is uppercased.
     * @return A valid identifier string.
     */
    static juce::String
    toValidID (juce::StringRef textToFormat, bool shouldBeUpperCase = false) noexcept;

    /**
     * @brief Ensures a file extension string is prefixed with "*.".
     *
     * @param ext File extension text.
     * @return Extension string in "*.ext" format.
     */
    static juce::String toFileExtension (juce::StringRef ext) noexcept;

    /**
     * @brief Builds a filename with extension.
     *
     * @param name Base filename.
     * @param extension Extension to append.
     * @return Combined filename with extension.
     */
    static juce::String toFileName (juce::StringRef name, juce::StringRef extension) noexcept;

    /**
     * @brief Extracts the filename component from a full path.
     *
     * @param path Full path string.
     * @return Filename portion of path.
     */
    static juce::String toFileName (const juce::String& path) noexcept;

    /**
     * @brief Builds a full path string from directory and filename.
     *
     * @param parentDirectory Directory path.
     * @param filename File name.
     * @return Full path string.
     */
    static juce::String
    toFullPathFileName (juce::StringRef parentDirectory, juce::StringRef filename) noexcept;

    /**
     * @brief Converts a name into a path‑safe string.
     *
     * @param name Input name.
     * @return Path‑safe string.
     */
    static juce::String toPathName (juce::StringRef name) noexcept;

    /**
     * @brief Uppercases the first character of a string.
     *
     * @param textToFormat Input text.
     * @return String with first character uppercased.
     */
    static juce::String upperFirstChar (juce::StringRef textToFormat) noexcept;

    /**
     * @brief Lowercases the first character of a string.
     *
     * @param textToFormat Input text.
     * @return String with first character lowercased.
     */
    static juce::String lowerFirstChar (juce::StringRef textToFormat) noexcept;

    /**
     * @brief Converts text to title case (each word capitalized).
     *
     * Words split on spaces and lower→upper camel transitions. A word that is
     * entirely uppercase (an abbreviation) passes through intact.
     *
     * @param textToFormat Input text.
     * @return Title‑cased string.
     */
    static juce::String toTitleCase (juce::StringRef textToFormat) noexcept;

    /**
     * @brief Converts text into PascalCase.
     *
     * Words split on spaces and lower→upper camel transitions, normalized and
     * joined with no separator. Abbreviation words pass through intact.
     *
     * @param textToFormat Input text.
     * @return PascalCase string.
     */
    static juce::String toPascalCase (juce::StringRef textToFormat) noexcept;

    /**
     * @brief Converts text into SCREAMING_SNAKE_CASE.
     *
     * @param textToFormat Input text.
     * @return Uppercase underscore-joined string.
     */
    static juce::String toScreamingSnakeCase (juce::StringRef textToFormat) noexcept;

    /**
     * @brief Converts text into snake_case.
     *
     * @param textToFormat Input text.
     * @return Lowercase underscore-joined string.
     */
    static juce::String toSnakeCase (juce::StringRef textToFormat) noexcept;

    /**
     * @brief Converts text into camelCase.
     *
     * Like @ref toPascalCase, but the first word is entirely lowercased,
     * unless it is an abbreviation, which passes through intact.
     *
     * @param textToFormat Input text.
     * @return camelCase string.
     */
    static juce::String toCamelCase (juce::StringRef textToFormat) noexcept;

    /**
     * @brief Converts text into kebab-case.
     *
     * @param textToFormat Input text.
     * @return Lowercase hyphen-joined string.
     */
    static juce::String toKebabCase (juce::StringRef textToFormat) noexcept;

    /**
     * @brief Formats a unit string (e.g. "db" → "dB", "hz" → "Hz").
     *
     * @param textToFormat Input text.
     * @return Formatted unit string.
     */
    static juce::String toUnit (juce::StringRef textToFormat) noexcept;

    /**
     * @brief Uppercases whole-word occurrences of the frequency-band
     *        abbreviations "hf", "mf", "lmf", "hmf", "lf" within a string.
     *
     * Each abbreviation is matched case-insensitively, on a word boundary
     * (not preceded or followed by a letter or digit), and replaced by its
     * uppercase form; every other character passes through unchanged.
     *
     * @param input Text to correct.
     * @return input with each matched abbreviation uppercased.
     */
    static juce::String correctKeywordCase (const juce::String& input) noexcept;

    /**
     * @brief Converts text into a normalized musical note name.
     *
     * @param textToFormat Input text.
     * @return Note name string or empty if invalid.
     */
    static juce::String toNoteName (juce::StringRef textToFormat) noexcept;

    /**
     * @brief Appends text with a space separator.
     */
    static juce::String
    appendWithSpace (juce::StringRef text, juce::StringRef textToAppend) noexcept;

    /**
     * @brief Appends text with a new line separator.
     */
    static juce::String appendNewline (juce::StringRef text, juce::StringRef textToAppend) noexcept;

    /**
     * @brief Prepends text with a new line separator.
     */
    static juce::String
    prependNewLine (juce::StringRef text, juce::StringRef textToPrepend) noexcept;

    /**
     * @brief Appends text with an underscore separator.
     */
    static juce::String
    appendWithUnderscore (juce::StringRef text, juce::StringRef textToAppend) noexcept;

    /**
     * @brief Appends text with a dash separator.
     */
    static juce::String
    appendWithDash (juce::StringRef text, juce::StringRef textToAppend) noexcept;

    /**
     * @brief Prepends text with an underscore separator.
     */
    static juce::String
    prependWithUnderscore (juce::StringRef text, juce::StringRef textToPrepend) noexcept;

    /**
     * @brief Wraps text with an opening character and its Chars::enclosure partner.
     *
     * @param text Text to wrap.
     * @param open Opening character.
     * @return text preceded by open and followed by open's Chars::enclosure partner.
     * @warning Asserts that open is a key in Chars::enclosure.
     */
    static juce::String withEnclosure (juce::StringRef text, juce::juce_wchar open) noexcept;

    /**
     * @brief Extracts the text between an opening character and its Chars::enclosure partner.
     *
     * @param text Text to unwrap.
     * @param open Opening character.
     * @return The portion of text between open and its Chars::enclosure partner.
     * @warning Asserts that open is a key in Chars::enclosure, and that text
     *          starts with open and ends with open's Chars::enclosure partner.
     */
    static juce::String withoutEnclosure (juce::StringRef text, juce::juce_wchar open) noexcept;

    /**
     * @brief Removes underscores from a string.
     */
    static juce::String removeUnderscore (juce::StringRef textWithUnderscore) noexcept;

    /**
     * @brief Joins a string's words into one token by removing whitespace.
     */
    static juce::String join (juce::StringRef textToJoin) noexcept;

    /**
     * @brief Extracts substring before a colon.
     */
    static juce::String getPreColon (juce::StringRef textWithColon) noexcept;

    /**
     * @brief Extracts substring after a colon.
     */
    static juce::String getPostColon (juce::StringRef textWithColon) noexcept;

    /**
     * @brief Returns the current year in Roman numerals.
     */
    static juce::String getYearInRoman() noexcept;

    /**
     * @brief Replaces a "\@placeholder\@"-delimited token with another string.
     *
     * @param wholeString        Text to search, containing a "\@placeholder\@" token.
     * @param placeholder        Token name, without the surrounding "@" delimiters.
     * @param stringReplacement  Text to substitute for the delimited token.
     * @param delimiter          Token delimiter, repeated on both sides of placeholder.
     */
    static juce::String replaceholder (juce::StringRef wholeString,
                                       juce::StringRef placeholder,
                                       juce::StringRef stringReplacement,
                                       juce::StringRef delimiter = Id::tripleColon) noexcept;

    /**
     * @brief Checks whether a "\@placeholder\@"-delimited token is present in a string.
     *
     * @param code        Text to search.
     * @param placeholder Token name, without the surrounding "@" delimiters.
     * @param delimiter   Token delimiter, repeated on both sides of placeholder.
     * @return True if the delimited placeholder token is found in code.
     */
    static bool hasPlaceholder (juce::StringRef code,
                                juce::StringRef placeholder,
                                juce::StringRef delimiter = Id::tripleColon) noexcept;

    /**
     * @brief Extracts the portion of a string from a substring's position to the end.
     *
     * @param wholeString      Text to search.
     * @param substring        Substring marking the start of the extracted portion.
     * @param includeSubstring If true, the result starts at substring; otherwise it
     *                         starts immediately after it.
     * @return Text from substring to the end of wholeString, or an empty string if
     *         substring is not found.
     */
    static juce::String from (juce::StringRef wholeString,
                              juce::StringRef substring,
                              bool includeSubstring) noexcept;

    /**
     * @brief Extracts the portion of a string from the start up to a substring's position.
     *
     * @param wholeString      Text to search.
     * @param substring        Substring marking the end of the extracted portion.
     * @param includeSubstring If true, the result ends immediately after substring;
     *                         otherwise it ends immediately before it.
     * @return Text from the start of wholeString up to substring, or wholeString
     *         unchanged if substring is not found.
     */
    static juce::String upTo (juce::StringRef wholeString,
                              juce::StringRef substring,
                              bool includeSubstring) noexcept;

    /**
     * @brief Trims leading and trailing whitespace from a string view.
     *
     * @param text Text to trim.
     * @return A view over text with leading and trailing whitespace removed, or an
     *         empty view if text is entirely whitespace.
     */
    static std::string_view trim (std::string_view text) noexcept;

    /**
     * @brief Parses text as a number of NumberType.
     * @tparam NumberType   The numeric type to parse into (default int).
     * @param text  The text to parse.
     * @return The parsed value.
     * @warning Asserts that text holds a valid NumberType (std::from_chars succeeds).
     */
    template <typename NumberType = int>
    static NumberType getNumber (std::string_view text) noexcept
    {
        NumberType value {};
        const auto [end, error] { std::from_chars (text.data(), text.data() + text.size(), value) };
        jassert (error == std::errc {});
        juce::ignoreUnused (end);
        return value;
    }

    /**
     * @brief Formats text with a trademark symbol for a product.
     */
    static juce::String
    toTrademark (juce::StringRef textToBeFormatted, juce::StringRef productName) noexcept;

    /**
     * @brief Returns the build year.
     */
    static juce::String getBuildYear() noexcept;

    /**
     * @brief Abbreviates a string.
     */
    static juce::String abbreviate (juce::StringRef textToBeFormatted) noexcept;

    /**
     * @brief Builds an SVG id string from pre‑ and post‑colon parts.
     */
    static juce::String toSVGid (juce::StringRef preColon, juce::StringRef postColon) noexcept;

    /**
     * @brief Checks whether a string contains only ASCII characters 0-127.
     */
    static bool isUsingStandardChars (std::string stringToCheck) noexcept;

    /**
     * @brief Creates a product page URL from a product name.
     */
    static juce::String createProductPageURL (const juce::String& productName) noexcept;

    /**
     * @brief Builds an email address from local part and domain.
     */
    static juce::String
    toEmail (juce::StringRef mail,
             juce::StringRef domain = juce::String (Id::companyWebsite)
                                          .fromLastOccurrenceOf (Id::protocolSeparator.toString(),
                                                                 false,
                                                                 false)) noexcept;

    /**
     * @brief Formats a version string as a default preset version.
     */
    static juce::String toDefaultPresetVersion (juce::StringRef versionString) noexcept;

    /**
     * @brief Converts a boolean to "true" or "false".
     */
    static juce::String fromBoolean (bool trueOrFalse) noexcept;

    /**
     * @brief Removes "-" from string, and replace it with whitespace;
     */
    static juce::String dashToWhitespace (juce::StringRef text) noexcept;

    //==============================================================================

    /**
     * @brief Checks if a string is numeric.
     */
    static bool isNumber (juce::StringRef text) noexcept;

    /**
     * @brief Converts a note name to a MIDI note number.
     */
    static int getMIDINoteNumberFromName (juce::StringRef noteName) noexcept;

    /**
     * @brief Checks if a note name is valid.
     */
    static bool isValidNoteName (juce::StringRef noteName) noexcept;

    /**
     * @brief Checks if a note name is sharp.
     */
    static bool isNoteNameSharp (juce::StringRef noteName) noexcept;

    /**
     * @brief Checks if a note name is flat.
     */
    static bool isNoteNameFlat (juce::StringRef noteName) noexcept;

    /**
     * @brief Checks if a string represents a kilo value (e.g. "k").
     */
    static bool isKilo (juce::StringRef textNumber) noexcept;

    /**
     * @brief Compares two version strings to see if one is older.
     */
    static bool isVersionOld (const juce::String& versionToCheck,
                              const juce::String& versionToCompare) noexcept;

    /**
     * @brief Converts a frequency in Hz to a MIDI note number.
     *
     * Uses the standard formula:
     *   note = 12 * log2(frequency / referencePitch) + 69
     *
     * @tparam ValueType Floating‑point type (e.g. float, double).
     * @param frequencyInHz Frequency in Hz to convert.
     * @param standardPitchReference Reference pitch for A4 (default = 440 Hz).
     * @return MIDI note number corresponding to the frequency.
     */
    template<typename ValueType>
    static int
    getMIDINoteNumberFromHz (const ValueType& frequencyInHz, ValueType standardPitchReference = 440)
    {
        return toInt (12 * std::log2 (frequencyInHz / standardPitchReference) + 69);
    }

    /**
     * @brief Converts a MIDI note number to its frequency in Hz.
     *
     * Uses the formula:
     *   frequency = referencePitch * 2^((noteNumber - 69) / 12)
     *
     * @tparam ValueType Floating‑point type (e.g. float, double).
     * @param noteNumber MIDI note number (0–163 valid).
     * @param standardPitchReference Reference pitch for A4 (default = 440 Hz).
     * @return Frequency in Hz, or the noteNumber itself if out of range.
     */
    template<typename ValueType>
    static ValueType
    getFrequencyInHzFromNoteNumber (int noteNumber, ValueType standardPitchReference = 440)
    {
        if (juce::isPositiveAndBelow (noteNumber, 164))
            return static_cast<ValueType> (
                standardPitchReference
                * std::pow (2,
                            (static_cast<ValueType> (noteNumber) - static_cast<ValueType> (69))
                                / static_cast<ValueType> (12)));

        return noteNumber;
    }

    /**
     * @brief Converts a note name (e.g. "C#4") to its frequency in Hz.
     *
     * Internally resolves the note name to a MIDI note number, then converts
     * that number to frequency using @ref getFrequencyInHzFromNoteNumber.
     *
     * @tparam ValueType Floating‑point type (e.g. float, double).
     * @param noteName Musical note name string.
     * @return Frequency in Hz corresponding to the note name.
     */
    template<typename ValueType>
    static ValueType getFrequencyInHzFromNoteName (const juce::String& noteName)
    {
        return getFrequencyInHzFromNoteNumber<ValueType> (getMIDINoteNumberFromName (noteName));
    }

    /**
     * @brief Converts a frequency in Hz to a musical note name.
     *
     * Optionally formats with sharps or flats, and includes octave number.
     *
     * @tparam ValueType Floating‑point type (e.g. float, double).
     * @param frequencyInHz Frequency in Hz to convert.
     * @param useSharps If true, use sharps (#); otherwise use flats (b).
     * @param includeOctaveNumber If true, append octave number to note name.
     * @param octaveNumForMiddleC Octave number to assign to middle C (default = 4).
     * @return Note name string (e.g. "A4", "C#3"), or empty if out of range.
     */
    template<typename ValueType>
    static juce::String getMIDINoteNameFromHz (ValueType frequencyInHz,
                                               bool useSharps = true,
                                               bool includeOctaveNumber = true,
                                               int octaveNumForMiddleC = 4)
    {
        int note { getMIDINoteNumberFromHz (frequencyInHz) };

        if (juce::isPositiveAndBelow (note, 164))
        {
            juce::String s (useSharps ? map::Sharps::getInstance()->get (note % 12) : map::Flats::getInstance()->get (note % 12));

            if (includeOctaveNumber)
                s << (note / 12 + (octaveNumForMiddleC - 5));

            return s;
        }

        return {};
    }

    /** @brief Formats a packed Bounds as "x,y wxh". */
    static juce::String fromBounds (const juce::var& v) noexcept;

    /**
     * @brief Converts text into a double-quoted C++ string literal.
     *
     * A backslash followed by a byte that is itself a declared escape letter
     * (Chars::escape, e.g. n, t, r) is an already-authored escape sequence and
     * passes through verbatim. A backslash followed by another backslash is one
     * literal backslash and is emitted doubled. Any other backslash is
     * self-escaped, emitted doubled. Double quotes are self-escaped. Control
     * bytes with a Chars::escape entry (backspace, tab, newline, vertical tab,
     * form feed, carriage return) are replaced by their backslash-letter form
     * (e.g. newline becomes \\n). Every other byte below space, and every byte
     * at or above 0x80, is replaced by a \\xHH hex escape. Every other byte
     * passes through unchanged. The whole result is wrapped in double quotes.
     *
     * @param input Text to convert.
     * @return input rendered as a double-quoted C++ string literal.
     */
    static juce::String toLiteral (const juce::String& input) noexcept;

    /**
     * @brief Converts whitespace-separated "U+" codepoint tokens into escaped
     *        UTF-8 byte sequences.
     *
     * Splits input on whitespace; each token is expected to be a "U+" token
     * (e.g. "U+00E9"). Every UTF-8 byte of that codepoint is emitted as a
     * \\xHH hex escape, and the escaped sequences from every token are
     * concatenated directly, without separators between them. This is the
     * counterpart to fromUTF8(), which tolerates arbitrary surrounding text
     * and emits raw bytes instead of the escaped form.
     *
     * @param input Whitespace-separated "U+" codepoint tokens.
     * @return Concatenated \\xHH escape sequences for every input token's UTF-8 bytes.
     */
    static juce::String toUTF8 (const juce::String& input) noexcept;

    /**
     * @brief Decodes every "U+" token in the input to that codepoint's UTF-8 bytes.
     *
     * Scans \c input once; each "U+" token (e.g. "U+00E9") is replaced by the raw
     * UTF-8 byte sequence for that codepoint, and every other byte passes through
     * unchanged. This is the prose-tolerant counterpart to toUTF8(), which requires
     * every whitespace-separated token in its input to be a "U+" token and emits
     * the ESCAPED form (\\xNN per byte) rather than raw bytes. Neither function is
     * juce::String::fromUTF8, which decodes a raw UTF-8 byte buffer into a
     * juce::String.
     *
     * @param input Text to scan for "U+" codepoint tokens.
     * @return Text with every "U+" token replaced by its UTF-8 byte sequence.
     */
    static std::string fromUTF8 (std::string_view input) noexcept;

    /**
     * @brief Converts a character, or an existing "0x" value, into a
     *        lowercase hex-prefixed string.
     *
     * If input already starts with "0x", its remaining digits are reparsed
     * as a codepoint value; otherwise the first character of input is used
     * as the codepoint. The codepoint's value is then rendered as an
     * "0x"-prefixed hexadecimal string.
     *
     * @param input A single character, or a "0x"-prefixed hexadecimal codepoint value.
     * @return The codepoint's value as an "0x"-prefixed hexadecimal string.
     */
    static juce::String toHex (const juce::String& input) noexcept;

    /**
     * @brief Converts a character, or an existing "0x" value, into a
     *        "U+XXXX" codepoint token.
     *
     * Resolves the codepoint the same way toHex() does, then renders it as
     * a "U+" token zero-padded to four uppercase hex digits.
     *
     * @param input A single character, or a "0x"-prefixed hexadecimal codepoint value.
     * @return The codepoint as a "U+XXXX" token, zero-padded to four uppercase hex digits.
     */
    static juce::String toCodepoint (const juce::String& input) noexcept;

    /**
     * @brief Qualifies a single-namespace symbol with the juce:: prefix.
     *
     * Counts the number of "::" occurrences in input; when input carries
     * exactly one, the result is prefixed with "juce::". Any other count
     * (zero, or two or more) leaves input unchanged.
     *
     * @param input Symbol name to qualify.
     * @return input prefixed with "juce::" when it carries exactly one "::",
     *         otherwise input unchanged.
     */
    static juce::String toSymbol (const juce::String& input) noexcept;

    /**
     * @brief Converts a hexadecimal codepoint value into a "U+XXXX" token.
     *
     * Strips a leading "0x" prefix if present, then zero-pads the remaining
     * hex digits to four characters and uppercases them. The digits are
     * reformatted as text, not reparsed as a numeric value.
     *
     * @param input A hexadecimal codepoint value, optionally "0x"-prefixed.
     * @return input as a "U+XXXX" token, zero-padded to four uppercase hex digits.
     */
    static juce::String toUnicode (const juce::String& input) noexcept;

    /**
     * @brief Builds an "Id::" member-access expression with a trailing
     *        ".toString()" call, for the given name.
     *
     * Converts input to camelCase, then wraps it as an "Id::" member access
     * followed by ".toString()".
     *
     * @param input Identifier name to qualify.
     * @return input in camelCase, prefixed with "Id::" and suffixed with ".toString()".
     */
    static juce::String fromId (const juce::String& input) noexcept;

    /**
     * @brief Builds a "map::" qualified expression for the given name.
     *
     * @param input Name to qualify.
     * @return input prefixed with "map::".
     */
    static juce::String fromMap (const juce::String& input) noexcept;

    /**
     * @brief Builds an "Id::" member-access expression for the given name.
     *
     * Converts input to camelCase and prefixes it with "Id::", without a
     * trailing ".toString()" call.
     *
     * @param input Identifier name to qualify.
     * @return input in camelCase, prefixed with "Id::".
     */
    static juce::String fromIdentifier (const juce::String& input) noexcept;

    /**
     * @brief Builds a juce::String::fromUTF8 (...) call expression for the
     *        given codepoint tokens.
     *
     * Passes input through toUTF8() to produce its escaped UTF-8 byte
     * sequence, then wraps that sequence in a juce::String::fromUTF8 ("...")
     * call expression. The result is target-language source text, not a
     * decoded string.
     *
     * @param input Whitespace-separated "U+" codepoint tokens.
     * @return A juce::String::fromUTF8 ("...") call expression containing
     *         input's escaped UTF-8 bytes.
     */
    static juce::String fromCodepoint (const juce::String& input) noexcept;

};

/**_____________________________END OF NAMESPACE______________________________*/
} /** namespace jam */
