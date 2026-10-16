#include "Pack.h"
#if JUCE_MAC
#include <copyfile.h>
#endif

const juce::String& Pack::getBackgroundFlag()
{
    static const juce::String backgroundFlag { Id::doubleDash + Id::background.toString() };
    return backgroundFlag;
}

juce::Result Pack::toArchive (const juce::ArgumentList& arguments, const juce::File& archive, const juce::StringArray& triples)
{
    jassert (triples.size() > 0 and triples.size() % tripleSize == 0);

    static const jam::Function::Map<juce::String, juce::Result> archiveFormats {
        []()
        {
            jam::Function::Map<juce::String, juce::Result> formats;

            formats.add<const juce::ArgumentList&, const juce::File&, const juce::Array<juce::File>&, const juce::StringArray&> (Id::zip, &Pack::toZip);
            formats.add<const juce::ArgumentList&, const juce::File&, const juce::Array<juce::File>&, const juce::StringArray&> (Id::dmg, &Pack::toDmg);

            return formats;
        }()
    };

    const auto extension { archive.getFileExtension().substring (1) };

    if (not archiveFormats.contains (extension))
        return juce::Result::fail (archive.getFullPathName() + Id::diagnosticSeparator
                                   + text::Diagnostics::failArchiveExtension);

    const auto items (getItems (triples));
    const auto found { isFound (arguments, items) };
    const auto unique { found.wasOk() ? isUnique (archive, items, triples) : found };

    return unique.wasOk()
               ? archiveFormats.get (extension, arguments, archive, items, triples)
               : unique;
}

juce::Array<juce::File> Pack::getItems (const juce::StringArray& triples)
{
    juce::Array<juce::File> items;

    for (int row { 0 }; row < triples.size() / tripleSize; ++row)
        items.add (juce::File::getCurrentWorkingDirectory().getChildFile (triples[row * tripleSize]));

    return items;
}

juce::File Pack::getBackground (const juce::ArgumentList& arguments)
{
    return juce::File::getCurrentWorkingDirectory().getChildFile (arguments.getValueForOption (getBackgroundFlag()));
}

juce::String Pack::getLinkName (const juce::StringArray& triples, int row)
{
    return triples[row * tripleSize + linkNameOffset];
}

juce::String Pack::getLinkTarget (const juce::StringArray& triples, int row)
{
    return triples[row * tripleSize + linkTargetOffset];
}

bool Pack::hasLink (const juce::StringArray& triples, int row)
{
    return getLinkName (triples, row).isNotEmpty() and getLinkTarget (triples, row).isNotEmpty();
}

juce::Result Pack::isFound (const juce::ArgumentList& arguments, const juce::Array<juce::File>& items)
{
    for (const auto& item : items)
        if (not item.exists())
            return juce::Result::fail (item.getFullPathName() + Id::diagnosticSeparator
                                       + text::Diagnostics::failNotFound);

    if (arguments.containsOption (getBackgroundFlag()))
    {
        const auto background { getBackground (arguments) };

        return background.existsAsFile()
                   ? juce::Result::ok()
                   : juce::Result::fail (background.getFullPathName() + Id::diagnosticSeparator
                                         + text::Diagnostics::failNotFound);
    }

    return juce::Result::ok();
}

juce::Result Pack::isUnique (const juce::File& archive, const juce::Array<juce::File>& items, const juce::StringArray& triples)
{
    juce::StringArray rootNames;

    for (int row { 0 }; row < items.size(); ++row)
    {
        rootNames.add (items.getReference (row).getFileName());

        if (hasLink (triples, row))
            rootNames.add (getLinkName (triples, row));
    }

    for (int index { 0 }; index < rootNames.size(); ++index)
        if (rootNames.indexOf (rootNames[index]) != index)
            return juce::Result::fail (archive.getFullPathName()
                                       + Id::diagnosticSeparator
                                       + text::Diagnostics::failDuplicate + rootNames[index]
                                       + Chars::doubleQuote);

    return juce::Result::ok();
}

juce::Result Pack::toZip (const juce::ArgumentList& arguments, const juce::File& archive, const juce::Array<juce::File>& items, const juce::StringArray& triples)
{
    if (BinaryWriter::hasLayout (arguments) or arguments.containsOption (getBackgroundFlag()))
        return juce::Result::fail (archive.getFullPathName() + Id::diagnosticSeparator
                                   + text::Diagnostics::failPackOption);

    juce::StringPairArray links { false };

    for (int row { 0 }; row < items.size(); ++row)
        if (hasLink (triples, row))
            links.set (getLinkName (triples, row), getLinkTarget (triples, row));

   #if JUCE_MAC
    return toStage (archive, items, [&archive, &links] (const juce::File&, const juce::Array<juce::File>& stagedItems)
    {
        return ZipWriter::toFile (archive, stagedItems, links);
    });
   #else
    return ZipWriter::toFile (archive, items, links);
   #endif
}

juce::Result Pack::toDmg (const juce::ArgumentList& arguments, const juce::File& archive, const juce::Array<juce::File>& items, const juce::StringArray& triples)
{
   #if JUCE_MAC
    return toStage (archive, items, [&arguments, &archive, &items, &triples] (const juce::File& stage, const juce::Array<juce::File>&)
    {
        const auto imageArguments { getImageArguments (stage, archive) };
        const auto staged { addStage (arguments, stage, archive, items, triples) };

        return staged.wasOk()
                   ? Toolchain::runProcess (imageArguments, imageArguments.joinIntoString (juce::String::charToString (Chars::space)))
                   : staged;
    });
   #else
    juce::ignoreUnused (arguments, items, triples);

    return juce::Result::fail (archive.getFullPathName() + Id::diagnosticSeparator
                               + text::Diagnostics::failArchiveHost);
   #endif
}

