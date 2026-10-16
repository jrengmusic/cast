#include "Model.h"

std::unique_ptr<Model> Model::parse (const juce::File& documentFile)
{
    auto document { std::make_unique<Model>() };

    if (documentFile.existsAsFile())
        parse (*document, documentFile.loadFileAsString(),
            documentFile.getRelativePathFrom (documentFile.getParentDirectory()),
            documentFile.getParentDirectory());

    return document;
}

std::unique_ptr<Model> Model::parse (const juce::String& text, const juce::String& origin,
    const juce::File& directory, const jam::HashMap<juce::Identifier, int>& reflowWidths)
{
    auto document { std::make_unique<Model>() };
    document->reflowWidths = reflowWidths;
    parse (*document, text, origin, directory);

    return document;
}

jam::HashMap<juce::Identifier, int> Model::getReflowWidths() const
{
    jam::HashMap<juce::Identifier, int> newReflowWidths;

    for (auto* table : getTables (Id::format))
        for (auto* row : getTableRows (*table))
            if (row->id != Id::lineWrap)
            {
                auto* nameCell { getTableCell (*row, Id::name) };
                const auto nameView { getValueView (*nameCell) };

                newReflowWidths.try_emplace (
                    juce::Identifier (jam::Format::toValidID (jam::Strings::fromView (nameView))),
                    getValue (*row, Id::width).getIntValue());
            }

    return newReflowWidths;
}

int Model::getLineWrap() const
{
    for (auto* table : getTables (Id::format))
        if (auto* row { getTableRow (*table, Id::lineWrap) })
            return getValue (*row, Id::width).getIntValue();

    return jam::MarkdownWriter::defaultLineWrap;
}

const juce::String& Model::getManifestOrigin() const noexcept
{
    return manifestOrigin;
}

juce::String Model::getValue (juce::StringRef file, juce::StringRef alias) const
{
    if (alias.isNotEmpty() and *alias.text == Chars::at)
    {
        const juce::String aliasText { alias };

        if (juce::Identifier::isValidIdentifier (aliasText))
        {
            for (auto* table : *this)
            {
                if (table->id == Id::index)
                {
                    if (table->contains (Id::path)
                        and table->get<juce::String> (Id::path)->compare (juce::String (file)) == 0)
                    {
                        if (auto* symbolCell {
                                getTableCell (*table, Id::symbol, juce::Identifier (aliasText)) })
                            return *symbolCell->get<juce::String> (Id::value);
                    }
                }
            }
        }
    }

    return {};
}

juce::String Model::getValue (const Element& row, const juce::String& alias) const
{
    const auto origin { *row.parent->get<juce::String> (Id::path) };
    return getValue (origin, alias);
}

const juce::String& Model::getValue (const Element& row, const juce::Identifier& column) const
{
    return *getTableCell (row, column)->get<juce::String> (Id::value);
}

juce::String Model::getColumnValue (const Element& row, const juce::Identifier& column) const
{
    auto* cell { getTableCell (row, column) };

    return cell != nullptr ? *cell->get<juce::String> (Id::value) : juce::String {};
}

bool Model::isShape (const Element& row, const juce::String& value) const
{
    return isAddress (value)
           and juce::File::createFileWithoutCheckingPath (
                   getValue (row, jam::Format::getPreColon (value).trim()))
                   .hasFileExtension (Extensions::cast);
}

juce::Identifier Model::getReservedName (const juce::Identifier& word)
{
    return juce::Identifier (jam::Format::toValidID (
        jam::Format::withEnclosure (word.toString(), Chars::openBracket)));
}

bool Model::isAddress (const juce::String& value) noexcept
{
    return value.startsWithChar (Chars::at);
}

juce::String Model::getStructure (const Element& scope) const
{
    for (auto* block : scope)
    {
        if (block->isTag (Id::p) and block->contains (Id::templatePath))
            return *block->get<juce::String> (Id::templatePath);

        if (block->isTag (Id::ul) and block->firstChild != nullptr
            and block->firstChild->contains (Id::templatePath))
            return *block->firstChild->get<juce::String> (Id::templatePath);
    }

    return {};
}

const Model::Element* Model::getSource (const Element& row, int indent, int ordinal) const
{
    return getPairedListItem (row, Id::list, indent, ordinal);
}

