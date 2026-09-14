/**
 * @file jam_PopupTextBox.h
 * @brief Fade-in-on-hover slider popup with exclusive-visibility sibling coordination.
 */
#pragma once

namespace jam
{
/*____________________________________________________________________________*/

/**
 * @brief A hidden-by-default slider popup that fades in on mouse interaction
 *        with a bound primary component, and shows its text editor on right-click.
 *
 * Only one PopupTextBox among its siblings stays visible at a time —
 * becoming visible fades out any other showing PopupTextBox sibling.
 */
class PopupTextBox : public juce::Slider
{
public:
    using Events = jam::MouseEvents<juce::Slider>;
    //==============================================================================
    PopupTextBox() { setBufferedToImage (true); }

    /** @brief Fades out any other visible sibling PopupTextBox when this one becomes visible. */
    void visibilityChanged() override { hideOthers(); }

    /**
     * @brief Binds this popup's fade/show behavior to a primary component's mouse events.
     *
     * No-op if @p primary does not implement Events (`jam::MouseEvents<juce::Slider>`).
     *
     * @param primary The component whose mouse events drive this popup's visibility.
     */
    void bindTo (juce::Component* primary)
    {
        if (auto* p { dynamic_cast<Events*> (primary) })
            hide (p);
    }

    //==============================================================================
private:
    /** @brief Wires fade-in/fade-out and show-editor callbacks onto p's mouse events. */
    void hide (Events* p)
    {
        setVisible (false);

        if (auto* label = getLabel())
        {
            label->onEditorHide = [this]
            {
                setVisible (false);
            };
        }

        auto fadeIn = [this]
        {
            if (not isBeingEdited() and not isVisible())
                jam::Animator::toggleFade (this, true);
        };

        auto fadeOut = [this]
        {
            if (not isBeingEdited())
                jam::Animator::toggleFade (this, false);
        };

        auto show = [this]
        {
            if (isVisible())
                showTextBox();
        };

        p->onMouseEnter = fadeIn;
        p->onMouseDrag = fadeIn;
        p->onMouseWheelMove = fadeIn;
        p->onMouseExit = fadeOut;
        p->onRightClick = show;
    }

    /** @return The first child juce::Label (the slider's text-box label), or nullptr. */
    juce::Label* getLabel() const noexcept
    {
        for (auto& child : getChildren())
            if (auto* label = dynamic_cast<juce::Label*> (child))
                return label;
        return nullptr;
    }

    /** @return true when the text-box label's inline editor is currently open. */
    bool isBeingEdited() const noexcept
    {
        if (auto* label = getLabel())
            return label->isBeingEdited();
        return false;
    }

    /** @brief Fades out every other visible PopupTextBox sibling under the same parent. */
    void hideOthers()
    {
        if (auto* parent = getParentComponent())
        {
            for (auto* child : parent->getChildren())
            {
                if (auto* textBox = dynamic_cast<PopupTextBox*> (child))
                    if (textBox != this and textBox->isShowing())
                        jam::Animator::toggleFade (textBox, false);
            }
        }
    }
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PopupTextBox)
};

/**_____________________________END OF NAMESPACE______________________________*/
}// namespace jam
