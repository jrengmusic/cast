/**
 * @file jam_AnimationScrambledText.h
 * @brief Animated transition between lines of text via character scrambling.
 */

namespace jam
{
/*____________________________________________________________________________*/

/**
 * @class AnimationScrambledText
 * @brief Cycles through lines of text, transitioning between them by scrambling
 * outgoing characters and un-scrambling incoming ones.
 *
 * Each cycle holds the current line for `holdDuration` ticks, then transitions over
 * `transition` ticks: a scrambling phase randomises characters of the outgoing line,
 * followed by an arranging phase that reveals the incoming line one character at a
 * time, both eased via getEaseInOut().
 */
class AnimationScrambledText : public AnimationBase
{
public:
    /**
     * @brief Constructs the animation with hold/transition timing.
     * @param holdDurationInSecond        Time each line is held before transitioning, in seconds.
     * @param transitionDurationInSecond  Time spent scrambling/arranging between lines, in seconds.
     * @param intervalHz                  Timer interval, in Hz.
     */
    AnimationScrambledText (float holdDurationInSecond = 4.0f, float transitionDurationInSecond = 2.0f, int intervalHz = 30);

    ~AnimationScrambledText() = default;

    //==============================================================================
    void timerCallback() override;

    void paint (juce::Graphics& g) override;

    //==============================================================================
    /**
     * @brief Sets the font used to render the text.
     * @param newFont  Font to draw with.
     */
    void setFont (const juce::FontOptions& newFont);

    /**
     * @brief Sets the lines of text to cycle through.
     * @param newLineOfText       Lines of text, padded and shuffled internally.
     * @param shouldBeUpperCase   `true` to convert lines to upper case.
     */
    void setText (const juce::StringArray& newLineOfText, bool shouldBeUpperCase = true);

    void start() override;

    void stop() override;

private:
    //==============================================================================
    juce::StringArray linesOfText;
    juce::String text;
    juce::FontOptions font;
    int index { 0 };
    int nextIndex { index + 1 };
    int scrambleCharIndex { 0 };
    int arrangeCharIndex { 0 };
    int holdDuration;
    int transition;
    int scramblingDuration;
    int arrangingDuration;
    int tick { 0 };

    std::vector<int> scrambleIndex;
    std::vector<int> arrangeIndex;

    float easeControl { 0.9f };
    float scramblingDurationProportion { 0.3f };
    float easeExponent { 0.75f };

    bool isTextReady { false };
    bool isScramblingPhase;
    bool isArrangingPhase;

    inline static const juce::String whitespace { juce::String::charToString (0xA0) };

    //==============================================================================
    /**
     * @brief Pads lines to a common width and returns them in randomised order.
     * @param linesOfText        Source lines of text.
     * @param shouldBeUpperCase  `true` to convert lines to upper case.
     * @return Padded lines, shuffled.
     */
    static const juce::StringArray getShuffled (const juce::StringArray& linesOfText, bool shouldBeUpperCase);

    /** @return A random alphanumeric character. */
    static const juce::juce_wchar getRandomAlphanumeric() noexcept;

    /** @return A random alphanumeric character, as a one-character string. */
    static const juce::String getRandomAlphanumericString() noexcept;

    /**
     * @brief Replaces the next scramble-ordered character with a random alphanumeric.
     * @param textToScramble  Text to mutate.
     * @param shuffledIndex   Character positions in scramble order.
     * @param index           Current position within @p shuffledIndex; advanced by one call.
     * @return Text with one character replaced.
     */
    static const juce::String
    getScrambled (const juce::String& textToScramble, const std::vector<int>& shuffledIndex, int& index) noexcept;

    /**
     * @brief Replaces the next arrange-ordered character with the destination character.
     * @param textToScramble    Text to mutate.
     * @param textDestination   Target text supplying replacement characters.
     * @param shuffledIndex     Character positions in arrange order.
     * @param index             Current position within @p shuffledIndex; advanced by one call.
     * @return Text with one character replaced.
     */
    static const juce::String getArranged (const juce::String& textToScramble,
                                           const juce::String& textDestination,
                                           const std::vector<int>& shuffledIndex,
                                           int& index) noexcept;

    /**
     * @brief Builds a randomly shuffled sequence of indices `[0, upperLimit)`.
     * @param upperLimit  Number of indices to generate.
     * @return Shuffled index sequence.
     */
    static const std::vector<int> getShuffledIndex (int upperLimit) noexcept;

    //==============================================================================
    /**
     * @brief Eased progress through the scrambling phase.
     * @param easeControl    Non-zero applies quadratic ease-in; zero is linear.
     * @param tick           Current tick count.
     * @param holdDuration   Hold duration, in ticks.
     * @param scrambling     Scrambling phase duration, in ticks.
     * @return Progress in the range [0, 1].
     */
    static const float getEaseIn (float easeControl, int tick, int holdDuration, int scrambling);

    /**
     * @brief Eased progress through the arranging phase.
     * @param easeControl    Non-zero applies exponential ease-out; zero is linear.
     * @param tick           Current tick count.
     * @param holdDuration   Hold duration, in ticks.
     * @param scrambling     Scrambling phase duration, in ticks.
     * @param arranging      Arranging phase duration, in ticks.
     * @param easeExponent   Exponent applied to the ease-out curve.
     * @return Progress in the range [0, 1].
     */
    static const float
    getEaseOut (float easeControl, int tick, int holdDuration, int scrambling, int arranging, float easeExponent);

    /**
     * @brief Combined ease-in/ease-out progress across both transition phases.
     * @param easeControl        Non-zero applies eased curves; zero is linear.
     * @param tick               Current tick count.
     * @param holdDuration       Hold duration, in ticks.
     * @param scrambling         Scrambling phase duration, in ticks.
     * @param arranging          Arranging phase duration, in ticks.
     * @param isScramblingPhase  `true` to ease the scrambling phase, `false` for the arranging phase.
     * @param easeExponent       Exponent applied to the ease-out curve.
     * @return Progress in the range [0, 1].
     */
    static const float getEaseInOut (float easeControl,
                                     int tick,
                                     int holdDuration,
                                     int scrambling,
                                     int arranging,
                                     bool isScramblingPhase,
                                     float easeExponent);

    /**
     * @brief Renders the current text with scrambled/arranged characters highlighted.
     * @param g  Graphics context to draw into.
     */
    void scrambleText (juce::Graphics& g) noexcept;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AnimationScrambledText)
};

/**_____________________________END OF NAMESPACE______________________________*/
} /** namespace jam */
