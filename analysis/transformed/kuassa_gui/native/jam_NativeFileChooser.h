/**
 * @file jam_NativeFileChooser.h
 * @brief Native (NSOpenPanel / IFileOpenDialog) directory picker.
 */
#if JUCE_MAC || JUCE_WINDOWS

namespace jam
{
/*____________________________________________________________________________*/

/** @brief Static namespace-like wrapper around the platform's native directory picker. */
struct NativeFileChooser
{
    /**
     * @brief Opens the native directory picker.
     * @param parentWindow  Window to attach the picker to.
     * @param startingPath  Initial directory shown by the picker.
     * @param onSelected    Called with the chosen directory path.
     */
    static void openDirectory (juce::Component* parentWindow,
                               const juce::String& startingPath,
                               std::function<void (const juce::String&)> onSelected);
};

/**_____________________________END OF NAMESPACE______________________________*/
}// namespace jam

#endif
