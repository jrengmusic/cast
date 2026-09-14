/**
 * @file jam_StyleKnob.h
 * @brief Frame-strip based rotary/linear slider LookAndFeel.
 */

namespace jam
{
/*____________________________________________________________________________*/

/**
 * @class StyleKnob
 * @brief LookAndFeel that renders sliders from a vertical or horizontal frame
 * strip image, selecting the frame for `RotaryVerticalDrag` from the slider
 * position and drawing `LinearVertical` as a moving thumb image.
 */
class StyleKnob : public juce::LookAndFeel_V4
{
public:
    StyleKnob()
    {
        juce::LookAndFeel_V4::setColour (juce::TextEditor::focusedOutlineColourId, juce::Colour());
        juce::LookAndFeel_V4::setColour (juce::Slider::textBoxOutlineColourId, juce::Colour());
    }

    ~StyleKnob() = default;

    //==============================================================================

    /**
     * @brief Sets the source frame strip image.
     * @param newImage  Frame strip image, containing consecutive slider frames.
     */
    void setImage (const juce::Image& newImage)
    {
        image = newImage;
    }

    /**
     * @brief Sets whether the frame strip is arranged horizontally.
     * @param shouldBeHorizontal  `true` for a horizontal strip, `false` for vertical.
     */
    void setImageOrientation (bool shouldBeHorizontal)
    {
        isHorizontal = shouldBeHorizontal;
    }

    /** @return The last slider position drawn by drawLinearSlider(). */
    float getSliderPosition() const noexcept
    {
        return sliderPosition;
    }

    //==============================================================================
    /** Returns a full-bounds layout for `RotaryVerticalDrag`, or an inset layout for `LinearVertical`. */
    juce::Slider::SliderLayout getSliderLayout (juce::Slider& slider) override
    {
        juce::Slider::SliderLayout layout;

        switch (slider.getSliderStyle())
        {
            case juce::Slider::RotaryVerticalDrag:
            {
                auto area { slider.getLocalBounds() };
                juce::Slider::SliderLayout layout;
                layout.sliderBounds = area;
                layout.textBoxBounds = area.removeFromBottom (0);
            }
            break;

            case juce::Slider::LinearVertical:
            {
                int deltaY { image.getHeight() / 2 };
                layout.sliderBounds = slider.getLocalBounds().reduced (2, deltaY);
            }
            break;

            default:
                break;
        }

        return layout;
    }

    /** Draws the frame strip's thumb image centred at the current slider position. */
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
        slider.setSliderSnapsToMousePosition (true);

        sliderPosition = sliderPos;

        juce::Rectangle<int> area { x, y, width, height };

        juce::Rectangle<int> thumbBounds {
            area.getCentreX() - image.getWidth() / 2,
            (int) sliderPos - image.getHeight() / 2,
            image.getWidth(),
            image.getHeight()
        };

        g.drawImageAt (image, area.getCentreX() - image.getWidth() / 2, sliderPos - image.getHeight() / 2);
    }

    /** Draws the frame strip frame selected by the slider's normalised position. */
    void drawRotarySlider (juce::Graphics& g,
                           int x,
                           int y,
                           int width,
                           int height,
                           float sliderPos,
                           const float rotaryStartAngle,
                           const float rotaryEndAngle,
                           juce::Slider& slider) override
    {
        int frame { jam::Value::map (sliderPos, 0, getNumFrames (slider) - 1) };

        drawImage (g, frame, slider);
    }

    //==============================================================================
private:
    juce::Image image;
    bool isHorizontal { false };
    float sliderPosition;

    //==============================================================================
    /**
     * @brief Number of frames in the strip, given the slider's own dimension.
     * @param slider  Slider whose width/height divides the strip's length.
     * @return Frame count.
     */
    int getNumFrames (juce::Slider& slider) const noexcept
    {
        return isHorizontal ? image.getWidth() / slider.getWidth()
                            : image.getHeight() / slider.getHeight();
    }

    /**
     * @brief Draws a single frame from the strip, scaled to the slider's bounds.
     * @param g       Graphics context to draw into.
     * @param frame   Zero-based frame index within the strip.
     * @param slider  Slider supplying the destination bounds.
     */
    void drawImage (juce::Graphics& g, int frame, juce::Slider& slider)
    {
        if (image.isValid())
        {
            int imagePosition { isHorizontal ? frame * slider.getWidth()
                                             : frame * slider.getHeight() };

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

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StyleKnob)
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
