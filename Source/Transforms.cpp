#include "Transforms.h"

juce::String Transforms::toComment (const juce::String& input, const juce::String& extension)
{
    const auto& syntax { map::commentSyntax.at (extension) };
    const auto comment { syntax.get (Id::comment) };
    const auto joined { input.replaceCharacter (Chars::newline, Chars::space) };

    if (comment.isEmpty())
        return toCommentBlock (joined, extension);

    return (comment + Chars::space + joined).trim();
}

std::pair<int, int> Transforms::getDelimiterLines (
    const jam::Strings& lines, const juce::String& beginValue, const juce::String& endValue)
{
    static constexpr int noLine { -1 };

    auto beginLine { noLine };

    for (int lineIndex { 0 }; lineIndex < lines.size() and beginLine < 0; ++lineIndex)
        if (lines.at (lineIndex).contains (beginValue))
            beginLine = lineIndex;

    auto endLine { noLine };

    for (int lineIndex { beginLine + 1 }; beginLine >= 0 and lineIndex < lines.size() and endLine < 0;
         ++lineIndex)
        if (lines.at (lineIndex).contains (endValue))
            endLine = lineIndex;

    return { beginLine, endLine };
}

jam::Strings Transforms::getSplicedLines (const jam::Strings& lines, int beginLine, int endLine,
    const jam::Strings& regionLines, int regionStart, int regionEnd)
{
    jam::Strings spliced;

    for (int lineIndex { 0 }; lineIndex <= beginLine; ++lineIndex)
        spliced.add (lines.at (lineIndex));

    for (int lineIndex { regionStart }; lineIndex < regionEnd; ++lineIndex)
        spliced.add (regionLines.at (lineIndex));

    for (int lineIndex { endLine }; lineIndex < lines.size(); ++lineIndex)
        spliced.add (lines.at (lineIndex));

    return spliced;
}

juce::String Transforms::getJoinedText (const jam::Strings& lines, const juce::String& original)
{
    static const auto newlineText { juce::String::charToString (Chars::newline) };

    auto joined { lines.joinIntoString (newlineText, 0, -1) };

    if (original.endsWith (newlineText) and not joined.endsWith (newlineText))
        joined += newlineText;

    return joined;
}

juce::String Transforms::getTransformedPath (
    const juce::String& relativePath, const juce::String& sourceFilePrefix, const juce::String& targetFilePrefix)
{
    return relativePath.replace (sourceFilePrefix, targetFilePrefix);
}

juce::String
Transforms::toCommentBlock (const juce::String& input, const juce::String& extension)
{
    static const auto spaceText { juce::String::charToString (Chars::space) };
    static const auto newlineText { juce::String::charToString (Chars::newline) };

    const auto& syntax { map::commentSyntax.at (extension) };
    const auto blockOpen { syntax.get (Id::blockOpen) };
    jam::Strings blockLines;

    if (blockOpen.isNotEmpty())
    {
        if (not input.containsChar (Chars::newline))
            return (blockOpen + Chars::space + input
                   + Chars::space + syntax.get (Id::blockClose))
                .trim();

        const auto blockLine { syntax.get (Id::blockLine) };
        const auto prefix { blockLine.isNotEmpty()
                                ? blockLine + Chars::space
                                : juce::String{} };

        blockLines.add (blockOpen);

        for (const auto& proseLine : jam::Strings::fromLines (input))
            blockLines.add (proseLine.isNotEmpty() ? prefix + proseLine : blockLine);

        blockLines.add (blockLine.isNotEmpty()
                             ? spaceText + syntax.get (Id::blockClose)
                             : syntax.get (Id::blockClose));
    }
    else
    {
        const auto prefix { syntax.get (Id::comment) + Chars::space };

        for (const auto& proseLine : jam::Strings::fromLines (input))
            blockLines.add (prefix + proseLine);
    }

    return blockLines.joinIntoString (newlineText, 0, -1);
}

