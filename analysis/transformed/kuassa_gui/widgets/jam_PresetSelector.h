/**
 * @file jam_PresetSelector.h
 * @brief Preset file selector — extends Selector with a dirty-state marker and preset load/attach hooks.
 */
namespace jam
{
/*____________________________________________________________________________*/

/**
 * @class PresetSelector
 * @brief `Selector` populated with preset files, with a leading dirty-state
 * marker indicating unsaved changes since the last preset load.
 */
class PresetSelector : public Selector
{
public:
    enum ColourIds
    {
        dirtyColourId = map::ColourId::presetSelectorDirtyColourId,
    };

    /** @brief Constructs, adding the dirty marker as a visible child. */
    PresetSelector()
    {
        jam::Component::addAndMakeVisible<DirtyMarker> (this, marker);
    }

    ~PresetSelector() = default;

    /**
     * @brief Binds this selector to an audio model for preset load/save.
     * @param modelToUse  Audio model to bind to.
     */
    void setModel (AudioModel& modelToUse)
    {
        if (modelToUse.createDefaultInitPreset())
        {
            if (auto* param { ParameterManager::getInstance() })
            {
                juce::PopupMenu presetMenu;
                files = jam::menu::addPresets (presetMenu,
                                               param->getFactoryDefaultPresetsDirectory (Id::all.toString()),
                                               param->getUserPresetsDirectory(),
                                               param->getPresetExtension());
                addItems (std::move (presetMenu));

                onChange = loadPreset (modelToUse);
                onAttachment = attachValues (modelToUse);
            }
        }
    }

    /** Lays the base selector out, then positions the dirty marker at the left edge. */
    void resized() override
    {
        Selector::resized();

        auto bounds { getLocalBounds() };
        auto markerArea { bounds.removeFromLeft (bounds.getHeight()) };
        marker->setBounds (markerArea);
    }

    //==============================================================================

    /** Rescans the preset directories and repopulates the item list. */
    void refreshItems() override
    {
        if (auto* param { ParameterManager::getInstance() })
        {
            juce::PopupMenu presetMenu;
            files = jam::menu::addPresets (presetMenu,
                                           param->getFactoryDefaultPresetsDirectory (Id::all.toString()),
                                           param->getUserPresetsDirectory(),
                                           param->getPresetExtension());
            addItems (std::move (presetMenu));
        }
    }

    /**
     * @return A callback that attaches this selector's value to the bound model's preset parameter.
     * @param modelToUse  Audio model to bind to.
     */
    std::function<void()> attachValues (AudioModel& modelToUse)
    {
        return [this, &modelToUse]()
        {
            if (auto* param { ParameterManager::getInstance() })
            {
                selected.referTo (modelToUse.getPresetValue (Id::value));
                file.referTo (modelToUse.getPresetValue (Id::file));
                marker->getValueObject().referTo (modelToUse.getPresetValue (Id::isDirty));

                auto userPresetDirectory { param->getUserPresetsDirectory() };
                auto currentPresetFile { userPresetDirectory.getChildFile (modelToUse.getCurrentPresetFileName()) };

                if (file.toString().compare (currentPresetFile.getFullPathName()))
                {
                    file.setValue (currentPresetFile.getFullPathName());
                    marker->setDirty (not currentPresetFile.existsAsFile());
                }

                if (modelToUse.isPresetDirty())
                {
                    setSelectedId (0);
                    setTextWhenNothingSelected (modelToUse.getCurrentPresetName());
                }
                else
                {
                    if (auto presetId { getItemId (modelToUse.getCurrentPresetName()) };
                        presetId >= 1)
                    {
                        setSelectedId (presetId);
                    }
                }
            }
        };
    }

    /**
     * @return A callback that loads the currently selected preset file.
     * @param modelToUse  Audio model to bind to.
     */
    std::function<void()> loadPreset (AudioModel& modelToUse)
    {
        return [this, &modelToUse]()
        {
            if (auto* param { ParameterManager::getInstance() })
            {
                if (files.size())
                {
                    if (auto index { param->getPresetIndex (files, modelToUse.getCurrentPresetFileName()) }; index >= 0)
                    {
                        modelToUse.loadPreset (files.getReference (index));
                        setPresetDirty (false);

                        auto currentPreset { modelToUse.getCurrentPresetName() };
                        auto fileName { modelToUse.getCurrentPresetFile().getFileNameWithoutExtension() };

                        if (currentPreset.compare (fileName) != 0)
                        {
                            setText (fileName);
                        }
                    }
                }
            }
        };
    }

    /**
     * @brief Sets the dirty-state marker.
     * @param shouldBeDirty  `true` to mark unsaved changes.
     */
    void setPresetDirty (bool shouldBeDirty) { marker->setDirty (shouldBeDirty); }

    //==============================================================================
private:
    juce::Array<juce::File> files;
    juce::Value file;

    //==============================================================================
    /** @brief Small circular indicator shown when the preset has unsaved changes. */
    class DirtyMarker
        : public juce::Component
        , public Model::ValueComponent<DirtyMarker>
        , juce::Value::Listener
    {
    public:
        /** @brief Constructs the marker. */
        DirtyMarker()
            : Model::ValueComponent<DirtyMarker> (Id::isDirty)
        {
            setInterceptsMouseClicks (false, true);
            dirty.addListener (this);
        }

        ~DirtyMarker() = default;

        /** Draws a filled circle when dirty; draws nothing otherwise. */
        void paint (juce::Graphics& g) override
        {
            if ((bool) dirty.getValue())
            {
                const auto& area = [this]
                {
                    auto bounds { getLocalBounds().toFloat() };
                    auto size { 0.5f * bounds.getHeight() };
                    return bounds.withSizeKeepingCentre (size, size);
                }();

                g.setColour (findColour (PresetSelector::dirtyColourId));
                g.fillEllipse (area);
            }
        }

        /** Repaints when the dirty value changes. */
        void valueChanged (juce::Value& value) override { repaint(); }

        /** @return Reference to the underlying `juce::Value` holding the dirty state. */
        juce::Value& getValueObject() noexcept override { return dirty; }

        //==============================================================================
        /**
         * @brief Sets the dirty state.
         * @param shouldBeDirty  `true` to mark unsaved changes.
         */
        void setDirty (bool shouldBeDirty) { dirty = shouldBeDirty; }

        /** @return `true` when marked dirty. */
        bool isDirty() const noexcept { return dirty.getValue(); }

        //==============================================================================
    private:
        juce::Value dirty { juce::var (false) };
        //==============================================================================
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DirtyMarker)
    };

    std::unique_ptr<DirtyMarker> marker;

    //==============================================================================
};

/**_____________________________END OF NAMESPACE______________________________*/
}// namespace jam