const Model::Element* Model::getColumnAddress (const Element& row, int indent, int sourceOrdinal, int index) const
{
    for (int offset { 0 }; offset <= index; ++offset)
    {
        auto* childSourceLine { getSource (row, indent, sourceOrdinal + 1 + offset) };

        if (childSourceLine == nullptr)
            return nullptr;

        if (not isColumnAddress (row, *childSourceLine->get<juce::String> (Id::value)))
            return nullptr;

        if (offset == index)
            return childSourceLine;
    }

    return nullptr;
}

const Model::Element* Model::getSeparator (const Element& row, int indent, int ordinal) const
{
    return getPairedListItem (row, Id::separator, indent, ordinal);
}

const Model::Element* Model::getRowJoin (const Element& row) const
{
    static const auto listMarker { getReservedName (Id::list) };

    return getPairedItem (row, Id::separator,
        [] (const Element& candidate)
        { return candidate.id == listMarker and not candidate.contains (Id::line); });
}

const Model::Element* Model::getDescription (const Element& row, const Element& line) const
{
    static const auto descriptionMarker { getReservedName (Id::description) };

    return getPairedItem (row, Id::list,
        [&line] (const Element& candidate)
        {
            return candidate.id == descriptionMarker and candidate.contains (Id::owner)
                   and *candidate.get<int> (Id::owner) == *line.get<int> (Id::shape);
        });
}

const Model::Element* Model::getParagraph (const Element& row, int indent, int ordinal) const
{
    const Element* item { nullptr };

    getTableCell (row, Id::structure)->applyFunctionRecursively (
        [&item, indent, ordinal] (const Element& candidate) -> bool
        {
            if (item == nullptr and candidate.isTag (Id::p) and candidate.contains (Id::line)
                and *candidate.get<int> (Id::level) == indent
                and *candidate.get<int> (Id::line) == ordinal)
                item = &candidate;

            return item == nullptr;
        });

    return item;
}

const Model::Element* Model::getMap (const Element& row, const Element& line, int occurrence) const
{
    static const auto listMarker { getReservedName (Id::list) };

    const Element* item { nullptr };
    int matchOrdinal { 0 };

    getTableCell (row, Id::list)->applyFunctionRecursively (
        [&item, &matchOrdinal, &line, occurrence] (const Element& candidate) -> bool
        {
            if (item == nullptr and candidate.parent->isTag (Id::ul) and candidate.id == listMarker
                and not candidate.contains (Id::line)
                and *candidate.get<int> (Id::shape) == getOwner (line)
                and candidate.get<juce::String> (Id::value)->isNotEmpty()
                and matchOrdinal++ == occurrence)
                item = &candidate;

            return item == nullptr;
        });

    return item != nullptr ? getTable (row, *item->get<juce::String> (Id::value)) : nullptr;
}

const Model::Element* Model::getMapCell (const Element& row, const Element& line, const juce::Identifier& name) const
{
    for (int occurrence { 0 }; auto* map { getMap (row, line, occurrence) }; ++occurrence)
        if (auto* cell { getTableCell (*map, Id::value, name) })
            return cell;

    return nullptr;
}

const Model::Element* Model::getNextLine (const Element& line) const
{
    static const auto listMarker { getReservedName (Id::list) };

    auto* walk { &line };
    const Element* candidate { &line };

    do
    {
        candidate = walk->firstChild != nullptr ? walk->firstChild : walk->nextSibling;

        while (candidate == nullptr and not isBlockType (*walk, map::BlockType::tableCell))
        {
            walk = walk->parent;

            if (isBlockType (*walk, map::BlockType::tableCell))
                break;

            candidate = walk->nextSibling;
        }

        if (candidate == nullptr)
            return nullptr;

        walk = candidate;
    }
    while (not (candidate->contains (Id::shape)
                and (candidate->isTag (Id::p) or candidate->id == listMarker or isWrapper (*candidate))));

    return candidate;
}

const Model::Element* Model::getBinding (const Element& row, const juce::Identifier& column, const Element& line,
                     const juce::Identifier& name) const
{
    static const auto listMarker { getReservedName (Id::list) };

    const Element* item { nullptr };
    const auto ordinal { *line.get<int> (Id::shape) };

    getTableCell (row, column)->applyFunctionRecursively (
        [&item, ordinal, name] (const Element& candidate) -> bool
        {
            if (item == nullptr and candidate.parent->isTag (Id::ul)
                and candidate.id != listMarker and candidate.id == name
                and getOwner (candidate) == ordinal)
                item = &candidate;

            return item == nullptr;
        });

    return item;
}

