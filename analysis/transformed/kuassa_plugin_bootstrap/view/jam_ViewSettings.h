/**
 * @file jam_ViewSettings.h
 * @brief Settings dialog view, built from SettingsLayout.html with its
 *        state persisted to the user's settings XML file.
 */

#pragma once

namespace jam
{
/*____________________________________________________________________________*/

/**
 * @class ViewSettings
 * @brief ViewContent specialisation for the settings dialog. Inherits the
 *        padding band and close button from ViewContent, and loads any
 *        persisted settings state into the built component tree.
 */
class ViewSettings : public ViewContent
{
public:
    //==============================================================================
    /**
     * @brief Builds the settings component tree and loads any persisted
     *        settings state into it.
     * @param model          Audio model passed to ViewManager::buildContent().
     * @param contentLayout  Parsed SettingsLayout.html document.
     * @return The built settings view.
     */
    static std::unique_ptr<ViewSettings> create (AudioModel& model, ViewPanel::Layout contentLayout)
    {
        std::unique_ptr<ViewSettings> settingsView { new ViewSettings (contentLayout) };

        auto* target { settingsView->getTargetComponent() };
        target->setComponentID (Id::toTag (Id::settings));

        ViewManager::buildContent (target, model, settingsView->children, settingsView->layout);

        loadPersistedSettings (*settingsView, target);

        settingsView->setScaledSize();

        return settingsView;
    }

    ~ViewSettings() = default;

private:
    /**
     * @brief Converts a persisted settings XML element into a ValueTree,
     *        recursively, carrying every string property except `text`.
     * @param element  Persisted settings XML element.
     * @return The equivalent ValueTree.
     */
    static juce::ValueTree toValueTree (const Document::Element& element)
    {
        juce::ValueTree tree { element.id };

        element.applyToProperties (
            [&tree] (const juce::Identifier& key, const Document::Value& value)
            {
                if (key != Id::text)
                    if (const auto* text { std::get_if<juce::String> (&value) })
                        tree.setProperty (key, *text, nullptr);
            });

        for (auto* child : element)
            tree.appendChild (toValueTree (*child), nullptr);

        return tree;
    }

    /**
     * @brief Loads the user's persisted settings XML, if present and valid,
     *        into the settings model and attaches the built component tree
     *        to it, wiring further changes back to the file.
     * @param settingsView  Settings view owning the settings model and file.
     * @param target        Built settings component tree to attach.
     */
    static void loadPersistedSettings (ViewSettings& settingsView, juce::Component* target)
    {
        if (auto* parameter { ParameterManager::getInstance() })
        {
            auto& settings { settingsView.settings };
            auto& settingsFile { settingsView.settingsFile };

            settingsFile = parameter->getUserSettings();

            if (settingsFile.existsAsFile())
            {
                if (const auto document { Xml::parse (settingsFile.loadFileAsString()) };
                    not document.root->id.isNull())
                {
                    auto state { toValueTree (*document.root) };

                    if (state.isValid())
                    {
                        settings.replaceState (state);
                        ViewManager::attachSettings (settings, target);

                        settings.onValueChanged = [&settings, &settingsFile]
                        {
                            settings.writeToXml (settingsFile);
                        };
                    }
                    else
                    {
#if JUCE_DEBUG
                        jam::debug::Log::write ("ViewSettings::loadPersistedSettings: invalid settings state: " + settingsFile.getFullPathName());
#endif
                        jassertfalse;
                    }
                }
                else
                {
#if JUCE_DEBUG
                    jam::debug::Log::write ("ViewSettings::loadPersistedSettings: malformed settings XML: " + settingsFile.getFullPathName());
#endif
                    jassertfalse;
                }
            }
        }
    }

    /**
     * @brief Forwards to ViewContent's constructor.
     * @param contentLayout  Parsed SettingsLayout.html document.
     */
    ViewSettings (ViewPanel::Layout contentLayout)
        : ViewContent (contentLayout)
    {
    }

    /** Settings state tree, persisted to `settingsFile`. */
    jam::SettingsModel settings;
    /** User's settings XML file, resolved from ParameterManager. */
    juce::File settingsFile;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ViewSettings)
};

/**_____________________________END_OF_NAMESPACE______________________________*/
} /** namespace jam */
