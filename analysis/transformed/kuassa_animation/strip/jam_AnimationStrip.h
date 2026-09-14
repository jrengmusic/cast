/**
 * @file jam_AnimationStrip.h
 * @brief Frame-strip sprite animation with a hold phase and eased playback.
 */

namespace jam
{
/*____________________________________________________________________________*/

/**
 * @class AnimationStrip
 * @brief Plays back frames from a vertical image strip, holding the last frame before
 * looping back through the strip with a sine-eased frame rate.
 *
 * setImageStrip() slices the source image into `numFrames` frames of the given height.
 * mouseDown() opens the URL set via setURL(), when one has been set.
 */
class AnimationStrip : public AnimationBase
{
public:
    /**
     * @brief Constructs the animation with timer and hold timing.
     * @param newIntervalHz    Timer interval, in Hz.
     * @param newHoldDuration  Hold duration before looping, in seconds.
     */
    AnimationStrip (int newIntervalHz = 24, int newHoldDuration = 9);
    ~AnimationStrip();

    void timerCallback() override;
    void paint (juce::Graphics& g) override;

    /**
     * @brief Sets the hold duration before playback loops.
     * @param seconds  Hold duration, in seconds.
     */
    void setHoldDuration (int seconds);

    /** Launches the URL set via setURL(), when one has been set. */
    void mouseDown (const juce::MouseEvent& e) override;

    void start() override;
    void stop() override;
    //==============================================================================

    /**
     * @brief Sets the source frame strip and the height of each frame.
     * @param imageStrip  Vertical strip image containing consecutive frames.
     * @param height      Height of a single frame, in pixels.
     */
    void setImageStrip (const juce::Image& imageStrip, int height);

    /**
     * @brief Sets the URL launched by mouseDown().
     * @param newUrl  Target URL.
     */
    void setURL (juce::StringRef newUrl);

    /**
     * @brief Sets the URL launched by mouseDown().
     * @param newUrl  Target URL.
     */
    void setURL (const char* newUrl);

    /**
     * @brief Sets the URL launched by mouseDown().
     * @param newUrl  Target URL.
     */
    void setURL (std::string_view newUrl);

private:
    juce::Image imageStrip;
    int frameWidth;
    int frameHeight;
    int holdFrame;
    int numFrames;
    int currentFrame;
    int duration;

    int counter { 0 };
    int holdDuration;

    juce::String website;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AnimationStrip)
};

/**_____________________________END OF NAMESPACE______________________________*/
} /** namespace jam */
