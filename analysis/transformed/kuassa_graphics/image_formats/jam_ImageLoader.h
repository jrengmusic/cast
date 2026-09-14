namespace jam
{
/*____________________________________________________________________________*/

static constexpr int pngSignatureSize { 8 };
static constexpr uint8_t pngSignature[pngSignatureSize] { 0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a };

static bool isPngSignature (const void* data, int size) noexcept
{
    return data != nullptr and size >= pngSignatureSize
        and std::memcmp (data, pngSignature, static_cast<size_t> (pngSignatureSize)) == 0;
}

/**
 * @brief Engine-owned decode/memoization cache for every embedded image resource.
 *
 * Reached process-wide via jam::Instance<ImageLoader>::getInstance(), and
 * held as the VulkanEngine::imageLoader member for the engine's lifetime.
 * Every getFromBinary overload resolves the requested filename through
 * BinaryData::Raw and memoizes the decoded result keyed by that resource's
 * stable BinaryData byte pointer (getOrRasterize) — the same pointer always
 * maps to the same juce::Image once decoded, so repeated lookups of the same
 * resource are a lock-guarded map hit rather than a re-decode. Format
 * dispatch is by content, not extension: a leading PNG signature routes
 * through this TU's vendored SIMD jam::decodePng; anything else falls
 * back to juce::ImageFileFormat::loadFrom. A decode that produces an invalid
 * juce::Image is never stored in the memo, so a later call retries the
 * decode rather than caching a failure. Owns decodeJobs, its own
 * juce::ThreadPool driving background decode-and-memoize work queued by
 * registerImages().
 */
class ImageLoader : public jam::Instance<ImageLoader>
{
public:
    /**
     * @brief Retrieves an image from a binary resource file.
     *
     * Resolves @p resourceName through BinaryData::Raw, then memoizes the
     * decoded result by that resource's byte pointer via
     * ImageLoader::getOrRasterize. A leading PNG signature decodes through
     * jam::decodePng (SIMD defilter, premultiplied result); any other
     * format falls back to juce::ImageFileFormat::loadFrom. Asserts if the
     * resource does not exist.
     *
     * @param resourceName The name of the binary resource file.
     * @return A juce::Image created from the binary resource.
     */
    static juce::Image getFromBinary (const juce::String& resourceName);

    /**
     * @brief Retrieves an image from a binary resource file, by Identifier.
     *
     * Forwards to the juce::String overload.
     *
     * @param resourceName The name of the binary resource file.
     * @return A juce::Image created from the binary resource.
     */
    static juce::Image getFromBinary (const juce::Identifier& resourceName)
    {
        return getFromBinary (resourceName.toString());
    }

    /**
     * @brief Queues every embedded BinaryData resource for background decode-and-memoize.
     *
     * Enumerates every embedded resource via BinaryData::forEachResource and
     * hands each one to decodeJobs as a job that calls getOrRasterize on it.
     * A resource whose bytes fail the PNG signature check and are then
     * rejected by juce::ImageFileFormat::loadFrom stores nothing in the
     * memo — non-image resources are silently skipped this way. Safe to
     * call once, at engine construction; every later getFromBinary call
     * either hits the memo populated here or decodes on demand.
     */
    void registerImages();

private:
    juce::Image getOrRasterize (const void* data, size_t size) noexcept;

    // Guards images map lookups and insertions only — never held across a
    // decode, so concurrent getOrRasterize calls for different resources
    // decode in parallel and only serialize on the map access itself.
    juce::CriticalSection imagesLock;

    // Memo of decoded images, keyed by the resource's stable BinaryData byte
    // pointer.
    jam::HashMap<const void*, juce::Image> images;

    // Decode/memoize jobs queued by registerImages. Declared after images and
    // imagesLock so it destructs first, joining every outstanding decode job
    // while the map and lock are still alive.
    juce::ThreadPool decodeJobs;
};

/**_____________________________END OF NAMESPACE______________________________*/
} /** namespace jam */