bool Model::isOutputTable (const Element& table) const noexcept
{
    return *table.get<bool> (Id::wiring);
}

bool Model::hasRegionBinding (const Element& row, const juce::Identifier& marker) const
{
    auto hasBinding { false };

    if (auto* structureScope { getTableCell (row, Id::structure) })
        structureScope->applyFunctionRecursively (
            [&hasBinding, &marker] (const Element& candidate) -> bool
            {
                if (not hasBinding and candidate.parent->isTag (Id::ul) and candidate.id == marker)
                    hasBinding = true;

                return not hasBinding;
            });

    return hasBinding;
}

bool Model::hasRegionBegin (const Element& row) const
{
    static const auto beginMarker { getReservedName (Id::begin) };

    return hasRegionBinding (row, beginMarker);
}

bool Model::hasRegionEnd (const Element& row) const
{
    static const auto endMarker { getReservedName (Id::end) };

    return hasRegionBinding (row, endMarker);
}

bool Model::isRegionRow (const Element& row) const
{
    return hasRegionBegin (row) and hasRegionEnd (row);
}

juce::File Model::getFile (juce::StringRef relativePath) const
{
    return directory.getChildFile (relativePath);
}

juce::String Model::getFileName (juce::StringRef path) const
{
    return path.isNotEmpty() ? getFile (path).getFileName() : juce::String{};
}

const Model::Element* Model::getTable (const Element& row, juce::StringRef reference) const
{
    static const auto colonText { juce::String::charToString (Chars::colon) };
    static const auto atText { juce::String::charToString (Chars::at) };

    const auto parts { jam::Strings::fromTokens (reference, colonText, {}) };
    const auto sourceName { parts.size() > 0 ? parts.at (0).trim() : juce::String{} };
    const auto declaredPath { getValue (row, sourceName) };

    if (declaredPath.isNotEmpty())
    {
        const auto tableName { parts.size() > 1 ? parts.at (1).trim() : juce::String{} };
        return getTable (declaredPath, tableName);
    }

    return getTable (*row.parent->get<juce::String> (Id::path),
                     sourceName.trimCharactersAtStart (atText));
}

bool Model::isColumnAddress (const Element& row, const juce::String& value) const
{
    static const auto colonText { juce::String::charToString (Chars::colon) };

    const auto parts { jam::Strings::fromTokens (value, colonText, {}) };
    const auto sourceName { parts.size() > 0 ? parts.at (0).trim() : juce::String{} };
    const auto columnIndex { getValue (row, sourceName).isNotEmpty() ? 2 : 1 };

    return isAddress (value) and parts.size() > columnIndex
           and not parts.at (columnIndex).containsChar (Chars::equals);
}

juce::Identifier Model::getColumn (const Element& row, const juce::String& value) const
{
    static const auto colonText { juce::String::charToString (Chars::colon) };

    const auto parts { jam::Strings::fromTokens (value, colonText, {}) };
    const auto sourceName { parts.size() > 0 ? parts.at (0).trim() : juce::String{} };
    const auto columnIndex { getValue (row, sourceName).isNotEmpty() ? 2 : 1 };

    return juce::Identifier (jam::Format::toValidID (parts.at (columnIndex).trim()));
}

bool Model::isFilteredAddress (const Element& row, const juce::String& value) const
{
    static const auto colonText { juce::String::charToString (Chars::colon) };

    const auto parts { jam::Strings::fromTokens (value, colonText, {}) };
    const auto sourceName { parts.size() > 0 ? parts.at (0).trim() : juce::String{} };
    const auto filterIndex { getValue (row, sourceName).isNotEmpty() ? 2 : 1 };

    return isAddress (value) and parts.size() > filterIndex
           and parts.at (filterIndex).containsChar (Chars::equals);
}

juce::Identifier Model::getFilterColumn (const Element& row, const juce::String& value) const
{
    static const auto colonText { juce::String::charToString (Chars::colon) };
    static const auto equalsText { juce::String::charToString (Chars::equals) };

    const auto parts { jam::Strings::fromTokens (value, colonText, {}) };
    const auto sourceName { parts.size() > 0 ? parts.at (0).trim() : juce::String{} };
    const auto filterIndex { getValue (row, sourceName).isNotEmpty() ? 2 : 1 };
    const auto filterPart { parts.at (filterIndex) };

    return juce::Identifier (jam::Format::toValidID (
        filterPart.upToFirstOccurrenceOf (equalsText, false, false).trim()));
}

