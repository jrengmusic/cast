namespace jam
{

AnimationLogo::AnimationLogo()
{
    setBufferedToImage (true);
    setIntervals ({ 0.0f, 1.0f, 2.0f, 2.3f, 3.8f, 4.0f, 4.2f, 4.3f, 4.35f, 4.6f, 4.65f, 4.8f, 4.9f });
    setDuration (5.0f);
    setInterval (30);

    onRunning = [this]
    {
        repaint();
    };
}

AnimationLogo::~AnimationLogo()
{
    if (isTimerRunning())
        stopTimer();
}

void AnimationLogo::paint (juce::Graphics& g)
{
    jam::Logo k;
    float edgeIndent { 20.0f };
    auto area { getLocalBounds().toFloat().reduced (edgeIndent) };
    float lineThickness { 1.0f };

    switch (int newSequence { getCurrentSequence() })
    {
        case 0:
        case 1:
        case 2:
        case 3:
            k.drawStroke (g, area, jam::Logo::default3(), lineThickness, newSequence, getNormalisedValue (newSequence));
            break;
        case 4:
        case 6:
            break;
        case 5:
        case 7:
        case 9:
            k.drawStroke (g, area, jam::Logo::default3(), lineThickness, 3, getNormalisedValue (3));
            [[fallthrough]];
        case 8:
        case 10:
            k.drawFill (g, area, getNormalisedValue (newSequence), false, true);
            break;
        default:
            k.drawStroke (g, area, jam::Logo::default3(), lineThickness, 3, getNormalisedValue (3));
            k.drawFill (g, area, 1.0f, true, false);
            break;
    }
}

void AnimationLogo::mouseDown (const juce::MouseEvent&) { juce::URL (jam::URL::getWebsite()).launchInDefaultBrowser(); }

void AnimationLogo::timerCallback()
{
    currentFrame++;

    if (currentFrame == numFrames)
        stopTimer();

    update();

    if (onRunning)
        onRunning();
}

//==============================================================================
void AnimationLogo::setDuration (float newDuration)
{
    duration = newDuration;
    numFrames = duration * timerIntervalHz;
}

void AnimationLogo::setInterval (int newValue)
{
    if (newValue != timerIntervalHz)
    {
        timerIntervalHz = newValue;
        numFrames = duration * timerIntervalHz;
    }
}

void AnimationLogo::setIntervals (const std::initializer_list<float>& intervals)
{
    for (auto& i : intervals)
    {
        jassert (i < duration);

        keyFrames.push_back (std::floor (Value::map (i, 0.0f, duration, 0.0f, numFrames)));
        normals.push_back (0.0f);
    }
}

//==============================================================================
void AnimationLogo::update()
{
    const int seq { getCurrentSequence() };
    const int nextSeq { seq + 1 };
    const float startFrame { keyFrames.at (seq) };
    const float endFrame { nextSeq > (keyFrames.size() - 1) ? numFrames : (keyFrames.at (nextSeq) - 1) };

    if (currentFrame >= startFrame)
        normals.at (seq) = Value::normalise<float> (currentFrame, startFrame, endFrame);
}

void AnimationLogo::reset() { currentFrame = 0.0f; }

//==============================================================================
float AnimationLogo::getNormalisedValue (int index) noexcept { return normals.at (index); }

int AnimationLogo::getCurrentSequence() noexcept
{
    if (auto keyFrame { std::find (keyFrames.begin(), keyFrames.end(), currentFrame) }; keyFrame != keyFrames.cend())
    {
        sequence = static_cast<int> (std::distance (keyFrames.begin(), keyFrame));
    }

    return sequence;
}

void AnimationLogo::start()
{
    if (isTimerRunning())
        stop();

    reset();
    startTimer (timerIntervalHz);
}

void AnimationLogo::stop() { stopTimer(); }

} // namespace jam
