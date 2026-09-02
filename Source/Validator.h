#pragma once
#include <JuceHeader.h>
#include "Items.h"
#include "Model.h"
#include "Shapes.h"
#include "TemplateDocument.h"

/**
 * @struct Validator
 * @brief Applies every manifest-level fatal to a parsed Model and
 *        TemplateDocument -- index integrity, uniqueness, formatting,
 *        structure, placeholders, and reference resolution.
 *
 * Validator extends jam::MarkdownValidator with cast's own manifest checks;
 * it owns no state and reports the first violation found, in check order.
 */
struct Validator : jam::MarkdownValidator
{
    /**
     * @brief Checks that @p line's own shape resolves to a code block in
     *        @p templateDocument.
     *
     * @param templateDocument The template document @p line's shape is
     *                         read from.
     * @param table            The table @p row belongs to, named in a
     *                         failure's location.
     * @param row              The row @p line belongs to, named in a
     *                         failure's location.
     * @param column           The column @p line was authored under,
     *                         named in a failure's location.
     * @param line             The structure line whose shape is checked.
     * @returns juce::Result::ok() when @p line's shape resolves, or a
     *          failure naming @p line's own info.
     */
    static juce::Result hasTemplate (const TemplateDocument& templateDocument, Element& table,
        Element& row, const juce::Identifier& column, Element& line)
    {
        if (templateDocument.getCodeBlock (line) == nullptr)
            return juce::Result::fail (getLocation (table, row, column.toString())
                                       + Id::diagnosticSeparator
                                       + text::Diagnostics::failTemplateMissing
                                       + Id::diagnosticSeparator + *line.get<juce::String> (Id::info));

        return juce::Result::ok();
    }

    /**
     * @brief Runs every manifest-level check against @p model and
     *        @p templateDocument, in order, stopping at the first failure.
     *
     * @param model            The parsed manifest and its data tables.
     * @param templateDocument The parsed template file the manifest wires
     *                         against.
     * @returns juce::Result::ok() when every check passes, or the first
     *          failing check's result.
     */
    static juce::Result isValid (const Model& model, const TemplateDocument& templateDocument)
    {
        return isManifest (model, templateDocument);
    }

    /**
     * @brief Invokes @p function once per bullet found by walking
     *        @p column's blockquote scopes, depth by depth, across every
     *        output table's rows.
     *
     * @param model    The model whose output tables are walked.
     * @param column   The column whose blockquote scopes carry the binding
     *                 lists to walk.
     * @param function Callable invoked as
     *                 @c function(table,row,bulletId,bulletValue) for each
     *                 bullet found, returning a juce::Result.
     * @returns juce::Result::ok() when @p function succeeds for every
     *          bullet, or the first failing invocation's result.
     */
    template <typename Function>
    static juce::Result
    forEachBinding (const Model& model, const juce::Identifier& column, Function&& function)
    {
        for (auto* table : model.getTables())
            if (model.isOutputTable (*table))
                for (auto* row : model.getTableRows (*table))
                    if (auto* scope { model.getTableCell (*row, column) })
                        if (const auto result { forEachBinding (*table, *row, *scope, function) };
                            not result.wasOk())
                            return result;

        return juce::Result::ok();
    }

    /**
     * @brief Invokes @p function once per bullet found by walking
     *        @p scope's blockquote lists, depth by depth, through the
     *        recursive overload.
     *
     * @param table    The table @p row belongs to, passed through to
     *                 @p function.
     * @param row      The row @p scope belongs to, passed through to
     *                 @p function.
     * @param scope    The blockquote scope walked.
     * @param function Callable invoked as
     *                 @c function(table,row,bulletId,bulletValue) for each
     *                 bullet found, returning a juce::Result.
     * @returns juce::Result::ok() when @p function succeeds for every
     *          bullet, or the first failing invocation's result.
     */
    template <typename Function>
    static juce::Result
    forEachBinding (Element& table, Element& row, Element& scope, Function&& function)
    {
        for (auto* child : scope)
        {
            if (child->isTag (Id::ul))
                for (auto* item : *child)
                    if (const auto result { function (
                            table, row, item->id, *item->get<juce::String> (Id::value)) };
                        not result.wasOk())
                        return result;

            if (child->isTag (Id::blockquote))
                if (const auto result { forEachBinding (table, row, *child, function) };
                    not result.wasOk())
                    return result;
        }

        return juce::Result::ok();
    }

