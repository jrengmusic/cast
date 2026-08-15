#pragma once
#include <JuceHeader.h>
#include "generated/Generated.h"
#include "Model.h"
#include "Operators.h"

struct TemplateDocument : jam::Document
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
                            const juce::String& code) const
    {
        TemplateDocument document;
        document.addText (*document.root, banner);
        build (model, row, {}, *root, document, *document.root, code);
        return document;
    }

    bool toFile (const juce::File& file) const
    {
        file.getParentDirectory().createDirectory();
        return file.replaceWithText (root->getAllSubText());
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

                treeConstruction
                    .add<const Document::Token&, TemplateDocument&, Element*&, jam::Array<Element*>&> (
                        map::TemplateTokenType::regionOpen,
                        [] (const Document::Token& token,
                            TemplateDocument& document,
                            Element*& parent,
                            jam::Array<Element*>& stack)
                        {
                            auto* region { document.addChild (
                                *parent, juce::Identifier (*token.get<juce::String> (Id::name))) };
                            stack.add (region);
                            parent = region;
                        });

                treeConstruction
                    .add<const Document::Token&, TemplateDocument&, Element*&, jam::Array<Element*>&> (
                        map::TemplateTokenType::regionClose,
                        [] (const Document::Token& token,
                            TemplateDocument& document,
                            Element*& parent,
                            jam::Array<Element*>& stack)
                        {
                            jassert (parent->id
                                     == juce::Identifier (*token.get<juce::String> (Id::name)));
                            stack.remove (stack.size() - 1);
                            parent = stack.size() > 0 ? stack.last() : document.root;
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
                const jam::Array<juce::String>& columns,
                Element& node,
                TemplateDocument& document,
                Element& parent,
                const juce::String& code) const
    {
        for (auto* child : node)
        {
            if (child->id == Id::text)
                document.addText (parent, *child->get<juce::String> (Id::text));
            else if (child->firstChild == nullptr)
                document.addText (
                    parent, child->id == Id::code ? code : getCell (model, row, columns, *child));
            else
                document.addText (parent, getCodeText (model, row, columns, *child, code));
        }
    }

    juce::String
    getCell (const Model& model, Element& row, const jam::Array<juce::String>&, Element& node) const
    {
        const auto headers { model.getTableHeaders (*row.parent) };
        const auto cell { headers.contains (node.id.toString()) ? model.getTableValue (row, node.id)
                                                                 : model.getToken (row, node.id) };
        const auto resolved { model.resolve (cell) };
        const auto value { node.id == Id::string and resolved.isEmpty()
                               ? jam::Format::toCamelCase (row.id.toString())
                               : resolved };

        if (node.contains (Id::transform))
        {
            const auto transform { node.get<juce::Identifier> (Id::transform)->toString() };

            if (model.isReference (value))
                return Transforms::getTransformed (transform, jam::Format::getPostColon (value));

            return Transforms::getTransformed (transform, value);
        }

        if (const auto format { model.getFormat (row, node.id) }; format.isNotEmpty())
            return Transforms::getTransformed (format, value);

        return value;
    }

    juce::String getCodeText (const Model& model,
                              Element& row,
                              const jam::Array<juce::String>& columns,
                              Element& node,
                              const juce::String& code) const
    {
        const auto headers { model.getTableHeaders (*row.parent) };
        const auto cell { headers.contains (node.id.toString())
                              ? model.getTableValue (row, node.id)
                              : model.getToken (row, node.id) };

        if (cell.isEmpty())
            return {};

        if (model.isTemplatePath (cell))
        {
            TemplateDocument bodyDocument;
            build (model, row, columns, node, bodyDocument, *bodyDocument.root, code);

            const auto& wrapper { getOrCreate (model.getFile (cell)) };
            TemplateDocument wrapperDocument;
            wrapper.build (model,
                           row,
                           columns,
                           *wrapper.root,
                           wrapperDocument,
                           *wrapperDocument.root,
                           bodyDocument.root->getAllSubText().trimEnd());
            return wrapperDocument.root->getAllSubText();
        }

        if (auto* sourceTable { model.getTables (juce::StringRef (cell)) })
            return getCodeText (model, row, node, sourceTable->id);

        TemplateDocument document;
        build (model, row, columns, node, document, *document.root, code);
        return document.root->getAllSubText();
    }

    juce::String getCodeText (const Model& model,
                              Element& row,
                              Element& node,
                              const juce::Identifier& source) const
    {
        const juce::Identifier lineBreakColumn { node.id.toString()
                                                 + juce::String::charToString (chars::space)
                                                 + Id::lineBreak.toString() };
        const auto separatorPath { model.getTableValue (row, lineBreakColumn) };
        const auto separator { separatorPath.isNotEmpty()
                                   ? getOrCreate (model.getFile (separatorPath))
                                         .build (model, row, {})
                                         .root->getAllSubText()
                                   : juce::String() };

        const auto sourceColumns { model.getTableHeaders (source) };
        jam::Array<juce::String> texts;

        for (auto* sourceRow : model.getTableRows (source))
        {
            if (not sourceColumns.contains (node.id.toString())
                or model.hasTableValue (*sourceRow, node.id))
            {
                TemplateDocument document;
                build (model, *sourceRow, sourceColumns, node, document, *document.root, {});

                if (const auto text { document.root->getAllSubText() }; text.isNotEmpty())
                    texts.addIfNotAlreadyThere (text);
            }
        }

        juce::String codeText;

        for (const auto& text : texts)
        {
            if (codeText.isEmpty())
                codeText = text;
            else
                codeText << separator << text;
        }

        return codeText;
    }

    void getPlaceholders (const Model& model,
                          Element& row,
                          Element& node,
                          jam::Array<juce::Identifier>& placeholders) const
    {
        for (auto* child : node)
        {
            if (child->id != Id::text and child->id != Id::code)
            {
                placeholders.addIfNotAlreadyThere (child->id);

                if (child->firstChild != nullptr)
                {
                    const auto cell { model.getTableValue (row, child->id) };

                    if (model.isTemplatePath (cell))
                    {
                        getPlaceholders (model, row, *child, placeholders);

                        const auto& wrapper { getOrCreate (model.getFile (cell)) };
                        wrapper.getPlaceholders (model, row, *wrapper.root, placeholders);
                    }
                    else if (not model.isReference (cell))
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
                    or source[position + tripleColon.size()] != chars::colon);
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

                int tokenType { map::TemplateTokenType::placeholder };

                if (word.isNotEmpty())
                {
                    static const auto rules { jam::Map::getKey (map::Rules::get()) };

                    if (rules.contains (word))
                        tokenType = rules.at (word);
                }

                if (tokenType != map::TemplateTokenType::placeholder and position < source.size()
                    and source[position] == chars::newline)
                    ++position;

                Document::Token token (cursor, position, tokenType);

                if (word.isNotEmpty() and tokenType == map::TemplateTokenType::placeholder)
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
