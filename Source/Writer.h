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
        const auto path { jam::File::getOrCreateDirectory (outputPath) };
        const auto rows { model.getTableRows (Id::generated) };
        jam::Array<bool> written;
        written.resize (rows.size());

        {
            juce::ThreadPool writePool;

            for (int i { 0 }; i < rows.size(); ++i)
                writePool.addJob (
                    [this, &path, &rows, &written, i]
                    {
                        auto* row { rows[i] };
                        written[i] = toFile (
                            path, model.getTableValue (*row, Id::output), getTemplate (*row));
                    });

            while (writePool.getNumJobs() > 0)
                juce::Thread::sleep (1);
        }

        for (const auto& result : written)
            if (not result)
                return false;

        return toFile (path.getChildFile (model.getTableValue (**rows.begin(), Id::output))
                           .getParentDirectory(),
                       files::castGenerated,
                       getTemplate (BinaryData::getString (files::castGenerated), {}));
    }

private:
    const Document& model;

    //==============================================================================
    bool
    toFile (const juce::File& path, const juce::String& filename, const juce::String& content) const
    {
        auto file { path.getChildFile (filename) };
        file.create();
        return file.replaceWithText (content);
    }

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

        for (auto* patch : model.getTableRows (Id::patch))
        {
            const auto placeholder { model.getTableValue (*patch, Id::placeholder) };

            if (model.hasTableValue (*patch, Id::fragment))
                code = jam::Format::replaceholder (
                    code, placeholder, getPatch (*patch, code, separator));
            else
                code = jam::Format::replaceSection (code, placeholder, getPatch (*patch, code, {}));
        }

        return code;
    }

    juce::String
    getPatch (Element& patch, juce::StringRef code, juce::StringRef separator) const noexcept
    {
        const juce::Identifier source { model.getTableValue (patch, Id::source) };
        const auto placeholder { model.getTableValue (patch, Id::placeholder) };
        const auto fragment { getFragment (patch, code) };
        const auto columns { model.getTableHeaders (source) };
        juce::String expansion;

        for (auto* row : model.getTableRows (source))
        {
            if (const auto code { getCode (source, *row, fragment, placeholder, columns) };
                code.isNotEmpty())
            {
                if (expansion.isEmpty())
                    expansion = code;
                else
                    expansion << separator << code;
            }
        }

        return expansion;
    }

    juce::String getFragment (Element& patch, juce::StringRef code) const noexcept
    {
        if (model.hasTableValue (patch, Id::fragment))
            return model.getTemplate (model.getTableValue (patch, Id::fragment));

        return jam::Format::getSection (code, model.getTableValue (patch, Id::placeholder));
    }

    juce::String getCode (const juce::Identifier& source,
                          Element& row,
                          juce::StringRef fragment,
                          const juce::String& placeholder,
                          const jam::Array<juce::String>& columns) const noexcept
    {
        for (const auto& column : columns)
            if (not model.hasTableValue (row, column)
                and not (source == Id::lexicon and column.compare (Id::value.toString()) == 0)
                and (placeholder.startsWith (column) or hasColumn (fragment, column)))
                return {};

        auto code { juce::String (fragment.text) };

        for (const auto& column : columns)
        {
            const auto cell { model.getTableValue (row, column) };
            const auto resolved { source == Id::lexicon and column.compare (Id::value.toString()) == 0 and cell.isEmpty()
                                      ? model.getTableValue (row, Id::name)
                                      : cell };
            code = getSubstituted (code, column, resolved);

            if (column.compare (Id::entry) == 0)
            {
                const auto value { model.getTableValue (
                    Id::lexicon, Id::value, juce::Identifier (cell)) };
                code = getSubstituted (code, Id::value.toString(), value.isEmpty() ? cell : value);
                code = getSubstituted (code, Id::type.toString(),
                                       model.getTableValue (Id::lexicon, Id::type, juce::Identifier (cell)));
            }
        }

        return code;
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
                code = jam::Format::replaceholder (
                    code, placeholder, Transforms::getTransformed (name, cell));
        }

        return code;
    }

    bool hasColumn (juce::StringRef fragment, const juce::String& column) const noexcept
    {
        if (jam::Format::hasPlaceholder (fragment, column))
            return true;

        for (const auto& [name, transform] : Transforms::getTransforms())
            if (jam::Format::hasPlaceholder (
                    fragment, column + juce::String::charToString (chars::colon) + name))
                return true;

        return false;
    }

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Writer)
};
