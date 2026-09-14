/**
 * @file jam_SliderUtils.h
 * @brief Slider configuration helpers.
 */
namespace jam
{
/*____________________________________________________________________________*/

/**
 * @brief Configures a slider to display values as percentages.
 *
 * @tparam SliderPointer A pointer type for sliders.
 * @param slider A pointer to the slider to configure.
 * @param isPositiveOnly Whether the range is restricted to positive values (default: true).
 * @param min The minimum value for the slider range (default: 0.0).
 * @param max The maximum value for the slider range (default: 100.0).
 * @param increment The step size for the slider range (default: 1.0).
 */
template <typename SliderPointer>
static void makePercent (SliderPointer* slider,
                         bool isPositiveOnly = true,
                         double min = 0.0,
                         double max = 100.0,
                         double increment = 1.0)
{
    slider->setRange (isPositiveOnly ? min : -max, max, increment);
    slider->textFromValueFunction = [] (auto value)
    {
        return juce::String (value) + " @";
    };
    slider->setDoubleClickReturnValue (true, 0.0);
}

/**_____________________________END OF NAMESPACE______________________________*/
}// namespace jam
