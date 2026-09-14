/**
 * @file jam_DebugBuildInfo.h
 * @brief DebugBuildInfo — build-identity overlay for plugin editors.
 */

#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

#if JUCE_DEBUG && JAM_USING_BUILD_INFO

namespace jam
{
/*____________________________________________________________________________*/

/**
 * @brief Displays build information at the top of the editor component.
 *
 * Shows the plugin name, version, wrapper format, build timestamp, and build
 * system (Ninja/Xcode/MSVC). Only compiled when JAM_USING_BUILD_INFO is
 * enabled.
 */
class DebugBuildInfo : public juce::TextEditor
{
public:
    /** @brief Builds the info text and attaches it to the given editor.
     *  @param editorToAdd  Host component this overlay is added to; ignored if null. */
    explicit DebugBuildInfo (juce::Component* editorToAdd)
    {
        if (editorToAdd == nullptr)
            return;

        setInterceptsMouseClicks (false, false);

        juce::StringArray lines;

#if JUCE_MODULE_AVAILABLE_juce_audio_processors
        lines.add (juce::String { ProjectInfo::projectName } + " v" + ProjectInfo::versionString
            + " " + juce::AudioProcessor::getWrapperTypeDescription (juce::PluginHostType::getPluginLoadedAs()));
#else
        lines.add (juce::String { ProjectInfo::projectName } + " v" + ProjectInfo::versionString);
#endif

        juce::String buildSystem { "Unknown" };

#if defined (__NINJA_BUILD__)
        buildSystem = "Ninja";
#elif defined (__XCODE_BUILD__)
        buildSystem = "Xcode";
#elif defined (__MSVC_BUILD__)
        buildSystem = "MSVC";
#endif

        lines.add ("Build " + getFormattedTimestamp() + " (" + buildSystem + ")");

        setReadOnly (true);
        setMultiLine (true);
        setText (lines.joinIntoString ("\n"), juce::dontSendNotification);
        setJustification (juce::Justification::centred);
        setAlwaysOnTop (true);

        editorToAdd->addAndMakeVisible (*this);
    }

private:
    /** @brief Formats `__DATE__`/`__TIME__` into a `MMM DD YYYY HH:MM` timestamp. */
    static inline juce::String getFormattedTimestamp()
    {
        juce::StringArray tokens;
        tokens.addTokens (__DATE__, false);
        tokens.move (0, 1);
        return tokens.joinIntoString (" ").toUpperCase() + " " + juce::String (__TIME__).upToLastOccurrenceOf (":", false, true);
    }

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DebugBuildInfo)
};

/**_____________________________END OF NAMESPACE______________________________*/
} // namespace jam

#endif// JUCE_DEBUG && JAM_USING_BUILD_INFO
