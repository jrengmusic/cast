#pragma once
#include <JuceHeader.h>
#include "generated/Generated.h"
#include "Model.h"
#include "Operators.h"

struct TemplateDocument : jam::MarkdownDocument
{
    TemplateDocument() = default;

    explicit TemplateDocument (jam::MarkdownDocument&& parsed)
        : jam::MarkdownDocument (std::move (parsed))
        , text (juce::String::fromUTF8 (getSource().data(), static_cast<int> (getSource().size())))
        , placeholders (
              [this]
              {
                  jam::Array<juce::Identifier> found;
                  const auto delimiter { Id::tripleColon.toString() };
                  int cursor { 0 };

                  while (true)
                  {
                      const auto start { text.indexOf (cursor, delimiter) };

                      if (start < 0)
                          break;

                      const auto end { text.indexOf (start + delimiter.length(), delimiter) };

                      if (end < 0)
                          break;

                      const auto interior { text.substring (start + delimiter.length(), end) };
                      found.addIfNotAlreadyThere (
                          juce::Identifier (jam::Format::getPreColon (interior)));
                      cursor = end + delimiter.length();
                  }

                  return found;
              }())
    {
    }

    static std::unique_ptr<TemplateDocument> parse (const juce::String& source)
    {
        return std::make_unique<TemplateDocument> (jam::MarkdownDocument::parse (source));
    }

    static const TemplateDocument& getOrCreate (const juce::File& file)
    {
        static juce::CriticalSection templateLock;
        static jam::HashMap<juce::String, std::unique_ptr<TemplateDocument>> templates;

        const auto key { file.getFullPathName() };

        {
            const juce::ScopedLock lock { templateLock };

            if (templates.contains (key))
                return *templates.at (key);
        }

        auto parsed { parse (file.loadFileAsString()) };

        const juce::ScopedLock lock { templateLock };
        templates.try_emplace (key, std::move (parsed));
        return *templates.at (key);
    }

    juce::String getText() const noexcept { return text; }

    const jam::Array<juce::Identifier>& getPlaceholders() const noexcept { return placeholders; }

    static juce::String getBinding (const Model& model,
                                    Element& row,
                                    int depth,
                                    const juce::String& value,
                                    const juce::Identifier& jack)
    {
        if (value.startsWithChar (Chars::at)
            and jam::Format::getPostColon (value).containsChar (Chars::colon))
            return model.getEntry (row, value);

        if (value.startsWithChar (Chars::at))
        {
            const auto symbol { model.getValue (row, value) };
            return getContent (model, row, depth, symbol.isNotEmpty() ? symbol : value, jack);
        }

        return value;
    }

    static juce::String getContent (const Model& model,
                                    Element& row,
                                    int depth,
                                    const juce::String& value,
                                    const juce::Identifier& jack)
    {
        if (juce::File::createFileWithoutCheckingPath (value).hasFileExtension (Extensions::cast))
            return getOrCreate (model.getOutput (value))
                       .getExpansion (model, row, depth, jack)
                       .trimCharactersAtEnd (juce::String::charToString (Chars::newline));

        return value;
    }

    static juce::String getSeparator (const Model& model,
                                      Element& row,
                                      const juce::Identifier& name,
                                      int depth)
    {
        juce::String separator;

        if (auto* separatorCell { model.getTableCell (row, Id::separator) })
        {
            Element* scope { separatorCell };

            for (int scopeDepth { 0 }; scopeDepth < depth and scope != nullptr; ++scopeDepth)
            {
                Element* nested { nullptr };

                for (auto* block : *scope)
                    if (block->isTag (Id::blockquote))
                        nested = block;

                scope = nested;
            }

            if (scope != nullptr)
                for (auto* block : *scope)
                    if (block->isTag (Id::ul))
                        for (auto* item : *block)
                            if (item->isTag (Id::li))
                            {
                                const auto itemText { item->getAllSubText() };
                                const auto key { jam::Format::getPreColon (itemText).trim() };

                                if (key == name.toString())
                                    separator = getBinding (model,
                                        row,
                                        depth,
                                        jam::Format::getPostColon (itemText).trim(),
                                        name);
                            }
        }

        return separator;
    }

