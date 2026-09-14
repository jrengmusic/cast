/**
 * @file jam_StyleShadow.h
 * @brief Frame-strip LookAndFeel for a shadow overlay mirroring a primary button's state.
 */

namespace jam
{
/*____________________________________________________________________________*/

/**
 * @class StyleShadow
 * @brief LookAndFeel for a shadow overlay component that draws itself by
 * reading its own down/toggle state — mirrored from its primary button by
 * `jam::OverlayImage::bindTo()` — and selecting its frame strip image
 * accordingly.
 */
class StyleShadow : public juce::LookAndFeel_V4
{
public:
    StyleShadow() = default;
    ~StyleShadow() = default;

    /**
     * @brief Sets the source frame strip image.
     * @param newImage  Frame strip image, containing the toggle/slider states.
     */
    void setImage (const juce::Image& newImage);
#if JUCE_MODULE_AVAILABLE_jam_gui
    /**
     * @brief Draws the frame matching @p button's own down/toggle state — the
     * shadow's mirrored copy of its primary's state, set by
     * `jam::OverlayImage::bindTo()`.
     * @param g                              Graphics context to draw into.
     * @param button                         The shadow component being drawn.
     * @param shouldDrawButtonAsHighlighted  Unused.
     * @param shouldDrawButtonAsDown         Unused.
     */
    void drawToggleButton (juce::Graphics& g,
                           juce::ToggleButton& button,
                           bool shouldDrawButtonAsHighlighted,
                           bool shouldDrawButtonAsDown) override;

    /**
     * @brief Draws the frame strip image at the y position matching the slider's value.
     * @param g              Graphics context to draw into.
     * @param x              Slider bounds x.
     * @param y              Slider bounds y.
     * @param width          Slider bounds width.
     * @param height         Slider bounds height.
     * @param sliderPos      Current slider position, in pixels.
     * @param minSliderPos   Minimum slider position, in pixels.
     * @param maxSliderPos   Maximum slider position, in pixels.
     * @param style          Slider style.
     * @param slider         The slider being drawn.
     */
    void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos, float minSliderPos, float maxSliderPos, const juce::Slider::SliderStyle style, juce::Slider& slider) override;
#endif // JUCE_MODULE_AVAILABLE_jam_gui
private:
    juce::Image image;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StyleShadow)
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