    /**
     * @brief Invokes @p function once per non-wiring cell across every
     *        non-index table's rows -- every cell but @c list,
     *        @c separator, and @c structure, the columns wiring an output
     *        row's own expansion.
     *
     * @param model    The model whose non-index tables are walked.
     * @param function Callable invoked as
     *                 @c function(table,row,columnId,rawText) for each
     *                 non-wiring cell, returning a juce::Result.
     * @returns juce::Result::ok() when @p function succeeds for every
     *          cell, or the first failing invocation's result.
     */
    template <typename Function>
    static juce::Result forEachEntry (const Model& model, Function&& function)
    {
        for (auto* table : model.getTables())
            if (not table->isTag (Id::index))
                for (auto* row : model.getTableRows (*table))
                    for (auto* cell : *row)
                        if (cell->id != Id::list and cell->id != Id::separator
                            and cell->id != Id::structure)
                            if (const auto result { function (
                                    *table, *row, cell->id, *cell->get<juce::String> (Id::rawText)) };
                                not result.wasOk())
                                return result;

        return juce::Result::ok();
    }

    /**
     * @brief Checks that the manifest's index table exists and that every
     *        row's symbol names a file that exists on disk.
     *
     * @param model The model whose index table is checked.
     * @returns juce::Result::ok() when the index exists and every entry
     *          resolves to an existing file, or a failure naming the
     *          missing table, symbol, output, or template.
     */
    static juce::Result isIndex (const Model& model)
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

    /**
     * @brief Checks that every output row declares a non-empty structure
     *        scope resolving to a template file, and that every one of
     *        its lines carrying a template path resolves to a code
     *        block.
     *
     * @param model            The model whose output tables are checked.
     * @param templateDocument The template document each structure
     *                         line's shape is read from.
     * @returns juce::Result::ok() when every row's structure resolves, or
     *          a failure naming the missing structure or the missing
     *          template.
     */
    static juce::Result isStructure (const Model& model, const TemplateDocument& templateDocument)
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

                    Element* failingLine { nullptr };

                    structureScope->applyFunctionRecursively (
                        [&failingLine, &templateDocument] (const Element& candidate) -> bool
                        {
                            if (failingLine == nullptr and candidate.contains (Id::templatePath)
                                and templateDocument.getCodeBlock (candidate) == nullptr)
                                failingLine = const_cast<Element*> (&candidate);

                            return failingLine == nullptr;
                        });