juce::String Model::getFilterValue (const Element& row, const juce::String& value) const
{
    static const auto colonText { juce::String::charToString (Chars::colon) };
    static const auto equalsText { juce::String::charToString (Chars::equals) };

    const auto parts { jam::Strings::fromTokens (value, colonText, {}) };
    const auto sourceName { parts.size() > 0 ? parts.at (0).trim() : juce::String{} };
    const auto filterIndex { getValue (row, sourceName).isNotEmpty() ? 2 : 1 };
    const auto filterPart { parts.at (filterIndex) };

    return filterPart.fromFirstOccurrenceOf (equalsText, false, false).trim();
}

const Model::Element* Model::getPairedListItem (const Element& row, const juce::Identifier& column, int indent, int ordinal) const
{
    static const auto listMarker { getReservedName (Id::list) };

    return getPairedItem (row, column,
        [indent, ordinal] (const Element& candidate)
        {
            return candidate.id == listMarker and candidate.contains (Id::line)
                   and *candidate.get<int> (Id::level) == indent
                   and *candidate.get<int> (Id::line) == ordinal;
        });
}

bool Model::isBlockType (const Element& element, int blockType)
{
    return element.contains (Id::type) and *element.get<int> (Id::type) == blockType;
}

bool Model::isWrapper (const Element& candidate)
{
    static const auto listMarker { getReservedName (Id::list) };

    return candidate.parent->isTag (Id::ul) and candidate.id != listMarker
           and candidate.contains (Id::templatePath);
}

int Model::getOwner (const Element& item)
{
    return *item.get<int> (isWrapper (item) ? Id::owner : Id::shape);
}

jam::Array<juce::String>
Model::getTableOrigins (const Model& document, const juce::String& manifestOrigin)
{
    jam::Array<juce::String> tableOrigins;

    for (auto* indexRow : document.getTableRows (Id::index))
    {
        const auto pathCell { document.getTableValue (*indexRow, Id::symbol) };

        if (juce::File::createFileWithoutCheckingPath (pathCell).hasFileExtension (
                Extensions::md)
            and pathCell.compare (manifestOrigin) != 0)
            tableOrigins.add (pathCell);
    }

    return tableOrigins;
}

void Model::parse (Model& document, const juce::String& text, const juce::String& origin,
    const juce::File& directory)
{
    document.directory = directory;
    document.manifestOrigin = origin;

    document.appendChildren (jam::MarkdownDocument::parse (text, origin));

    parse (document, directory, getTableOrigins (document, origin));
    addDescriptions (document);

    for (auto* table : *document.root)
        if (jam::MarkdownDocument::isTable (*table))
            table->add<bool> (Id::wiring,
                not table->isTag (Id::index) and table->contains (Id::path)
                    and table->get<juce::String> (Id::path)->compare (document.manifestOrigin) == 0
                    and document.getTableCell (*table, Id::structure, Id::headerRow) != nullptr);

    for (auto* table : *document.root)
        if (jam::MarkdownDocument::isTable (*table))
        {
            const auto* headerRow { getTableHeaderRow (*table) };

            for (auto* row : *table)
                if (row != headerRow and not isTableBorder (*row))
                    addRow (document, *row);
        }
}

std::pair<jam::Array<int>, jam::Array<int>> Model::getExcess (const Element* listCell,
    const Element* structureCell, const Model& document, const Element& row)
{
    jam::Array<int> counts;
    jam::Array<int> blanks;
    jam::Array<int> structureCounts;

    if (listCell != nullptr)
        addListCount (*listCell, 0, counts, blanks, document, row);

    if (structureCell != nullptr)
        addListCount (*structureCell, 0, structureCounts, document, row);

    counts.resize (juce::jmax (counts.size(), structureCounts.size()));
    structureCounts.resize (counts.size());
    blanks.resize (counts.size());

    jam::Array<int> excess;
    excess.resize (counts.size());

    for (int level { 0 }; level < counts.size(); ++level)
        excess.set (level, juce::jmax (0, counts.at (level) - structureCounts.at (level)));

    return { std::move (excess), std::move (blanks) };
}

