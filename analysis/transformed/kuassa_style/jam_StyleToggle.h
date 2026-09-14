/**
 * @file jam_StyleToggle.h
 * @brief Frame-strip LookAndFeel for toggle buttons and IncDecButtons-style sliders.
 */

namespace jam
{
/*____________________________________________________________________________*/

/**
 * @class StyleToggle
 * @brief LookAndFeel drawing toggle buttons and sliders from a frame strip image,
 * selecting the frame from enabled/toggle/pressed state.
 */
class StyleToggle : public juce::LookAndFeel_V4
{
public:
    StyleToggle() { setColour (juce::Slider::textBoxOutlineColourId, juce::Colour()); }

    ~StyleToggle() = default;

    //==============================================================================
    /**
     * @brief Sets the source frame strip image.
     * @param newImage  Frame strip image, containing the disabled/off/on/down frames.
     */
    void setImage (const juce::Image& newImage) { image = newImage; }

    /**
     * @brief Sets an unused overlay image.
     * @param newImage  Overlay image.
     */
    void setImageOverlay (const juce::Image& newImage) { imageOverlay = newImage; }

    /**
     * @brief Sets whether the frame strip is arranged horizontally.
     * @param shouldBeHorizontal  `true` for a horizontal strip, `false` for vertical.
     */
    void setImageOrientation (bool shouldBeHorizontal) { isHorizontal = shouldBeHorizontal; }

    //==============================================================================
    /** Draws the frame matching the button's disabled/off/on/down state. */
    void drawToggleButton (juce::Graphics& g,
                           juce::ToggleButton& button,
                           bool shouldDrawButtonAsHighlighted,
                           bool shouldDrawButtonAsDown) override
    {
        if (image.isValid())
        {
            const int frame = [this, &button, shouldDrawButtonAsDown]
            {
                if (button.isEnabled())
                    return jam::toInt (shouldDrawButtonAsDown ? down : (button.getToggleState() + off));

                return jam::toInt (disabled);
            }();

            int imagePosition { isHorizontal ? frame * button.getWidth() : frame * button.getHeight() };

            int imageX { isHorizontal ? imagePosition : 0 };
            int imageY { isHorizontal ? 0 : imagePosition };

            g.drawImage (image,
                         0,
                         0,
                         button.getWidth(),
                         button.getHeight(),
                         imageX,
                         imageY,
                         button.getWidth(),
                         button.getHeight());
        }
    }

    /** Hides the slider's text box, then draws the frame matching the slider's value/mouse state. */
    void drawLinearSlider (juce::Graphics& g,
                           int x,
                           int y,
                           int width,
                           int height,
                           float sliderPos,
                           float minSliderPos,
                           float maxSliderPos,
                           const juce::Slider::SliderStyle style,
                           juce::Slider& slider) override
    {
        slider.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);

        if (image.isValid())
        {
            int numFrames { isHorizontal ? image.getWidth() / slider.getWidth()
                                         : image.getHeight() / slider.getHeight() };

            int frame { 0 };

            if (numFrames == 2)
            {
                frame = (slider.isEnabled() && slider.getValue() > 0.0) ? 1 : 0;
            }
            else
            {
                int downFrame { numFrames - 1 };

                if (slider.isEnabled())
                    frame = slider.isMouseButtonDown()
                                ? downFrame
                                : jam::Value::map (jam::Value::normalise (sliderPos, (height + 1), 1), 1, downFrame - 1);
                else
                    frame = disabled;
            }

            int imagePosition { isHorizontal ? frame * slider.getWidth() : frame * slider.getHeight() };

            int imageX { isHorizontal ? imagePosition : 0 };
            int imageY { isHorizontal ? 0 : imagePosition };

            g.drawImage (image,
                         0,
                         0,
                         slider.getWidth(),
                         slider.getHeight(),
                         imageX,
                         imageY,
                         slider.getWidth(),
                         slider.getHeight());
        }
    }

private:
    juce::Image image;
    juce::Image imageOverlay;
    bool isHorizontal { false };

    /** @brief Frame indices within the strip. */
    enum
    {
        disabled,
        off,
        on,
        down
    };
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StyleToggle)
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
