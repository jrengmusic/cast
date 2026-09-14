#include <JuceHeader.h>

// Fallback resource table used when no generated BinaryData resources are linked.
namespace BinaryDataFallbacks
/* ____________________________________________________________________________*/
{
const int namedResourceListSize { 0 };
const char** namedResourceList { nullptr };

const char* getNamedResource (const char* resourceName, int& size) { return nullptr; }

const char* getNamedResourceOriginalFilename (const char* resourceName) { return nullptr; }
/**_____________________________END OF NAMESPACE______________________________*/
} /** namespace BinaryDataFallbacks */

namespace BinaryData
{
using namespace BinaryDataFallbacks;

Raw::Raw (const char* fileToFind)
{
    for (int index { 0 }; index < namedResourceListSize; ++index)
    {
        auto binaryName { namedResourceList[index] };
        auto fileName { getNamedResourceOriginalFilename (binaryName) };

        if (not strcmp (fileName, fileToFind))
        {
            data = getNamedResource (binaryName, size);
            break;
        }
    }
}

bool Raw::exists() const noexcept
{
    return not (data == nullptr);
}

Raw::Raw (const juce::String& fileToFind)
    : Raw (static_cast<const char*> (fileToFind.toUTF8()))
{
}

void forEachResource (void (*callback) (const void* data, int size))
{
    for (int index { 0 }; index < namedResourceListSize; ++index)
    {
        auto binaryName { namedResourceList[index] };
        int size { 0 };
        const char* data { getNamedResource (binaryName, size) };

        if (data != nullptr)
            callback (data, size);
    }
}

void forEachResource (void (*callback) (const char* originalFilename, const void* data, int size))
{
    for (int index { 0 }; index < namedResourceListSize; ++index)
    {
        auto binaryName { namedResourceList[index] };
        auto originalFilename { getNamedResourceOriginalFilename (binaryName) };
        int size { 0 };
        const char* data { getNamedResource (binaryName, size) };

        if (data != nullptr)
            callback (originalFilename, data, size);
    }
}
/**_____________________________END OF NAMESPACE______________________________*/
}// namespace BinaryData
