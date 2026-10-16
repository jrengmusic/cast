#include "Validator.h"

juce::Result Validator::hasTemplate (const TemplateDocument& templateDocument, const Element& table,
    const Element& row, const juce::Identifier& column, const Element& line)
{
    if (templateDocument.getCodeBlock (line) == nullptr)
        return juce::Result::fail (getLocation (table, row, column.toString())
                                   + Id::diagnosticSeparator
                                   + text::Diagnostics::failTemplateMissing
                                   + Id::diagnosticSeparator + *line.get<juce::String> (Id::info));

    return juce::Result::ok();
}

juce::Result Validator::isValid (const Model& model, const TemplateDocument& templateDocument)
{
    return isManifest (model, templateDocument);
}

juce::Result Validator::isIndex (const Model& model)
{
    for (auto* indexTable : model.getTables (Id::index))
        for (auto* row : model.getTableRows (*indexTable))
        {
            const auto pathCell { model.getTableValue (*row, Id::symbol) };

            if (pathCell.isEmpty())
                return juce::Result::fail (getLocation (*indexTable, *row, Id::symbol.toString())
                                           + Id::diagnosticSeparator
                                           + text::Diagnostics::failNotFound);

            if (juce::File::createFileWithoutCheckingPath (pathCell)
                    .hasFileExtension (Extensions::md)
                and not model.getFile (pathCell).existsAsFile())
                return juce::Result::fail (getLocation (*indexTable, *row, Id::symbol.toString())
                                           + Id::diagnosticSeparator
                                           + text::Diagnostics::failOutputMissing
                                           + Id::diagnosticSeparator + pathCell);

            if (juce::File::createFileWithoutCheckingPath (pathCell)
                    .hasFileExtension (Extensions::cast)
                and not model.getFile (pathCell).existsAsFile())
                return juce::Result::fail (getLocation (*indexTable, *row, Id::symbol.toString())
                                           + Id::diagnosticSeparator
                                           + text::Diagnostics::failTemplateMissing
                                           + Id::diagnosticSeparator + pathCell);
        }

    return juce::Result::ok();
}

juce::Result Validator::isStructure (const Model& model, const TemplateDocument& templateDocument)
{
    for (auto* table : model.getTables())
        if (model.isOutputTable (*table))
            for (auto* row : model.getTableRows (*table))
            {
                auto* structureScope { model.getTableCell (*row, Id::structure) };

                if (structureScope == nullptr or model.getStructure (*structureScope).isEmpty())
                    return juce::Result::fail (
                        getLocation (*table, *row, Id::structure.toString())
                        + Id::diagnosticSeparator + text::Diagnostics::failStructureMissing);

                const Element* failingLine { nullptr };

                structureScope->applyFunctionRecursively (
                    [&failingLine, &templateDocument] (const Element& candidate) -> bool
                    {
                        if (failingLine == nullptr and candidate.contains (Id::templatePath)
                            and templateDocument.getCodeBlock (candidate) == nullptr)
                            failingLine = &candidate;

                        return failingLine == nullptr;
                    });

                if (failingLine != nullptr)
                    return hasTemplate (templateDocument, *table, *row, Id::structure, *failingLine);
            }

    return juce::Result::ok();
}

juce::Result Validator::isBindingCountValid (const Element& table, const Element& row,
    const juce::Identifier& column, const Element& scope)
{
    static const auto listMarker { Model::getReservedName (Id::list) };

    for (auto* child : scope)
    {
        if (child->isTag (Id::ul))
            for (auto* item : *child)
                if (item->id != listMarker and item->contains (Id::shape))
                {
                    const Element* first { nullptr };

                    scope.applyFunctionRecursively (
                        [&first, item] (const Element& candidate) -> bool
                        {
                            if (first == nullptr and candidate.parent->isTag (Id::ul)
                                and candidate.id == item->id and candidate.contains (Id::shape)
                                and Model::getOwner (candidate) == Model::getOwner (*item))
                                first = &candidate;

                            return first == nullptr;
                        });

                    if (first != item)
                        return juce::Result::fail (getLocation (table, row, column.toString())
                                                   + Id::diagnosticSeparator
                                                   + text::Diagnostics::failBindingDuplicate
                                                   + Id::diagnosticSeparator + item->id.toString());
                }

        if (child->isTag (Id::blockquote))
            if (const auto result { isBindingCountValid (table, row, column, *child) };
                not result.wasOk())
                return result;
    }

    return juce::Result::ok();
}

