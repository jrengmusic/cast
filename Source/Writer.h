#pragma once
#include <JuceHeader.h>
#include "Model.h"
#include "TemplateDocument.h"

static constexpr int indentWidth { 4 };

struct Writer
{
    using Elements = jam::Array<jam::Document::Element*>;

    Writer (const Model& document)
        : model (document)
    {
    }

    bool toFile (const juce::File& outputPath)
    {
        std::atomic<bool> written { true };

        for (auto* table : model.getTables())
        {
            const auto columns { model.getTableHeaders (*table) };

            if (columns.contains (Id::file.toString()) and table->id != Id::index)
            {
                const auto rows { model.getTableRows (*table) };

                jam::Array<juce::String> files;
                jam::HashMap<juce::String, Elements> rowsByFile;

                for (auto* row : rows)
                {
                    const auto origin { *row->parent->get<juce::String> (Id::path) };
                    const auto file {
                        model.getValue (origin, model.getTableValue (*row, Id::file))
                    };
                    files.addIfNotAlreadyThere (file);
                    rowsByFile[file].add (row);
                }

                runJobs (
                    files.size(),
                    [this, &outputPath, &files, &rowsByFile, &written] (int index)
                    {
                        const auto& file { files.at (index) };
                        const auto& fileRows { rowsByFile.at (file) };

                        auto outputFile { jam::File::getOrCreate (outputPath, file) };
                        written = outputFile.replaceWithText (
                            getBanner (file) + buildFile (fileRows),
                            false,
                            false,
                            juce::String::charToString (chars::newline).toRawUTF8());
                    });
            }
        }

        return written;
    }

private:
    const Model& model;

    /** @brief Resolves a structure binding value per sigil law (three-part entry, alias, or literal). */
    juce::String getBinding (Model::Element& row, const juce::String& value) const
    {
        if (value.startsWithChar (chars::at)
            and jam::Format::getPostColon (value).containsChar (chars::colon))
            return model.getEntry (row, value);

        if (value.startsWithChar (chars::at))
        {
            const auto symbol { model.getValue (row, value) };
            return symbol.isNotEmpty() ? symbol : value;
        }

        return value;
    }

    /** @brief The row's blockquote wrap chain, innermost (depth 1) first, outermost last. */
    Elements getWraps (Model::Element& row) const
    {
        Elements wraps;

        if (auto* structure { model.getStructure (row) })
        {
            Model::Element* wrap { nullptr };

            for (auto* block : *structure)
                if (block->isTag (Id::blockquote))
                    wrap = block;

            while (wrap != nullptr)
            {
                wraps.add (wrap);
                Model::Element* nested { nullptr };

                for (auto* block : *wrap)
                    if (block->isTag (Id::blockquote))
                        nested = block;

                wrap = nested;
            }
        }

        return wraps;
    }

    /** @brief The wrap's head alias (`#alias` in `#alias: Name`), used to resolve its template file. */
    juce::String getWrapAlias (Model::Element& wrap) const
    {
        juce::String alias;

        for (auto* block : wrap)
            if (block->isTag (Id::p))
                alias = jam::Format::getPreColon (block->getAllSubText()).trim();

        return alias;
    }

    /** @brief The wrap's own token bindings, plus Id::name resolved from its head paragraph. */
    jam::HashMap<juce::Identifier, juce::String> getTokens (Model::Element& row,
                                                                Model::Element& wrap) const
    {
        jam::HashMap<juce::Identifier, juce::String> tokens;

        for (auto* block : wrap)
        {
            if (block->isTag (Id::p))
            {
                const auto head { block->getAllSubText() };
                tokens.try_emplace (
                    Id::name, getBinding (row, jam::Format::getPostColon (head).trim()));
            }
            else if (block->isTag (Id::ul))
            {
                for (auto* item : *block)
                    if (item->isTag (Id::li))
                    {
                        const auto text { item->getAllSubText() };
                        const auto key { jam::Format::getPreColon (text).trim() };
                        const auto value { jam::Format::getPostColon (text).trim() };
                        tokens.try_emplace (juce::Identifier (key), getBinding (row, value));
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

    juce::String applyWrap (Model::Element& row,
                             Model::Element& wrap,
                             const juce::String& code,
                             int depth) const
    {
        auto tokens { getTokens (row, wrap) };

        if (code.isNotEmpty())
        {
            juce::String body { code };

            if (depth > 0)
            {
                const auto indent { juce::String::repeatedString (
                    juce::String::charToString (chars::space), depth * indentWidth) };
                juce::StringArray lines { juce::StringArray::fromLines (code) };

                for (auto& line : lines)
                    if (line.isNotEmpty())
                        line = indent + line;

                body = lines.joinIntoString (juce::String::charToString (chars::newline), 0, -1);
            }

            tokens.addOrReplace (Id::body, body);
        }

        const auto& wrapper {
            TemplateDocument::getOrCreate (model.getFile (row, getWrapAlias (wrap)))
        };
        return wrapper.build (model, row, {}, tokens).root->getAllSubText();
    }

    /** @brief Builds one row's body, applying every wrap from deepest to shallowest, excluding the outermost. */
    juce::String buildRow (Model::Element& row) const
    {
        const auto wraps { getWraps (row) };
        juce::String code;

        if (not wraps.isEmpty())
        {
            const auto tokens { getTokens (row, *wraps.at (wraps.size() - 1)) };

            if (tokens.contains (Id::body))
            {
                const auto binding { tokens.at (Id::body) };
                const auto castExtension { juce::String::charToString (chars::dot)
                                           + extensions::cast };

                code = (binding.endsWith (castExtension)
                           ? TemplateDocument::getOrCreate (model.getOutput (binding))
                                 .getExpansion (model, row, Id::body, binding)
                           : binding)
                          .trimCharactersAtEnd (juce::String::charToString (chars::newline));
            }
        }

        for (int depth { wraps.size() - 1 }; depth >= 1; --depth)
            code = applyWrap (row, *wraps.at (depth), code, depth);

        return code;
    }

    /** @brief Joins a file's rows through their inner wraps, then applies the shared outermost wrap once. */
    juce::String buildFile (const Elements& fileRows) const
    {
        auto* firstRow { fileRows.first() };
        const auto outermostWraps { getWraps (*firstRow) };
        const auto hasOutermostWrap { not outermostWraps.isEmpty() };

        juce::String code;

        for (auto* row : fileRows)
        {
            const auto rowCode { buildRow (*row) };

            if (code.isEmpty())
                code = rowCode;
            else
            {
                const auto lineBreakCell { model.getTableValue (*row, Id::lineBreak) };
                const auto separator {
                    lineBreakCell.isNotEmpty() ? getBinding (*row, lineBreakCell)
                                               : juce::String()
                };
                code << separator << rowCode;
            }
        }

        if (hasOutermostWrap)
            code = applyWrap (*firstRow, *outermostWraps.at (0), code, 0);

        return code;
    }

    juce::String getBanner (const juce::String& file) const noexcept
    {
        static const auto document { jam::MarkdownDocument::parse (
            BinaryData::getString (files::castOutput)) };

        const auto extension { jam::Format::onlyExtensionFromFilename (file) };
        juce::String banner;

        const auto& syntax { map::commentSyntax.at (extension) };

        banner << syntax.at (Id::bannerOpen) << chars::newline
               << document.getCodeBlock (Id::banner)->getAllSubText() << chars::newline
               << syntax.at (Id::bannerClose) << chars::newline
               << chars::newline << syntax.at (Id::pragma)
               << chars::newline << chars::newline;

        return banner;
    }

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Writer)
};