juce::String Transforms::toBrief (const juce::String& input, const juce::String& extension)
{
    const auto& syntax { map::commentSyntax.at (extension) };
    const auto blockOpen { syntax.get (Id::blockOpen) };

    if (blockOpen.isEmpty())
        return toComment (input, extension);

    return (blockOpen + Chars::space
           + syntax.get (Id::brief) + Chars::space + input.replaceCharacter (Chars::newline, Chars::space)
           + Chars::space + syntax.get (Id::blockClose))
        .trim();
}

juce::String Transforms::getCommentSyntaxKey (const juce::String& file)
{
    static const auto dotText { juce::String::charToString (Chars::dot) };

    const auto outputFile { juce::File::createFileWithoutCheckingPath (file) };

    if (auto syntaxEntry { map::manifestSyntax.find (outputFile.getFileName()) };
        syntaxEntry != map::manifestSyntax.end())
    {
        const auto& [syntaxKey, extension] { *syntaxEntry };

        return extension;
    }

    const auto extension { outputFile.getFileExtension() };

    if (map::commentSyntax.contains (extension))
        return extension;

    return dotText + Extensions::h;
}

juce::String Transforms::getFencePrefix (const juce::String& fenceName)
{
    static const auto closeBracket { Chars::enclosure.get (Chars::openBracket) };
    static const auto closeBracketText { juce::String::charToString (closeBracket) };

    if (fenceName.startsWithChar (Chars::openBracket)
        and fenceName.containsChar (closeBracket))
        return jam::Format::withoutEnclosure (
            jam::Format::upTo (fenceName, closeBracketText, true), Chars::openBracket);

    return {};
}

bool Transforms::contains (const juce::String& name) noexcept
{
    return getTransforms().contains (name);
}

juce::String Transforms::getTransformed (const juce::String& name,
                                    const juce::String& input,
                                    const juce::String& extension)
{
    jassert (contains (name));

    return getTransforms().get (name, input, extension);
}

bool Transforms::isWordBoundaryChar (juce::juce_wchar character) noexcept
{
    return character == Chars::nullCharacter
           or not (juce::CharacterFunctions::isLetterOrDigit (character) or character == Chars::underscore);
}

int Transforms::getNextWholeWordMatch (const juce::String& text, const juce::String& word, int cursor)
{
    for (auto position { text.indexOf (cursor, word) }; position >= 0; position = text.indexOf (cursor, word))
    {
        const auto beforeChar { position > 0 ? text[position - 1] : Chars::nullCharacter };
        const auto matchEnd { position + word.length() };
        const auto afterChar { matchEnd < text.length() ? text[matchEnd] : Chars::nullCharacter };

        if (isWordBoundaryChar (beforeChar) and isWordBoundaryChar (afterChar))
            return position;

        cursor = position + 1;
    }

    return -1;
}

juce::String
Transforms::getWordBoundaryReplaced (const juce::String& text, const juce::String& source, const juce::String& target)
{
    juce::String result;
    int cursor { 0 };

    for (auto position { getNextWholeWordMatch (text, source, cursor) }; position >= 0;
         position = getNextWholeWordMatch (text, source, cursor))
    {
        result += text.substring (cursor, position) + target;
        cursor = position + source.length();
    }

    result += text.substring (cursor);
    return result;
}

bool Transforms::containsWholeWord (const juce::String& text, const juce::String& word)
{
    return getNextWholeWordMatch (text, word, 0) >= 0;
}