juce::Result Validator::isBindingCountValid (const Model& model)
{
    for (auto* table : model.getTables())
        if (model.isOutputTable (*table))
            for (auto* row : model.getTableRows (*table))
                for (const auto& column : { Id::structure, Id::separator })
                    if (auto* scope { model.getTableCell (*row, column) })
                        if (const auto result { isBindingCountValid (*table, *row, column, *scope) };
                            not result.wasOk())
                            return result;

    return juce::Result::ok();
}

juce::Result Validator::isMarkerCountValid (const TemplateDocument& templateDocument, const Element& table,
    const Element& row, const Element& structureScope)
{
    juce::String shapeId;

    structureScope.applyFunctionRecursively (
        [&shapeId, &templateDocument] (const Element& candidate) -> bool
        {
            if (shapeId.isEmpty() and candidate.contains (Id::templatePath))
            {
                const auto& blockText {
                    *templateDocument.getCodeBlock (candidate)->get<juce::String> (Id::value)
                };
                int count { 0 };
                int cursor { 0 };

                for (auto position { blockText.indexOf (cursor, Id::tripleColon) }; position >= 0;
                     position = blockText.indexOf (cursor, Id::tripleColon))
                {
                    ++count;
                    cursor = position + Id::tripleColon.length();
                }

                if (count % 2 != 0)
                    shapeId = *candidate.get<juce::String> (Id::info);
            }

            return shapeId.isEmpty();
        });

    if (shapeId.isNotEmpty())
        return juce::Result::fail (getLocation (table, row, Id::structure.toString())
                                   + Id::diagnosticSeparator + text::Diagnostics::failMarkerUnterminated
                                   + Id::diagnosticSeparator + shapeId);

    return juce::Result::ok();
}

juce::Result
Validator::isMarkerCountValid (const Model& model, const TemplateDocument& templateDocument)
{
    for (auto* table : model.getTables())
        if (model.isOutputTable (*table))
            for (auto* row : model.getTableRows (*table))
                if (auto* structureScope { model.getTableCell (*row, Id::structure) })
                    if (const auto result {
                            isMarkerCountValid (templateDocument, *table, *row, *structureScope) };
                        not result.wasOk())
                        return result;

    return juce::Result::ok();
}

juce::Result Validator::isReservedName (const TemplateDocument& templateDocument, const Element& table,
    const Element& row, const juce::Identifier& column, const Element& scope)
{
    static const jam::HashSet<juce::Identifier> templateNames {
        Model::getReservedName (Id::list),
        Model::getReservedName (Id::description),
        Model::getReservedName (Id::banner) };

    juce::String failingName;

    scope.applyFunctionRecursively (
        [&failingName, &templateDocument] (const Element& candidate) -> bool
        {
            if (failingName.isEmpty() and candidate.contains (Id::templatePath))
                for (const auto& interior : TemplateDocument::getMarkers (
                         *templateDocument.getCodeBlock (candidate)->get<juce::String> (Id::value)))
                    if (failingName.isEmpty() and interior.startsWithChar (Chars::openBracket)
                        and interior.endsWithChar (Chars::closeBracket)
                        and not templateNames.contains (juce::Identifier (jam::Format::toValidID (interior))))
                        failingName = interior;

            return failingName.isEmpty();
        });

    if (failingName.isNotEmpty())
        return juce::Result::fail (getLocation (table, row, column.toString())
                                   + Id::diagnosticSeparator + text::Diagnostics::failReservedName
                                   + Id::diagnosticSeparator + failingName);

    return juce::Result::ok();
}

juce::Result Validator::isReservedName (const Model& model, const TemplateDocument& templateDocument)
{
    static const jam::HashSet<juce::Identifier> bulletNames {
        Model::getReservedName (Id::list),
        Model::getReservedName (Id::description),
        Model::getReservedName (Id::begin),
        Model::getReservedName (Id::end) };

    for (auto* table : model.getTables())
        if (model.isOutputTable (*table))
            for (auto* row : model.getTableRows (*table))
                for (const auto& column : { Id::structure, Id::separator, Id::list })
                    if (auto* scope { model.getTableCell (*row, column) })
                        if (const auto result {
                                isReservedName (templateDocument, *table, *row, column, *scope) };
                            not result.wasOk())
                            return result;

    for (const auto& column : { Id::structure, Id::separator, Id::list })
        if (const auto result { forEachBinding (model, column,
                [&column] (const Element& table, const Element& row, const juce::Identifier& entryId,
                    const juce::String&) -> juce::Result
                {
                    if (entryId.toString().startsWithChar (Chars::underscore)
                        and entryId.toString().endsWithChar (Chars::underscore)
                        and not bulletNames.contains (entryId))
                        return juce::Result::fail (getLocation (table, row, column.toString())
                                                   + Id::diagnosticSeparator
                                                   + text::Diagnostics::failReservedName
                                                   + Id::diagnosticSeparator + entryId.toString());

                    return juce::Result::ok();
                }) };
            not result.wasOk())
            return result;

    return juce::Result::ok();
}