    static constexpr int indentWidth { 4 };

    static jam::Array<Element*> getWraps (const Model& model, Element& row)
    {
        jam::Array<Element*> wraps;

        if (auto* structure { model.getStructure (row) })
        {
            wraps.add (structure);
            Element* wrap { nullptr };

            for (auto* block : *structure)
                if (block->isTag (Id::blockquote))
                    wrap = block;

            while (wrap != nullptr)
            {
                wraps.add (wrap);
                Element* nested { nullptr };

                for (auto* block : *wrap)
                    if (block->isTag (Id::blockquote))
                        nested = block;

                wrap = nested;
            }
        }

        return wraps;
    }

    static juce::String getWrapAlias (const Model& model, Element& wrap)
    {
        juce::String alias;

        for (auto* block : wrap)
            if (block->isTag (Id::p))
                alias = jam::Format::getPreColon (block->getAllSubText()).trim();

        return alias;
    }

    static jam::HashMap<juce::Identifier, juce::String>
    getTokens (const Model& model, Element& row, int depth, Element& wrap)
    {
        jam::HashMap<juce::Identifier, juce::String> tokens;

        const auto hasScopeHead { std::any_of (wrap.begin(), wrap.end(),
            [] (Element* block)
            {
                if (not block->isTag (Id::p))
                    return false;

                const auto alias { jam::Format::getPreColon (block->getAllSubText()).trim() };
                return alias.startsWithChar (Chars::at) or alias.startsWithChar (Chars::hash);
            }) };

        bool afterScopeParagraph { not hasScopeHead };

        for (auto* block : wrap)
        {
            if (block->isTag (Id::p))
            {
                const auto head { block->getAllSubText() };
                const auto alias { jam::Format::getPreColon (head).trim() };

                if (not afterScopeParagraph
                    and (alias.startsWithChar (Chars::at) or alias.startsWithChar (Chars::hash)))
                    afterScopeParagraph = true;

                if (afterScopeParagraph and head.containsChar (Chars::colon))
                    tokens.try_emplace (Id::name,
                        getBinding (model, row, depth, jam::Format::getPostColon (head).trim(), Id::name));
            }
            else if (block->isTag (Id::ul) and afterScopeParagraph)
            {
                for (auto* item : *block)
                    if (item->isTag (Id::li))
                    {
                        const auto itemText { item->getAllSubText() };
                        const auto key { jam::Format::getPreColon (itemText).trim() };
                        const auto value { jam::Format::getPostColon (itemText).trim() };
                        tokens.try_emplace (juce::Identifier (key),
                            getBinding (model, row, depth, value, juce::Identifier (key)));
                    }
            }
        }

        if (tokens.contains (Id::name))
        {
            const auto name { tokens.at (Id::name) };

            for (auto& [tokenKey, tokenValue] : tokens)
                if (tokenKey != Id::name and jam::Format::hasPlaceholder (tokenValue, Id::name))
                    tokenValue = jam::Format::replaceholder (tokenValue, Id::name, name);
        }

        return tokens;
    }

    static Element* getOutermostWrap (Element& wrap)
    {
        Element* outermost { &wrap };

        for (auto* child : wrap)
            if (child->isTag (Id::blockquote))
                outermost = getOutermostWrap (*child);

        return outermost;
    }

