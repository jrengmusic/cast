#pragma once
#include <JuceHeader.h>
#include "Model.h"
#include "TemplateDocument.h"

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
                            getBanner (file) + getFile (fileRows),
                            false,
                            false,
                            juce::String::charToString (Chars::newline).toRawUTF8());
                    });
            }
        }

        return written;
    }

private:
    const Model& model;

    juce::String getBinding (Model::Element& row,
                             const juce::String& value,
                             const juce::Identifier& jack) const
    {
        if (value.startsWithChar (Chars::at)
            and jam::Format::getPostColon (value).containsChar (Chars::colon))
            return model.getEntry (row, value);

        if (value.startsWithChar (Chars::at))
        {
            const auto symbol { model.getValue (row, value) };
            return TemplateDocument::getContent (
                model, row, symbol.isNotEmpty() ? symbol : value, jack);
        }

        return value;
    }

    juce::String applyWrap (Model::Element& row,
                             Model::Element& wrap,
                             const juce::String& code,
                             int depth) const
    {
        auto tokens { TemplateDocument::getTokens (model, row, wrap) };

        if (code.isNotEmpty())
            tokens.addOrReplace (Id::body, code);

        if (depth > 0 and code.isNotEmpty())
        {
            const auto indent { juce::String::repeatedString (
                juce::String::charToString (Chars::space),
                depth * TemplateDocument::indentWidth) };

            const auto indented { indent
                                  + code.replace (
                                      juce::String::charToString (Chars::newline),
                                      juce::String::charToString (Chars::newline) + indent) };

            tokens.addOrReplace (Id::body, indented);
        }

        const auto& wrapper { TemplateDocument::getOrCreate (
            model.getFile (row, TemplateDocument::getWrapAlias (model, wrap))) };
        return wrapper.build (model, row, {}, tokens).root->getAllSubText()
                   .trimCharactersAtEnd (juce::String::charToString (Chars::newline));
    }

    juce::String getRow (Model::Element& row) const
    {
        const auto wraps { TemplateDocument::getWraps (model, row) };
        juce::String code;

        if (not wraps.isEmpty())
        {
            const auto tokens { TemplateDocument::getTokens (
                model, row, *wraps.at (wraps.size() - 1)) };

            if (tokens.contains (Id::body))
                code = tokens.at (Id::body);
        }

        for (int depth { wraps.size() - 1 }; depth >= 1; --depth)
            code = applyWrap (row, *wraps.at (depth), code, depth);

        return code;
    }

    juce::String getFile (const Elements& fileRows) const
    {
        auto* firstRow { fileRows.first() };
        const auto outermostWraps { TemplateDocument::getWraps (model, *firstRow) };
        const auto hasOutermostWrap { not outermostWraps.isEmpty() };

        juce::String code;

        for (auto* row : fileRows)
        {
            const auto rowCode { getRow (*row) };

            if (code.isEmpty())
                code = rowCode;
            else
            {
                const auto separatorCell { model.getTableValue (*row, Id::separator) };

                if (separatorCell.startsWithChar (Chars::dash))
                {
                    const auto prefix { jam::Format::getPreColon (separatorCell).trim() };
                    const auto reference { jam::Format::getPostColon (separatorCell).trim() };
                    const auto resolved { getBinding (*row, reference, Id::separator) };

                    if (prefix.endsWith (Id::line.toString()))
                    {
                        code << Chars::newline << Chars::newline
                             << resolved
                             << Chars::newline << Chars::newline
                             << rowCode;
                    }
                    else
                    {
                        code << resolved << rowCode;
                    }
                }
                else if (separatorCell.isNotEmpty())
                {
                    const auto resolved { getBinding (*row, separatorCell, Id::separator) };
                    code << resolved << rowCode;
                }
                else
                {
                    code << Chars::newline << rowCode;
                }
            }
        }

        if (hasOutermostWrap)
            code = applyWrap (*firstRow, *outermostWraps.at (0), code,
                               outermostWraps.at (0)->isTag (Id::blockquote) ? 1 : 0);

        return code;
    }

    juce::String getBanner (const juce::String& file) const noexcept
    {
        static const auto document { jam::MarkdownDocument::parse (
            BinaryData::getString (files::castOutput)) };

        const auto extension { jam::Format::onlyExtensionFromFilename (file) };
        juce::String banner;

        const auto& syntax { map::commentSyntax.at (extension) };

        banner << syntax.at (Id::bannerOpen) << Chars::newline
               << document.getCodeBlock (Id::banner)->getAllSubText() << Chars::newline
               << syntax.at (Id::bannerClose) << Chars::newline
               << Chars::newline << syntax.at (Id::pragma)
               << Chars::newline << Chars::newline;

        return banner;
    }

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Writer)
};
