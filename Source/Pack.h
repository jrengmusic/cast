#pragma once
#include <JuceHeader.h>
#if JUCE_MAC
#include <copyfile.h>
#endif
#include "generated/Generated.h"
#include "Model.h"
#include "Toolchain.h"
#include "BinaryWriter.h"
#include "ZipWriter.h"

/**
 * @struct Pack
 * @brief Builds the archives that the selected @c ## pack rows declare
 *        (SPEC §6.12).
 *
 * Pack keeps no state. Rows whose @c archive paths resolve to one file
 * make one archive. The @c archive extension picks the format: @c zip writes
 * through ZipWriter, and @c dmg stages the items in a folder and runs
 * @c hdiutil on macOS. Any other host fails a @c dmg archive.
 */
struct Pack
{
    /**
     * @brief Builds every archive that a selected @c ## pack row names,
     *        once per archive.
     *
     * @param model             The model that holds the @c ## pack tables.
     * @param toolchainArgument The CLI-selected toolchain group. Only rows
     *                          that Toolchain::isRowSelected() accepts take
     *                          part.
     * @returns juce::Result::ok() when every archive is built, or the
     *          first failure.
     */
    static juce::Result toArchives (const Model& model, const juce::String& toolchainArgument)
    {
        for (auto* table : model.getTables (Id::pack))
            for (auto* row : model.getTableRows (*table))
                if (Toolchain::isRowSelected (model, *row, toolchainArgument)
                    and isFirstArchiveRow (model, *row, toolchainArgument))
                    if (const auto result { toArchive (model, Toolchain::getWorkingFile (model.getValue (*row, Id::archive)), toolchainArgument) };
                        not result.wasOk())
                        return result;

        return juce::Result::ok();
    }

private:
    /**
     * @brief Calls @p function for every selected row of @p archiveFile.
     *
     * @param model             The model that holds the @c ## pack tables.
     * @param archiveFile       The archive whose rows are visited.
     * @param toolchainArgument The CLI-selected toolchain group.
     * @param function          Callable invoked as @c function(row),
     *                          returning a juce::Result.
     * @returns juce::Result::ok() when @p function succeeds for every
     *          row, or the first failing result.
     */
    template <typename Function>
    static juce::Result forEachArchiveRow (const Model& model, const juce::File& archiveFile, const juce::String& toolchainArgument, Function&& function)
    {
        for (auto* table : model.getTables (Id::pack))
            for (auto* row : model.getTableRows (*table))
                if (Toolchain::isRowSelected (model, *row, toolchainArgument)
                    and Toolchain::getWorkingFile (model.getValue (*row, Id::archive)) == archiveFile)
                    if (const auto result { function (*row) }; not result.wasOk())
                        return result;

        return juce::Result::ok();
    }

    /**
     * @brief Answers whether @p row is the first selected row of its
     *        archive.
     *
     * @param model             The model that holds the @c ## pack tables.
     * @param row               The row to test.
     * @param toolchainArgument The CLI-selected toolchain group.
     * @returns @c true when no earlier selected row names the same
     *          archive.
     */
    static bool isFirstArchiveRow (const Model& model, const Model::Element& row, const juce::String& toolchainArgument)
    {
        const auto archiveFile { Toolchain::getWorkingFile (model.getValue (row, Id::archive)) };

        for (auto* table : model.getTables (Id::pack))
            for (auto* candidate : model.getTableRows (*table))
                if (Toolchain::isRowSelected (model, *candidate, toolchainArgument)
                    and Toolchain::getWorkingFile (model.getValue (*candidate, Id::archive)) == archiveFile)
                    return candidate == &row;

        return false;
    }

    /**
     * @brief Builds one archive, after it checks that every item of its
     *        rows exists.
     *
     * @param model             The model that holds the @c ## pack tables.
     * @param archiveFile       The archive to build. Its extension picks
     *                          the format.
     * @param toolchainArgument The CLI-selected toolchain group.
     * @returns juce::Result::ok() when the archive is built, or a failure
     *          naming the missing item or the failed step.
     */
    static juce::Result toArchive (const Model& model, const juce::File& archiveFile, const juce::String& toolchainArgument)
    {
        static const jam::Function::Map<juce::String, juce::Result> archiveFormats {
            []()
            {
                jam::Function::Map<juce::String, juce::Result> map;

                map.add<const Model&, const juce::File&, const juce::String&> (Id::zip, &Pack::toZip);
                map.add<const Model&, const juce::File&, const juce::String&> (Id::dmg, &Pack::toDmg);

                return map;
            }()
        };

        const auto found { forEachArchiveRow (model, archiveFile, toolchainArgument,
            [&model] (const Model::Element& row)
            {
                const auto item { Toolchain::getWorkingFile (model.getValue (row, Id::item)) };

                return item.exists()
                           ? juce::Result::ok()
                           : juce::Result::fail (item.getFullPathName() + Id::diagnosticSeparator
                                                 + text::Diagnostics::failNotFound);
            }) };

        return found.wasOk()
                   ? archiveFormats.get (archiveFile.getFileExtension().substring (1), model, archiveFile, toolchainArgument)
                   : found;
    }

