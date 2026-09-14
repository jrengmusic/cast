/**
 * @file jam_OverlayToggleShadow.h
 * @brief Drop-shadow overlay behind a toggle button's cap, hidden while the toggle is active.
 */

namespace jam
{
/*____________________________________________________________________________*/

/**
 * @class OverlayToggleShadow
 * @brief Extends `OverlayDrawShadow` to render a toggle cap's bevelled background
 * and its drop shadows, hiding the shadows while `StyleCustom::getState()`
 * reports the toggle as active.
 */
class OverlayToggleShadow
    : public OverlayDrawShadow
    , public juce::Button::Listener
{
public:
    /**
     * @brief Constructs, binding to a `juce::ToggleButton` master.
     * @param master  Toggle button this overlay shadows. Must outlive this overlay.
     */
    OverlayToggleShadow (juce::Component& master)
        : OverlayDrawShadow (master)
        , button (*dynamic_cast<juce::ToggleButton*> (&master))
    {
        button.addListener (this);
        updateBounds();
    }

    ~OverlayToggleShadow() {}

    /** Repositions the overlay around the toggle cap, offset by its LAF-derived extrusion. */
    void updateBounds() override
    {
        const auto& extrusion = [this]
        {
            if (auto* laf { dynamic_cast<jam::StyleCustom*> (&button.getLookAndFeel()) })
                return laf->getCapExtrusion() * 0.5f;

            return 0.0f;
        }();

        setBounds (getShadowBounds (master, origin, extrusion));

        auto origin_centre_x { origin_normalised_x * getWidth() };
        auto origin_centre_y { origin_normalised_y * getHeight() };
        origin.setCentre (origin_centre_x, origin_centre_y);
    }

    /** Repaints on click. */
    void buttonClicked (juce::Button*) override
    {
        repaint();
    }

    /** Repaints on hover/press state change. */
    void buttonStateChanged (juce::Button*) override
    {
        repaint();
    }

    //==============================================================================
    /** Draws the cap background, then the drop shadows. */
    void paint (juce::Graphics& g) override
    {
        drawBackground (g);
        drawShadows (g);
    }

    /**
     * @brief Draws the bevelled cap background via the LAF's cap colour/bevel/background area.
     * @param g  Graphics context to draw into.
     */
    void drawBackground (juce::Graphics& g)
    {
        if (auto* laf { dynamic_cast<jam::StyleCustom*> (&button.getLookAndFeel()) })
        {
            auto area { laf->getBackgroundArea (button) };
            area.setCentre (origin.getCentre());
            auto colour { laf->getCapColour (button) };
            float bevelSize { laf->getBevelSize() };

            juce::Path bevel;
            bevel.addEllipse (area);
            g.setGradientFill (jam::colours::getVerticalGradient (area, colour, 1.5f, 0.72f, -10.0f, false));
            g.fillPath (bevel);

            juce::Path p;
            p.addEllipse (area.reduced (bevelSize));
            g.setColour (colour.darker (0.25f));
            g.fillPath (p);
        }
    }

    /**
     * @brief Draws the shadow lobes, unless the LAF reports the toggle as active.
     * @param g  Graphics context to draw into.
     */
    void drawShadows (juce::Graphics& g)
    {
        if (auto* laf { dynamic_cast<jam::StyleCustom*> (&button.getLookAndFeel()) })
        {
            if (not laf->getState (button))
            {
                for (auto& s : shadows)
                {
                    s.gradient.opacity = shadowOpacity;
                    juce::Graphics graphics { s.image };
                    graphics.setGradientFill (s.gradient.getGradient (s.shape));
                    graphics.fillPath (s.shape.getPath());
                    g.drawImageAt (s.blur.render (s.image), 0, 0);
                }
            }
        }
    }

private:
    juce::ToggleButton& button; ///< Toggle button this overlay shadows and listens to.

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OverlayToggleShadow)
};

/**_____________________________END OF NAMESPACE______________________________*/
} /** namespace jam */
