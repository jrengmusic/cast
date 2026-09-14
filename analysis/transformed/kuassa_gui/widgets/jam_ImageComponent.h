/**
 * @file jam_ImageComponent.h
 * @brief Value-driven static image / filmstrip display with cross-fade frame transitions.
 */

namespace jam
{
/*____________________________________________________________________________*/

/** A component that displays a static image or a multi-frame filmstrip, driven
 *  by a bound @c juce::Value in the range [0, 1].
 *
 *  For single-frame images the component behaves as a plain image display.
 *  For filmstrip images (vertical stack of equal-height frames) the Value
 *  selects the active frame via @c Value::map.  When the frame changes the
 *  outgoing frame is handed to @c ImageTransition, which fades it out on top
 *  of the newly selected frame, producing a smooth transition with no click.
 */
class ImageComponent
    : public juce::Component
    , public Model::Object
    , private juce::Value::Listener
{
public:
    ImageComponent()
    {
        setOpaque (false);
        setInterceptsMouseClicks (false, false);
        value.addListener (this);
    }

    ~ImageComponent() override
    {
        value.removeListener (this);
    }

    /** @brief Sets the source image (single frame or filmstrip), keeping the current placement.
     *  @param newImage The image to display.
     */
    void setImage (const juce::Image& newImage)
    {
        if (image != newImage)
        {
            image = newImage;
            repaint();
        }
    }

    /** @brief Sets the source image (single frame or filmstrip) and its placement together.
     *  @param newImage       The image to display.
     *  @param placementToUse How the image is fitted/aligned within the component bounds.
     */
    void setImage (const juce::Image& newImage, juce::RectanglePlacement placementToUse)
    {
        if (image != newImage or placement != placementToUse)
        {
            image = newImage;
            placement = placementToUse;
            repaint();
        }
    }

    /** @brief Returns the currently assigned source image (single frame or filmstrip). */
    const juce::Image& getImage() const noexcept { return image; }

    /** @brief Sets how the current frame is fitted/aligned within the component bounds.
     *  @param newPlacement The placement to use.
     */
    void setImagePlacement (juce::RectanglePlacement newPlacement)
    {
        if (placement != newPlacement)
        {
            placement = newPlacement;
            repaint();
        }
    }

    /** @brief Returns the current image placement. */
    juce::RectanglePlacement getImagePlacement() const noexcept { return placement; }

    /** Return the component's bound juce::Value.
     *  @return Reference to the juce::Value owned by the component.
     */
    juce::Value& getValueObject() noexcept override { return value; }

    /** @brief Draws the current frame at full opacity, then paints any in-progress
     *  ImageTransition fade on top of it.
     *  @param g The graphics context to paint into.
     */
    void paint (juce::Graphics& g) override
    {
        if (image.isValid())
        {
            g.setOpacity (1.0f);
            const auto sub { image.getClippedImage (frameSourceRect (currentFrameIndex)) };
            g.drawImage (sub, getLocalBounds().toFloat(), placement);

            transition.paint (g, getLocalBounds(), placement);
        }
    }

private:
    /** @brief Starts an ImageTransition fade from the outgoing frame when the mapped frame changes. */
    void valueChanged (juce::Value&) override
    {
        const int newFrame { frameIndex() };

        if (image.isValid() and frameCount() > 1 and newFrame != currentFrameIndex)
            transition.start (image.getClippedImage (frameSourceRect (currentFrameIndex)));

        currentFrameIndex = newFrame;
        repaint();
    }

    /** @return Number of equal-height frames stacked vertically in the source image, at least 1.
     *  @pre getHeight() > 0 (asserted) — the builder sets bounds after this component exists.
     */
    int frameCount() const noexcept
    {
        const int componentHeight { getHeight() };
        jassert (componentHeight > 0);
        const int imageHeight { image.getHeight() };
        jassert (imageHeight >= componentHeight);
        return std::max (1, imageHeight / componentHeight);
    }

    /** @return The frame index mapped from the bound Value's [0, 1] range to [0, frameCount() - 1]. */
    int frameIndex() const noexcept
    {
        return Value::map<float> (static_cast<float> (value.getValue()), 0, frameCount() - 1);
    }

    /** @return The source-image sub-rectangle for the frame at index. */
    juce::Rectangle<int> frameSourceRect (int index) const noexcept
    {
        return { 0, index * getHeight(), image.getWidth(), getHeight() };
    }

    juce::Value value { 0.0f };
    juce::Image image;
    juce::RectanglePlacement placement { juce::RectanglePlacement::centred };
    ImageTransition transition { [this] { repaint(); } };
    int currentFrameIndex { 0 };

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ImageComponent)
};

/**_____________________________END OF NAMESPACE______________________________*/
} /** namespace jam */
