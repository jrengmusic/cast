#pragma once
#include <JuceHeader.h>
#include "Model.h"
#include "TemplateDocument.h"

struct Writer : jam::Document::Writer
{
    using Elements = jam::Array<jam::Document::Element*>;
    using jam::Document::Writer::toFile;

    Writer (const Model& document)
        : model (document)
    {
    }

    juce::String getText (const jam::Document& document) const override
    {
        return document.root->getAllSubText();
    }

    bool toFile (const juce::File& outputPath)
    {
        jam::Array<juce::File> templateFiles;

        for (auto* indexTable : model.getTables (Id::index))
            for (auto* indexRow : model.getTableRows (*indexTable))
            {
                const auto pathCell { model.getTableValue (*indexRow, Id::symbol) };

                if (juce::File::createFileWithoutCheckingPath (pathCell).hasFileExtension (
                        Extensions::cast))
                    templateFiles.addIfNotAlreadyThere (model.getOutput (pathCell));
            }

        runJobs (templateFiles.size(),
            [&templateFiles] (int index)
            {
                TemplateDocument::getOrCreate (templateFiles.at (index));
            });

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

                        TemplateDocument output;
                        output.addChild (*output.root, Id::text)
                            ->add<juce::String> (Id::text, getBanner (file));
                        getFile (output, fileRows);

                        const auto outputFile { jam::File::getOrCreate (outputPath, file) };

                        if (not toFile (output, outputFile))
                            written = false;
                    });
            }
        }

        return written;
    }

private:
    const Model& model;

    juce::String applyWrap (Model::Element& row,
                            Model::Element& wrap,
                            int depth,
                            const juce::String& innerCode,
                            const jam::Array<juce::Identifier>& expansionKeys,
                            bool injectInner) const
    {
        const auto alias { TemplateDocument::getWrapAlias (model, wrap) };
        auto tokens { TemplateDocument::getTokens (model, row, depth, wrap) };
        const auto& wrapperDocument { TemplateDocument::getOrCreate (model.getFile (row, alias)) };

        if (injectInner)
        {
            const auto& templatePlaceholders { wrapperDocument.getPlaceholders() };

            jam::Array<juce::Identifier> injectionKeys;

            for (const auto& placeholder : templatePlaceholders)
                if (expansionKeys.contains (placeholder))
                    injectionKeys.add (placeholder);

            jassert (injectionKeys.size() == 1);

            if (injectionKeys.size() != 1)
                jam::debug::Log::write (
                    jam::MarkdownValidator::getLocation (
                        *row.parent, row, Id::structure.toString())
                    + Id::diagnosticSeparator + text::Diagnostics::failNoSource);

            if (injectionKeys.size() == 1)
            {
                const auto indentedCode { depth == 0 or innerCode.isEmpty()
                                             ? innerCode
                                             : juce::String::repeatedString (
                                                   juce::String::charToString (Chars::space),
                                                   depth * TemplateDocument::indentWidth)
                                                   + innerCode.replace (
                                                       juce::String::charToString (Chars::newline),
                                                       juce::String::charToString (Chars::newline)
                                                           + juce::String::repeatedString (
                                                               juce::String::charToString (
                                                                   Chars::space),
                                                               depth
                                                                   * TemplateDocument::indentWidth)) };
                tokens.addOrReplace (injectionKeys.at (0), indentedCode);
            }
        }

        return wrapperDocument.build (model, row, row, depth, tokens)
                   .trimCharactersAtEnd (juce::String::charToString (Chars::newline));
    }

    juce::String getRow (Model::Element& row) const
    {
        const auto wraps { TemplateDocument::getWraps (model, row) };

        jam::Array<juce::Identifier> expansionKeys;

        for (int wiringDepth { 0 }; wiringDepth < wraps.size(); ++wiringDepth)
            for (auto& [wiringKey, wiringValue] : model.getSource (row, wiringDepth))
                expansionKeys.addIfNotAlreadyThere (wiringKey);

        juce::String code;
        bool hasInnerContent { false };

        for (int depth { wraps.size() - 1 }; depth >= 1; --depth)
        {
            auto* wrap { wraps.at (depth) };
            const auto alias { TemplateDocument::getWrapAlias (model, *wrap) };

            if (alias.isNotEmpty())
            {
                code = applyWrap (row, *wrap, depth, code, expansionKeys, hasInnerContent);
                hasInnerContent = true;
            }
            else if (not hasInnerContent)
            {
                const auto tokens { TemplateDocument::getTokens (model, row, depth, *wrap) };

                for (const auto& key : expansionKeys)
                    if (tokens.contains (key))
                    {
                        code = tokens.at (key);
                        hasInnerContent = true;
                    }
            }
        }

        return code;
    }

    void getFile (TemplateDocument& output, const Elements& fileRows) const
    {
        auto* firstRow { fileRows.first() };
        const auto wraps { TemplateDocument::getWraps (model, *firstRow) };
        const auto hasOutermostWrap { wraps.size() > 1 };

        const auto separatorCell { model.getTableValue (*firstRow, Id::separator) };
        auto joinText { juce::String::charToString (Chars::newline) };

        if (separatorCell.isNotEmpty() and not separatorCell.startsWithChar (Chars::dash))
        {
            const auto resolved {
                TemplateDocument::getBinding (model, *firstRow, 0, separatorCell, Id::separator)
            };

            joinText = juce::String::charToString (Chars::newline)
                     + juce::String::charToString (Chars::newline) + resolved
                     + juce::String::charToString (Chars::newline)
                     + juce::String::charToString (Chars::newline);
        }

        if (hasOutermostWrap)
        {
            jam::Strings rowTexts;

            for (auto* row : fileRows)
                rowTexts.add (getRow (*row));

            constexpr int depth { 0 };

            jam::Array<juce::Identifier> expansionKeys;

            for (int wiringDepth { 0 }; wiringDepth < wraps.size(); ++wiringDepth)
                for (auto& [wiringKey, wiringValue] : model.getSource (*firstRow, wiringDepth))
                    expansionKeys.addIfNotAlreadyThere (wiringKey);

            const auto code { rowTexts.joinIntoString (joinText, 0, -1) };
            const auto wrapped {
                applyWrap (*firstRow, *wraps.at (depth), depth, code, expansionKeys, true)
            };

            output.addChild (*output.root, Id::text)->add<juce::String> (Id::text, wrapped);
        }
        else
        {
            for (int index { 0 }; index < fileRows.size(); ++index)
            {
                if (index > 0)
                    output.addChild (*output.root, Id::text)->add<juce::String> (Id::text, joinText);

                output.addChild (*output.root, Id::text)
                    ->add<juce::String> (Id::text, getRow (*fileRows.at (index)));
            }
        }
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