void Transforms::addCaseTransforms (jam::Function::Map<juce::String, juce::String>& map)
{
    map.add<const juce::String&, const juce::String&> (
        jam::Format::toCamelCase (Id::toUpper),
        [] (const juce::String& input, const juce::String&)
        {
            return input.toUpperCase();
        });
    map.add<const juce::String&, const juce::String&> (
        jam::Format::toCamelCase (Id::toTitle),
        [] (const juce::String& input, const juce::String&)
        {
            return jam::Format::toTitleCase (input);
        });
    map.add<const juce::String&, const juce::String&> (
        jam::Format::toCamelCase (Id::toKebab),
        [] (const juce::String& input, const juce::String&)
        {
            return jam::Format::toKebabCase (input);
        });
    map.add<const juce::String&, const juce::String&> (
        jam::Format::toCamelCase (Id::toPascal),
        [] (const juce::String& input, const juce::String&)
        {
            return jam::Format::toPascalCase (input);
        });
    map.add<const juce::String&, const juce::String&> (
        jam::Format::toCamelCase (Id::toCamel),
        [] (const juce::String& input, const juce::String&)
        {
            return jam::Format::toCamelCase (input);
        });
    map.add<const juce::String&, const juce::String&> (
        jam::Format::toCamelCase (Id::toSnake),
        [] (const juce::String& input, const juce::String&)
        {
            return jam::Format::toSnakeCase (input);
        });
    map.add<const juce::String&, const juce::String&> (
        jam::Format::toCamelCase (Id::toScreamingSnake),
        [] (const juce::String& input, const juce::String&)
        {
            return jam::Format::toScreamingSnakeCase (input);
        });
}

void Transforms::addEncodingTransforms (jam::Function::Map<juce::String, juce::String>& map)
{
    map.add<const juce::String&, const juce::String&> (
        jam::Format::toCamelCase (Id::toUTF8),
        [] (const juce::String& input, const juce::String&)
        {
            return jam::Format::toUTF8 (input);
        });
    map.add<const juce::String&, const juce::String&> (
        jam::Format::toCamelCase (Id::fromUTF8),
        [] (const juce::String& input, const juce::String&)
        {
            const std::string_view text { input.getCharPointer().getAddress() };
            const auto result { jam::Format::fromUTF8 (text) };

            return jam::Strings::fromView (result);
        });
    map.add<const juce::String&, const juce::String&> (
        jam::Format::toCamelCase (Id::toLiteral),
        [] (const juce::String& input, const juce::String&)
        {
            return jam::Format::toLiteral (input);
        });
    map.add<const juce::String&, const juce::String&> (
        jam::Format::toCamelCase (Id::toHex),
        [] (const juce::String& input, const juce::String&)
        {
            return jam::Format::toHex (input);
        });
    map.add<const juce::String&, const juce::String&> (
        jam::Format::toCamelCase (Id::toCodepoint),
        [] (const juce::String& input, const juce::String&)
        {
            return jam::Format::toCodepoint (input);
        });
    map.add<const juce::String&, const juce::String&> (
        jam::Format::toCamelCase (Id::fromCodepoint),
        [] (const juce::String& input, const juce::String&)
        {
            return jam::Format::fromCodepoint (input);
        });
}

void Transforms::addTextTransforms (jam::Function::Map<juce::String, juce::String>& map)
{
    map.add<const juce::String&, const juce::String&> (
        jam::Format::toCamelCase (Id::join),
        [] (const juce::String& input, const juce::String&)
        {
            return jam::Format::join (input);
        });
    map.add<const juce::String&, const juce::String&> (
        jam::Format::toCamelCase (Id::toFileName),
        [] (const juce::String& input, const juce::String&)
        {
            return static_cast<juce::String (*) (const juce::String&)> (
                &jam::Format::toFileName) (input);
        });
}

void Transforms::addCommentTransforms (jam::Function::Map<juce::String, juce::String>& map)
{
    map.add<const juce::String&, const juce::String&> (
        jam::Format::toCamelCase (Id::toComment), &Transforms::toComment);
    map.add<const juce::String&, const juce::String&> (
        jam::Format::toCamelCase (Id::toCommentBlock), &Transforms::toCommentBlock);
    map.add<const juce::String&, const juce::String&> (
        jam::Format::toCamelCase (Id::brief), &Transforms::toBrief);
}

const jam::Function::Map<juce::String, juce::String>& Transforms::getTransforms() noexcept
{
    static const jam::Function::Map<juce::String, juce::String> transforms {
        []()
        {
            jam::Function::Map<juce::String, juce::String> map;

            addCaseTransforms (map);
            addEncodingTransforms (map);
            addTextTransforms (map);
            addCommentTransforms (map);

            return map;
        }()
    };

    return transforms;
}