juce::Result Validator::isShapeSupplied (const Model& model, const Element& table, const Element& row,
    const juce::Identifier& column, const Element& precedingShape)
{
    juce::String sourceValue;

    if (not precedingShape.isTag (Id::p))
    {
        const auto indent { *precedingShape.get<int> (Id::level) };
        const auto ordinal { *precedingShape.get<int> (Id::line) };
        auto* sourceItem { model.getSource (row, indent, ordinal) };
        sourceValue = *sourceItem->get<juce::String> (Id::value);

        if (model.isColumnAddress (row, sourceValue))
            if (const auto result { hasTable (model, table, row, column, sourceValue) };
                not result.wasOk())
                return result;
    }

    return juce::Result::ok();
}

std::pair<juce::Result, const Validator::Element*> Validator::isPlaceholderScope (const Model& model,
    const TemplateDocument& templateDocument, const Element& table, const Element& row,
    const juce::Identifier& column, const Element& scope, const Element* precedingShape)
{
    static const auto listMarker { Model::getReservedName (Id::list) };

    for (auto* block : scope)
    {
        if (block->isTag (Id::p) and block->contains (Id::templatePath) and column == Id::structure)
        {
            if (precedingShape != nullptr)
                if (const auto result {
                        isShapeSupplied (model, table, row, column, *precedingShape) };
                    not result.wasOk())
                    return { result, precedingShape };

            precedingShape = block;
        }

        if (block->isTag (Id::ul))
            for (auto* item : *block)
            {
                if (item->contains (Id::templatePath))
                    if (const auto result { hasTemplate (templateDocument, table, row, column, *item) };
                        not result.wasOk())
                        return { result, precedingShape };

                if (item->id == listMarker and column == Id::structure)
                {
                    if (precedingShape != nullptr)
                        if (const auto result {
                                isShapeSupplied (model, table, row, column, *precedingShape) };
                            not result.wasOk())
                            return { result, precedingShape };

                    precedingShape = item;
                }
            }

        if (block->isTag (Id::blockquote))
        {
            auto [blockResult, blockShape] { isPlaceholderScope (model, templateDocument, table, row,
                column, *block, precedingShape) };

            precedingShape = blockShape;

            if (not blockResult.wasOk())
                return { blockResult, precedingShape };
        }
    }

    return { juce::Result::ok(), precedingShape };
}

juce::Result
Validator::isPlaceholders (const Model& model, const TemplateDocument& templateDocument)
{
    for (const auto& column : { Id::structure, Id::separator })
        for (auto* table : model.getTables())
            if (model.isOutputTable (*table))
                for (auto* row : model.getTableRows (*table))
                    if (auto* scope { model.getTableCell (*row, column) })
                    {
                        auto [scopeResult, precedingShape] { isPlaceholderScope (model, templateDocument,
                            *table, *row, column, *scope, nullptr) };

                        if (not scopeResult.wasOk())
                            return scopeResult;

                        if (precedingShape != nullptr and column == Id::structure)
                            if (const auto result {
                                    isShapeSupplied (model, *table, *row, column, *precedingShape) };
                                not result.wasOk())
                                return result;
                    }

    return juce::Result::ok();
}

juce::Result Validator::isSourceCountValid (const Model& model, const TemplateDocument& templateDocument)
{
    for (auto* table : model.getTables())
        if (model.isOutputTable (*table))
            for (auto* row : model.getTableRows (*table))
                if (model.getTableCell (*row, Id::structure) != nullptr)
                    if (auto* firstLine { Shapes::getFirstLine (model, *row) })
                        if (Items::getNextShapeLine (model, templateDocument, *firstLine) != nullptr)
                            return juce::Result::fail (getLocation (*table, *row, Id::structure.toString())
                                                       + Id::diagnosticSeparator
                                                       + text::Diagnostics::failAmbiguous);

    return juce::Result::ok();
}

