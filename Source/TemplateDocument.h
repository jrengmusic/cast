#pragma once
#include <JuceHeader.h>
#include "generated/Generated.h"
#include "Model.h"
#include "Operators.h"

struct TemplateDocument : jam::Document, jam::Document::Writer
{
    static std::unique_ptr<TemplateDocument> parse (const juce::String& source)
    {
        auto document { std::make_unique<TemplateDocument>() };
        const std::string data { source.toRawUTF8(), source.getNumBytesAsUTF8() };
        document->Document::parse (data.data(), static_cast<int> (data.size()));
        return document;
    }

    jam::Array<juce::Identifier> getPlaceholders (const Model& model, Element& row) const
    {
        jam::Array<juce::Identifier> placeholders;
        getPlaceholders (model, row, *root, placeholders);
        return placeholders;
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

    TemplateDocument build (const Model& model, Element& row, const juce::String& banner) const
    {
        return build (model, row, banner, {});
    }

    TemplateDocument build (const Model& model,
                            Element& row,
                            const juce::String& banner,
                            const jam::HashMap<juce::Identifier, juce::String>& tokens) const
    {
        TemplateDocument document;
        document.addText (*document.root, banner);

        const auto listCell { model.getTableValue (row, Id::list) };
        auto* sourceTable { model.getTables (row, listCell) };

        if (sourceTable != nullptr and not getPlaceholders (model, row).contains (Id::list))
            document.addText (
                *document.root, getText (model, row, *root, sourceTable->id, tokens));
        else
            build (model, row, row, {}, *root, document, *document.root, tokens);

        return document;
    }

    using Document::getText;

    juce::String getText (const Document& document) const override
    {
        return document.root->getAllSubText();
    }

    juce::String getExpansion (const Model& model,
                               Element& row,
                               const juce::Identifier& jack,
                               const juce::String& symbolPath) const
    {
        const auto& fragment { getOrCreate (model.getOutput (symbolPath)) };
        const auto directives { model.getListDirectives (row) };

        if (not directives.contains (jack))
            return fragment.build (model, row, {}).root->getAllSubText();

        const auto source { juce::Identifier (directives.at (jack)) };
        const auto tables { model.getTables() };

        const auto hasColumnSource { std::any_of (tables.begin(), tables.end(),
            [&model, &source] (Element* table)
            {
                return model.isOutputTable (*table)
                       and model.getTableHeaders (*table).contains (source.toString());
            }) };

        const auto currentFile { model.getValue (row, model.getTableValue (row, Id::file)) };
        jam::Strings texts;
        jam::Strings seenValues;

        for (auto* table : tables)
        {
            if (model.isOutputTable (*table))
            {
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
                        TemplateDocument document;
                        build (model, row, *sourceRow, {}, *fragment.root, document, *document.root, {});

                        if (auto text { document.root->getAllSubText() }; text.isNotEmpty())
                        {
                            text = text.trimCharactersAtEnd (
                                juce::String::charToString (Chars::newline));
                            texts.addIfNotAlreadyThere (text, false);
                        }
                    }
                }
            }
        }

