#include "Constraints.h"
#include "Cells.h"
#include <jam_core/function_map/jam_Function.h>
#include <jam_markdown/parser/jam_Markdown.h>
#include <regex>

namespace cast
{

struct PredicateContext
{
    juce::String                                             value;
    juce::String                                             args;
    int                                                      rowIndex;
    int                                                      columnIndex;
    const jam::Document&                                     tableDoc;
    const jam::HashMap<juce::String, const jam::Document*>& tables;
    const juce::String&                                      sourceFile;
};

static juce::String cellAt (const jam::Document& doc, int row, int col) noexcept
{
    const auto& rows  { *doc.get<jam::Array<jam::Document>> (Id::children) };
    const auto& cells { *rows.at (row).get<jam::Array<jam::Document>> (Id::children) };
    return (cells.size() > col) ? *cells.at (col).get<juce::String> (Id::text)
                                : juce::String {};
}

static juce::String formatError (const juce::String& f, int r, int c,
                                  const juce::String& pred, const juce::String& msg)
{
    return f + ":" + juce::String (r) + ":" + juce::String (c) + ": " + pred + ": " + msg;
}

static juce::Result predicateMatches (const PredicateContext& ctx)
{
    const std::regex rx { ctx.args.toStdString() };
    const auto ok { std::regex_match (ctx.value.toStdString(), rx) };
    return ok ? juce::Result::ok()
              : juce::Result::fail (formatError (ctx.sourceFile, ctx.rowIndex + 1,
                                                 ctx.columnIndex + 1, "matches",
                                                 "value does not match: " + ctx.value));
}

static juce::Result predicateUnique (const PredicateContext& ctx)
{
    const auto& rows { *ctx.tableDoc.get<jam::Array<jam::Document>> (Id::children) };
    jam::Array<juce::String> seen;
    for (int r { 1 }; r < rows.size(); ++r)
    {
        const auto v { cellAt (ctx.tableDoc, r, ctx.columnIndex) };
        if (seen.contains (v))
            return juce::Result::fail (formatError (ctx.sourceFile, r + 1, ctx.columnIndex + 1,
                                                    "unique", "duplicate: " + v));
        seen.add (v);
    }
    return juce::Result::ok();
}

static juce::Result predicateExistsIn (const PredicateContext& ctx)
{
    const auto dot { ctx.args.indexOf (".") };
    if (dot < 0) return juce::Result::fail (ctx.sourceFile + ": existsIn: bad args: " + ctx.args);
    const auto tbl { ctx.args.substring (0, dot) };
    const auto col { ctx.args.substring (dot + 1) };
    const auto found { ctx.tables.find (tbl) };
    if (found == ctx.tables.end())
        return juce::Result::fail (ctx.sourceFile + ": existsIn: table not found: " + tbl);
    const auto& [foundTableName, foundTableDocPtr] { *found };
    const auto& refDoc { *foundTableDocPtr };
    const auto refCol  { findColumnIndex (refDoc, col) };
    if (refCol < 0) return juce::Result::fail (ctx.sourceFile + ": existsIn: col not found: " + col);
    const auto& refRows { *refDoc.get<jam::Array<jam::Document>> (Id::children) };
    for (int r { 1 }; r < refRows.size(); ++r)
        if (cellAt (refDoc, r, refCol) == ctx.value) return juce::Result::ok();
    return juce::Result::fail (formatError (ctx.sourceFile, ctx.rowIndex + 1, ctx.columnIndex + 1,
                                            "existsIn",
                                            "value not found in " + ctx.args + ": " + ctx.value));
}

static juce::Result predicateOneOf (const PredicateContext& ctx)
{
    const auto tokens { juce::StringArray::fromTokens (ctx.args, "|", "") };
    return tokens.contains (ctx.value)
               ? juce::Result::ok()
               : juce::Result::fail (formatError (ctx.sourceFile, ctx.rowIndex + 1,
                                                  ctx.columnIndex + 1,
                                                  "oneOf", "value not in {" + ctx.args + "}: " + ctx.value));
}

static juce::Result predicateRange (const PredicateContext& ctx)
{
    const auto sp { ctx.args.indexOf (" ") };
    if (sp < 0) return juce::Result::fail (ctx.sourceFile + ": range: args: minCol maxCol");
    const auto minCol { findColumnIndex (ctx.tableDoc, ctx.args.substring (0, sp)) };
    const auto maxCol { findColumnIndex (ctx.tableDoc, ctx.args.substring (sp + 1).trim()) };
    if (minCol < 0 or maxCol < 0)
        return juce::Result::fail (ctx.sourceFile + ": range: col not found: " + ctx.args);
    const auto v   { ctx.value.getDoubleValue() };
    const auto lo  { cellAt (ctx.tableDoc, ctx.rowIndex, minCol).getDoubleValue() };
    const auto hi  { cellAt (ctx.tableDoc, ctx.rowIndex, maxCol).getDoubleValue() };
    return (v >= lo and v <= hi) ? juce::Result::ok()
                                 : juce::Result::fail (formatError (ctx.sourceFile, ctx.rowIndex + 1,
                                                                    ctx.columnIndex + 1, "range",
                                                                    ctx.value + " outside ["
                                                                    + juce::String (lo) + ", "
                                                                    + juce::String (hi) + "]"));
}

static bool columnContainsValue (const jam::Document& doc, int col, const juce::String& value)
{
    const auto& rows { *doc.get<jam::Array<jam::Document>> (Id::children) };

    for (int r { 1 }; r < rows.size(); ++r)
    {
        if (cellAt (doc, r, col) == value)
            return true;
    }

    return false;
}

static juce::Result predicateParity (const PredicateContext& ctx)
{
    const auto dot { ctx.args.indexOf (".") };
    if (dot < 0) return juce::Result::fail (ctx.sourceFile + ": parity: bad args: " + ctx.args);
    const auto tbl { ctx.args.substring (0, dot) };
    const auto col { ctx.args.substring (dot + 1) };
    const auto found { ctx.tables.find (tbl) };
    if (found == ctx.tables.end())
        return juce::Result::fail (ctx.sourceFile + ": parity: table not found: " + tbl);
    const auto& [foundTableName, foundTableDocPtr] { *found };
    const auto& refDoc  { *foundTableDocPtr };
    const auto refCol   { findColumnIndex (refDoc, col) };
    if (refCol < 0) return juce::Result::fail (ctx.sourceFile + ": parity: col not found: " + col);
    const auto& rows    { *ctx.tableDoc.get<jam::Array<jam::Document>> (Id::children) };
    const auto& refRows { *refDoc.get<jam::Array<jam::Document>> (Id::children) };
    jam::Array<juce::String> lhs;
    for (int r { 1 }; r < rows.size(); ++r) lhs.add (cellAt (ctx.tableDoc, r, ctx.columnIndex));
    for (int r { 1 }; r < refRows.size(); ++r)
    {
        const auto v { cellAt (refDoc, r, refCol) };
        if (not lhs.contains (v))
            return juce::Result::fail (ctx.sourceFile + ": parity: ref key not in local: " + v);
    }
    for (const auto& v : lhs)
    {
        if (not columnContainsValue (refDoc, refCol, v))
            return juce::Result::fail (ctx.sourceFile + ": parity: local key not in ref: " + v);
    }
    return juce::Result::ok();
}

static juce::Result predicateFileExists (const PredicateContext& ctx)
{
    const auto target { juce::File (ctx.args).getChildFile (ctx.value) };
    return target.existsAsFile()
               ? juce::Result::ok()
               : juce::Result::fail (formatError (ctx.sourceFile, ctx.rowIndex + 1,
                                                  ctx.columnIndex + 1, "fileExists",
                                                  "not found: " + target.getFullPathName()));
}

static juce::Result predicateOnePerGroup (const PredicateContext& ctx)
{
    const auto& rows { *ctx.tableDoc.get<jam::Array<jam::Document>> (Id::children) };
    const auto grpCol { findColumnIndex (ctx.tableDoc, ctx.args) };
    if (grpCol < 0) return juce::Result::fail (ctx.sourceFile + ": onePerGroup: col not found: " + ctx.args);
    jam::Array<juce::String> allGroups;
    jam::HashMap<juce::String, int> counts;
    for (int r { 1 }; r < rows.size(); ++r)
    {
        const auto grpVal { cellAt (ctx.tableDoc, r, grpCol) };
        allGroups.addIfNotAlreadyThere (grpVal);
        if (cellAt (ctx.tableDoc, r, ctx.columnIndex).isNotEmpty()) counts[grpVal]++;
    }
    for (const auto& g : allGroups)
    {
        const auto found { counts.find (g) };
        const auto n     { (found != counts.end()) ? found->second : 0 };
        if (n != 1)
            return juce::Result::fail (ctx.sourceFile + ": onePerGroup: group '" + g
                                       + "' has " + juce::String (n) + " marked");
    }
    return juce::Result::ok();
}

static jam::Function::Map<juce::String, juce::Result> buildPredicateMap()
{
    jam::Function::Map<juce::String, juce::Result> map;
    map.add<const PredicateContext&> ("matches",      &predicateMatches);
    map.add<const PredicateContext&> ("unique",       &predicateUnique);
    map.add<const PredicateContext&> ("existsIn",     &predicateExistsIn);
    map.add<const PredicateContext&> ("oneOf",        &predicateOneOf);
    map.add<const PredicateContext&> ("range",        &predicateRange);
    map.add<const PredicateContext&> ("parity",       &predicateParity);
    map.add<const PredicateContext&> ("fileExists",   &predicateFileExists);
    map.add<const PredicateContext&> ("onePerGroup",  &predicateOnePerGroup);
    return map;
}

static const juce::StringArray perColumnPredicates { "unique", "parity", "onePerGroup" };

static juce::Result applyConstraintToTable (
    jam::Function::Map<juce::String, juce::Result>& predicates,
    const ConstraintEntry& entry,
    const juce::String& predicateName,
    const juce::String& predicateArgs,
    const jam::Document& tableDoc,
    const jam::HashMap<juce::String, const jam::Document*>& tables,
    const juce::String& sourceFile)
{
    const auto colIdx { findColumnIndex (tableDoc, entry.columnName) };
    if (colIdx < 0) return juce::Result::ok();
    const auto& rows { *tableDoc.get<jam::Array<jam::Document>> (Id::children) };
    if (perColumnPredicates.contains (predicateName))
    {
        const PredicateContext ctx { {}, predicateArgs, 0, colIdx, tableDoc, tables, sourceFile };
        return predicates.get (predicateName, ctx);
    }
    for (int r { 1 }; r < rows.size(); ++r)
    {
        const PredicateContext ctx { cellAt (tableDoc, r, colIdx), predicateArgs, r, colIdx,
                                     tableDoc, tables, sourceFile };
        const auto res { predicates.get (predicateName, ctx) };
        if (not res.wasOk()) return res;
    }
    return juce::Result::ok();
}

juce::Result Constraints::validate (
    const jam::Array<ConstraintEntry>& constraints,
    const jam::HashMap<juce::String, const jam::Document*>& tables,
    const juce::String& sourceFile)
{
    static jam::Function::Map<juce::String, juce::Result> predicates { buildPredicateMap() };

    for (const auto& entry : constraints)
    {
        const auto sp   { entry.predicate.indexOf (" ") };
        const auto name { sp >= 0 ? entry.predicate.substring (0, sp) : entry.predicate };
        const auto args { sp >= 0 ? entry.predicate.substring (sp + 1).trim() : juce::String {} };

        if (not predicates.contains (name))
            return juce::Result::fail (sourceFile + ": unknown predicate: " + name);

        for (const auto& [tableName, tableDocPtr] : tables)
        {
            const auto res { applyConstraintToTable (predicates, entry, name, args,
                                                      *tableDocPtr, tables, sourceFile) };
            if (not res.wasOk()) return res;
        }
    }

    return juce::Result::ok();
}

} // namespace cast