juce::Result Validator::isItemSourceCountValid (const Model& model, const TemplateDocument& templateDocument)
{
    static const auto listMarker { Model::getReservedName (Id::list) };

    for (auto* table : model.getTables())
        if (model.isOutputTable (*table))
            for (auto* row : model.getTableRows (*table))
                if (auto* structureScope { model.getTableCell (*row, Id::structure) })
                {
                    const Element* overSuppliedItem { nullptr };

                    structureScope->applyFunctionRecursively (
                        [&overSuppliedItem, &templateDocument, &model, row] (const Element& candidate) -> bool
                        {
                            if (overSuppliedItem == nullptr and candidate.parent->isTag (Id::ul)
                                and candidate.id == listMarker and candidate.contains (Id::templatePath))
                            {
                                const auto arity { Items::getArity (templateDocument, candidate) };

                                if (arity >= 2
                                    and model.getColumnAddress (*row, *candidate.get<int> (Id::level),
                                            *candidate.get<int> (Id::line), arity) != nullptr)
                                    overSuppliedItem = &candidate;
                            }

                            return overSuppliedItem == nullptr;
                        });

                    if (overSuppliedItem != nullptr)
                        return juce::Result::fail (getLocation (*table, *row, Id::structure.toString())
                                                   + Id::diagnosticSeparator
                                                   + text::Diagnostics::failAmbiguous);
                }

    return juce::Result::ok();
}

juce::Result Validator::isPaired (const Model& model, const Element& table, const Element& row,
    const juce::Identifier& column, const Element& scope)
{
    static const auto listMarker { Model::getReservedName (Id::list) };
    static const auto descriptionMarker { Model::getReservedName (Id::description) };

    for (auto* block : scope)
    {
        if (block->isTag (Id::ul))
            for (auto* item : *block)
            {
                if (item->id == listMarker and column != Id::list and item->contains (Id::line))
                {
                    const auto indent { *item->get<int> (Id::level) };
                    const auto ordinal { *item->get<int> (Id::line) };

                    if (model.getSource (row, indent, ordinal) == nullptr)
                        return juce::Result::fail (getLocation (table, row, column.toString())
                                                   + Id::diagnosticSeparator
                                                   + text::Diagnostics::failOrphan);
                }

                if (item->id == descriptionMarker and column != Id::structure)
                {
                    const auto& value { *item->get<juce::String> (Id::value) };

                    if (Model::isAddress (value))
                    {
                        auto* referencedCodeBlock { model.getCodeBlock (
                            juce::Identifier (jam::Format::getPostColon (value).trim())) };

                        if (model.getTable (row, value) == nullptr
                            and (referencedCodeBlock == nullptr
                                 or not referencedCodeBlock->contains (Id::description)))
                            return juce::Result::fail (getLocation (table, row, column.toString())
                                                       + Id::diagnosticSeparator
                                                       + text::Diagnostics::failOrphan);
                    }
                }
            }

        if (block->isTag (Id::blockquote))
            if (const auto result { isPaired (model, table, row, column, *block) };
                not result.wasOk())
                return result;
    }

    return juce::Result::ok();
}

juce::Result Validator::isPaired (const Model& model)
{
    for (auto* table : model.getTables())
        if (model.isOutputTable (*table))
            for (auto* row : model.getTableRows (*table))
            {
                if (auto* structureScope { model.getTableCell (*row, Id::structure) })
                    if (const auto result {
                            isPaired (model, *table, *row, Id::structure, *structureScope) };
                        not result.wasOk())
                        return result;

                if (auto* separatorScope { model.getTableCell (*row, Id::separator) })
                    if (const auto result {
                            isPaired (model, *table, *row, Id::separator, *separatorScope) };
                        not result.wasOk())
                        return result;

                if (auto* listScope { model.getTableCell (*row, Id::list) })
                    if (const auto result { isPaired (model, *table, *row, Id::list, *listScope) };
                        not result.wasOk())
                        return result;
            }

    return juce::Result::ok();
}

juce::Result Validator::isReference (const Model& model)
{
    static const auto descriptionMarker { Model::getReservedName (Id::description) };

    for (const auto& column : { Id::structure, Id::separator, Id::list })
        if (const auto result { forEachBinding (model, column,
                [&model, &column] (const Element& table, const Element& row, const juce::Identifier& entryId,
                    const juce::String& entryValue) -> juce::Result
                {
                    if ((entryId != descriptionMarker or column == Id::structure)
                        and Model::isAddress (entryValue) and not model.isShape (row, entryValue))
                        return hasTable (model, table, row, column, entryValue);

                    return juce::Result::ok();
                }) };
            not result.wasOk())
            return result;

    return forEachEntry (model,
        [&model] (const Element& table, const Element& row, const juce::Identifier& column,
            const juce::String& entryValue) -> juce::Result
        {
            if (column != Id::description and column != Id::brief and Model::isAddress (entryValue))
                return hasTable (model, table, row, column, entryValue);

            return juce::Result::ok();
        });
}