    static juce::Result isWrapHead (const Model& model, Element& table, Element& row, Element& wrap)
    {
        Element* head { nullptr };
        int headCount { 0 };

        for (auto* child : wrap)
            if (child->isTag (Id::p))
            {
                head = child;
                ++headCount;
            }

        if (headCount == 0)
        {
            bool hasNestedWrap { false };
            bool hasContentTokens { false };

            for (auto* child : wrap)
            {
                if (child->isTag (Id::blockquote))
                    hasNestedWrap = true;

                if (child->isTag (Id::ul))
                    hasContentTokens = true;
            }

            if (not hasNestedWrap and not hasContentTokens)
                return juce::Result::fail (
                    jam::MarkdownValidator::getLocation (table, row, Id::structure.toString())
                    + Id::diagnosticSeparator + text::Diagnostics::failNotFound);
        }

        if (headCount > 1)
            return juce::Result::fail (
                jam::MarkdownValidator::getLocation (table, row, Id::structure.toString())
                + Id::diagnosticSeparator + text::Diagnostics::failNotFound);

        if (headCount == 1)
        {
            const auto alias { jam::Format::getPreColon (head->getAllSubText()).trim() };

            if (model.getValue (row, alias).isEmpty())
                return juce::Result::fail (
                    jam::MarkdownValidator::getLocation (table, row, Id::structure.toString())
                    + Id::diagnosticSeparator + text::Diagnostics::failAliasMissing
                    + Id::diagnosticSeparator + alias);
        }

        for (auto* child : wrap)
            if (child->isTag (Id::blockquote))
                if (const auto nested { isWrapHead (model, table, row, *child) }; not nested.wasOk())
                    return nested;

        return juce::Result::ok();
    }

    juce::String getExpansion (const Model& model,
                               Element& row,
                               int depth,
                               const juce::Identifier& jack) const
    {
        const auto wiring { model.getSource (row, depth) };

        if (not wiring.contains (jack))
            return build (model, row, row, depth, {})
                       .trimCharactersAtEnd (juce::String::charToString (Chars::newline));

        const auto wiringValue { wiring.at (jack) };
        const auto isTableReference { wiringValue.startsWithChar (Chars::at)
                                      and wiringValue.containsChar (Chars::colon) };
        auto* wiredTable { isTableReference ? model.getTables (row, wiringValue) : nullptr };

        jam::Strings texts;

        if (wiredTable != nullptr)
        {
            for (auto* sourceRow : model.getTableRows (*wiredTable))
            {
                const auto sourceText { build (model, row, *sourceRow, depth, {})
                                            .trimCharactersAtEnd (
                                                juce::String::charToString (Chars::newline)) };

                if (sourceText.isNotEmpty())
                    texts.addIfNotAlreadyThere (sourceText, false);
            }
        }
        else
        {
            const auto source { juce::Identifier (wiringValue) };
            const auto tables { model.getTables() };

            const auto hasColumnSource { std::any_of (tables.begin(), tables.end(),
                [&model, &source] (Element* table)
                {
                    return model.isOutputTable (*table)
                           and model.getTableHeaders (*table).contains (source.toString());
                }) };

            const auto currentFile { model.getValue (row, model.getTableValue (row, Id::file)) };
            jam::Strings seenValues;

            for (auto* table : tables)
                if (model.isOutputTable (*table))
                    for (auto* sourceRow : model.getTableRows (*table))
                    {
                        bool matches { not hasColumnSource
                                          and model.getToken (*sourceRow, source).isNotEmpty() };

                        if (hasColumnSource)
                        {
                            const auto value {
                                model.getValue (*sourceRow, model.getTableValue (*sourceRow, source))
                            };

                            matches = value.isNotEmpty() and value != currentFile
                                      and not seenValues.contains (value, false);

                            if (matches)
                                seenValues.add (value);
                        }

                        if (matches)
                        {
                            const auto sourceText { build (model, row, *sourceRow, depth, {})
                                                        .trimCharactersAtEnd (
                                                            juce::String::charToString (
                                                                Chars::newline)) };

                            if (sourceText.isNotEmpty())
                                texts.addIfNotAlreadyThere (sourceText, false);
                        }
                    }
        }

        const auto separator { getSeparator (model, row, jack, depth) };
        return texts.joinIntoString (
            separator.isNotEmpty() ? separator : juce::String::charToString (Chars::newline), 0, -1);
    }

