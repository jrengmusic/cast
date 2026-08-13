#pragma once
#include <JuceHeader.h>
#include "Model.h"
#include "Operators.h"

struct Writer
{
    // Ensure Element is the actual element type, not a container
    using Element = jam::Document::Element;

    Writer (const Document& document)
        : model (document)
    {
    }

    bool toFile (const juce::File& outputPath)
    {
        const auto rows { model.getTableRows (Id::generated) };

        if (std::all_of (rows.begin(), rows.end(), forEachFile (outputPath)))
        {
            auto path { jam::File::getOrCreateDirectory (outputPath) };
            auto file { path.getChildFile (model.getTableValue (**rows.begin(), Id::output))
                            .getParentDirectory()
                            .getChildFile (files::castGenerated) };

            file.create();
            return file.replaceWithText (getTemplate (BinaryData::getString (files::castGenerated), {}));
        }

        return false;
    }

private:
    const Document& model;

    //==============================================================================
    juce::String getTemplate (Element& row) const noexcept
    {
        return getTemplate (model.getTemplate (model.getTableValue (row, Id::templatePath)),
                            model.getTemplate (model.getTableValue (row, Id::separator)));
    }

    juce::String getTemplate (juce::String source, juce::StringRef separator) const noexcept
    {
        auto bannerDoc { jam::MarkdownDocument::parse (BinaryData::getString (files::castOutput)) };
        juce::String code;

        code << bannerDoc.getCodeBlock (Id::banner)->getAllSubText() << chars::newline
             << chars::newline << source;

        const auto patches { model.getTableRows (Id::patch) };

        return std::accumulate (patches.begin(), patches.end(), code,
                                [this, &separator] (juce::String code, Element* patch)
                                {
                                    const auto placeholder { model.getTableValue (*patch, Id::placeholder) };

                                    if (model.getTableValue (*patch, Id::fragment).isEmpty())
                                        return jam::Format::replaceSection (code, placeholder,
                                                                            getPatch (*patch, code, {}));

                                    return jam::Format::replaceholder (code, placeholder,
                                                                       getPatch (*patch, code, separator));
                                });
    }

    juce::String getPatch (Element& patch, juce::StringRef code, juce::StringRef separator) const noexcept
    {
        const juce::Identifier source { model.getTableValue (patch, Id::source) };
        const auto placeholder { model.getTableValue (patch, Id::placeholder) };
        const auto fragment { getFragment (patch, code) };
        const auto columns { model.getTableHeaders (source) };
        const auto rows { model.getTableRows (source) };

        return std::accumulate (
            rows.begin(), rows.end(), juce::String(),
            [this, &fragment, &columns, &placeholder, &separator] (juce::String expansion, Element* row)
            {
                const auto selected { std::all_of (
                    columns.begin(), columns.end(),
                    [this, &placeholder, row] (const juce::String& column)
                    {
                        return not placeholder.startsWith (column)
                            or model.hasTableValue (*row, column);
                    }) };

                const auto code { selected ? getCode (*row, fragment, columns) : juce::String() };

                if (code.isEmpty())
                    return expansion;

                if (expansion.isEmpty())
                    return code;

                return expansion + separator + code;
            });
    }

    juce::String getFragment (Element& patch, juce::StringRef code) const noexcept
    {
        const auto file { model.getTableValue (patch, Id::fragment) };

        if (file.isEmpty())
            return jam::Format::getSection (code, model.getTableValue (patch, Id::placeholder));

        return model.getTemplate (file);
    }

    juce::String getCode (Element& row,
                          juce::StringRef fragment,
                          const jam::Array<juce::String>& columns) const noexcept
    {
        const auto complete { std::all_of (
            columns.begin(), columns.end(),
            [this, &fragment, &row] (const juce::String& column)
            {
                const auto referenced { jam::Format::hasPlaceholder (fragment, column)
                    or std::any_of (Transforms::getTransforms().begin(),
                                    Transforms::getTransforms().end(),
                                    [&fragment, &column] (const auto& entry)
                                    {
                                        const auto& [name, transform] = entry;
                                        return jam::Format::hasPlaceholder (
                                            fragment, column + juce::String::charToString (chars::colon) + name);
                                    }) };

                return model.hasTableValue (row, column) or not referenced;
            }) };

        if (complete)
            return std::accumulate (
                columns.begin(), columns.end(), juce::String (fragment.text),
                [this, &row] (juce::String code, const juce::String& column)
                {
                    const auto cell { model.getTableValue (row, column) };
                    code = getSubstituted (code, column, cell);

                    if (column == Id::entry.toString())
                    {
                        const auto value { model.getTableValue (Id::lexicon, Id::value,
                                                                juce::Identifier (cell)) };
                        code = getSubstituted (code, Id::value.toString(),
                                               value.isEmpty() ? cell : value);
                    }

                    return code;
                });

        return {};
    }

    juce::String getSubstituted (juce::String code,
                                 const juce::String& column,
                                 const juce::String& cell) const noexcept
    {
        code = jam::Format::replaceholder (code, column, cell);

        for (const auto& [name, transform] : Transforms::getTransforms())
        {
            const auto placeholder { column + juce::String::charToString (chars::colon) + name };

            if (jam::Format::hasPlaceholder (code, placeholder))
                code = jam::Format::replaceholder (code, placeholder,
                                                   Transforms::getTransformed (name, cell));
        }

        return code;
    }

    std::function<bool (Element*)> forEachFile (const juce::File& outputPath) const
    {
        auto path { jam::File::getOrCreateDirectory (outputPath) };

        return [this, &path] (Element* row) -> bool
        {
            auto filename { model.getTableValue (*row, Id::output) };
            auto file { path.getChildFile (filename) };
            file.create();
            return file.replaceWithText (getTemplate (*row));
        };
    }

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Writer)
};