juce::Result
Validator::isMap (const Model& model, const Element& table, const Element& row, const Element& scope)
{
    static const auto listMarker { Model::getReservedName (Id::list) };

    for (auto* block : scope)
    {
        if (block->isTag (Id::ul))
            for (auto* item : *block)
                if (item->id == listMarker and not item->contains (Id::line))
                {
                    if (not item->contains (Id::shape))
                        return juce::Result::fail (getLocation (table, row, Id::list.toString())
                                                   + Id::diagnosticSeparator
                                                   + text::Diagnostics::failMapOrphan);

                    const auto& value { *item->get<juce::String> (Id::value) };

                    if (value.isNotEmpty())
                    {
                        auto* mapTable { model.getTable (row, value) };

                        if (mapTable == nullptr)
                            return juce::Result::fail (getLocation (table, row, Id::list.toString())
                                                       + Id::diagnosticSeparator
                                                       + text::Diagnostics::failTableMissing
                                                       + Id::diagnosticSeparator + value);

                        if (not model.getTableHeaders (*mapTable).contains (Id::value.toString()))
                            return juce::Result::fail (getLocation (table, row, Id::list.toString())
                                                       + Id::diagnosticSeparator
                                                       + text::Diagnostics::failColumnUnknown
                                                       + Id::diagnosticSeparator + Id::value.toString());
                    }
                }

        if (block->isTag (Id::blockquote))
            if (const auto result { isMap (model, table, row, *block) }; not result.wasOk())
                return result;
    }

    return juce::Result::ok();
}

juce::Result Validator::isMap (const Model& model)
{
    for (auto* table : model.getTables())
        if (model.isOutputTable (*table))
            for (auto* row : model.getTableRows (*table))
                if (auto* scope { model.getTableCell (*row, Id::list) })
                    if (const auto result { isMap (model, *table, *row, *scope) };
                        not result.wasOk())
                        return result;

    return juce::Result::ok();
}

juce::Result Validator::hasTable (const Model& model, const Element& table, const Element& row,
    const juce::Identifier& column, const juce::String& entryValue)
{
    static const auto colonText { juce::String::charToString (Chars::colon) };
    static const auto equalsText { juce::String::charToString (Chars::equals) };

    const auto parts { jam::Strings::fromTokens (entryValue, colonText, {}) };
    const auto sourceName { parts.size() > 0 ? parts.at (0).trim() : juce::String{} };
    const auto isAlias { model.getValue (row, sourceName).isNotEmpty() };
    const auto columnIndex { isAlias ? 2 : 1 };

    if (isAlias and parts.size() <= 1)
        return juce::Result::ok();

    auto* referencedTable { model.getTable (row, entryValue) };

    if (referencedTable == nullptr)
        return juce::Result::fail (getLocation (table, row, column.toString())
                                   + Id::diagnosticSeparator
                                   + text::Diagnostics::failTableMissing
                                   + Id::diagnosticSeparator + entryValue);

    if (parts.size() > columnIndex)
    {
        const auto columnName { parts.at (columnIndex).upToFirstOccurrenceOf (
            equalsText, false, false).trim() };

        if (not model.getTableHeaders (*referencedTable)
                    .contains (jam::Format::toValidID (columnName)))
            return juce::Result::fail (getLocation (table, row, column.toString())
                                       + Id::diagnosticSeparator
                                       + text::Diagnostics::failColumnUnknown
                                       + Id::diagnosticSeparator + columnName);
    }

    return juce::Result::ok();
}

juce::Result Validator::isRegionPaired (const Model& model)
{
    for (auto* table : model.getTables())
        if (model.isOutputTable (*table))
            for (auto* row : model.getTableRows (*table))
                if (model.hasRegionBegin (*row) != model.hasRegionEnd (*row))
                    return juce::Result::fail (getLocation (*table, *row, Id::file.toString())
                                               + Id::diagnosticSeparator + text::Diagnostics::failRegionPair);

    return juce::Result::ok();
}

