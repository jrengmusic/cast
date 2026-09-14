/**
 * @file jam_Raw.h
 * @brief Filename-keyed access to embedded JUCE BinaryData resources.
 */

namespace BinaryData
{
/*____________________________________________________________________________*/

/**
 * @brief Wrapper for accessing JUCE BinaryData resources by original filename.
 *
 * The Raw struct provides a convenient way to fetch binary resources
 * (such as fonts, images, or XML files) that have been embedded into
 * JUCE's BinaryData system. It resolves the original filename to the
 * corresponding resource data and size.
 */
struct Raw
{
    /**
     * @brief Construct a Raw object from a C‑string filename.
     *
     * Attempts to locate the resource with the given filename in the
     * BinaryData arrays. If found, @c data and @c size are set accordingly.
     *
     * @param fileToFind The original filename of the resource to locate.
     */
    explicit Raw (const char* fileToFind);

    /**
     * @brief Construct a Raw object from a JUCE String filename.
     *
     * Convenience overload that forwards to the const char* constructor.
     *
     * @param fileToFind The original filename of the resource to locate.
     */
    explicit Raw (const juce::String& fileToFind);

    /**
     * @brief Check whether the resource was successfully found.
     *
     * @return @c true if the resource exists and @c data is valid,
     *         @c false otherwise.
     */
    bool exists() const noexcept;

    /** Pointer to the resource data, or nullptr if not found. */
    const char* data { nullptr };

    /** Size of the resource in bytes, or 0 if not found. */
    int size { 0 };

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Raw)
};

/**
 * @brief Resource fetcher function returning a data/size pair from BinaryData.
 *
 * This function constructs a BinaryData::Raw object for the given filename
 * and returns its data pointer and size as a pair. It can be passed directly
 * as a ResourceFetcher callback.
 *
 * ### Example
 * @code
 * // Fetch a font resource directly with structured binding
 * if (auto [data, size] = BinaryData::fetcher ("OpenSans-Regular.ttf"); data != nullptr)
 * {
 *     juce::Font myFont (juce::Typeface::createSystemTypefaceFor (data, size));
 *     // use myFont...
 * }
 * @endcode
 *
 * @note If the resource is not found, this function returns {nullptr, 0}.
 *       This allows safe use without risk of crashes.
 *
 * @param filenameUTF8 The original filename of the resource to fetch.
 * @return A pair containing the resource data pointer and its size in bytes.
 *         If the resource is not found, the pointer will be nullptr and size 0.
 */
static inline std::pair<const void*, int> fetcher (const char* filenameUTF8)
{
    BinaryData::Raw r (filenameUTF8);
    return { r.data, r.size };
}

/**
 * @brief Invokes @p callback with the data pointer and byte size of every
 *        embedded BinaryData resource that resolves non-null.
 *
 * Iterates the generated resource table and, for each entry whose resolved
 * data pointer is non-null, calls @p callback with that pointer and its
 * byte size. Its definition lives in the translation unit that sees the
 * generated resource tables; it knows nothing about resource types.
 *
 * @param callback Invoked with each resource's data pointer and byte size.
 */
void forEachResource (void (*callback) (const void* data, int size));

void forEachResource (void (*callback) (const char* originalFilename, const void* data, int size));

/**
 * @brief Reads an embedded BinaryData resource as a decoded string.
 *
 * Resolves the original filename in BinaryData, then decodes the resource
 * bytes via `juce::String::createStringFromData`.
 *
 * @param resourceFileName Original filename of the embedded resource.
 * @return The decoded string; empty if the resource does not exist.
 */
static juce::String getString (const juce::String& resourceFileName)
{
    using namespace BinaryData;

    Raw binary (resourceFileName);

    return juce::String::createStringFromData (binary.data, binary.size);
}

static juce::String getString (const juce::Identifier& resourceName)
{
    return getString (resourceName.toString());
}

/**
 * @brief Creates a system typeface from an embedded font resource.
 *
 * Resolves the original filename in BinaryData, then registers the font
 * via `juce::Typeface::createSystemTypefaceFor`.  Returns `nullptr` if
 * the resource is not found.
 *
 * @param filenameUTF8  Original filename of the embedded TTF/OTF resource.
 * @return Registered typeface, or `nullptr` if the resource does not exist.
 */
static inline juce::Typeface::Ptr createTypeface (const char* filenameUTF8)
{
    juce::Typeface::Ptr result;
    BinaryData::Raw r (filenameUTF8);

    if (r.exists())
    {
        result = juce::Typeface::createSystemTypefaceFor (r.data, static_cast<size_t> (r.size));
    }

    return result;
}

/**_____________________________END OF NAMESPACE______________________________*/
}// namespace BinaryData
