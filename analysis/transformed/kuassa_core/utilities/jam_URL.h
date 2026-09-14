/**
 * @file jam_URL.h
 * @brief Consuming-project website URL, defined by the downstream build.
 */

namespace jam
{
/*____________________________________________________________________________*/

struct URL
{
    /** @brief Returns the consuming project's website URL. */
    static const juce::String getWebsite() noexcept;
};

/**_____________________________END OF NAMESPACE______________________________*/
}// namespace jam
