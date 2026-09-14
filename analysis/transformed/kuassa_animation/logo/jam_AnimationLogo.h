namespace jam
{

class AnimationLogo : public AnimationBase
{
public:
    AnimationLogo();
    ~AnimationLogo();

    void start() override;
    void stop() override;

    void paint (juce::Graphics& g) override;

    void mouseDown (const juce::MouseEvent& e) override;

    void timerCallback() override;

    void setDuration (float newDuration);

    void setInterval (int newValue) override;

    void setIntervals (const std::initializer_list<float>& intervals);

    std::function<void()> onRunning;

    void update();

    void reset();

    float getNormalisedValue (int index) noexcept;

    int getCurrentSequence() noexcept;

private:
    float duration { 5.0f };
    float numFrames { duration * timerIntervalHz };

    std::vector<float> normals;
    std::vector<float> keyFrames;
    int sequence { 0 };
    float currentFrame { 0.0f };

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AnimationLogo)
};

} // namespace jam
