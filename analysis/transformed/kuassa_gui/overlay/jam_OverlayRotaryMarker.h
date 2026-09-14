/**
 * @file jam_OverlayRotaryMarker.h
 * @brief Fades between a dim and a bright dot-ring marker around a knob on hover.
 */

namespace jam
{
/*____________________________________________________________________________*/

/**
 * @class OverlayRotaryMarker
 * @brief Non-interactive overlay drawing a ring of dots around a rotary knob,
 * fading from a dim 3-dot marker to a bright subdivided one when the mouse
 * enters the master component's group.
 */
class OverlayRotaryMarker
    : public juce::Component
    , public jam::MouseEnterParent
{
public:
    /**
     * @brief Constructs, positioning the overlay relative to @p masterComponent.
     * @param masterComponent  Component this overlay marks. Must outlive this overlay.
     */
    OverlayRotaryMarker (juce::Component& masterComponent)
        : MouseEnterParent (masterComponent)
        , master (masterComponent)
    {
        setInterceptsMouseClicks (false, false);

        for (auto& d : dots)
            addChildComponent (d.get());

        dots.at (0)->setVisible (true);

        updateBounds();
    }

    ~OverlayRotaryMarker() {}

    /** Sizes both dot layers to this overlay's full bounds. */
    void resized() override
    {
        for (auto& d : dots)
            d->setBounds (getLocalBounds());
    }

    /** Repositions the overlay around the master knob, offset by its LAF-derived extrusion. */
    void updateBounds()
    {
        const auto& bounds = [this]
        {
            int dy { 0 };
            const int expansion { 20 };

            if (auto* laf { dynamic_cast<jam::StyleCustom*> (&master.getLookAndFeel()) })
                dy = jam::toInt ((laf->getKnobExtrusion() - getDotSize()));

            auto origin { master.getBounds().removeFromTop (master.getWidth()) };
            return origin.expanded (expansion, expansion).translated (0, dy);
        }();

        setBounds (bounds);
    }

    //==============================================================================

    /** Fades out the dim marker and fades in the bright marker. */
    void mouseEnterParent (const juce::MouseEvent& e) override
    {
        Animator::toggleFade (dots.at (0), false);
        Animator::toggleFade (dots.at (1), true);
    }

    /** Fades out the bright marker and fades in the dim marker. */
    void mouseExitParent (const juce::MouseEvent& e) override
    {
        Animator::toggleFade (dots.at (0), true);
        Animator::toggleFade (dots.at (1), false);
    }

    /** @return The dim marker's dot size. */
    float getDotSize() const noexcept
    {
        return dots.at (0)->getDotSize();
    }

    //==============================================================================
private:
    juce::Component& master; ///< Rotary knob this overlay marks.

    /** @brief Draws a ring of dots (optionally subdivided) around the overlay's centre. */
    class Dots : public juce::Component
    {
    public:
        /**
         * @brief Constructs a dot ring.
         * @param numOfDots     Number of primary dots in the ring.
         * @param numOfSubDots  Number of subdivision dots between primary dots; 0 for none.
         */
        Dots (int numOfDots, int numOfSubDots = 0)
            : division (numOfDots)
            , subDivision (numOfSubDots)
        {
        }

        ~Dots() {}

        /** Draws the primary dots, plus subdivision dots when configured. */
        void paint (juce::Graphics& g) override
        {
            const float lineLength { 10.0f };
            const float arcDegree { 140.0f };
            const float rotationDegree { 0.0f };

            auto area { getLocalBounds().toFloat() };

            if (subDivision)
            {
                const float subDotSize { 0.75f };

                auto dotMarkers { jam::Rotary::DotSub (area.getCentre(),
                                                 area.getWidth(),
                                                 division,
                                                 subDivision,
                                                 lineLength,
                                                 dotSize,
                                                 subDotSize,
                                                 arcDegree,
                                                 rotationDegree) };

                g.setColour (getLookAndFeel().findColour (juce::TextButton::textColourOnId));
                dotMarkers.draw (g);
            }
            else
            {
                auto dotMarkers { jam::Rotary::Dot (area.getCentre(),
                                              area.getWidth(),
                                              division,
                                              lineLength,
                                              dotSize,
                                              arcDegree,
                                              rotationDegree) };

                g.setColour (getLookAndFeel().findColour (juce::TextButton::textColourOffId).withAlpha (0.5f));

                dotMarkers.draw (g);
            }
        }

        /** @return This ring's dot size. */
        float getDotSize() const noexcept
        {
            return dotSize;
        }


        //==============================================================================
    private:
        const int division;    ///< Number of primary dots in the ring.
        const int subDivision; ///< Number of subdivision dots between primary dots; 0 for none.
        float dotSize { 8.0f }; ///< This ring's dot size.

        //==============================================================================
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Dots)
    };

    Owner<Dots> dots { std::make_unique<Dots> (3), std::make_unique<Dots> (3, 4) };
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OverlayRotaryMarker)
};

/**_____________________________END OF NAMESPACE______________________________*/
} /** namespace jam */