juce::Result Validator::isRegionFilePresent (const Model& model)
{
    for (auto* table : model.getTables())
        if (model.isOutputTable (*table))
            for (auto* row : model.getTableRows (*table))
                if (model.isRegionRow (*row))
                {
                    const auto& file { model.getValue (*row, Id::file) };

                    if (not model.getFile (file).existsAsFile())
                        return juce::Result::fail (getLocation (*table, *row, Id::file.toString())
                                                   + Id::diagnosticSeparator + text::Diagnostics::failRegionFile
                                                   + Id::diagnosticSeparator + file);
                }

    return juce::Result::ok();
}

juce::Result Validator::isRegionDelimited (
    const Model& model, const TemplateDocument& templateDocument, const Element& table, const Element& row)
{
    const auto [beginLine, endLine] { getRegionLines (model, templateDocument, row) };

    if (endLine >= 0)
        return juce::Result::ok();

    return juce::Result::fail (getLocation (table, row, Id::file.toString())
                               + Id::diagnosticSeparator + text::Diagnostics::failRegionDelimiter);
}

std::pair<int, int> Validator::getRegionLines (
    const Model& model, const TemplateDocument& templateDocument, const Element& row)
{
    const auto beginValue { Shapes::getRegionValue (model, templateDocument, row, Id::begin) };
    const auto endValue { Shapes::getRegionValue (model, templateDocument, row, Id::end) };
    const auto& file { model.getValue (row, Id::file) };
    const auto lines (jam::Strings::fromLines (model.getFile (file).loadFileAsString()));

    return Transforms::getDelimiterLines (lines, beginValue, endValue);
}

juce::Result Validator::isRegionDisjoint (const Model& model, const TemplateDocument& templateDocument)
{
    jam::HashMap<juce::String, jam::Array<std::pair<int, int>>> fileRegions;

    for (auto* table : model.getTables())
        if (model.isOutputTable (*table))
            for (auto* row : model.getTableRows (*table))
                if (model.isRegionRow (*row))
                {
                    const auto [beginLine, endLine] { getRegionLines (model, templateDocument, *row) };
                    auto [entry, inserted] { fileRegions.try_emplace (model.getValue (*row, Id::file)) };
                    auto& [entryFile, entryRegions] { *entry };
                    const auto region { std::make_pair (beginLine, endLine) };
                    const auto isRepeat { entryRegions.contains (region) and entryRegions.last() != region };

                    for (const auto& [otherBegin, otherEnd] : entryRegions)
                        if (isRepeat or (region != std::make_pair (otherBegin, otherEnd)
                                         and beginLine <= otherEnd and otherBegin <= endLine))
                            return juce::Result::fail (getLocation (*table, *row, Id::file.toString())
                                                       + Id::diagnosticSeparator
                                                       + text::Diagnostics::failRegionOverlap);

                    entryRegions.add (region);
                }

    return juce::Result::ok();
}

juce::Result Validator::isRegionDelimited (const Model& model, const TemplateDocument& templateDocument)
{
    for (auto* table : model.getTables())
        if (model.isOutputTable (*table))
            for (auto* row : model.getTableRows (*table))
                if (model.isRegionRow (*row))
                    if (const auto result { isRegionDelimited (model, templateDocument, *table, *row) };
                        not result.wasOk())
                        return result;

    return juce::Result::ok();
}

juce::Result Validator::isRegionExclusive (const Model& model)
{
    jam::HashMap<juce::String, bool> classifications;

    for (auto* table : model.getTables())
        if (model.isOutputTable (*table))
            for (auto* row : model.getTableRows (*table))
            {
                const auto& file { model.getValue (*row, Id::file) };
                const auto isRegion { model.isRegionRow (*row) };
                auto [entry, inserted] { classifications.try_emplace (file, isRegion) };
                auto& [entryFile, entryIsRegion] { *entry };

                if (not inserted and entryIsRegion != isRegion)
                    return juce::Result::fail (getLocation (*table, *row, Id::file.toString())
                                               + Id::diagnosticSeparator
                                               + text::Diagnostics::failRegionShared
                                               + Id::diagnosticSeparator + file);
            }

    return juce::Result::ok();
}