void Model::addRow (Model& document, Element& row)
{
    if (const auto* headerRow { document.getTableHeaderRow (*row.parent) })
        document.addValues (*headerRow, row);

    auto* listCell { row.getChildByID (Id::list) };
    auto* structureCell { row.getChildByID (Id::structure) };
    auto* separatorCell { row.getChildByID (Id::separator) };

    if (listCell != nullptr)
        addBindings (*listCell, document, row);

    if (structureCell != nullptr)
        addBindings (*structureCell, document, row);

    if (separatorCell != nullptr)
        addBindings (*separatorCell, document, row);

    auto [excess, blanks] { getExcess (listCell, structureCell, document, row) };

    jam::Array<int> shapeOrdinals;
    jam::Array<int> listShapeOrdinals;
    jam::Array<int> separatorShapeOrdinals;
    jam::Array<int> structureExcess;
    jam::Array<int> rowJoinExcess { 1 };

    if (structureCell != nullptr)
        addColumn (*structureCell, structureExcess, shapeOrdinals, document, row);
    if (listCell != nullptr)
        addColumn (*listCell, excess, listShapeOrdinals, document, row);
    if (separatorCell != nullptr)
        addColumn (*separatorCell, rowJoinExcess, separatorShapeOrdinals, document, row);

    addMaps (document, row, blanks, shapeOrdinals);
    addDescriptionReferences (row);
}

void Model::addColumn (Element& cell, jam::Array<int>& excess,
    jam::Array<int>& shapeOrdinals, const Model& document, const Element& row)
{
    jam::Array<int> ordinals;
    jam::Array<int> descriptionOrdinals, mapOrdinal, paragraphOwner, bindingOwner;
    int lineIndex { 0 };

    addLines (cell, 0, ordinals, shapeOrdinals, descriptionOrdinals, mapOrdinal, excess, lineIndex,
        document, row, paragraphOwner, bindingOwner);
}

void Model::addDescriptions (Model& document)
{
    Element* precedingBlock { nullptr };
    bool precedingBoundToTable { false };

    for (auto* child : document)
    {
        if (isBlockType (*child, map::BlockType::table))
        {
            juce::String description;

            if (precedingBoundToTable)
                description = precedingBlock->getAllSubText();

            child->add<juce::String> (Id::description, description);
        }

        const auto isNamedFence { isBlockType (*child, map::BlockType::codeBlock)
                                  and child->contains (Id::info)
                                  and child->get<juce::String> (Id::info)->isNotEmpty() };

        const auto boundToNextTable { child->nextSibling != nullptr
                                      and isBlockType (*child->nextSibling, map::BlockType::table)
                                      and (isBlockType (*child, map::BlockType::paragraph)
                                           or isBlockType (*child, map::BlockType::codeBlock))
                                      and child->get<juce::String> (Id::path)->compare (
                                              *child->nextSibling->get<juce::String> (Id::path)) == 0 };

        if (isNamedFence and not boundToNextTable)
            child->add<juce::String> (Id::description, child->getAllSubText());

        precedingBlock = child;
        precedingBoundToTable = boundToNextTable;
    }
}

void Model::parse (Model& document, const juce::File& parent,
                   const jam::Array<juce::String>& tableOrigins)
{
    jam::Array<jam::MarkdownDocument> parsedTables;
    parsedTables.resize (tableOrigins.size());

    Jobs::run (tableOrigins.size(),
        [&parent, &tableOrigins, &parsedTables] (int index)
        {
            const auto& origin { tableOrigins.at (index) };
            const auto file { parent.getChildFile (origin) };

            parsedTables.at (index) = jam::MarkdownDocument::parse (
                file.loadFileAsString(), origin);
        });

    for (auto& table : parsedTables)
        document.appendChildren (std::move (table));
}

void Model::addBindings (Element& scope, const Model& document, const Element& row)
{
    juce::String precedingBinding;
    addBindings (scope, precedingBinding, document, row);
}

void
Model::addBindings (Element& scope, juce::String& precedingBinding, const Model& document, const Element& row)
{
    static const auto listMarker { getReservedName (Id::list) };

    for (auto* block : scope)
    {
        if (block->isTag (Id::p) and document.isShape (row, block->getAllSubText()))
            precedingBinding.clear();

        if (block->isTag (Id::ul))
            for (auto* item : *block)
            {
                const auto valueView { document.getValueView (*item) };
                auto value { jam::Strings::fromView (valueView) };

                if (value.isEmpty() and item->id != listMarker)
                    value = jam::Format::toCamelCase (precedingBinding);

                item->add<juce::String> (Id::value, value);

                precedingBinding = document.isShape (row, value) ? juce::String{} : value;
            }

        if (block->isTag (Id::blockquote))
            addBindings (*block, precedingBinding, document, row);
    }
}