    juce::String build (const Model& model,
                        Element& row,
                        Element& sourceRow,
                        int depth,
                        const jam::HashMap<juce::Identifier, juce::String>& tokens) const
    {
        constexpr int lineIsVacant { 0 };
        constexpr int lineIsBlank { 1 };
        constexpr int lineIsContent { 2 };

        jam::Strings assembledLines;
        juce::Array<int> lineClassifications;
        const auto& placeholders { getPlaceholders() };
        const auto extension { jam::Format::onlyExtensionFromFilename (
            model.getValue (row, model.getTableValue (row, Id::file))) };

        for (const auto& templateLine : jam::Strings::fromLines (text))
        {
            auto lineText { templateLine };
            bool lineHasPlaceholder { false };

            for (const auto& name : placeholders)
            {
                if (jam::Format::hasPlaceholder (lineText, name.toString()))
                {
                    const auto value {
                        getReplacement (model, row, sourceRow, depth, tokens, name, extension)
                    };
                    lineHasPlaceholder = true;

                    if (value.isNotEmpty())
                        lineText = jam::Format::replaceholder (lineText, name.toString(), value);
                    else
                    {
                        const auto marker { Id::tripleColon.toString() + name.toString()
                                           + Id::tripleColon.toString() };
                        const auto spacedMarker { juce::String::charToString (Chars::space) + marker };
                        lineText = lineText.replace (spacedMarker, juce::String());
                        lineText = jam::Format::replaceholder (lineText, name.toString(), juce::String());
                    }
                }

                for (const auto& [transformName, transformFunction] : Transforms::getTransforms())
                {
                    const auto tagged { name.toString() + juce::String::charToString (Chars::colon)
                                       + transformName };

                    if (jam::Format::hasPlaceholder (lineText, tagged))
                    {
                        lineHasPlaceholder = true;

                        const auto replacement {
                            getReplacement (model, row, sourceRow, depth, tokens, name, extension)
                        };
                        const auto isPresent { tokens.contains (name) or replacement.isNotEmpty() };
                        const auto value { isPresent
                                              ? Transforms::getTransformed (
                                                    transformName, replacement, extension)
                                              : juce::String() };

                        if (value.isNotEmpty())
                            lineText = jam::Format::replaceholder (lineText, tagged, value);
                        else
                        {
                            const auto marker { Id::tripleColon.toString() + tagged
                                               + Id::tripleColon.toString() };
                            const auto spacedMarker {
                                juce::String::charToString (Chars::space) + marker
                            };
                            lineText = lineText.replace (spacedMarker, juce::String());
                            lineText = jam::Format::replaceholder (lineText, tagged, juce::String());
                        }
                    }
                }
            }

            lineClassifications.add (lineText.trim().isEmpty()
                                         ? (lineHasPlaceholder ? lineIsVacant : lineIsBlank)
                                         : lineIsContent);

            assembledLines.add (lineText);
        }

        bool hasEmittedContent { false };
        bool pendingBlank { false };
        bool suppressNextBlank { false };
        jam::Strings emittedLines;

        for (int lineIndex { 0 }; lineIndex < assembledLines.size(); ++lineIndex)
        {
            const auto lineText { assembledLines.at (lineIndex) };
            const auto classification { lineClassifications[lineIndex] };

            if (classification == lineIsContent)
            {
                if (pendingBlank and hasEmittedContent)
                    emittedLines.add (juce::String());

                emittedLines.add (lineText);
                hasEmittedContent = true;
                pendingBlank = false;
                suppressNextBlank = false;
            }
            else if (classification == lineIsBlank)
            {
                if (suppressNextBlank)
                    suppressNextBlank = false;
                else
                    pendingBlank = true;
            }
            else
            {
                pendingBlank = false;
                suppressNextBlank = true;
            }
        }

        const auto assembled { emittedLines.joinIntoString (
            juce::String::charToString (Chars::newline), 0, -1) };

        jassert (not assembled.contains (Id::tripleColon.toString()));

        if (assembled.contains (Id::tripleColon.toString()))
            jam::debug::Log::write (
                jam::MarkdownValidator::getLocation (*row.parent, row, Id::structure.toString())
                + Id::diagnosticSeparator + text::Diagnostics::failNoSource);

        return assembled;
    }

private:
    juce::String text;
    const jam::Array<juce::Identifier> placeholders {};