#if JUCE_MAC
juce::StringArray Pack::getImageArguments (const juce::File& stage, const juce::File& archiveFile)
{
    static constexpr const char* createVerb { "create" };
    static constexpr const char* overwriteFlag { "-ov" };
    static constexpr const char* sourceFolderFlag { "-srcfolder" };
    static constexpr const char* volumeNameFlag { "-volname" };
    static constexpr const char* fileSystemFlag { "-fs" };
    static constexpr const char* fileSystem { "HFS+" };
    static constexpr const char* formatFlag { "-format" };
    static constexpr const char* compressedFormat { "UDZO" };

    return juce::StringArray { Id::hdiutil, createVerb, overwriteFlag, sourceFolderFlag, stage.getFullPathName(),
                               volumeNameFlag, archiveFile.getFileNameWithoutExtension(),
                               fileSystemFlag, fileSystem,
                               formatFlag, compressedFormat, archiveFile.getFullPathName() };
}

juce::Result Pack::addStage (const juce::ArgumentList& arguments, const juce::File& stage, const juce::File& archive, const juce::Array<juce::File>& items, const juce::StringArray& triples)
{
    if (const auto result { addStageLinks (stage, triples, items.size()) }; not result.wasOk())
        return result;

    if (const auto result { addStageBackground (arguments, stage) }; not result.wasOk())
        return result;

    return addStageLayout (arguments, stage, archive, items, triples);
}

juce::Result Pack::addStageItems (const juce::Array<juce::File>& items, const juce::Array<juce::File>& stagedItems)
{
    for (int index { 0 }; index < items.size(); ++index)
    {
        const auto& item { items.getReference (index) };
        const auto& copy { stagedItems.getReference (index) };

        if (copyfile (item.getFullPathName().toRawUTF8(), copy.getFullPathName().toRawUTF8(), nullptr,
                      COPYFILE_CLONE | COPYFILE_RECURSIVE) != 0)
            return juce::Result::fail (copy.getFullPathName() + Id::diagnosticSeparator
                                       + text::Diagnostics::failOutputWrite);

        if (const auto result { IconWriter::toBundle (copy) }; not result.wasOk())
            return result;
    }

    return juce::Result::ok();
}

juce::Result Pack::addStageLinks (const juce::File& stage, const juce::StringArray& triples, int rowCount)
{
    for (int row { 0 }; row < rowCount; ++row)
    {
        if (hasLink (triples, row))
        {
            const auto linkFile { stage.getChildFile (getLinkName (triples, row)) };

            if (not juce::File::createSymbolicLink (linkFile, getLinkTarget (triples, row), true))
                return juce::Result::fail (linkFile.getFullPathName() + Id::diagnosticSeparator
                                           + text::Diagnostics::failOutputWrite);
        }
    }

    return juce::Result::ok();
}

juce::Array<juce::File> Pack::getStagedItems (const juce::File& stage, const juce::Array<juce::File>& items)
{
    juce::Array<juce::File> stagedItems;

    for (const auto& item : items)
        stagedItems.add (stage.getChildFile (item.getFileName()));

    return stagedItems;
}

juce::String Pack::getBackgroundName (const juce::ArgumentList& arguments)
{
    return arguments.containsOption (getBackgroundFlag())
               ? Id::backgroundPrefix + getBackground (arguments).getFileExtension()
               : juce::String();
}

juce::Result Pack::addStageBackground (const juce::ArgumentList& arguments, const juce::File& stage)
{
    if (arguments.containsOption (getBackgroundFlag()))
    {
        const auto background { getBackground (arguments) };
        const auto backgroundCopy { stage.getChildFile (getBackgroundName (arguments)) };

        if (copyfile (background.getFullPathName().toRawUTF8(), backgroundCopy.getFullPathName().toRawUTF8(), nullptr, COPYFILE_CLONE) != 0)
            return juce::Result::fail (backgroundCopy.getFullPathName() + Id::diagnosticSeparator
                                       + text::Diagnostics::failOutputWrite);
    }

    return juce::Result::ok();
}

juce::Result Pack::addStageLayout (const juce::ArgumentList& arguments, const juce::File& stage, const juce::File& archive, const juce::Array<juce::File>& items, const juce::StringArray& triples)
{
    const auto dsStore { stage.getChildFile (Id::dsStore) };
    juce::StringArray itemNames;
    juce::StringArray linkNames;

    for (int row { 0 }; row < items.size(); ++row)
    {
        itemNames.add (items.getReference (row).getFileName());
        linkNames.add (hasLink (triples, row) ? getLinkName (triples, row) : juce::String());
    }

    const auto store { BinaryWriter::getStore (arguments, itemNames, linkNames, archive.getFileNameWithoutExtension(), getBackgroundName (arguments)) };

    return store.has_value() and dsStore.replaceWithData (store->getData(), store->getSize())
               ? juce::Result::ok()
               : juce::Result::fail (dsStore.getFullPathName() + Id::diagnosticSeparator
                                     + text::Diagnostics::failOutputWrite);
}
#endif
