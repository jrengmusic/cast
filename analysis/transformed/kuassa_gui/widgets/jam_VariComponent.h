/**
 * @file jam_VariComponent.h
 * @brief Group-switcher container whose visible child group is driven by map::Display.
 */
#pragma once

namespace jam
{
/*____________________________________________________________________________*/

/**
 * @brief Group-switcher container whose active group is driven by map::Display.
 *
 * VariComponent owns N child components partitioned into display groups. A
 * right-click cycles through groups defined by map::Display (e.g. "NUMBERS" →
 * numeric knobs visible, "ANALYZER" → analyzer visible). The active group key
 * is a juce::Value persisted as a string property on the related parameter's
 * ValueTree node under the key Id::variDisplay.
 *
 * Lifecycle:
 *  1. setGroups() partitions children by component index.
 *  2. bindToModel() seeds activeIndex from map::Display::getDefault() if empty,
 *     then attaches to the AudioModel ValueTree and restores saved group.
 *  3. Right-click triggers cycleNext(), which advances the active key through
 *     map::Display's ordered entries and updates activeIndex.
 *  4. valueChanged() re-applies the group visibility on every activeIndex change
 *     (e.g. from a remote value tree listener).
 *
 * Requires a live map::Display context (PluginProcessor or test harness must
 * declare one). If getInstance() returns nullptr, bindToModel and cycleNext are
 * no-ops.
 */
class VariComponent
    : public juce::Component
    , public Model::ValueComponent<VariComponent>
    , private juce::Value::Listener
{
public:
    /** @brief Constructs the component, registers as Component, and subscribes to activeIndex. */
    VariComponent()
        : Model::ValueComponent<VariComponent> (Id::variComponent)
    {
        activeIndex.addListener (this);
        addMouseListener (this, true);
        setBufferedToImage (true);
        setComponentEffect (&matte);
        matte.setFeather (defaultFeatherRadius);
    }

    ~VariComponent() override { activeIndex.removeListener (this); }

    /** @brief Returns the activeIndex Value for external seeding (e.g. registerConfig). */
    juce::Value& getValueObject() noexcept override { return activeIndex; }

    /** @brief Repaints when enablement changes. */
    void enablementChanged() override { repaint(); }

    /** @brief Sets the alpha-mask image used by the ImageMatte effect and triggers a repaint.
     *  @param image  ARGB image whose alpha channel is applied as the clip mask;
     *                pass an invalid image to clear the mask. */
    void setClipImage (const juce::Image& image) noexcept
    {
        matte.setClipImage (image);
        repaint();
    }

    /**
     * @brief Partitions children into named display groups by component index.
     *
     * Each inner vector lists the child indices (zero-based, in child-order)
     * that are visible when that group is active. Children not in the active
     * group are hidden. Children are attached in index order by the builder;
     * group index to child index is stable by construction.
     *
     * @param newGroups Ordered list of groups; index 0 is the first display mode.
     */
    void setGroups (jam::Array<jam::Array<int>> newGroups) noexcept { groups = std::move (newGroups); }

    /**
     * @brief Applies the display mode to every child StyleVariDisplay and
     * broadcasts the change.
     *
     * @param mode  One of `map::VariDisplayMode::value` (hz, khz, note).
     */
    void setDisplayMode (int mode)
    {
        for (auto* child : getChildren())
        {
            if (auto* laf { dynamic_cast<jam::StyleVariDisplay*> (&child->getLookAndFeel()) })
                laf->setDisplayMode (mode);
        }

        sendLookAndFeelChange();
    }

    void bindToModel (const juce::String& parameterID, AudioModel& state)
    {
        if (parameterID.isNotEmpty())
        {
            if (auto* m { map::Display::getInstance() })
            {
                if (activeIndex.getValue().toString().isEmpty())
                    activeIndex.setValue (m->getDefault());

                state.attach (activeIndex, parameterID, Id::variDisplay);
                setActiveGroup (m->get (activeIndex.getValue().toString()) - 1);
            }
        }
    }

    /** @brief Fills every child to bounds. */
    void resized() override
    {
        const auto bounds { getLocalBounds() };

        for (auto* child : getChildren())
            child->setBounds (bounds);
    }

    /** @brief Right-click cycles to the next display group via cycleNext(). */
    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.mods.isRightButtonDown())
        {
            cycleNext();
        }
    }

private:
    /** @brief Feather radius (pixels) applied to the ImageMatte effect on construction. */
    static constexpr float defaultFeatherRadius { 2.0f };

    /** @brief Re-applies group visibility from the map::Display-mapped activeIndex. */
    void valueChanged (juce::Value&) override
    {
        if (auto* m { map::Display::getInstance() })
            setActiveGroup (m->get (activeIndex.getValue().toString()) - 1);
    }

    /**
     * @brief Shows children in the specified group and hides all others.
     *
     * When groups is empty every child is shown. An out-of-range groupIndex
     * clamps to 0 (first group). Always repaints.
     *
     * @param groupIndex Zero-based index into the groups vector.
     */
    void setActiveGroup (int groupIndex)
    {
        if (groups.isEmpty())
        {
            for (auto* child : getChildren())
                child->setVisible (true);
        }
        else
        {
            if (groupIndex < 0 or groupIndex >= groups.size())
                groupIndex = 0;

            const auto& activeGroup { groups.at (groupIndex) };

            for (int i { 0 }; i < getNumChildComponents(); ++i)
            {
                auto* child { getChildComponent (i) };
                const bool inGroup { std::find (activeGroup.begin(), activeGroup.end(), i) != activeGroup.end() };
                child->setVisible (inGroup);
            }
        }

        repaint();
    }

    /**
     * @brief Advances activeIndex to the next entry in map::Display's ordered map.
     *
     * Wraps around to the first entry after the last. No-op when fewer than two entries
     * exist or no map::Display context is live. Writing activeIndex triggers
     * valueChanged(), which calls setActiveGroup().
     */
    void cycleNext()
    {
        if (auto* m { map::Display::getInstance() })
        {
            const auto& entries { m->get() };
            const auto current { activeIndex.getValue().toString() };

            if (entries.size() >= 2 and m->contains (current))
            {
                const int nextKey { (static_cast<int> (m->get (current)) % static_cast<int> (entries.size())) + 1 };
                activeIndex.setValue (m->get (nextKey));
            }
        }
    }

    juce::Value activeIndex;
    jam::Array<jam::Array<int>> groups;
    ImageMatte matte;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VariComponent)
};

/**_____________________________END OF NAMESPACE______________________________*/
}// namespace jam