    juce::String getReplacement (const Model& model,
                                 Element& row,
                                 Element& sourceRow,
                                 int depth,
                                 const jam::HashMap<juce::Identifier, juce::String>& tokens,
                                 const juce::Identifier& name,
                                 const juce::String& extension) const
    {
        juce::String value;

        if (tokens.contains (name))
        {
            value = tokens.at (name);
        }
        else
        {
            if (model.getTableHeaders (*sourceRow.parent).contains (name.toString()))
            {
                value = getCell (model, sourceRow, name, extension);
            }
            else
            {
                const auto sourceToken { model.getToken (sourceRow, name) };
                const auto beforeHeadValue { sourceToken.isNotEmpty() ? sourceToken
                                                                      : model.getToken (row, name) };

                if (beforeHeadValue.isNotEmpty())
                {
                    value = juce::File::createFileWithoutCheckingPath (beforeHeadValue)
                                    .hasFileExtension (Extensions::cast)
                               ? getContent (model, row, depth, beforeHeadValue, name)
                               : beforeHeadValue;
                }
                else
                {
                    Element* wiredFirstRow { nullptr };
                    const auto wrapCount { getWraps (model, row).size() };

                    for (int wiringDepth { 0 }; wiringDepth < wrapCount and wiredFirstRow == nullptr;
                         ++wiringDepth)
                        for (auto& [wiringKey, wiringValue] : model.getSource (row, wiringDepth))
                            if (wiredFirstRow == nullptr)
                                if (auto* wiredTable { model.getTables (row, wiringValue) })
                                    if (model.getTableHeaders (*wiredTable).contains (name.toString()))
                                        wiredFirstRow = model.getTableRows (*wiredTable).first();

                    if (wiredFirstRow != nullptr)
                    {
                        value = getCell (model, *wiredFirstRow, name, extension);
                    }
                    else
                    {
                        const auto sourceHeaders { model.getTableHeaders (*sourceRow.parent) };
                        jam::Strings texts;

                        for (const auto& header : sourceHeaders)
                            if (header != *sourceHeaders.begin())
                                texts.add (
                                    getCell (model, sourceRow, juce::Identifier (header), extension));

                        const auto separator { getSeparator (model, row, name, depth) };

                        if (separator.isNotEmpty() or texts.size() <= 1)
                            value = texts.joinIntoString (separator, 0, -1);
                        else
                        {
                            jassertfalse;
                            jam::debug::Log::write (
                                jam::MarkdownValidator::getLocation (
                                    *row.parent, row, Id::structure.toString())
                                + Id::diagnosticSeparator + text::Diagnostics::failNoSource
                                + Id::diagnosticSeparator + name.toString());
                        }
                    }
                }
            }
        }

        return value;
    }

    juce::String getCell (const Model& model,
                         Element& sourceRow,
                         const juce::Identifier& name,
                         const juce::String& extension) const
    {
        const auto headers { model.getTableHeaders (*sourceRow.parent) };

        if (not headers.contains (name.toString()))
            return {};

        if (*headers.begin() == name.toString())
            return sourceRow.id.toString();

        auto* cellElement { model.getTableCell (sourceRow, name) };
        const auto isLiteral { cellElement != nullptr and cellElement->firstChild != nullptr
                              and cellElement->firstChild->isTag (Id::code)
                              and cellElement->firstChild->nextSibling == nullptr };
        const auto cell { model.getTableValue (sourceRow, name) };

        juce::String value;

        if (isLiteral)
            value = cell;
        else if (cell.isEmpty())
            value = sourceRow.id.toString();
        else if (Transforms::contains (cell))
            value = Transforms::getTransformed (cell, sourceRow.id.toString(), extension);
        else
            value = cell;

        if (const auto format { model.getFormat (sourceRow, name) }; format.isNotEmpty())
            value = Transforms::getTransformed (format, value, extension);

        return value;
    }
};
