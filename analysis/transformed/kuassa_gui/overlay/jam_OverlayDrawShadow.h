/**
 * @file jam_OverlayDrawShadow.h
 * @brief Three-lobe drop-shadow overlay drawn behind a rotary slider's label pointer.
 */

namespace jam
{
/*____________________________________________________________________________*/

/**
 * @class OverlayDrawShadow
 * @brief Non-interactive overlay rendering a fixed three-lobe shadow arrangement
 * behind a master component (typically a knob/slider), with an optional
 * rounded-rect background behind the master's text-box label.
 */
class OverlayDrawShadow : public juce::Component
{
public:
    /**
     * @brief Constructs, positioning the overlay relative to @p masterComponent.
     * @param masterComponent  Component this overlay shadows. Must outlive this overlay.
     */
    OverlayDrawShadow (juce::Component& masterComponent)
        : master (masterComponent)
    {
        setInterceptsMouseClicks (false, false);
        updateBounds();
    }

    virtual ~OverlayDrawShadow() {}

    /** Reallocates the shadow images for the new size. */
    void resized() override
    {
        resetShadows();
    }

    /**
     * @brief Sets the shadow opacity multiplier.
     * @param newValue  New opacity, applied to every shadow lobe's gradient.
     */
    void setShadowOpacity (float newValue)
    {
        if (newValue != shadowOpacity)
        {
            shadowOpacity = newValue;
            resetShadows();
        }
    }

    /**
     * @brief Sets whether a rounded-rect background is drawn behind the master's label.
     * @param shouldDraw  `true` to draw the label background.
     */
    void setShouldDrawLabelBackground (bool shouldDraw)
    {
        shouldDrawLabelBackground = shouldDraw;
    }

    /** Repositions the overlay from the master's bounds and re-centres the shadow origin. */
    virtual void updateBounds()
    {
        setBounds (getShadowBounds (master, origin));

        auto origin_centre_x { origin_normalised_x * getWidth() };
        auto origin_centre_y { origin_normalised_y * getHeight() };
        origin.setCentre (origin_centre_x, origin_centre_y);
    }

    //==============================================================================
    /** Draws the optional label background, then all three shadow lobes. */
    void paint (juce::Graphics& g) override
    {
        if (shouldDrawLabelBackground)
        {
            auto labelBounds { getLabelBounds (master, *this) };
            auto area { labelBounds.withSizeKeepingCentre (getLabelWidth (master) + labelBounds.getHeight() / 2, labelBounds.getHeight()) };
            g.setColour (findColour (juce::DocumentWindow::backgroundColourId));
            float cornerSize { 8.0f };
            g.fillRoundedRectangle (area.toFloat(), cornerSize);
        }

        for (auto& s : shadows)
        {
            s.gradient.opacity = shadowOpacity;
            juce::Graphics graphics { s.image };
            graphics.setGradientFill (s.gradient.getGradient (s.shape));
            graphics.fillPath (s.shape.getPath());
            g.drawImageAt (s.blur.render (s.image), 0, 0);
        }
    }

    juce::Rectangle<float> origin; ///< Shadow origin rectangle, re-centred by updateBounds().

    /**
     * @brief Derives this overlay's bounds from the master component's bounds.
     * @param master     Component the overlay is sized relative to.
     * @param origin     Receives the origin rectangle (master's top square region, inset).
     * @param extrusion  Pixels trimmed from the top of the master's bounds before deriving origin.
     * @return Overlay bounds, in the master's parent coordinate space.
     */
    static const juce::Rectangle<int> getShadowBounds (juce::Component& master,
                                                       juce::Rectangle<float>& origin,
                                                       float extrusion = 0.0f) noexcept
    {
        auto masterBounds { master.getBounds().toFloat() };
        masterBounds.removeFromTop (extrusion);

        origin = masterBounds.removeFromTop (masterBounds.getWidth()).reduced (inset);

        float height { masterBounds.getWidth() * heightProportions };
        float width { height * widthAspect };
        juce::Rectangle<int> bounds { jam::toInt (width), jam::toInt (height) };

        float dx { (0.5f - origin_normalised_x) * width };
        float dy { (0.5f - origin_normalised_y) * height };

        bounds.setCentre (origin.getCentreX() + dx, origin.getCentreY() + dy);

        return bounds;
    }

    /**
     * @brief Derives the master slider's text-box label bounds, in @p parent's coordinate space.
     * @param master  Master component; must be a `juce::Slider`.
     * @param parent  Component whose coordinate space the result is expressed in.
     */
    static const juce::Rectangle<float> getLabelBounds (juce::Component& master,
                                                        juce::Component& parent)
    {
        auto textBounds { master.getLookAndFeel().getSliderLayout (*dynamic_cast<juce::Slider*> (&master)).textBoxBounds };

        return parent.getLocalArea (&master, textBounds).toFloat();
    }

    /**
     * @brief Returns the master's LAF-derived label width, or 0 when the LAF is not a `jam::StyleCustom`.
     * @param master  Master component; must be a `juce::Slider`.
     */
    static const float getLabelWidth (juce::Component& master)
    {
        if (auto* laf { dynamic_cast<jam::StyleCustom*> (&master.getLookAndFeel()) })
        {
            return laf->getLabelWidth (*dynamic_cast<juce::Slider*> (&master));
        }

        return 0.0f;
    }
    //==============================================================================
protected:
    juce::Component& master; ///< Component this overlay is positioned and shadowed relative to.
    bool shouldDrawLabelBackground { false }; ///< Whether paint() draws the rounded-rect label background.
    /** normalised delta from origin centre to bounds centre */
    static constexpr float origin_normalised_x { 0.416667f };
    static constexpr float origin_normalised_y { 0.232323f };
    static constexpr float heightProportions { 2.75f };
    static constexpr float widthAspect { 0.8f };
    static constexpr int inset { 8 };

    std::array<jam::Shadow<OverlayDrawShadow>, 3> shadows { {
        { { *this, -110.5f, 0.7766f, 0.46287f },
          { 0.25f, 1.0f, 0.05f, -0.2f, 1.0f },
          jam::Blur { 12 } },
        { { *this, -90.0f, 0.84f, 0.60265f },
          { 0.8f, 1.0f, 0.0f, 0.0f, 1.2f },
          jam::Blur { 20 } },
        { { *this, -49.0f, 0.856f, 0.4367f },
          { 0.25f, 1.0f, 0.0f, 0.0f, 0.9f },
          jam::Blur { 14 } },
    } };

    float shadowOpacity { 0.225f };

    //==============================================================================
    /** @brief Reallocates each shadow lobe's backing image to the overlay's current size and repaints. */
    void resetShadows()
    {
        for (auto& s : shadows)
            s.image = juce::Image (juce::Image::ARGB, getWidth(), getHeight(), true, juce::SoftwareImageType());

        repaint();
    }

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OverlayDrawShadow)
};

/**_____________________________END OF NAMESPACE______________________________*/
} /** namespace jam */
