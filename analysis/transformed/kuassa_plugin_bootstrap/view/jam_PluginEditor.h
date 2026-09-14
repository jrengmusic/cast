/**
 * @file jam_PluginEditor.h
 * @brief Abstract plugin editor base -- Vulkan engine ownership, the
 *        editor/panel init sequence, appearance and bypass dispatch, and
 *        registry-driven callback wiring.
 */

#pragma once

namespace jam
{
/*____________________________________________________________________________*/

/**
 * @class PluginEditor
 * @brief Abstract base for the plugin's AudioProcessorEditor. Owns the
 *        Vulkan rendering engine, theme, view, and panel; concrete editors
 *        implement the `initialise*` hooks called from initialise(), which
 *        also wires the appearance/bypass/oversampling registry callbacks
 *        and the desktop/settings-file listeners.
 */
class PluginEditor
    : public juce::AudioProcessorEditor
    , public juce::FocusChangeListener
    , public juce::DarkModeSettingListener
    , public File::Watcher::Listener
    , public juce::ValueTree::Listener
{
public:
    /**
     * @brief Constructs the Vulkan engine at the given size and stores the
     *        model and layout references used throughout initialisation.
     * @param processorToConnectTo  Owning audio processor.
     * @param newModel              Audio model this editor observes and mutates.
     * @param newLayout             Plugin editor layout, validated by the concrete editor.
     * @param uiSize                Initial Vulkan swapchain extent.
     */
    PluginEditor (juce::AudioProcessor& processorToConnectTo, AudioModel& newModel, PluginEditorLayout& newLayout, Size<int> uiSize)
        : juce::AudioProcessorEditor (&processorToConnectTo)
        , vulkanEngine { std::in_place,
                         vk::Extent2D { static_cast<uint32_t> (uiSize.unpack<0>()),
                                        static_cast<uint32_t> (uiSize.unpack<1>()) } }
        , model (newModel)
        , layout (newLayout)
    {
    }

    /**
     * @brief Tears down in strict lifecycle order: removes this editor's
     *        peer from the Vulkan engine before any other teardown, so a
     *        native paint during removal cannot resurrect a swapchain on
     *        the dying window; then dismisses any modal, removes listeners,
     *        and clears the LookAndFeel.
     */
    ~PluginEditor() override
    {
        vulkanEngine->removePeer (*this);

        jam::Component::dismissModal (this);
        settingsWatcher.removeListener (this);

        auto& desktop { juce::Desktop::getInstance() };
        desktop.removeDarkModeSettingListener (this);
        desktop.removeFocusChangeListener (this);

        model.state.removeListener (this);

        setLookAndFeel (nullptr);
    }

    /**
     * @brief Follows the OS dark-mode change when the plugin's default
     *        appearance is set to track the desktop.
     */
    void darkModeSettingChanged() override
    {
        const juce::String defaultAppearance { ParameterManager::getInstance()->getDefaultAppearance() };

        if (defaultAppearance.equalsIgnoreCase (
                map::Appearance::getInstance()->get (map::Appearance::automatic)))
        {
            const juce::String appearance { map::Appearance::getInstance()->get (
                StyleManager::isDark (defaultAppearance) ? map::Appearance::dark : map::Appearance::light) };

            model.setAppearance (appearance);
        }
    }

    /** @brief Unused -- this editor does not react to global focus changes. */
    void globalFocusChanged (juce::Component*) override {}

    /**
     * @brief Stores the host's desktop scale on the model and re-lays the
     *        editor out at the new scale.
     * @param newScale  Desktop scale factor reported by the host.
     */
    void setScaleFactor (float newScale) override
    {
        juce::AudioProcessorEditor::setScaleFactor (newScale);
        model.setDesktopScale (newScale);
        resized();
    }

    /**
     * @brief Bounds the panel above the editor view (reserving an extra
     *        panel row under an AU sandbox host), then bounds and re-lays
     *        the editor view out below it.
     */
    void resized() override
    {
        const auto extraPanelHeight { isAUSandboxHost() ? model.getUIPanelHeight() : 0 };

        if (panel != nullptr)
        {
            panel->setBounds (getLocalBounds().withTrimmedBottom (extraPanelHeight));
        }

        view->setBounds (view->getViewBounds (model));

        jam::Component::resizeEditor (*view, model);
    }

    /** @brief Re-applies the theme's font rasterisation and embolden settings after a LookAndFeel change. */
    void lookAndFeelChanged() override
    {
        theme->setFontRasterization();
        theme->setEmbolden();
    }

    /**
     * @brief Captures a snapshot for the appearance transition, then applies
     *        the model's current appearance to the theme and view.
     */
    virtual void setAppearance()
    {
        transition.start (createComponentSnapshot (getLocalBounds()));

        theme->setAppearance (model.getAppearance());
        view->setAppearance();
        sendLookAndFeelChange();
    }

    /** @brief Applies the model's current bypass state to the editor view. */
    virtual void setBypassed() { view->setBypassed(); }

#if JAM_USING_OVERSAMPLING
    /** @brief Applies the model's current oversampling factor to the concrete editor. Implemented by the concrete editor. */
    virtual void setOversamplingFactor() = 0;
#endif// JAM_USING_OVERSAMPLING

    /**
     * @brief Dispatches a registry callback, asynchronously on the message
     *        thread, for a value-tree node whose `id` matches a registered
     *        callback key.
     * @param tree      Value-tree node whose property changed.
     * @param property  Changed property; only `Id::value` triggers a dispatch.
     */
    void valueTreePropertyChanged (juce::ValueTree& tree, const juce::Identifier& property) override
    {
        if (property == Id::value and tree.hasProperty (Id::id))
        {
            const juce::String key { tree.getProperty (Id::id).toString() };

            if (registry and registry->callbacks.contains (key))
            {
                auto safeEditor { juce::Component::SafePointer<PluginEditor> (this) };

                juce::MessageManager::callAsync (
                    [safeEditor, key]
                    {
                        if (safeEditor != nullptr)
                            safeEditor->registry->callbacks.get (key, static_cast<juce::Component&> (*safeEditor));
                    });
            }
        }
    }

    //==============================================================================
protected:
    /** Coalescing window, in milliseconds, for the settings-file watcher. */
    static constexpr int coalesceWindowTimeMs { 300 };

    /** @brief Builds `theme` and applies the model's initial appearance. Implemented by the concrete editor. */
    virtual void initialiseTheme() = 0;

    /** @brief Builds `registry` and registers its component factories and bind callbacks. Implemented by the concrete editor. */
    virtual void initialiseRegistry() = 0;

    /** @brief Builds `panel` (and, where applicable, additional panel rows). Implemented by the concrete editor. */
    virtual void initialisePanels() = 0;

    /** @brief Builds `view`. Implemented by the concrete editor. */
    virtual void initialiseView() = 0;

    /** @brief Wires panel-triggered runtime callbacks once the panel and registry exist. Implemented by the concrete editor. */
    virtual void attachPanelCallbacks() = 0;

    /** @brief Adds any editor-specific listeners once the view and panel exist. Implemented by the concrete editor. */
    virtual void initialiseListeners() = 0;

    /**
     * @brief Runs the full initialisation sequence: theme, registry,
     *        appearance/bypass/oversampling callback registration, panels,
     *        panel callbacks, view, then desktop and settings-file listeners.
     */
    void initialise()
    {
        initialiseTheme();
        initialiseRegistry();

        if (registry)
        {
            registry->callbacks.add<juce::Component&> (Id::toType (Id::appearance),
                                                        [] (juce::Component& c)
                                                        {
                                                            static_cast<PluginEditor&> (c).setAppearance();
                                                        });

            registry->callbacks.add<juce::Component&> (Id::toType (Id::bypass),
                                                        [] (juce::Component& c)
                                                        {
                                                            static_cast<PluginEditor&> (c).setBypassed();
                                                        });

#if JAM_USING_OVERSAMPLING
            registry->callbacks.add<juce::Component&> (Id::toType (Id::oversampling),
                                                        [] (juce::Component& c)
                                                        {
                                                            static_cast<PluginEditor&> (c).setOversamplingFactor();
                                                        });
#endif// JAM_USING_OVERSAMPLING
        }

        initialisePanels();

        attachPanelCallbacks();
        initialiseView();

        sendLookAndFeelChange();
        model.state.addListener (this);
        juce::Desktop::getInstance().addFocusChangeListener (this);
        juce::Desktop::getInstance().addDarkModeSettingListener (this);

        settingsWatcher.addListener (this);
        settingsWatcher.addFolder (ParameterManager::getUserSettingsDirectory());
        settingsWatcher.coalesceEvents (coalesceWindowTimeMs);

        initialiseListeners();
    }

    /** Shared Vulkan rendering engine, owning this editor's swapchain peer. */
    SharedInstance<VulkanEngine> vulkanEngine;
    /** Shared style manager resolving colours, fonts, and appearance. */
    SharedInstance<StyleManager> styleManager;
    /** Concrete theme, built by initialiseTheme(). */
    std::unique_ptr<StyleTheme> theme;
    /** Main editor view, built by initialiseView(). */
    std::unique_ptr<ViewEditor> view;
    /** Top panel row, built by initialisePanels(). */
    std::unique_ptr<ViewPanel> panel;
    /** Shared component registry, built by initialiseRegistry(). */
    SharedInstance<Registry> registry;
    /** Audio model this editor observes and mutates. */
    AudioModel& model;
    /** Plugin editor layout, validated by the concrete editor. */
    PluginEditorLayout& layout;

    /** Crossfade transition played on an appearance change. */
    ImageTransition transition { [this] { repaint(); } };
    /** File chooser used by license-import and preset dialogs. */
    FileChooser fileChooser { this };
    /** Watches the user settings folder for external file changes. */
    File::Watcher settingsWatcher;

private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};

/**_____________________________END_OF_NAMESPACE______________________________*/
} /** namespace jam */
