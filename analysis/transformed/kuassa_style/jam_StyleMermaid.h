/**
 * @file jam_StyleMermaid.h
 * @brief Manager-driven LookAndFeel for mermaid diagrams.
 */

namespace jam
{
/*____________________________________________________________________________*/

/**
 * @class StyleMermaid
 * @brief The resolved style for a Mermaid diagram.
 *
 * Wraps a StyleManager to expose the Mermaid stylesheet's registered colour
 * ids, the shared font, and per-metric measurements keyed by the
 * `Mermaid::StyleSheet` ordinals. Layout and draw passes read it through
 * `getMetrics()` and `getFont()`.
 */
class StyleMermaid : public StyleCustom
{
public:
    /**
     * @brief Constructs the style.
     *
     * @param styleManager The owning style manager.
     * @param appearance   The initial appearance (defaults to dark).
     */
    explicit StyleMermaid (StyleManager& styleManager, juce::StringRef appearance = Id::dark)
        : style (styleManager)
    {
        setAppearance (appearance);
    }
    ~StyleMermaid() override = default;

    /**
     * @brief Applies the given appearance's Mermaid stylesheet.
     *
     * @param lightOrDark The appearance identifier (Id::light / Id::dark).
     */
    void setAppearance (juce::StringRef lightOrDark)
    {
        style.setAppearanceForStyle (files::mermaidStyleSheet, *this, lightOrDark);
    }

    /** @brief Returns the shared Mermaid font, sized by the state-font-size metric. */
    juce::Font getFont() const noexcept
    {
        return juce::Font { style.getFont (Id::common) }.withPointHeight (style.getMetrics<float> (
            Mermaid::StyleSheet::getInstance()->get (Mermaid::StyleSheet::stateFontSize)));
    }

    /**
     * @brief Returns metric @p metricId, scaled by the state-font-size metric.
     *
     * @tparam NumberType The numeric type to cast the measurement to.
     * @param metricId    The Mermaid::StyleSheet metric ordinal.
     * @return The scaled metric value.
     */
    template<typename NumberType = float>
    NumberType getMetrics (int metricId) const noexcept
    {
        return static_cast<NumberType> (
            style.getMetrics<double> (Mermaid::StyleSheet::getInstance()->get (metricId))
            * style.getMetrics<double> (
                Mermaid::StyleSheet::getInstance()->get (Mermaid::StyleSheet::stateFontSize)));
    }

private:
    StyleManager& style;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StyleMermaid)
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
