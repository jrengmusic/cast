/**
 * @file jam_Scrollbar.h
 * @brief Interactive proportional scrollbar — wraps juce::ScrollBar, bound via jam::Value.
 *
 * Inherits jam::Model::ValueComponent for ValueTree attachment. The scroll offset is
 * a juce::Value bound to a ValueTree property via referTo — bidirectional sync,
 * no manual callback. LookAndFeel-driven rendering identical to juce::Viewport.
 */

#pragma once

namespace jam
{
/*____________________________________________________________________________*/

/**
 * @brief Interactive proportional scrollbar wrapping juce::ScrollBar.
 *
 * Owns a juce::Value representing scroll offset. Caller attaches it to a
 * ValueTree property via getValueObject().referTo(). When the user drags
 * or clicks, the Value updates the property. When code writes the property,
 * the scrollbar thumb moves.
 *
 * LookAndFeel drives all rendering — SSOT for scrollbar appearance.
 *
 */
class Scrollbar : public juce::Component,
                  public jam::Model::ValueComponent<Scrollbar>,
                  private juce::ScrollBar::Listener,
                  private juce::Value::Listener
{
public:
    /** @brief Constructs, showing the (non-auto-hiding) scrollbar and wiring listeners
     *  on both the wrapped juce::ScrollBar and the bound juce::Value.
     */
    explicit Scrollbar() noexcept;

    /** @brief Sets the total capacity and visible row count for thumb sizing.
     *
     *  @param capacity    Total scrollable range (history row count).
     *  @param visibleRows Visible viewport row count (determines thumb size).
     */
    void setRange (int capacity, int visibleRows) noexcept;

    /** @brief Returns the bound juce::Value (jam::Model::Object interface). */
    juce::Value& getValueObject() noexcept override;

    /** @internal */
    void resized() override;

private:
    /** @brief Maps the dragged juce::ScrollBar position back to scrollValue. */
    void scrollBarMoved (juce::ScrollBar* bar, double newRangeStart) override;
    /** @brief Maps a written scrollValue back onto the juce::ScrollBar's current range. */
    void valueChanged (juce::Value& value) override;

    juce::ScrollBar scrollBar { true };  ///< Vertical. LookAndFeel-driven rendering.
    juce::Value scrollValue;             ///< Bound to ValueTree scrollOffset property.
    int currentCapacity { 0 };
    int currentVisibleRows { 0 };

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Scrollbar)
};

/**______________________________END OF NAMESPACE______________________________*/
} // namespace jam