juce::Result Validator::isFormatted (const Model& model)
{
    for (auto* table : model.getTables())
    {
        auto* headerRow { model.getTableRow (*table, Id::headerRow) };

        for (auto* cell : *headerRow)
            if (cell->id == Id::format and cell->nextSibling != nullptr
                and cell->nextSibling->id == Id::format)
                return juce::Result::fail (getLocation (*table, *headerRow, Id::format.toString())
                                           + Id::diagnosticSeparator
                                           + text::Diagnostics::failFormatAdjacent);
    }

    return forEachCell (model, Id::format.toString(),
        [] (const Element& table, const Element& row, const juce::String& transform) -> juce::Result
        {
            if (transform.isNotEmpty() and not Transforms::contains (transform))
                return juce::Result::fail (getLocation (table, row, Id::format.toString())
                                           + Id::diagnosticSeparator
                                           + text::Diagnostics::failUnknownTransform
                                           + Id::diagnosticSeparator + transform);

            return juce::Result::ok();
        });
}

juce::Result Validator::isContiguous (const Model& model, const Element& table)
{
    jam::HashSet<juce::String> closedFiles;
    juce::String previousFile;

    for (auto* row : model.getTableRows (table))
    {
        const auto& file { model.getValue (*row, Id::file) };

        if (file.compare (previousFile) != 0)
        {
            if (previousFile.isNotEmpty())
                closedFiles.insert (previousFile);

            previousFile = file;
        }

        if (closedFiles.contains (file))
            return juce::Result::fail (getLocation (table, *row, Id::file.toString())
                                       + Id::diagnosticSeparator
                                       + text::Diagnostics::failDuplicate + file
                                       + Chars::doubleQuote);
    }

    return juce::Result::ok();
}

juce::Result Validator::isUniqueColumn (const Model& model, const Element& table,
    const jam::Array<const Element*>& rows, const juce::Identifier& column)
{
    jam::HashSet<juce::String> seen;

    for (auto* row : rows)
        if (auto* cell { model.getTableCell (*row, column) })
        {
            if (not Model::isAddress (Model::getRawText (*cell, model.jam::Document::getSource())))
                if (const auto& value { *cell->get<juce::String> (Id::value) }; value.isNotEmpty())
                {
                    if (seen.contains (value))
                        return juce::Result::fail (getLocation (table, *row, column.toString())
                                                   + Id::diagnosticSeparator
                                                   + text::Diagnostics::failDuplicate + value
                                                   + Chars::doubleQuote);

                    seen.insert (value);
                }
        }

    return juce::Result::ok();
}

juce::Result Validator::isUniqueTable (const Model& model, const Element& table)
{
    const auto rows { model.getTableRows (table) };

    for (const auto& column : { Id::name, Id::key, Id::alias, Id::file })
        if (const auto result { isUniqueColumn (model, table, rows, column) };
            not result.wasOk())
            return result;

    return juce::Result::ok();
}

juce::Result Validator::isUniqueAlias (const Model& model)
{
    for (auto* table : model.getTables())
        if (model.getTableHeaders (*table).contains (Id::alias.toString()))
        {
            jam::HashSet<juce::String> seen;

            for (auto* row : model.getTableRows (*table))
                if (auto* cell { model.getTableCell (*row, Id::alias) })
                {
                    const auto value { Model::getRawText (*cell, model.jam::Document::getSource()) };

                    if (seen.contains (value))
                        return juce::Result::fail (getLocation (*table, *row, Id::alias.toString())
                                                   + Id::diagnosticSeparator
                                                   + text::Diagnostics::failDuplicate + value
                                                   + Chars::doubleQuote);

                    seen.insert (value);
                }
        }

    return juce::Result::ok();
}

juce::Result Validator::isUnique (const Model& model)
{
    for (auto* table : model.getTables())
        if (not model.isOutputTable (*table) and not table->isTag (Id::index))
            if (const auto result { isUniqueTable (model, *table) }; not result.wasOk())
                return result;

    return juce::Result::ok();
}

juce::Result Validator::isFencePrefix (const Model& model)
{
    static const auto dotText { juce::String::charToString (Chars::dot) };

    const auto noBanner { Id::noBanner.toString() };

    for (auto* table : model.getTables())
        if (model.isOutputTable (*table))
            for (auto* row : model.getTableRows (*table))
                for (const auto& column : { Id::structure, Id::separator, Id::list })
                if (auto* structureScope { model.getTableCell (*row, column) })
                {
                    const Element* failingLine { nullptr };

                    structureScope->applyFunctionRecursively (
                        [&failingLine, &noBanner] (const Element& candidate) -> bool
                        {
                            if (failingLine == nullptr and candidate.contains (Id::templatePath))
                            {
                                const auto& fenceName { *candidate.get<juce::String> (Id::info) };

                                if (fenceName.startsWithChar (Chars::openBracket))
                                {
                                    const auto word { Transforms::getFencePrefix (fenceName) };

                                    if (word.isEmpty()
                                        or (word.compare (noBanner) != 0
                                            and not map::commentSyntax.contains (
                                                dotText + word)))
                                        failingLine = &candidate;
                                }
                            }

                            return failingLine == nullptr;
                        });

                    if (failingLine != nullptr)
                        return juce::Result::fail (getLocation (*table, *row, column.toString())
                                                   + Id::diagnosticSeparator
                                                   + text::Diagnostics::failFencePrefix
                                                   + Id::diagnosticSeparator
                                                   + *failingLine->get<juce::String> (Id::info));
                }

    return juce::Result::ok();
}