        return texts.joinIntoString (juce::String::charToString (Chars::newline), 0, -1);
    }

    static juce::String getContent (const Model& model,
                                    Element& row,
                                    const juce::String& value,
                                    const juce::Identifier& jack)
    {
        const auto castExtension { juce::String::charToString (Chars::dot) + Extensions::cast };

        if (value.endsWith (castExtension))
            return getOrCreate (model.getOutput (value))
                       .getExpansion (model, row, jack, value)
                       .trimCharactersAtEnd (juce::String::charToString (Chars::newline));

        return value;
    }

    static constexpr int indentWidth { 4 };

    static jam::Array<Element*> getWraps (const Model& model, Element& row)
    {
        jam::Array<Element*> wraps;

        if (auto* structure { model.getStructure (row) })
        {
            bool hasTopLevelScope { false };

            for (auto* block : *structure)
                if (block->isTag (Id::p) and not block->parent->isTag (Id::blockquote))
                {
                    const auto alias { jam::Format::getPreColon (block->getAllSubText()).trim() };

                    if (alias.startsWithChar (Chars::at) or alias.startsWithChar (Chars::hash))
                        hasTopLevelScope = true;
                }

            if (hasTopLevelScope)
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
    getTokens (const Model& model, Element& row, Element& wrap)
    {
        jam::HashMap<juce::Identifier, juce::String> tokens;
        bool afterScopeParagraph { wrap.isTag (Id::blockquote) };

        const auto getBinding = [&model, &row] (const juce::String& value,
                                                const juce::Identifier& jack) -> juce::String
        {
            if (value.startsWithChar (Chars::at)
                and jam::Format::getPostColon (value).containsChar (Chars::colon))
                return model.getEntry (row, value);

            if (value.startsWithChar (Chars::at))
            {
                const auto symbol { model.getValue (row, value) };
                return getContent (model, row, symbol.isNotEmpty() ? symbol : value, jack);
            }

            return value;
        };

        for (auto* block : wrap)
        {
            if (block->isTag (Id::p))
            {
                const auto head { block->getAllSubText() };
                const auto alias { jam::Format::getPreColon (head).trim() };

                if (not afterScopeParagraph
                    and (alias.startsWithChar (Chars::at) or alias.startsWithChar (Chars::hash)))
                    afterScopeParagraph = true;

                if (afterScopeParagraph)
                    tokens.try_emplace (
                        Id::name,
                        getBinding (jam::Format::getPostColon (head).trim(), Id::name));
            }
            else if (block->isTag (Id::ul) and afterScopeParagraph)
            {
                for (auto* item : *block)
                    if (item->isTag (Id::li))
                    {
                        const auto text { item->getAllSubText() };
                        const auto key { jam::Format::getPreColon (text).trim() };
                        const auto value { jam::Format::getPostColon (text).trim() };
                        tokens.try_emplace (
                            juce::Identifier (key), getBinding (value, juce::Identifier (key)));
                    }
            }
            else if (block->isTag (Id::blockquote))
            {
                const auto indent { juce::String::repeatedString (
                    juce::String::charToString (Chars::space), indentWidth) };

                for (auto* contentBlock : *block)
                    if (contentBlock->isTag (Id::ul))
                        for (auto* item : *contentBlock)
                            if (item->isTag (Id::li))
                            {
                                const auto text { item->getAllSubText() };
                                const auto key { jam::Format::getPreColon (text).trim() };
                                const auto value { jam::Format::getPostColon (text).trim() };
                                const auto bound { getBinding (value, juce::Identifier (key)) };
                                const auto indented { indent
                                                      + bound.replace (
                                                          juce::String::charToString (
                                                              Chars::newline),
                                                          juce::String::charToString (
                                                              Chars::newline)
                                                              + indent) };
                                tokens.try_emplace (juce::Identifier (key), indented);
                            }
            }
        }

        if (tokens.contains (Id::name))
        {
            const auto marker { Id::tripleColon + Id::name.toString() + Id::tripleColon };
            const auto name { tokens.at (Id::name) };

            for (auto& [tokenKey, tokenValue] : tokens)
                if (tokenKey != Id::name and tokenValue.contains (marker))
                    tokenValue = tokenValue.replace (marker, name);
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
                if (const auto nested { isWrapHead (model, table, row, *child) };
                    not nested.wasOk())
                    return nested;

        return juce::Result::ok();
    }

protected:
    const Vocabulary& getVocabulary() const override { return getTemplateVocabulary(); }

    int getToken (Cursor& cursor, int) override
    {
        const auto& source { cursor.source };
        const auto position { cursor.getOffset() };

        auto token { isMarker (source, position) ? getMarker (source, position)
                                                 : getTextToken (source, position) };
        const auto length { static_cast<int> (token.length()) };
        tokens.add (std::move (token));
        return length;
    }

    void build() override
    {
        static const auto treeConstruction {
            []
            {
                jam::Function::Map<int, void> treeConstruction;

                treeConstruction
                    .add<const Document::Token&, TemplateDocument&, Element*&, jam::Array<Element*>&> (
                        map::TemplateTokenType::text,
                        [] (const Document::Token& token,
                            TemplateDocument& document,
                            Element*& parent,
                            jam::Array<Element*>&)
                        {
                            document.addText (*parent, document.getText (token));
                        });

                treeConstruction
                    .add<const Document::Token&, TemplateDocument&, Element*&, jam::Array<Element*>&> (
                        map::TemplateTokenType::placeholder,
                        [] (const Document::Token& token,
                            TemplateDocument& document,
                            Element*& parent,
                            jam::Array<Element*>&)
                        {
                            auto& element { *document.addChild (
                                *parent, juce::Identifier (*token.get<juce::String> (Id::name))) };

                            if (token.contains (Id::transform))
                                element.add<juce::Identifier> (
                                    Id::transform,
                                    juce::Identifier (*token.get<juce::String> (Id::transform)));
                        });

                return treeConstruction;
            }()
        };

        jam::Array<Element*> stack;
        Element* parent { root };

        for (const auto& token : tokens)
            treeConstruction.get (token.type, token, *this, parent, stack);
    }

private:
    jam::Array<Document::Token> tokens;
    inline static const std::string tripleColon { Id::tripleColon.toRawUTF8() };

    void addText (Element& parent, const juce::String& content)
    {
        if (content.isNotEmpty())
        {
            auto& node { *addChild (parent, Id::text) };
            node.add<juce::String> (Id::text, content);
        }
    }

    void build (const Model& model,
                Element& row,
                Element& sourceRow,
                const jam::Array<juce::String>& columns,
                Element& node,
                TemplateDocument& document,
                Element& parent,
                const jam::HashMap<juce::Identifier, juce::String>& tokens) const
    {
        constexpr int lineIsVacant { 0 };
        constexpr int lineIsBlank { 1 };
        constexpr int lineIsContent { 2 };

        jam::Strings assembledLines;
        juce::Array<int> lineClassifications;

        juce::String currentLineText;
        bool currentLineHasPlaceholder { false };
        bool currentLineHasNonEmptyPlaceholder { false };
        bool currentLineHasNonWhitespaceLiteral { false };
        bool pendingSpaceCollapse { false };

        const auto finishLine = [&]
        {
            assembledLines.add (currentLineText);

            if (currentLineHasPlaceholder)
            {
                if (currentLineHasNonEmptyPlaceholder or currentLineHasNonWhitespaceLiteral)
                    lineClassifications.add (lineIsContent);
                else
                    lineClassifications.add (lineIsVacant);
            }
            else
            {
                if (currentLineHasNonWhitespaceLiteral)
                    lineClassifications.add (lineIsContent);
                else
                    lineClassifications.add (lineIsBlank);
            }

            currentLineText.clear();
            currentLineHasPlaceholder = false;
            currentLineHasNonEmptyPlaceholder = false;
            currentLineHasNonWhitespaceLiteral = false;
        };

        for (auto* child : node)
        {
            if (child->id == Id::text)
            {
                const auto content { *child->get<juce::String> (Id::text) };
                const auto segments { jam::Strings::fromLines (content) };

                for (int segmentIndex = 0; segmentIndex < segments.size(); ++segmentIndex)
                {
                    if (segmentIndex > 0)
                        finishLine();

                    const auto segment { segments.at (segmentIndex) };

                    if (pendingSpaceCollapse and currentLineText.endsWithChar (Chars::space)
                        and segment.startsWithChar (Chars::space))
                        currentLineText = currentLineText.dropLastCharacters (1);

                    pendingSpaceCollapse = false;
                    currentLineText += segment;

                    if (segment.trim().isNotEmpty())
                        currentLineHasNonWhitespaceLiteral = true;
                }
            }
            else
            {
                const auto text { child->firstChild == nullptr
                                      ? getCell (model, row, sourceRow, columns, *child, tokens)
                                      : getText (model, row, sourceRow, columns, *child, tokens) };

                currentLineHasPlaceholder = true;
                currentLineText += text;

                if (text.isNotEmpty())
                    currentLineHasNonEmptyPlaceholder = true;

                pendingSpaceCollapse = text.isEmpty();
            }
        }

        finishLine();

        bool hasEmittedContent { false };
        bool pendingBlank { false };
        bool suppressNextBlank { false };
        jam::Strings emittedLines;

        for (int lineIndex = 0; lineIndex < assembledLines.size(); ++lineIndex)
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

        const auto finalText { emittedLines.joinIntoString (
            juce::String::charToString (Chars::newline), 0, -1) };
        document.addText (parent, finalText);
    }

    juce::String getCell (const Model& model,
                          Element& row,
                          Element& sourceRow,
                          const jam::Array<juce::String>&,
                          Element& node,
                          const jam::HashMap<juce::Identifier, juce::String>& tokens) const
    {
        juce::String value;
        juce::String typeAlias;

        if (tokens.empty())
        {
            const auto headers { model.getTableHeaders (*sourceRow.parent) };
            const auto token { model.getToken (sourceRow, node.id) };
            const auto cell { headers.contains (node.id.toString())
                                  ? model.getTableValue (sourceRow, node.id)
                                  : token.isNotEmpty() ? token
                                                       : model.getToken (row, node.id) };
            const auto symbol { model.getValue (sourceRow, cell) };
            value = node.id == Id::string and cell.isEmpty()
                       ? jam::Format::toCamelCase (sourceRow.id.toString())
                       : symbol.isNotEmpty() ? symbol
                                             : cell;

            if (node.id == Id::name and value.isEmpty())
                value = jam::Format::getPostColon (sourceRow.id.toString());

            if (node.id != Id::name
                and value.contains (Id::tripleColon + Id::name.toString() + Id::tripleColon))
                value = value.replace (Id::tripleColon + Id::name.toString() + Id::tripleColon,
                                       jam::Format::getPostColon (sourceRow.id.toString()));

            const auto typeToken { model.getToken (sourceRow, Id::type) };
            typeAlias = headers.contains (Id::type.toString())
                           ? model.getTableValue (sourceRow, Id::type)
                           : typeToken.isNotEmpty() ? typeToken
                                                    : model.getToken (row, Id::type);
        }
        else
        {
            value = tokens.contains (node.id) ? tokens.at (node.id) : juce::String();
        }

        const auto castExtension { juce::String::charToString (Chars::dot) + Extensions::cast };
        juce::String symbolPath;

        if (value.endsWith (castExtension))
            symbolPath = value;
        else if (const auto derived { model.getValue (sourceRow, value) };
                 derived.endsWith (castExtension))
            symbolPath = derived;

        if (symbolPath.isNotEmpty())
            return getExpansion (model, row, node.id, symbolPath);

        if (node.contains (Id::transform))
        {
            const auto transform { node.get<juce::Identifier> (Id::transform)->toString() };

            if (model.isReference (sourceRow, value))
                return Transforms::getTransformed (transform, jam::Format::getPostColon (value));

            return Transforms::getTransformed (transform, value);
        }

        if (const auto format { model.getFormat (sourceRow, node.id) }; format.isNotEmpty())
            return Transforms::getTransformed (format, value);

        if (node.id == Id::value and typeAlias.isNotEmpty())
            if (const auto format { model.getFormat (row, juce::StringRef (typeAlias)) };
                format.isNotEmpty())
                return Transforms::getTransformed (format, value);

        return value;
    }

    juce::String getText (const Model& model,
                              Element& row,
                              Element& sourceRow,
                              const jam::Array<juce::String>& columns,
                              Element& node,
                              const jam::HashMap<juce::Identifier, juce::String>& tokens) const
    {
        const auto headers { model.getTableHeaders (*sourceRow.parent) };
        const auto token { model.getToken (sourceRow, node.id) };
        const auto cell { tokens.contains (node.id) ? tokens.at (node.id)
                          : headers.contains (node.id.toString())
                              ? model.getTableValue (sourceRow, node.id)
                              : token.isNotEmpty() ? token
                                                   : model.getToken (row, node.id) };

        if (cell.isEmpty())
            return {};

        if (model.isTemplatePath (sourceRow, cell))
        {
            const auto& wrapper { getOrCreate (model.getFile (sourceRow, cell)) };
            TemplateDocument wrapperDocument;
            wrapper.build (model,
                           row,
                           sourceRow,
                           columns,
                           *wrapper.root,
                           wrapperDocument,
                           *wrapperDocument.root,
                           tokens);
            return wrapperDocument.root->getAllSubText();
        }

        if (auto* sourceTable { model.getTables (sourceRow, juce::StringRef (cell)) })
            return getText (model, row, node, sourceTable->id, tokens);

        TemplateDocument document;
        build (model, row, sourceRow, columns, node, document, *document.root, tokens);
        return document.root->getAllSubText();
    }

    juce::String getText (const Model& model,
                              Element& row,
                              Element& node,
                              const juce::Identifier& source,
                              const jam::HashMap<juce::Identifier, juce::String>& tokens) const
    {
        const auto separatorCell { model.getTableValue (row, Id::separator) };
        juce::String separator;
        bool isLineSeparator { false };

        if (separatorCell.startsWithChar (Chars::dash))
        {
            const auto prefix { jam::Format::getPreColon (separatorCell).trim() };
            const auto reference { jam::Format::getPostColon (separatorCell).trim() };

            isLineSeparator = prefix.endsWith (Id::line.toString());

            if (reference.startsWithChar (Chars::at)
                and jam::Format::getPostColon (reference).containsChar (Chars::colon))
                separator = model.getEntry (row, reference);
            else if (reference.startsWithChar (Chars::at))
                separator = getOrCreate (model.getFile (row, reference))
                                .build (model, row, {})
                                .root->getAllSubText();
        }
        else if (separatorCell.startsWithChar (Chars::at)
                 and jam::Format::getPostColon (separatorCell).containsChar (Chars::colon))
        {
            separator = model.getEntry (row, separatorCell);
        }
        else if (separatorCell.isNotEmpty())
        {
            separator = getOrCreate (model.getFile (row, separatorCell))
                            .build (model, row, {})
                            .root->getAllSubText();
        }

        const auto sourceColumns { model.getTableHeaders (source) };
        jam::Strings texts;

        const auto tableRows { model.getTableRows (source) };

        for (auto* sourceRow : tableRows)
        {
            if (sourceColumns.contains (node.id.toString())
                    ? model.hasTableValue (*sourceRow, node.id)
                    : true)
            {
                TemplateDocument document;
                build (model, row, *sourceRow, sourceColumns, node, document, *document.root, tokens);

                if (const auto text { document.root->getAllSubText() }; text.isNotEmpty())
                    texts.addIfNotAlreadyThere (text, false);
            }
        }

        if (isLineSeparator)
        {
            const auto lineJoin { juce::String::charToString (Chars::newline)
                                  + juce::String::charToString (Chars::newline)
                                  + separator
                                  + juce::String::charToString (Chars::newline)
                                  + juce::String::charToString (Chars::newline) };
            return texts.joinIntoString (lineJoin, 0, -1);
        }

        if (separator.isNotEmpty())
            return texts.joinIntoString (separator, 0, -1);

        return texts.joinIntoString (juce::String::charToString (Chars::newline), 0, -1);
    }

    void getPlaceholders (const Model& model,
                          Element& row,
                          Element& node,
                          jam::Array<juce::Identifier>& placeholders) const
    {
        for (auto* child : node)
        {
            if (child->id != Id::text)
            {
                placeholders.addIfNotAlreadyThere (child->id);

                if (child->firstChild != nullptr)
                {
                    const auto cell { model.getTableValue (row, child->id) };

                    if (model.isTemplatePath (row, cell))
                    {
                        getPlaceholders (model, row, *child, placeholders);

                        const auto& wrapper { getOrCreate (model.getFile (row, cell)) };
                        wrapper.getPlaceholders (model, row, *wrapper.root, placeholders);
                    }
                    else if (not model.isReference (row, cell))
                    {
                        getPlaceholders (model, row, *child, placeholders);
                    }
                }
            }
        }
    }

    static bool isMarker (const std::string& source, size_t position) noexcept
    {
        return source.compare (position, tripleColon.size(), tripleColon) == 0
               and (position + tripleColon.size() >= source.size()
                    or source[position + tripleColon.size()] != Chars::colon);
    }

    static Document::Token getMarker (const std::string& source, size_t cursor)
    {
        auto position { cursor + tripleColon.size() };
        const auto interiorStart { position };

        while (position < source.size())
        {
            if (source.compare (position, tripleColon.size(), tripleColon) == 0)
            {
                const auto interior { juce::String::fromUTF8 (
                    source.data() + interiorStart, static_cast<int> (position - interiorStart)) };
                const auto name { jam::Format::getPreColon (interior) };
                const auto word { jam::Format::getPostColon (interior) };

                position += tripleColon.size();

                const int tokenType { map::TemplateTokenType::placeholder };

                Document::Token token (cursor, position, tokenType);

                if (word.isNotEmpty())
                    token.add<juce::String> (Id::transform, word);

                token.add<juce::String> (Id::name, name);
                return token;
            }

            ++position;
        }

        jassertfalse;
        return Document::Token (cursor, position, map::TemplateTokenType::text);
    }

    static Document::Token getTextToken (const std::string& source, size_t cursor) noexcept
    {
        auto end { cursor };
        ++end;

        while (end < source.size() and not isMarker (source, end))
            ++end;

        return Document::Token (cursor, end, map::TemplateTokenType::text);
    }

    static const Vocabulary& getTemplateVocabulary()
    {
        static const jam::LookupTable<int, int, 256> classes { map::Byte::text, {} };
        static const Vocabulary vocabulary { classes, {}, {} };
        return vocabulary;
    }
};
