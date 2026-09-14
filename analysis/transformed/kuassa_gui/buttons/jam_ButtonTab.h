/**
 * @file        jam_ButtonTab.h
 * @brief       SVG-backed tab button with editable Label.
 */
#pragma once

namespace jam
{
/*____________________________________________________________________________*/

/**
 * @brief SVG-backed tab button with an owned juce::Label for editable text display.
 *
 * Extends ButtonSVG — no paintButton override. SVG graphics are painted by the
 * consuming LookAndFeel via drawTabButton. The Label paints itself via
 * StyleCustom::drawTabLabel — same delegation pattern as ButtonSVG::paintButton.
 *
 * Text source: Label reflects getButtonText() transformed by
 * StyleCustom::getTabText() (e.g. uppercase). Font and colour are
 * synced from the LAF in lookAndFeelChanged() and buttonStateChanged().
 *
 * Vertical rotation: ButtonBar calls setLabelLayout() to apply rotation transform and
 * swapped bounds whenever orientation changes or tab positions are updated.
 *
 * Editing: showEditor() activates the Label's inline text editor. The editor is
 * a standard juce::Label editor — commit on return/focus-lost.
 *
 * @see jam::ButtonSVG
 * @see jam::StyleCustom
 */
class ButtonTab : public ButtonSVG
{
public:
    /** @brief Default constructor. Adds the label as a visible child and sets up defaults. */
    ButtonTab()
    {
        addAndMakeVisible (label);
        label.setJustificationType (juce::Justification::centred);
        label.setInterceptsMouseClicks (false, false);
        label.setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
        label.setEditable (false, false, false);
    }

    /** @brief Positions the label within button bounds. */
    void resized() override
    {
        label.setBounds (getLocalBounds());
    }

    /** @brief Sets the label transform and bounds for vertical bar rotation.
     *  Called by ButtonBar when orientation changes or tab positions are updated.
     *  @param transform  Rotation transform for the label (identity for horizontal).
     *  @param bounds     Label bounds in the rotated coordinate space.
     */
    void setLabelLayout (const juce::AffineTransform& transform, juce::Rectangle<int> bounds)
    {
        label.setTransform (transform);
        label.setBounds (bounds);
    }

    /** @brief Activates the label's inline text editor. */
    void showEditor()
    {
        label.showEditor();
    }

    /** @brief ButtonTab-owned label that delegates paint to the custom LAF method.
     *  Same pattern as ButtonBar::Background and ButtonBar::SlidingHighlight.
     */
    struct TabLabel : public juce::Label
    {
        /** @brief Paint via StyleCustom::drawTabLabel. */
        void paint (juce::Graphics& g) override
        {
            static_cast<jam::StyleCustom&> (getLookAndFeel())
                .drawTabLabel (g, *this);
        }
    };

    /** @brief Editable text label. Public for Value binding (getTextValue().referTo()). */
    TabLabel label;

private:
    /**
     * @brief Syncs label text colour from toggle state (textColourOnId / textColourOffId).
     */
    void buttonStateChanged() override
    {
        label.setColour (juce::Label::textColourId,
            findColour (getToggleState() ? juce::TextButton::textColourOnId
                                         : juce::TextButton::textColourOffId));
    }

    /**
     * @brief Syncs label font and text transform from StyleCustom.
     */
    void lookAndFeelChanged() override
    {
        auto& custom { static_cast<jam::StyleCustom&> (getLookAndFeel()) };
        label.setFont (custom.getTabFont());
        label.setText (custom.getTabText (getButtonText()), juce::dontSendNotification);
    }

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ButtonTab)
};

/**______________________________END OF NAMESPACE______________________________*/
} // namespace jam
