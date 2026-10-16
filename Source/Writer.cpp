#include "Writer.h"

Writer::Writer (const Model& newModel, const TemplateDocument& newTemplateDocument)
    : model (newModel)
    , templateDocument (newTemplateDocument)
{
}

juce::String Writer::getText (const jam::Document& document) const
{
    return document.getRoot()->getAllSubText();
}

juce::Result Writer::toFile (const juce::File& outputPath) const
{
    static const auto newlineText { juce::String::charToString (Chars::newline) };

    jam::Strings failures;
    const auto tables { model.getTables() };

    for (auto* table : tables)
        if (model.isOutputTable (*table))
            failures.addArray (toFile (*table, outputPath), 0, -1);

    if (failures.size() > 0)
        return juce::Result::fail (failures.joinIntoString (newlineText, 0, -1));

    return juce::Result::ok();
}

jam::Array<int> Writer::getGroupStarts (const jam::Array<const Model::Element*>& rows) const
{
    jam::Array<int> groupStarts;

    for (int index { 0 }; index < rows.size(); ++index)
        if (index == 0
            or model.getValue (*rows.at (index), Id::file)
                   .compare (model.getValue (*rows.at (index - 1), Id::file)) != 0)
            groupStarts.add (index);

    return groupStarts;
}

jam::Array<juce::File> Writer::getOutputFiles (const juce::File& outputPath,
    const jam::Array<const Model::Element*>& rows, const jam::Array<int>& groupStarts) const
{
    jam::Array<juce::File> outputFiles;
    outputFiles.resize (groupStarts.size());

    for (int index { 0 }; index < groupStarts.size(); ++index)
    {
        const auto& firstRow { *rows.at (groupStarts.at (index)) };
        const auto& file { model.getValue (firstRow, Id::file) };

        if (model.isRegionRow (firstRow))
            outputFiles.at (index) = outputPath.getChildFile (file);
        else
        {
            outputFiles.at (index) = jam::File::getOrCreate (outputPath, file);
            outputFiles.at (index).getParentDirectory().createDirectory();
        }
    }

    return outputFiles;
}

juce::String Writer::toFile (const jam::Array<const Model::Element*>& rows,
    const jam::Array<const Model::Element*>& tables, const jam::Array<int>& groupStarts,
    const jam::Array<juce::File>& outputFiles, int index) const
{
    static const auto dotText { juce::String::charToString (Chars::dot) };

    const auto start { groupStarts.at (index) };
    const auto& outputFile { outputFiles.at (index) };
    const auto groupEnd { index + 1 < groupStarts.size() ? groupStarts.at (index + 1) : rows.size() };

    auto* firstLine { Shapes::getFirstLine (model, *rows.at (start)) };
    const auto fencePrefix { Transforms::getFencePrefix (*firstLine->get<juce::String> (Id::info)) };
    const auto noBanner { Id::noBanner.toString() };
    const auto extension { fencePrefix.isNotEmpty() and fencePrefix.compare (noBanner) != 0
                                ? dotText + fencePrefix
                                : Transforms::getCommentSyntaxKey (outputFile.getFileName()) };

    if (model.isRegionRow (*rows.at (start)))
        return toRegionFile (rows, tables, start, groupEnd, outputFile, extension);

    auto description { getFileDescription (*rows.at (start), outputFile.getFileName()) };

    if (description.isNotEmpty())
        description = Transforms::toCommentBlock (description, extension);

    const Model::Element* noBannerLine { nullptr };

    for (int rowIndex { start }; rowIndex < groupEnd and noBannerLine == nullptr; ++rowIndex)
        for (const auto& column : { Id::structure, Id::separator, Id::list })
            if (auto* structureScope { model.getTableCell (*rows.at (rowIndex), column) })
                structureScope->applyFunctionRecursively (
                    [&noBannerLine, &noBanner] (const Model::Element& candidate) -> bool
                    {
                        if (noBannerLine == nullptr and candidate.contains (Id::templatePath)
                            and Transforms::getFencePrefix (*candidate.get<juce::String> (Id::info))
                                    .compare (noBanner) == 0)
                            noBannerLine = &candidate;

                        return noBannerLine == nullptr;
                    });

    const auto banner { noBannerLine != nullptr ? juce::String{} : getBanner (extension, description) };

    jam::MarkdownDocument rendered;
    apply (rendered, tables, rows, start, groupEnd, extension);

    jam::MarkdownDocument output;
    output.addChild (*output.getRoot(), Id::text)
        ->add<juce::String> (Id::text, getBody (getText (rendered), banner));

    const auto current { outputFile.loadFileAsString() };
    const auto canonical { getText (output) };

    return canonical.compare (current) != 0 and not toFile (output, outputFile)
               ? outputFile.getFullPathName() + Id::diagnosticSeparator
                     + text::Diagnostics::failOutputWrite
               : juce::String{};
}