void
Model::addListCount (const Element& scope, int indent, jam::Array<int>& counts, const Model& document, const Element& row)
{
    jam::Array<int> blanks;
    addListCount (scope, indent, counts, blanks, document, row);
}

void Model::addListCount (const Element& scope, int indent, jam::Array<int>& counts, jam::Array<int>& blanks,
    const Model& document, const Element& row)
{
    static const auto listMarker { getReservedName (Id::list) };

    if (indent == counts.size())
        counts.resize (indent + 1);

    if (indent == blanks.size())
        blanks.resize (indent + 1);

    for (auto* block : scope)
    {
        if (block->isTag (Id::ul))
            for (auto* item : *block)
                if (item->id == listMarker
                    and not document.isColumnAddress (row, *item->get<juce::String> (Id::value)))
                {
                    ++counts.at (indent);

                    if (item->get<juce::String> (Id::value)->isEmpty())
                        ++blanks.at (indent);
                }

        if (block->isTag (Id::blockquote))
            addListCount (*block, indent + 1, counts, blanks, document, row);
    }
}

void Model::addItem (Element& item, int indent, jam::Array<int>& ordinals,
    jam::Array<int>& descriptionOrdinals, jam::Array<int>& mapOrdinal, const jam::Array<int>& excess,
    int& lineIndex, const Model& document, const Element& row, const jam::Array<int>& paragraphOwner,
    jam::Array<int>& bindingOwner)
{
    static const auto listMarker { getReservedName (Id::list) };
    static const auto descriptionMarker { getReservedName (Id::description) };

    const auto blockText { *item.get<juce::String> (Id::value) };
    const auto isShapeValue { document.isShape (row, blockText) };

    if (item.id == listMarker)
    {
        const auto isColumn { document.isColumnAddress (row, blockText) };
        if (isColumn or mapOrdinal.at (indent) >= excess.at (indent))
        {
            item.add<int> (Id::level, indent);
            item.add<int> (Id::line, ordinals.at (indent)++);
            item.add<int> (Id::shape, lineIndex);
            ++lineIndex;
            bindingOwner.set (indent, paragraphOwner.at (indent));
        }
        if (not isColumn) ++mapOrdinal.at (indent);
    }
    else if (item.id == descriptionMarker)
    {
        item.add<int> (Id::level, indent);
        item.add<int> (Id::line, descriptionOrdinals.at (indent)++);
        item.add<int> (Id::shape, bindingOwner.at (indent));
    }
    else if (isShapeValue)
    {
        item.add<int> (Id::level, indent);
        item.add<int> (Id::owner, paragraphOwner.at (indent));
        item.add<int> (Id::shape, lineIndex);
        bindingOwner.set (indent, lineIndex);
        ++lineIndex;
    }
    else
        item.add<int> (Id::shape, bindingOwner.at (indent));

    if (isShapeValue)
    {
        item.add<juce::String> (Id::templatePath,
            document.getValue (row, jam::Format::getPreColon (blockText).trim()));
        item.add<juce::String> (Id::info, jam::Format::getPostColon (blockText).trim());
    }
}

void Model::addParagraph (Element& block, int indent, jam::Array<int>& shapeOrdinals, int& lineIndex,
    jam::Array<int>& paragraphOwner, jam::Array<int>& bindingOwner, const Model& document,
    const Element& row)
{
    const auto blockText { block.getAllSubText() };

    if (document.isShape (row, blockText))
    {
        block.add<int> (Id::level, indent);
        block.add<int> (Id::line, shapeOrdinals.at (indent)++);
        block.add<juce::String> (Id::templatePath,
            document.getValue (row, jam::Format::getPreColon (blockText).trim()));
        block.add<juce::String> (Id::info, jam::Format::getPostColon (blockText).trim());
        block.add<int> (Id::shape, lineIndex);
        paragraphOwner.set (indent, lineIndex);
        bindingOwner.set (indent, lineIndex);
        ++lineIndex;
    }
}