    /**
     * @brief Writes @p archiveFile as a ZIP through ZipWriter.
     *
     * Each row adds its item. A row with a @c link also adds a symbolic
     * link entry named by its @c linkName.
     *
     * @param model             The model that holds the @c ## pack tables.
     * @param archiveFile       The archive to write.
     * @param toolchainArgument The CLI-selected toolchain group.
     * @returns The result of ZipWriter::toFile().
     */
    static juce::Result toZip (const Model& model, const juce::File& archiveFile, const juce::String& toolchainArgument)
    {
        juce::Array<juce::File> items;
        juce::StringPairArray links { false };

        const auto collected { forEachArchiveRow (model, archiveFile, toolchainArgument,
            [&model, &items, &links] (const Model::Element& row)
            {
                items.add (Toolchain::getWorkingFile (model.getValue (row, Id::item)));

                if (const auto link { model.getColumnValue (row, Id::link) }; link.isNotEmpty())
                    links.set (model.getColumnValue (row, Id::linkName), link);

                return juce::Result::ok();
            }) };

        return collected.wasOk() ? ZipWriter::toFile (archiveFile, items, links) : collected;
    }

    /**
     * @brief Writes @p archiveFile as a disk image.
     *
     * On macOS it copies the items and links into a stage folder beside
     * the archive, writes the @c .DS_Store, runs @c hdiutil, and deletes
     * the stage. On other hosts it fails.
     *
     * @param model             The model that holds the @c ## pack tables.
     * @param archiveFile       The disk image to write.
     * @param toolchainArgument The CLI-selected toolchain group.
     * @returns juce::Result::ok() when the image is built and the stage is
     *          removed, or a failure naming the stage, the archive, or the
     *          @c hdiutil command line.
     */
    static juce::Result toDmg (const Model& model, const juce::File& archiveFile, const juce::String& toolchainArgument)
    {
       #if JUCE_MAC
        const auto stage { archiveFile.getSiblingFile (archiveFile.getFileNameWithoutExtension()).getNonexistentSibling (false) };
        const auto stageFailure { juce::Result::fail (stage.getFullPathName() + Id::diagnosticSeparator
                                                      + text::Diagnostics::failOutputWrite) };

        if (stage.createDirectory().wasOk())
        {
            const auto arguments { getImageArguments (stage, archiveFile) };
            const auto staged { addStage (model, stage, archiveFile, toolchainArgument) };
            const auto created { staged.wasOk()
                                     ? Toolchain::runProcess (arguments, arguments.joinIntoString (juce::String::charToString (Chars::space)))
                                     : staged };
            const auto removed { stage.deleteRecursively() };

            return created.wasOk() ? (removed ? juce::Result::ok() : stageFailure) : created;
        }

        return stageFailure;
       #else
        juce::ignoreUnused (model, toolchainArgument);

        return juce::Result::fail (archiveFile.getFullPathName() + Id::diagnosticSeparator
                                   + text::Diagnostics::failArchiveHost);
       #endif
    }

   #if JUCE_MAC
    /**
     * @brief Returns the @c hdiutil command line that creates a
     *        compressed image.
     *
     * @param stage       The folder that the image is built from.
     * @param archiveFile The disk image to create. Its name without
     *                    extension is the volume name.
     * @returns The argv, with @c hdiutil first.
     */
    static juce::StringArray getImageArguments (const juce::File& stage, const juce::File& archiveFile)
    {
        static constexpr const char* createVerb { "create" };
        static constexpr const char* overwriteFlag { "-ov" };
        static constexpr const char* sourceFolderFlag { "-srcfolder" };
        static constexpr const char* volumeNameFlag { "-volname" };
        static constexpr const char* formatFlag { "-format" };
        static constexpr const char* compressedFormat { "UDZO" };

        return juce::StringArray { Id::hdiutil, createVerb, overwriteFlag, sourceFolderFlag, stage.getFullPathName(),
                                   volumeNameFlag, archiveFile.getFileNameWithoutExtension(),
                                   formatFlag, compressedFormat, archiveFile.getFullPathName() };
    }