int Writer::getRunEnd (const jam::Array<const Model::Element*>& rows, int runStart, int groupEnd,
    const juce::String& beginValue, const juce::String& endValue) const
{
    auto runEnd { runStart + 1 };

    while (runEnd < groupEnd
           and Shapes::getRegionValue (model, templateDocument, *rows.at (runEnd), Id::begin)
                   .compare (beginValue) == 0
           and Shapes::getRegionValue (model, templateDocument, *rows.at (runEnd), Id::end)
                   .compare (endValue) == 0)
        ++runEnd;

    return runEnd;
}

jam::Strings Writer::getSplicedLines (const jam::Strings& lines, const jam::Array<const Model::Element*>& tables,
    const jam::Array<const Model::Element*>& rows, int runStart, int runEnd, const juce::String& extension,
    const juce::String& beginValue, const juce::String& endValue) const
{
    const auto [beginLine, endLine] { Transforms::getDelimiterLines (lines, beginValue, endValue) };

    jam::MarkdownDocument rendered;
    apply (rendered, tables, rows, runStart, runEnd, extension);

    jam::Strings renderedLines;
    renderedLines.addLines (getText (rendered).trimEnd());

    return Transforms::getSplicedLines (lines, beginLine, endLine, renderedLines, 0, renderedLines.size());
}

juce::String Writer::toRegionFile (const jam::Array<const Model::Element*>& rows,
    const jam::Array<const Model::Element*>& tables, int start, int groupEnd,
    const juce::File& outputFile, const juce::String& extension) const
{
    static const auto newlineText { juce::String::charToString (Chars::newline) };

    const auto current { outputFile.loadFileAsString() };
    auto lines (jam::Strings::fromLines (current));
    auto runStart { start };

    while (runStart < groupEnd)
    {
        const auto beginValue { Shapes::getRegionValue (model, templateDocument, *rows.at (runStart), Id::begin) };
        const auto endValue { Shapes::getRegionValue (model, templateDocument, *rows.at (runStart), Id::end) };
        const auto runEnd { getRunEnd (rows, runStart, groupEnd, beginValue, endValue) };
        lines = getSplicedLines (lines, tables, rows, runStart, runEnd, extension, beginValue, endValue);
        runStart = runEnd;
    }

    const auto canonical { Transforms::getJoinedText (lines, current) };

    return canonical.compare (current) != 0
               and not outputFile.replaceWithText (canonical, false, false, newlineText.toRawUTF8())
               ? outputFile.getFullPathName() + Id::diagnosticSeparator
                     + text::Diagnostics::failOutputWrite
               : juce::String{};
}

jam::Strings Writer::toFile (const Model::Element& table, const juce::File& outputPath) const
{
    jam::Strings failures;
    const auto tables { model.getTables() };
    const auto rows { model.getTableRows (table) };
    const auto groupStarts { getGroupStarts (rows) };
    const auto outputFiles { getOutputFiles (outputPath, rows, groupStarts) };

    jam::Array<juce::String> tableFailures;
    tableFailures.resize (groupStarts.size());

    Jobs::run (groupStarts.size(),
        [this, &rows, &groupStarts, &tables, &tableFailures, &outputFiles] (int index)
        { tableFailures.at (index) = toFile (rows, tables, groupStarts, outputFiles, index); });

    for (const auto& failedFile : tableFailures)
        if (failedFile.isNotEmpty())
            failures.add (failedFile);

    return failures;
}

juce::String Writer::getRowJoin (const Model::Element& firstRow) const
{
    auto* rowJoinLine { model.getRowJoin (firstRow) };

    if (rowJoinLine != nullptr)
        return templateDocument.getValue (
            model, firstRow, *rowJoinLine->get<juce::String> (Id::value));

    return {};
}