void Model::addLines (Element& cell, int indent, jam::Array<int>& ordinals,
    jam::Array<int>& shapeOrdinals, jam::Array<int>& descriptionOrdinals, jam::Array<int>& mapOrdinal,
    jam::Array<int>& excess, int& lineIndex, const Model& document, const Element& row,
    jam::Array<int>& paragraphOwner, jam::Array<int>& bindingOwner)
{
    if (indent == ordinals.size()) ordinals.resize (indent + 1);
    if (indent == shapeOrdinals.size()) shapeOrdinals.resize (indent + 1);
    if (indent == descriptionOrdinals.size()) descriptionOrdinals.resize (indent + 1);
    if (indent == mapOrdinal.size()) mapOrdinal.resize (indent + 1);
    if (indent == excess.size()) excess.resize (indent + 1);
    if (indent == paragraphOwner.size()) paragraphOwner.resize (indent + 1);
    if (indent == bindingOwner.size()) bindingOwner.resize (indent + 1);

    for (auto* block : cell)
    {
        if (block->isTag (Id::p))
            addParagraph (*block, indent, shapeOrdinals, lineIndex, paragraphOwner, bindingOwner, document, row);
        if (block->isTag (Id::ul))
            for (auto* item : *block)
                addItem (*item, indent, ordinals, descriptionOrdinals, mapOrdinal, excess, lineIndex,
                    document, row, paragraphOwner, bindingOwner);
        if (block->isTag (Id::blockquote))
            addLines (*block, indent + 1, ordinals, shapeOrdinals, descriptionOrdinals, mapOrdinal,
                excess, lineIndex, document, row, paragraphOwner, bindingOwner);
    }
}

void Model::addMaps (const Model& document, const Element& row, Element& scope, int indent,
    const jam::Array<int>& blanks, const jam::Array<int>& paragraphCount, jam::Array<int>& group)
{
    static const auto listMarker { getReservedName (Id::list) };

    if (indent == group.size())
        group.resize (indent + 1);

    for (auto* block : scope)
    {
        if (block->isTag (Id::ul))
            for (auto* item : *block)
                if (item->id == listMarker and not item->contains (Id::line))
                {
                    const auto groupCount { (indent < blanks.size() ? blanks.at (indent) : 0) + 1 };
                    const auto totalParagraphs {
                        indent < paragraphCount.size() ? paragraphCount.at (indent) : 0
                    };
                    const auto owner { totalParagraphs >= groupCount
                                           ? totalParagraphs - groupCount + group.at (indent)
                                           : -1 };

                    if (owner >= 0)
                        if (auto* paragraph { document.getParagraph (row, indent, owner) })
                            item->add<int> (Id::shape, *paragraph->get<int> (Id::shape));

                    if (item->get<juce::String> (Id::value)->isEmpty())
                        ++group.at (indent);
                }

        if (block->isTag (Id::blockquote))
            addMaps (document, row, *block, indent + 1, blanks, paragraphCount, group);
    }
}

void Model::addMaps (const Model& document, Element& row, const jam::Array<int>& blanks,
    const jam::Array<int>& paragraphCount)
{
    if (auto* listCell { row.getChildByID (Id::list) })
    {
        jam::Array<int> group;
        addMaps (document, row, *listCell, 0, blanks, paragraphCount, group);
    }
}

void Model::addDescriptionReferences (const Element& structureCell, Element& scope, int indent)
{
    static const auto descriptionMarker { getReservedName (Id::description) };

    for (auto* block : scope)
    {
        if (block->isTag (Id::ul))
            for (auto* item : *block)
                if (item->id == descriptionMarker)
                {
                    const Element* shapeLine { nullptr };
                    int shapeOrdinal { 0 };
                    const auto descriptionOrdinal { *item->get<int> (Id::line) };

                    structureCell.applyFunctionRecursively (
                        [&shapeLine, &shapeOrdinal, indent, descriptionOrdinal] (const Element& candidate) -> bool
                        {
                            if (shapeLine == nullptr and candidate.contains (Id::templatePath)
                                and (candidate.isTag (Id::p) or isWrapper (candidate))
                                and *candidate.get<int> (Id::level) == indent
                                and shapeOrdinal++ == descriptionOrdinal)
                                shapeLine = &candidate;

                            return shapeLine == nullptr;
                        });

                    if (shapeLine != nullptr)
                        item->add<int> (Id::owner, *shapeLine->get<int> (Id::shape));
                }

        if (block->isTag (Id::blockquote))
            addDescriptionReferences (structureCell, *block, indent + 1);
    }
}

