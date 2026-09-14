namespace jam
{
/*____________________________________________________________________________*/

AnimationScrambledText::AnimationScrambledText (float holdDurationInSecond,
                                                float transitionDurationInSecond,
                                                int intervalHz)
    : holdDuration (static_cast<int> (holdDurationInSecond))
{
    timerIntervalHz = intervalHz;
    holdDuration = toInt (holdDurationInSecond * timerIntervalHz);
    transition = toInt (transitionDurationInSecond * timerIntervalHz);
    scramblingDuration = toInt (scramblingDurationProportion * transition);
    arrangingDuration = toInt ((1.0f - scramblingDurationProportion) * transition);
}

//==============================================================================
void AnimationScrambledText::timerCallback()
{
    if (tick < holdDuration + transition)
    {
        ++tick;

        isScramblingPhase = tick > holdDuration && tick <= holdDuration + scramblingDuration;
        isArrangingPhase = tick > holdDuration + scramblingDuration && tick <= holdDuration + transition;

        float effectiveProgress { getEaseInOut (easeControl, tick, holdDuration, scramblingDuration, arrangingDuration, isScramblingPhase, easeExponent) };

        if (isScramblingPhase)
        {
            scrambleCharIndex = juce::jlimit (0, text.length() - 1, toInt (scrambleIndex.size() * effectiveProgress));
            text = getScrambled (text, scrambleIndex, scrambleCharIndex);
        }
        else if (isArrangingPhase)
        {
            arrangeCharIndex = juce::jlimit (0, linesOfText[nextIndex].length() - 1, toInt (linesOfText[nextIndex].length() * effectiveProgress));
            text = getArranged (text, linesOfText[nextIndex], arrangeIndex, arrangeCharIndex);
        }
        else
        {
            text = linesOfText[index];
        }
    }
    else
    {
        index = (index + 1) % linesOfText.size();
        nextIndex = (index + 1) % linesOfText.size();

        tick = 0;
        scrambleCharIndex = arrangeCharIndex = 0;
    }

    repaint();
}

void AnimationScrambledText::paint (juce::Graphics& g)
{
    if (isTextReady)
        scrambleText (g);
}

void AnimationScrambledText::scrambleText (juce::Graphics& g) noexcept
{
    auto colour { findColour (juce::TextButton::textColourOnId) };
    auto bounds { getLocalBounds().toFloat() };

    juce::AttributedString attributedString { text.replace (" ", whitespace) };
    attributedString.setFont (font);
    attributedString.setJustification (juce::Justification::centredTop);
    attributedString.setColour (colour);

    if (isScramblingPhase)
    {
        juce::Colour scrambleColour { findColour (juce::Label::textColourId) };
        int currentGlyphPosition { scrambleIndex.at (scrambleCharIndex) };
        if (currentGlyphPosition >= 0 && currentGlyphPosition < text.length())
            attributedString.setColour (juce::Range<int> (currentGlyphPosition, currentGlyphPosition + 1), scrambleColour);
    }

    if (isArrangingPhase)
    {
        juce::Colour arrangeColour { findColour (juce::TextButton::textColourOffId) };
        int currentGlyphPosition { arrangeIndex.at (arrangeCharIndex) };
        if (currentGlyphPosition >= 0 && currentGlyphPosition < text.length())
            attributedString.setColour (juce::Range<int> (currentGlyphPosition, currentGlyphPosition + 1), arrangeColour);
    }

    juce::TextLayout textLayout;
    textLayout.createLayout (attributedString, bounds.getWidth());

    float layoutHeight { 0.0f };
    for (int i = 0; i < textLayout.getNumLines(); ++i)
        layoutHeight += textLayout.getLine (i).getLineBounds().getHeight();

    float offsetY { bounds.getY() + (bounds.getHeight() - layoutHeight) / 2.0f };

    g.saveState();
    g.addTransform (juce::AffineTransform::translation (bounds.getX(), offsetY));
    textLayout.draw (g, bounds.withX (0).withY (0));
    g.restoreState();
}

//==============================================================================
void AnimationScrambledText::setFont (const juce::FontOptions& newFont)
{
    font = newFont;
}

void AnimationScrambledText::setText (const juce::StringArray& newLinesOfText,
                                      bool shouldBeUpperCase)
{
    linesOfText = getShuffled (newLinesOfText, shouldBeUpperCase);
    scrambleIndex = getShuffledIndex (linesOfText[0].length());
    arrangeIndex = getShuffledIndex (linesOfText[0].length());
    isTextReady = true;
}

//==============================================================================
void AnimationScrambledText::start()
{
    if (isTimerRunning())
        stopTimer();

    startTimerHz (timerIntervalHz);
}

void AnimationScrambledText::stop()
{
    stopTimer();
}

//==============================================================================
const juce::StringArray AnimationScrambledText::getShuffled (const juce::StringArray& linesOfText,
                                                              bool shouldBeUpperCase)
{
    juce::StringArray shuffled;

    if (linesOfText.isEmpty())
        return shuffled;

    int textLength = linesOfText[0].replace (" ", whitespace).length();

    for (const auto& line : linesOfText)
    {
        if (line.length() > textLength)
        {
            textLength = line.length();
        }
    }

    for (const auto& t : linesOfText)
    {
        auto text { t.replace (" ", whitespace) };

        if (shouldBeUpperCase)
            text = text.toUpperCase();

        shuffled.add (text);
    }

    for (auto& t : shuffled)
    {
        int padding = textLength - t.length();
        int leftPadding = padding / 2;
        int rightPadding = padding - leftPadding;

        t = whitespace.repeatedString (whitespace, leftPadding) + t + whitespace.repeatedString (whitespace, rightPadding);
    }

    std::random_device rd;
    std::mt19937 generator (rd());
    std::shuffle (shuffled.begin(), shuffled.end(), generator);

    return shuffled;
}

const juce::juce_wchar AnimationScrambledText::getRandomAlphanumeric() noexcept
{
    static std::mt19937 generator { std::random_device {}() };
    constexpr char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    std::uniform_int_distribution<size_t> distribution { 0, sizeof (charset) - 2 };
    return charset[distribution (generator)];
}

const juce::String AnimationScrambledText::getRandomAlphanumericString() noexcept
{
    return juce::String::charToString (getRandomAlphanumeric());
}

const juce::String AnimationScrambledText::getScrambled (const juce::String& textToScramble,
                                                          const std::vector<int>& shuffledIndex,
                                                          int& index) noexcept
{
    juce::String text { textToScramble };

    if (index < 0 || index >= static_cast<int> (shuffledIndex.size()))
        return text;

    text = text.replaceSection (shuffledIndex.at (index), 1, getRandomAlphanumericString());

    ++index;
    index %= text.length();

    return text;
}

const juce::String AnimationScrambledText::getArranged (const juce::String& textToScramble,
                                                         const juce::String& textDestination,
                                                         const std::vector<int>& shuffledIndex,
                                                         int& index) noexcept
{
    juce::String text { textToScramble };

    if (index < 0 || index >= static_cast<int> (shuffledIndex.size()) || index >= static_cast<int> (textDestination.length()))
        return text;

    int position = shuffledIndex.at (index);

    if (position < 0 || position >= text.length())
        return text;

    text = text.replaceSection (position, 1, textDestination.substring (position, position + 1));

    ++index;
    index %= text.length();

    return text;
}

const std::vector<int> AnimationScrambledText::getShuffledIndex (int upperLimit) noexcept
{
    std::vector<int> index;

    if (upperLimit <= 0)
        return index;

    index.reserve (static_cast<size_t> (upperLimit));
    for (int i = 0; i < upperLimit; ++i)
        index.push_back (i);

    static std::mt19937 generator { std::random_device {}() };
    std::shuffle (index.begin(), index.end(), generator);

    return index;
}

//==============================================================================
const float AnimationScrambledText::getEaseIn (float easeControl,
                                               int tick,
                                               int holdDuration,
                                               int scrambling)
{
    float phaseTick = tick - holdDuration;
    float progress = phaseTick / static_cast<float> (scrambling);
    progress = juce::jlimit (0.0f, 1.0f, progress);

    return (easeControl == 0.0f) ? progress : progress * progress;
}

const float AnimationScrambledText::getEaseOut (float easeControl,
                                                int tick,
                                                int holdDuration,
                                                int scrambling,
                                                int arranging,
                                                float easeExponent)
{
    float phaseTick = tick - (holdDuration + scrambling);
    float progress = phaseTick / static_cast<float> (arranging);
    progress = juce::jlimit (0.0f, 1.0f, progress);

    float effectiveProgress = (easeControl == 0.0f)
                                  ? progress
                                  : 1.0f - std::pow ((1.0f - progress), easeExponent);

    return effectiveProgress;
}

const float AnimationScrambledText::getEaseInOut (float easeControl,
                                                  int tick,
                                                  int holdDuration,
                                                  int scrambling,
                                                  int arranging,
                                                  bool isScramblingPhase,
                                                  float easeExponent)
{
    float progress = isScramblingPhase
                         ? getEaseIn (easeControl, tick, holdDuration, scrambling)
                         : getEaseOut (easeControl, tick, holdDuration, scrambling, arranging, easeExponent);

    return (progress < 0.5f) ? 2.0f * progress : 2.0f * progress - 1.0f;
}

/**_____________________________END_OF_NAMESPACE______________________________*/
} /** namespace jam */
