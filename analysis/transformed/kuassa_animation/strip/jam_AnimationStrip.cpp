namespace jam
{
/*____________________________________________________________________________*/

AnimationStrip::AnimationStrip (int newIntervalHz, int newHoldDuration)
    : holdDuration (newHoldDuration)
{
    timerIntervalHz = newIntervalHz;
    holdFrame = timerIntervalHz * holdDuration;
    currentFrame = 1.0f;
}

AnimationStrip::~AnimationStrip()
{
    stopTimer();
}

void AnimationStrip::timerCallback()
{
    if (counter < duration)
    {
        counter++;
        counter %= (int) duration;
    }

    if (counter > holdFrame)
    {
        float normal { std::sin (Value::map ((float) counter, (float) holdFrame, (float) duration, 0.0f, juce::MathConstants<float>::pi)) };
        currentFrame = toInt (std::floor (Value::map (normal, 0.0f, (float) numFrames - 1)));
    }
    else
    {
        currentFrame = numFrames - 1;
    }

    repaint();
}

void AnimationStrip::mouseDown (const juce::MouseEvent&)
{
    if (website != juce::String())
        juce::URL (website).launchInDefaultBrowser();
}

void AnimationStrip::setImageStrip (const juce::Image& newImageStrip, int height)
{
    imageStrip = newImageStrip;
    numFrames = imageStrip.getHeight() / height;
    frameWidth = imageStrip.getWidth();
    frameHeight = height;
    duration = numFrames + holdFrame - 1;
}

void AnimationStrip::setURL (juce::StringRef newUrl)
{
    website = newUrl;
}

void AnimationStrip::setURL (const char* newUrl)
{
    website = newUrl;
}

void AnimationStrip::setURL (std::string_view newUrl)
{
    website = newUrl.data();
}

void AnimationStrip::paint (juce::Graphics& g)
{
    auto area { getLocalBounds().toFloat() };

    // Scales the strip frame to fit the component bounds while keeping its
    // aspect ratio, never upscaling beyond the source frame's own size.
    float scale { Value::clipMax (1.0f, static_cast<float> (area.getHeight()) / static_cast<float> (frameHeight)) };

    float scaledWidth { scale * frameWidth };
    float scaledHeight { scale * frameHeight };
    juce::Rectangle<float> drawArea { area.withSizeKeepingCentre (scaledWidth, scaledHeight) };

    g.drawImage (imageStrip,
                drawArea.getX(),
                drawArea.getY(),
                drawArea.getWidth(),
                drawArea.getHeight(),
                0,
                currentFrame * frameHeight,
                frameWidth,
                frameHeight);
}

void AnimationStrip::setHoldDuration (int seconds)
{
    holdDuration = seconds;
}

void AnimationStrip::start()
{
    if (isTimerRunning())
        stopTimer();

    currentFrame = 0;
    startTimerHz (timerIntervalHz);
}

void AnimationStrip::stop()
{
    stopTimer();
}

/**_____________________________END OF NAMESPACE______________________________*/
} /** namespace jam */