                    if (failingLine != nullptr)
                        return hasTemplate (templateDocument, *table, *row, Id::structure, *failingLine);
                }

        return juce::Result::ok();
    }

    /**
     * @brief Checks that no two bindings under @p scope share the same
     *        name, then checks the same for every nested blockquote scope.
     *
     * @param table  The table @p row belongs to, named in a failure's
     *               location.
     * @param row    The row @p scope belongs to, named in a failure's
     *               location.
     * @param column The column @p scope was read from, named in a
     *               failure's location.
     * @param scope  The blockquote scope whose bindings are checked.
     * @param seen   The binding names seen for the current shape, reset at
     *               each shape line.
     * @returns juce::Result::ok() when every scope's bindings are unique,
     *          or a failure naming the duplicate binding.
     */
    static juce::Result isBindingCountValid (Element& table, Element& row,
        const juce::Identifier& column, Element& scope, jam::HashSet<juce::Identifier>& seen)
    {
        for (auto* child : scope)
        {
            if (child->isTag (Id::p) and child->contains (Id::templatePath))
                seen.clear();

            if (child->isTag (Id::ul))
                for (auto* item : *child)
                {
                    if (item->id == Id::list)
                        seen.clear();
                    else
                    {
                        if (seen.contains (item->id))
                            return juce::Result::fail (getLocation (table, row, column.toString())
                                                       + Id::diagnosticSeparator
                                                       + text::Diagnostics::failBindingDuplicate
                                                       + Id::diagnosticSeparator + item->id.toString());

                        seen.insert (item->id);
                    }
                }

            if (child->isTag (Id::blockquote))
                if (const auto result { isBindingCountValid (table, row, column, *child, seen) };
                    not result.wasOk())
                    return result;
        }

        return juce::Result::ok();
    }

    /**
     * @brief Checks every output row's @c structure and @c separator
     *        scopes for duplicate binding names, through
     *        isBindingCountValid().
     *
     * @param model The model whose output tables are checked.
     * @returns juce::Result::ok() when every row's bindings are unique, or
     *          the first failing row's result.
     */
    static juce::Result isBindingCountValid (const Model& model)
    {
        for (auto* table : model.getTables())
            if (model.isOutputTable (*table))
                for (auto* row : model.getTableRows (*table))
                    for (const auto& column : { Id::structure, Id::separator })
                        if (auto* scope { model.getTableCell (*row, column) })
                        {
                            jam::HashSet<juce::Identifier> seen;

                            if (const auto result {
                                    isBindingCountValid (*table, *row, column, *scope, seen) };
                                not result.wasOk())
                                return result;
                        }

        return juce::Result::ok();
    }

    /**
     * @brief Checks that every shape line under @p structureScope
     *        resolves to a code block carrying an even count of
     *        @c ::: markers -- every marker opened is closed.
     *
     * @param templateDocument The template document each shape line's
     *                         code block is read from.
     * @param table            The table @p row belongs to, named in a
     *                         failure's location.
     * @param row              The row @p structureScope belongs to,
     *                         named in a failure's location.
     * @param structureScope   The structure scope whose shape lines are
     *                         checked.
     * @returns juce::Result::ok() when every shape's markers pair, or a
     *          failure naming the unterminated shape.
     */
    static juce::Result isMarkerCountValid (const TemplateDocument& templateDocument, Element& table,
        Element& row, Element& structureScope)
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

    /**
     * @brief Checks every output row's structure scope for unterminated
     *        shape markers, through the recursive overload.
     *
     * @param model            The model whose output tables are checked.
     * @param templateDocument The template document each shape line's
     *                         code block is read from.
     * @returns juce::Result::ok() when every row's markers pair, or the
     *          first failing row's result.
     */
    static juce::Result
    isMarkerCountValid (const Model& model, const TemplateDocument& templateDocument)
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

    /**
     * @brief Checks that @p precedingShape, when it is a list-item shape
     *        addressing a column, resolves through hasTable().
     *
     * @param model          The model @p precedingShape's source is
     *                       resolved against.
     * @param table          The table @p row belongs to, named in a
     *                       failure's location.
     * @param row            The row @p precedingShape belongs to, named
     *                       in a failure's location.
     * @param column         The column @p precedingShape was authored
     *                       under, named in a failure's location.
     * @param precedingShape The most recently seen shape line, checked
     *                       when it is a list-item shape.
     * @returns juce::Result::ok() when @p precedingShape is a paragraph
     *          shape or its column-address source resolves, or a failure
     *          naming the unresolved reference.
     */
    static juce::Result isShapeSupplied (const Model& model, Element& table, Element& row,
        const juce::Identifier& column, Element& precedingShape)
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

    /**
     * @brief Walks @p scope, checking every shape line's template through
     *        hasTemplate(), and, at each new shape line, the previously
     *        seen shape through isShapeSupplied(), then recurses into
     *        every nested blockquote.
     *
     * @param model            The model @p row belongs to.
     * @param templateDocument The template document each shape line's
     *                         code block is read from.
     * @param table            The table @p row belongs to, named in a
     *                         failure's location.
     * @param row              The row @p scope belongs to, named in a
     *                         failure's location.
     * @param column           The column @p scope was read from, named
     *                         in a failure's location.
     * @param scope            The blockquote scope walked.
     * @param precedingShape   The most recently seen shape line, or
     *                         @c nullptr when @p scope opens with none
     *                         seen yet, checked through isShapeSupplied()
     *                         at the next shape line.
     * @returns @p precedingShape's own successor -- the last shape line
     *          seen across @p scope and its nested blockquotes -- paired
     *          with juce::Result::ok() when every checked shape resolves,
     *          or the first failing check's result.
     */
    static std::pair<juce::Result, Element*> isPlaceholderScope (const Model& model,
        const TemplateDocument& templateDocument, Element& table, Element& row,
        const juce::Identifier& column, Element& scope, Element* precedingShape)
    {
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

                    if (item->id == Id::list and column == Id::structure)
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

    static juce::Result
    isPlaceholders (const Model& model, const TemplateDocument& templateDocument)
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

    /**
     * @brief Checks that every output row's structure scope supplies no
     *        more sources than its shapes demand -- each paragraph shape,
     *        wrapper, and item shape's own arity (SPEC §6.4), and each
     *        expansion @c list bullet counting as one supplied source.
     *
     * @param model            The model whose output tables are checked.
     * @param templateDocument The template document each shape line's
     *                         arity is read from.
     * @returns juce::Result::ok() when every row's supplied sources stay
     *          within its demanded arity, or a failure naming the
     *          ambiguous row.
     */
    static juce::Result isSourceCountValid (const Model& model, const TemplateDocument& templateDocument)
    {
        for (auto* table : model.getTables())
            if (model.isOutputTable (*table))
                for (auto* row : model.getTableRows (*table))
                    if (auto* structureScope { model.getTableCell (*row, Id::structure) })
                    {
                        int demanded { 0 };
                        int supplied { 0 };

                        structureScope->applyFunctionRecursively (
                            [&demanded, &supplied, &templateDocument] (const Element& candidate)
                            {
                                if (candidate.isTag (Id::p) and candidate.contains (Id::templatePath))
                                {
                                    ++supplied;
                                    demanded += Items::getArity (
                                        templateDocument, const_cast<Element&> (candidate));
                                }

                                if (candidate.parent->isTag (Id::ul) and candidate.id == Id::list)
                                {
                                    ++supplied;

                                    if (candidate.contains (Id::templatePath))
                                        demanded += Items::getArity (
                                            templateDocument, const_cast<Element&> (candidate));
                                }

                                if (candidate.parent->isTag (Id::ul) and candidate.id != Id::list
                                    and candidate.contains (Id::templatePath))
                                    demanded += Items::getArity (
                                        templateDocument, const_cast<Element&> (candidate));
                            });

                        if (supplied - 1 > demanded)
                            return juce::Result::fail (getLocation (*table, *row, Id::structure.toString())
                                                       + Id::diagnosticSeparator
                                                       + text::Diagnostics::failAmbiguous);
                    }

        return juce::Result::ok();
    }

    /**
     * @brief Checks that every expansion @c list bullet under @p scope
     *        pairs with a structure @c list line at its own (depth,
     *        ordinal), and that every @-sigiled comment bullet's address
     *        resolves through Model::getTable() -- the alias form or,
     *        absent an alias hit, the local form -- or names a code block
     *        carrying documentation. A comment bullet whose value is not
     *        @-sigiled is plain prose, never a reference (SPEC §4), and is
     *        exempt. Bullets carrying no @c Id::line stamp -- the row join
     *        and map bullets -- are exempt from the pairing check. A
     *        @c structure comment bullet is exempt from this address check
     *        -- it is the file-documentation binding, validated instead by
     *        isReference() through hasTable().
     *
     * @param model  The model @p row belongs to.
     * @param table  The table @p row belongs to, named in a failure's
     *               location.
     * @param row    The row @p scope belongs to, named in a failure's
     *               location.
     * @param column The column @p scope was read from, named in a
     *               failure's location.
     * @param scope  The blockquote scope walked.
     * @returns juce::Result::ok() when every bullet pairs and resolves,
     *          or a failure naming the orphan bullet.
     */
    static juce::Result isPaired (const Model& model, Element& table, Element& row,
        const juce::Identifier& column, Element& scope)
    {
        for (auto* block : scope)
        {
            if (block->isTag (Id::ul))
                for (auto* item : *block)
                {
                    if (item->id == Id::list and column != Id::list and item->contains (Id::line))
                    {
                        const auto indent { *item->get<int> (Id::level) };
                        const auto ordinal { *item->get<int> (Id::line) };

                        if (model.getSource (row, indent, ordinal) == nullptr)
                            return juce::Result::fail (getLocation (table, row, column.toString())
                                                       + Id::diagnosticSeparator
                                                       + text::Diagnostics::failOrphan);
                    }

                    if (item->id == Id::comment and column != Id::structure)
                    {
                        const auto& value { *item->get<juce::String> (Id::value) };

                        if (Model::isAddress (value))
                        {
                            auto* referencedCodeBlock { model.getCodeBlock (
                                juce::Identifier (jam::Format::getPostColon (value).trim())) };

                            if (model.getTable (row, value) == nullptr
                                and (referencedCodeBlock == nullptr
                                     or not referencedCodeBlock->contains (Id::comment)))
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

    /**
     * @brief Checks every output row's @c structure, @c separator, and
     *        @c list scopes, through the recursive overload.
     *
     * @param model The model whose output tables are checked.
     * @returns juce::Result::ok() when every row's bullets pair and
     *          resolve, or the first failing row's result.
     */
    static juce::Result isPaired (const Model& model)
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

    /**
     * @brief Checks that every @-sigiled binding across @p model's
     *        @c structure, @c separator, and @c list scopes, and every
     *        @-sigiled non-wiring cell, resolves through hasTable() --
     *        a shape address is exempt, resolved instead by
     *        Validator::isStructure(). A @c separator or @c list comment
     *        binding is exempt -- it is item prose, validated instead by
     *        isPaired(). A @c structure comment binding is not exempt --
     *        it is the file-documentation binding, validated here like any
     *        other address.
     *
     * @param model The model whose bindings and cells are checked.
     * @returns juce::Result::ok() when every @-sigiled entry resolves,
     *          or the first failing entry's result.
     */
    static juce::Result isReference (const Model& model)
    {
        for (const auto& column : { Id::structure, Id::separator, Id::list })
            if (const auto result { forEachBinding (model, column,
                    [&model, &column] (Element& table, Element& row, const juce::Identifier& entryId,
                        const juce::String& entryValue) -> juce::Result
                    {
                        if ((entryId != Id::comment or column == Id::structure)
                            and Model::isAddress (entryValue) and not model.isShape (row, entryValue))
                            return hasTable (model, table, row, column, entryValue);

                        return juce::Result::ok();
                    }) };
                not result.wasOk())
                return result;

        return forEachEntry (model,
            [&model] (Element& table, Element& row, const juce::Identifier& column,
                const juce::String& entryValue) -> juce::Result
            {
                if (Model::isAddress (entryValue))
                    return hasTable (model, table, row, column, entryValue);

                return juce::Result::ok();
            });
    }

    /**
     * @brief Checks that every map bullet under @p scope carries a
     *        @c shape ordinal, and, when it names a table, that the
     *        table exists and declares a @c value column, then recurses
     *        into every nested blockquote.
     *
     * @param model The model @p row belongs to.
     * @param table The table @p row belongs to, named in a failure's
     *              location.
     * @param row   The row @p scope belongs to, named in a failure's
     *              location.
     * @param scope The blockquote scope walked.
     * @returns juce::Result::ok() when every map bullet resolves, or a
     *          failure naming the orphan map, missing table, or unknown
     *          column.
     */
    static juce::Result
    isMap (const Model& model, Element& table, Element& row, Element& scope)
    {
        for (auto* block : scope)
        {
            if (block->isTag (Id::ul))
                for (auto* item : *block)
                    if (item->id == Id::list and not item->contains (Id::line))
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

    /**
     * @brief Checks every output row's @c list column's map bullets,
     *        through the recursive overload.
     *
     * @param model The model whose output tables are checked.
     * @returns juce::Result::ok() when every row's map bullets resolve,
     *          or the first failing row's result.
     */
    static juce::Result isMap (const Model& model)
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

    /**
     * @brief Checks that @p entryValue's first segment resolves -- against
     *        @p row's writing file's index when it hits an alias, or,
     *        absent an alias hit, as a table of @p row's own document --
     *        and, when @p entryValue names a table or column, that each
     *        part resolves in turn.
     *
     * @param model      The model @p row belongs to.
     * @param table      The table @p row belongs to, named in a failure's
     *                   location.
     * @param row        The row @p entryValue was authored on.
     * @param column     The column @p entryValue was authored under, named
     *                   in a failure's location.
     * @param entryValue The @-sigiled address: the alias form
     *                   @c \@alias\[:table\[:column\[=value\]\]\], or,
     *                   absent an alias hit, the local form
     *                   @c \@table\[:column\[=value\]\]. A filter's
     *                   @c =value suffix is stripped before the column
     *                   part is checked.
     * @returns juce::Result::ok() when every declared part resolves, or a
     *          failure naming the missing table or column.
     */
    static juce::Result hasTable (const Model& model, Element& table, Element& row,
        const juce::Identifier& column, const juce::String& entryValue)
    {
        const auto parts { jam::Strings::fromTokens (
            entryValue, juce::String::charToString (Chars::colon), {}) };
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
                juce::String::charToString (Chars::equals), false, false).trim() };

            if (not model.getTableHeaders (*referencedTable)
                        .contains (jam::Format::toValidID (columnName)))
                return juce::Result::fail (getLocation (table, row, column.toString())
                                           + Id::diagnosticSeparator
                                           + text::Diagnostics::failColumnUnknown
                                           + Id::diagnosticSeparator + columnName);
        }

        return juce::Result::ok();
    }

    /**
     * @brief Checks that no header row carries two adjacent @c format
     *        columns, and that every authored @c format cell names a
     *        known transform.
     *
     * @param model The model whose tables are checked.
     * @returns juce::Result::ok() when every table's @c format columns are
     *          well formed and every named transform is known, or a
     *          failure naming the adjacency or the unknown transform.
     */
    static juce::Result isFormatted (const Model& model)
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
            [] (Element& table, Element& row, const juce::String& transform) -> juce::Result
            {
                if (transform.isNotEmpty() and not Transforms::contains (transform))
                    return juce::Result::fail (getLocation (table, row, Id::format.toString())
                                               + Id::diagnosticSeparator
                                               + text::Diagnostics::failUnknownTransform
                                               + Id::diagnosticSeparator + transform);

                return juce::Result::ok();
            });
    }

    /**
     * @brief Checks that every row declaring a given @c file value stays
     *        contiguous with the rows that already declared that file --
     *        no other file's rows may interleave between them.
     *
     * @param model The model @p table belongs to.
     * @param table The output table whose rows are checked.
     * @returns juce::Result::ok() when every file's rows stay contiguous,
     *          or a failure naming the file that reappears after its group
     *          closed.
     */
    static juce::Result isContiguous (const Model& model, Element& table)
    {
        jam::HashSet<juce::String> closedFiles;
        juce::String previousFile;

        for (auto* row : model.getTableRows (table))
        {
            const auto& file { model.getValue (*row, Id::file) };

            if (file != previousFile)
            {
                if (previousFile.isNotEmpty())
                    closedFiles.insert (previousFile);

                previousFile = file;
            }

            if (closedFiles.contains (file))
                return juce::Result::fail (getLocation (table, *row, Id::file.toString())
                                           + Id::diagnosticSeparator
                                           + text::Diagnostics::failDuplicate + file
                                           + juce::String::charToString (Chars::doubleQuote));
        }

        return juce::Result::ok();
    }

    /**
     * @brief Checks that @p column's resolved, non-reference values are
     *        unique, byte-exact, across @p rows.
     *
     * A cell whose authored text is an @-sigiled reference is exempt --
     * a reference legitimately repeats what its address already resolves
     * to once.
     *
     * @param model  The model @p rows belong to.
     * @param table  The table @p rows belong to, named in a failure's
     *               location.
     * @param rows   The rows whose @p column entries are checked.
     * @param column The column checked for duplicate values.
     * @returns juce::Result::ok() when every non-reference value is
     *          unique, or a failure naming the duplicate value.
     */
    static juce::Result isUniqueColumn (const Model& model, Element& table,
        const jam::Array<Element*>& rows, const juce::Identifier& column)
    {
        jam::HashSet<juce::String> seen;

        for (auto* row : rows)
            if (auto* cell { model.getTableCell (*row, column) })
            {
                const auto rawText { *cell->get<juce::String> (Id::rawText) };

                if (not Model::isAddress (rawText))
                    if (const auto& value { *cell->get<juce::String> (Id::value) }; value.isNotEmpty())
                    {
                        if (seen.contains (value))
                            return juce::Result::fail (getLocation (table, *row, column.toString())
                                                       + Id::diagnosticSeparator
                                                       + text::Diagnostics::failDuplicate + value
                                                       + juce::String::charToString (Chars::doubleQuote));

                        seen.insert (value);
                    }
            }

        return juce::Result::ok();
    }

    /**
     * @brief Checks each of @p table's identity columns -- @c name,
     *        @c key, @c alias, and @c file -- for uniqueness through
     *        isUniqueColumn().
     *
     * @param model The model @p table belongs to.
     * @param table The table whose identity columns are checked.
     * @returns juce::Result::ok() when every identity column is unique,
     *          or the first failing column's result.
     */
    static juce::Result isUniqueTable (const Model& model, Element& table)
    {
        const auto rows { model.getTableRows (table) };

        for (const auto& column : { Id::name, Id::key, Id::alias, Id::file })
            if (const auto result { isUniqueColumn (model, table, rows, column) };
                not result.wasOk())
                return result;

        return juce::Result::ok();
    }

    /**
     * @brief Checks that every table carrying an @c alias column declares
     *        each alias value once.
     *
     * @param model The model whose tables are checked.
     * @returns juce::Result::ok() when every table's alias values are
     *          unique, or a failure naming the duplicate alias.
     */
    static juce::Result isUniqueAlias (const Model& model)
    {
        for (auto* table : model.getTables())
            if (model.getTableHeaders (*table).contains (Id::alias.toString()))
            {
                jam::HashSet<juce::String> seen;

                for (auto* row : model.getTableRows (*table))
                    if (auto* cell { model.getTableCell (*row, Id::alias) })
                    {
                        const auto& value { *cell->get<juce::String> (Id::rawText) };

                        if (seen.contains (value))
                            return juce::Result::fail (getLocation (*table, *row, Id::alias.toString())
                                                       + Id::diagnosticSeparator
                                                       + text::Diagnostics::failDuplicate + value
                                                       + juce::String::charToString (Chars::doubleQuote));

                        seen.insert (value);
                    }
            }

        return juce::Result::ok();
    }

    /**
     * @brief Checks every non-index, non-wiring table for column
     *        uniqueness, through isUniqueTable() -- a data table kept in
     *        the manifest is gated like any other; only wiring tables are
     *        exempt (SPEC §5.3).
     *
     * @param model The model whose data tables are checked.
     * @returns juce::Result::ok() when every checked table is unique, or
     *          the first failing table's result.
     */
    static juce::Result isUnique (const Model& model)
    {
        for (auto* table : model.getTables())
            if (not model.isOutputTable (*table) and not table->isTag (Id::index))
                if (const auto result { isUniqueTable (model, *table) }; not result.wasOk())
                    return result;

        return juce::Result::ok();
    }

    /**
     * @brief Checks that every output row's own file resolves a known
     *        comment-syntax key, through Transforms::getCommentSyntaxKey().
     *
     * @param model The model whose output tables are checked.
     * @returns juce::Result::ok() when every row's file resolves a known
     *          comment syntax, or a failure naming the unresolved key.
     */
    static juce::Result isCommentSyntax (const Model& model)
    {
        for (auto* table : model.getTables())
            if (model.isOutputTable (*table))
                for (auto* row : model.getTableRows (*table))
                {
                    const auto syntaxKey { Transforms::getCommentSyntaxKey (
                        model.getValue (*row, Id::file)) };

                    if (not map::commentSyntax.contains (syntaxKey))
                        return juce::Result::fail (getLocation (*table, *row, Id::file.toString())
                                                   + Id::diagnosticSeparator
                                                   + text::Diagnostics::failNotFound
                                                   + Id::diagnosticSeparator + syntaxKey);
                }

        return juce::Result::ok();
    }

    /**
     * @brief Checks that every declared @c ## toolchain table's header row
     *        declares both a @c command and a @c flag column (SPEC §6.9) --
     *        the invariant Processor::generate() trusts unconditionally
     *        when it reads a row's @c command and @c flag cells.
     *
     * @param model The model whose @c ## toolchain tables are checked.
     * @returns juce::Result::ok() when every declared @c ## toolchain
     *          table carries both columns, or a failure naming the table
     *          missing one.
     */
    static juce::Result isToolchain (const Model& model)
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

    /**
     * @brief Runs every manifest-level check against @p model and
     *        @p templateDocument, in declared order, stopping at the
     *        first failure -- the one driver isValid() reads through.
     *
     * @param model            The parsed manifest and its data tables.
     * @param templateDocument The parsed template file the manifest wires
     *                         against.
     * @returns juce::Result::ok() when every check passes, or the first
     *          failing check's result.
     */
    static juce::Result isManifest (const Model& model, const TemplateDocument& templateDocument)
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

        if (const auto result { isCommentSyntax (model) }; not result.wasOk())
            return result;

        if (const auto result { isStructure (model, templateDocument) }; not result.wasOk())
            return result;

        if (const auto result { isBindingCountValid (model) }; not result.wasOk())
            return result;

        if (const auto result { isMarkerCountValid (model, templateDocument) }; not result.wasOk())
            return result;

        if (const auto result { isPaired (model) }; not result.wasOk())
            return result;

        if (const auto result { isReference (model) }; not result.wasOk())
            return result;

        if (const auto result { isMap (model) }; not result.wasOk())
            return result;

        if (const auto result { isPlaceholders (model, templateDocument) }; not result.wasOk())
            return result;

        return isSourceCountValid (model, templateDocument);
    }
};
