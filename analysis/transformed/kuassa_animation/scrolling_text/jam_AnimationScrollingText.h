/**
 * @file jam_AnimationScrollingText.h
 * @brief Vertically scrolling ticker of one or more lines of text.
 */

namespace jam
{
/*__________________________________________________________________________________________*/

/**
 * @class AnimationScrollingText
 * @brief Scrolls lines of text upward and wraps back to the bottom once fully off-screen.
 */
class AnimationScrollingText : public AnimationBase
{
public:
    /**
     * @brief Constructs the animation with initial text.
     * @param lineOfText  Lines of text to scroll.
     */
    AnimationScrollingText (const juce::StringArray& lineOfText);

    AnimationScrollingText();
    ~AnimationScrollingText();

    //==============================================================================
    /**
     * @brief Sets the text colour.
     * @param newColour  Colour used to draw the text.
     */
    void setColour (const juce::Colour& newColour);

    /**
     * @brief Sets the font used to render the text.
     * @param newFont  Font to draw with.
     */
    void setFont (const juce::FontOptions& newFont);

    /**
     * @brief Replaces the lines of text being scrolled.
     * @param lineOfText  New lines of text. Ignored when empty.
     */
    void setText (const juce::StringArray& lineOfText);

    void start() override;
    void stop() override;
    //==============================================================================

    void paint (juce::Graphics& g) override;
    void timerCallback() override;

    //==============================================================================

    void resized() override;

private:
    juce::StringArray text;
    juce::FontOptions font;
    juce::Colour colour { juce::Colours::black };
    int yPosition { 0 };
    int textLinesHeight;
    juce::Rectangle<int> textArea;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AnimationScrollingText)
};

/**____________________________________END OF NAMESPACE____________________________________*/
}// namespace jam
