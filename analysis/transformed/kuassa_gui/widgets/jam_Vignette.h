#pragma once

namespace jam
{
/*____________________________________________________________________________*/

/**
 * @brief Paints a four-edge gradient overlay that fades the background colour
 *        toward transparent from each edge toward the component's centre.
 */
class Vignette : public juce::Component
{
public:
    Vignette() { setBufferedToImage (true); }
    ~Vignette() override = default;

    /** @brief Sets how far the gradient extends inward from each edge, in pixels.
     *  @param newDistance The edge-to-inner-stop distance in pixels.
     */
    void setEdgeDistance (float newDistance) noexcept { edgeDistance = newDistance; }

    /** @brief Returns the current edge-to-inner-stop distance in pixels. */
    float getEdgeDistance() const noexcept { return edgeDistance; }

    /**
     * @brief Draws the four edge gradients using the current backgroundColourId.
     * @param g The graphics context to paint into.
     */
    void paint (juce::Graphics& g) override
    {
        const auto bounds { getLocalBounds().toFloat() };
        const auto c { findColour (juce::ResizableWindow::backgroundColourId) };

        const auto innerRect { bounds.reduced (edgeDistance) };
        const auto transparent { c.withAlpha (0.0f) };

        const EdgeGradient edges[] = {
            // top
            { { bounds.getCentreX(), bounds.getY() },
              { innerRect.getCentreX(), innerRect.getY() },
              { bounds.getX(), bounds.getY(), bounds.getWidth(), edgeDistance } },

            // bottom
            { { bounds.getCentreX(), bounds.getBottom() },
              { innerRect.getCentreX(), innerRect.getBottom() },
              { bounds.getX(), innerRect.getBottom(), bounds.getWidth(), edgeDistance } },

            // left
            { { bounds.getX(), bounds.getCentreY() },
              { innerRect.getX(), innerRect.getCentreY() },
              { bounds.getX(), bounds.getY(), edgeDistance, bounds.getHeight() } },

            // right
            { { bounds.getRight(), bounds.getCentreY() },
              { innerRect.getRight(), innerRect.getCentreY() },
              { innerRect.getRight(), bounds.getY(), edgeDistance, bounds.getHeight() } }
        };

        for (const auto& edge : edges)
        {
            juce::ColourGradient gradient { c, edge.outerPoint, transparent, edge.innerPoint, false };
            g.setGradientFill (gradient);
            g.fillRect (edge.fillArea);
        }
    }

private:
    /** @brief One edge's gradient endpoints and the rectangle it fills. */
    struct EdgeGradient
    {
        juce::Point<float> outerPoint;    ///< Gradient start point, at the component edge.
        juce::Point<float> innerPoint;    ///< Gradient end point, at the inner-rect edge.
        juce::Rectangle<float> fillArea;  ///< Rectangle painted with this edge's gradient.
    };

    float edgeDistance { 40.0f };

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Vignette)
};

/**_____________________________END OF NAMESPACE______________________________*/
} /** namespace jam */