void Model::addDescriptionReferences (Element& row)
{
    auto* listCell { row.getChildByID (Id::list) };
    auto* structureCell { row.getChildByID (Id::structure) };

    if (listCell != nullptr and structureCell != nullptr)
        addDescriptionReferences (*structureCell, *listCell, 0);
}

const Model::Element* Model::getLiteral (const Element& cell)
{
    const Element* codeChild { nullptr };

    cell.applyFunctionRecursively (
        [&codeChild] (const Element& node) -> bool
        {
            if (codeChild == nullptr
                and (node.isTag (Id::code) or isBlockType (node, map::BlockType::codeBlock)))
                codeChild = &node;

            return codeChild == nullptr;
        });

    return codeChild;
}

bool Model::isFencedCell (const Element& cell) const
{
    for (auto* child : cell)
        if (isFencedBlock (*child))
            return true;

    return false;
}

juce::String Model::getAuthoredText (const Element& cell, const Element* literal, bool isDescriptionColumn,
    bool isDescriptionProse, const juce::String& transform) const
{
    if (literal != nullptr and not isDescriptionProse)
    {
        juce::String text { literal->getAllSubText() };

        if (Transforms::contains (transform))
            text = Transforms::getTransformed (transform, text, {});

        return isDescriptionColumn ? text : jam::Format::toLiteral (text);
    }

    if (isDescriptionProse)
        return getRawText (cell, jam::Document::getSource());

    return cell.getAllSubText();
}

juce::String Model::getJoinedValue (const Element& headerCell, const Element& cell, const juce::String& value) const
{
    if (reflowWidths.contains (headerCell.id) and not isFencedCell (cell))
        return value.removeCharacters (juce::String::charToString (Chars::newline));

    return value;
}

void Model::addValue (const Element& headerCell, const Element& row, Element& cell)
{
    juce::String transform;

    if (auto* formatCell { cell.nextSibling })
        if (formatCell->id == Id::format)
            transform = formatCell->getAllSubText();

    const auto* literal { getLiteral (cell) };
    const auto isDescriptionColumn { headerCell.id == Id::description or headerCell.id == Id::brief };
    const auto isDescriptionProse { isDescriptionColumn
                                and (literal == nullptr
                                     or cell.getAllSubText().compare (
                                            literal->getAllSubText()) != 0) };

    auto value { getJoinedValue (headerCell, cell,
        getAuthoredText (cell, literal, isDescriptionColumn, isDescriptionProse, transform)) };

    if (not isDescriptionColumn and headerCell.id != Id::alias and isAddress (value))
        value = getValue (row, value);

    if (literal == nullptr and Transforms::contains (transform))
        value = Transforms::getTransformed (transform, value, {});

    cell.add<juce::String> (Id::value, value);
}

void Model::addValues (const Element& headerRow, Element& row)
{
    for (auto* cell : row)
        addValue (*headerRow.getChildByID (cell->id), row, *cell);
}

const Model::Element* Model::getUnnamedTable (const juce::String& declaredPath) const
{
    jam::Array<const Element*> fileTables;

    for (auto* candidate : *this)
        if (isBlockType (*candidate, map::BlockType::table) and not candidate->isTag (Id::index))
            if (candidate->contains (Id::path)
                and candidate->get<juce::String> (Id::path)->compare (declaredPath) == 0)
                fileTables.add (candidate);

    if (fileTables.size() == 1)
        return fileTables.at (0);

    const auto fileStem { juce::File::createFileWithoutCheckingPath (declaredPath)
                              .getFileNameWithoutExtension() };

    for (auto* candidate : fileTables)
        if (candidate->id == juce::Identifier (jam::Format::toValidID (fileStem)))
            return candidate;

    return nullptr;
}

const Model::Element* Model::getTable (const juce::String& declaredPath, const juce::String& tableName) const
{
    if (tableName.isNotEmpty())
    {
        const juce::Identifier tableId { jam::Format::toValidID (tableName) };

        for (auto* candidate : *this)
            if (candidate->id == tableId)
                if (candidate->contains (Id::path)
                    and candidate->get<juce::String> (Id::path)->compare (declaredPath) == 0)
                    return candidate;

        return nullptr;
    }

    return getUnnamedTable (declaredPath);
}