juce::Result Validator::isToolchain (const Model& model)
{
    for (auto* table : model.getTables (Id::toolchain))
    {
        auto* headerRow { model.getTableRow (*table, Id::headerRow) };

        if (model.getTableCell (*headerRow, Id::command) == nullptr
            or model.getTableCell (*headerRow, Id::flag) == nullptr)
            return juce::Result::fail (getLocation (*table, *headerRow, Id::toolchain.toString())
                                       + Id::diagnosticSeparator
                                       + text::Diagnostics::failToolchainColumn);
    }

    return juce::Result::ok();
}

juce::Result Validator::isFormat (const Model& model)
{
    for (auto* table : model.getTables (Id::format))
    {
        auto* headerRow { model.getTableRow (*table, Id::headerRow) };

        if (model.getTableCell (*headerRow, Id::name) == nullptr
            or model.getTableCell (*headerRow, Id::width) == nullptr)
            return juce::Result::fail (getLocation (*table, *headerRow, Id::format.toString())
                                       + Id::diagnosticSeparator
                                       + text::Diagnostics::failFormatColumn);

        if (const auto result { isUniqueTable (model, *table) }; not result.wasOk())
            return result;

        for (auto* row : model.getTableRows (*table))
        {
            const auto width { model.getValue (*row, Id::width) };

            if (juce::String (width.getIntValue()).compare (width) != 0 or width.getIntValue() <= 0)
                return juce::Result::fail (getLocation (*table, *row, Id::width.toString())
                                           + Id::diagnosticSeparator
                                           + text::Diagnostics::failFormatWidth);
        }
    }

    return juce::Result::ok();
}

juce::Result Validator::isManifest (const Model& model, const TemplateDocument& templateDocument)
{
    if (const auto result { isIndex (model) }; not result.wasOk())
        return result;

    if (const auto result { isToolchain (model) }; not result.wasOk())
        return result;

    if (const auto result { isUniqueAlias (model) }; not result.wasOk())
        return result;

    if (const auto result { isUnique (model) }; not result.wasOk())
        return result;

    if (const auto result { isFormatted (model) }; not result.wasOk())
        return result;

    for (auto* table : model.getTables())
        if (model.isOutputTable (*table))
            if (const auto result { isContiguous (model, *table) }; not result.wasOk())
                return result;

    if (const auto result { isRegionExclusive (model) }; not result.wasOk())
        return result;

    if (const auto result { isRegionPaired (model) }; not result.wasOk())
        return result;

    if (const auto result { isRegionFilePresent (model) }; not result.wasOk())
        return result;

    if (const auto result { isRegionDelimited (model, templateDocument) }; not result.wasOk())
        return result;

    if (const auto result { isRegionDisjoint (model, templateDocument) }; not result.wasOk())
        return result;

    if (const auto result { isFencePrefix (model) }; not result.wasOk())
        return result;

    if (const auto result { isStructure (model, templateDocument) }; not result.wasOk())
        return result;

    if (const auto result { isBindingCountValid (model) }; not result.wasOk())
        return result;

    if (const auto result { isMarkerCountValid (model, templateDocument) }; not result.wasOk())
        return result;

    if (const auto result { isReservedName (model, templateDocument) }; not result.wasOk())
        return result;

    if (const auto result { isPaired (model) }; not result.wasOk())
        return result;

    if (const auto result { isReference (model) }; not result.wasOk())
        return result;

    if (const auto result { isMap (model) }; not result.wasOk())
        return result;

    if (const auto result { isPlaceholders (model, templateDocument) }; not result.wasOk())
        return result;

    if (const auto result { isSourceCountValid (model, templateDocument) }; not result.wasOk())
        return result;

    return isItemSourceCountValid (model, templateDocument);
}