    /**
     * @brief Fills @p stage with the items and the layout.
     *
     * @param model             The model that holds the @c ## pack tables.
     * @param stage             The stage folder.
     * @param archiveFile       The archive the stage is for.
     * @param toolchainArgument The CLI-selected toolchain group.
     * @returns The first failure of addStageItems() or addStageLayout(),
     *          or juce::Result::ok().
     */
    static juce::Result addStage (const Model& model, const juce::File& stage, const juce::File& archiveFile, const juce::String& toolchainArgument)
    {
        if (const auto result { addStageItems (model, stage, archiveFile, toolchainArgument) }; not result.wasOk())
            return result;

        return addStageLayout (model, stage, archiveFile, toolchainArgument);
    }

    /**
     * @brief Clones every item of @p archiveFile into @p stage and creates
     *        its symbolic link when the row declares one.
     *
     * @param model             The model that holds the @c ## pack tables.
     * @param stage             The stage folder.
     * @param archiveFile       The archive the stage is for.
     * @param toolchainArgument The CLI-selected toolchain group.
     * @returns juce::Result::ok() when every copy and link succeeds, or a
     *          failure naming the path that failed.
     */
    static juce::Result addStageItems (const Model& model, const juce::File& stage, const juce::File& archiveFile, const juce::String& toolchainArgument)
    {
        return forEachArchiveRow (model, archiveFile, toolchainArgument,
            [&model, &stage] (const Model::Element& row)
            {
                const auto item { Toolchain::getWorkingFile (model.getValue (row, Id::item)) };
                const auto copy { stage.getChildFile (item.getFileName()) };
                const auto link { model.getColumnValue (row, Id::link) };
                const auto linkFile { stage.getChildFile (model.getColumnValue (row, Id::linkName)) };

                if (copyfile (item.getFullPathName().toRawUTF8(), copy.getFullPathName().toRawUTF8(), nullptr,
                              COPYFILE_CLONE | COPYFILE_RECURSIVE) != 0)
                    return juce::Result::fail (copy.getFullPathName() + Id::diagnosticSeparator
                                               + text::Diagnostics::failOutputWrite);

                if (link.isNotEmpty() and not juce::File::createSymbolicLink (linkFile, link, true))
                    return juce::Result::fail (linkFile.getFullPathName() + Id::diagnosticSeparator
                                               + text::Diagnostics::failOutputWrite);

                return juce::Result::ok();
            });
    }

    /**
     * @brief Writes the @c .DS_Store of @p stage through
     *        BinaryWriter::getStore() when a @c ## pack layout table is
     *        declared.
     *
     * @param model             The model that holds the @c ## pack tables.
     * @param stage             The stage folder.
     * @param archiveFile       The archive the stage is for.
     * @param toolchainArgument The CLI-selected toolchain group.
     * @returns juce::Result::ok() when no layout is declared or the file
     *          is written, or a failure naming the @c .DS_Store path.
     */
    static juce::Result addStageLayout (const Model& model, const juce::File& stage, const juce::File& archiveFile, const juce::String& toolchainArgument)
    {
        const auto layoutTables { model.getTables (Id::packLayout) };
        const auto dsStore { stage.getChildFile (Id::dsStore) };

        if (layoutTables.size() > 0)
        {
            juce::StringArray itemNames;
            juce::StringArray linkNames;

            [[maybe_unused]] const auto collected { forEachArchiveRow (model, archiveFile, toolchainArgument,
                [&model, &itemNames, &linkNames] (const Model::Element& row)
                {
                    itemNames.add (Toolchain::getWorkingFile (model.getValue (row, Id::item)).getFileName());
                    linkNames.add (model.getColumnValue (row, Id::linkName));

                    return juce::Result::ok();
                }) };

            const auto store { BinaryWriter::getStore (model, *layoutTables.at (0), itemNames, linkNames) };

            return store.has_value() and dsStore.replaceWithData (store->getData(), store->getSize())
                       ? juce::Result::ok()
                       : juce::Result::fail (dsStore.getFullPathName() + Id::diagnosticSeparator
                                             + text::Diagnostics::failOutputWrite);
        }

        return juce::Result::ok();
    }
   #endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Pack)
};