juce::String Writer::getFileDescription (const Model::Element& firstRow, const juce::String& file) const
{
    auto* firstLine { Shapes::getFirstLine (model, firstRow) };

    static const auto descriptionMarker { Model::getReservedName (Id::description) };

    auto* descriptionBinding { model.getBinding (firstRow, Id::structure, *firstLine, descriptionMarker) };

    if (descriptionBinding != nullptr)
    {
        const auto& descriptionValue { *descriptionBinding->get<juce::String> (Id::value) };

        if (Model::isAddress (descriptionValue))
        {
            auto* headerTable { model.getTable (firstRow, descriptionValue) };

            const auto fileName { jam::Format::toFileName (file) };
            const auto column { model.isColumnAddress (firstRow, descriptionValue)
                                     ? model.getColumn (firstRow, descriptionValue)
                                     : Id::description };

            for (auto* headerRow : model.getTableRows (*headerTable))
            {
                auto* fileCell { model.getTableCell (*headerRow, Id::file) };

                if (fileCell != nullptr
                    and model.getFileName (*fileCell->get<juce::String> (Id::value))
                            .compare (fileName)
                        == 0)
                    if (auto* headerDescriptionCell { model.getTableCell (*headerRow, column) })
                        return *headerDescriptionCell->get<juce::String> (Id::value);
            }
        }
    }

    return {};
}

void Writer::apply (jam::MarkdownDocument& output, const jam::Array<const Model::Element*>& tables,
           const jam::Array<const Model::Element*>& rows, int index, int groupEnd,
           const juce::String& extension) const
{
    static const auto newlineText { juce::String::charToString (Chars::newline) };

    auto* firstRow { rows.at (index) };
    const auto rowJoinValue { getRowJoin (*firstRow) };
    const auto joinText { rowJoinValue.isNotEmpty()
                              ? newlineText
                                    + Chars::newline + rowJoinValue
                                    + Chars::newline
                                    + Chars::newline
                              : newlineText };

    jam::Array<const Model::Element*> groupRows;
    jam::Array<const Model::Element*> groupLines;

    for (; index < groupEnd; ++index)
    {
        auto* row { rows.at (index) };
        groupRows.add (row);
        groupLines.add (Shapes::getFirstLine (model, *row));
    }

    output.addChild (*output.getRoot(), Id::text)
        ->add<juce::String> (Id::text,
            Shapes::getShape (model, templateDocument, tables, groupRows, groupLines, joinText,
                0, extension));

    output.addChild (*output.getRoot(), Id::text)
        ->add<juce::String> (Id::text, newlineText);
}

juce::String Writer::getBody (const juce::String& shapeText, const juce::String& banner)
{
    static const auto newlineText { juce::String::charToString (Chars::newline) };
    static const auto bannerMarker { Model::getReservedName (Id::banner) };

    const auto marker { Items::getMarker (shapeText, bannerMarker) };

    if (marker.isEmpty())
        return banner.isNotEmpty() ? banner + Chars::newline
                                         + Chars::newline
                                         + shapeText
                                   : shapeText;

    const auto substituted { jam::Format::replaceholder (shapeText,
        marker.substring (Id::tripleColon.length(), marker.length() - Id::tripleColon.length()),
        banner) };

    if (banner.isNotEmpty())
        return substituted;

    const auto shapeLines { jam::Strings::fromLines (shapeText) };
    const auto substitutedLines { jam::Strings::fromLines (substituted) };
    jam::Strings bodyLines;

    for (int lineIndex { 0 }; lineIndex < substitutedLines.size(); ++lineIndex)
        if (not (shapeLines.at (lineIndex).contains (marker)
                 and substitutedLines.at (lineIndex).trim().isEmpty()))
            bodyLines.add (substitutedLines.at (lineIndex));

    return bodyLines.joinIntoString (newlineText, 0, -1);
}

juce::String Writer::getBanner (const juce::String& extension, const juce::String& description) const
{
    static const auto document { jam::MarkdownDocument::parse (
        BinaryData::getString (files::castOutput)) };

    const auto& syntax { map::commentSyntax.at (extension) };
    juce::String banner;

    if (auto* bannerBlock { document.getCodeBlock (Id::banner) })
    {
        banner << (syntax.get (Id::bannerOpen).isNotEmpty()
                       ? syntax.get (Id::bannerOpen)
                             + Chars::newline
                             + bannerBlock->getAllSubText()
                             + Chars::newline
                             + syntax.get (Id::bannerClose)
                       : Transforms::toCommentBlock (bannerBlock->getAllSubText(), extension));

        if (description.isNotEmpty())
            banner << Chars::newline << Chars::newline << description;
    }

    return banner;
}
