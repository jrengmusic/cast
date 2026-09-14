/**
 * @file jam_Shadow.h
 * @brief Single radial-gradient drop-shadow lobe, rendered relative to a parent's origin.
 */

namespace jam
{
/*____________________________________________________________________________*/

/**
 * @struct Shadow
 * @brief One shadow lobe — a shape/gradient/blur triple rendered into an owned
 * `juce::Image`, positioned relative to `parent.origin`.
 *
 * @tparam ShadowType  Owning component type, exposing `origin` (a `juce::Rectangle<float>`)
 * and `getHeight()`.
 */
template <typename ShadowType>
struct Shadow
{
    /** @brief Geometry of one shadow lobe — a quad fanning out from `parent.origin` into an ellipse. */
    struct Shape
    {
        ShadowType& parent;  ///< Owning component supplying `origin` and bounds.
        float angle;         ///< Angle, in degrees, of the shadow's cast direction.
        float radius;        ///< Ellipse size, proportional to `parent.origin`'s dimensions.
        float distance;      ///< Ellipse centre distance from `parent.origin`'s centre, proportional to `parent.getHeight()`.

        //==============================================================================
        /** @return The shadow lobe's fill path — a quad plus its terminal ellipse. */
        juce::Path getPath() const noexcept
        {
            auto& origin { parent.origin };

            float ellipseWidth { radius * origin.getWidth() };
            float ellipseHeight { radius * origin.getHeight() };

            juce::Rectangle<float> ellipse { ellipseWidth, ellipseHeight };
            ellipse.setCentre (getCircumference().getEnd());

            auto leftCircum { centredLine (ellipse.getCentre(), 0.0f, ellipseWidth * 0.5f, juce::degreesToRadians (180.0f - angle)) };
            auto rightCircum { centredLine (ellipse.getCentre(), 0.0f, ellipseWidth * 0.5f, juce::degreesToRadians (360.0f - angle)) };

            juce::Path p;
            p.startNewSubPath ({ origin.getX(), origin.getCentreY() });
            p.lineTo ({ origin.getRight(), origin.getCentreY() });
            p.lineTo (rightCircum.getEnd());
            p.lineTo (leftCircum.getEnd());
            p.closeSubPath();
            p.addEllipse (ellipse);

            return p;
        }

        /** @return The line from `parent.origin`'s centre to the ellipse centre, at `angle`/`distance`. */
        juce::Line<float> getCircumference() const noexcept
        {
            return centredLine (parent.origin.getCentre(), 0.0f, distance * parent.getHeight(), juce::degreesToRadians (90.0f - angle));
        }

        //==============================================================================
    } shape;

    /** @brief Colour/alpha ramp applied along a shadow lobe's circumference line. */
    struct Gradient
    {
        float opacity;                              ///< Overall opacity multiplier, applied to both alpha stops.
        float startAlpha;                           ///< Alpha at the gradient's start point.
        float endAlpha;                             ///< Alpha at the gradient's end point.
        float start;                                ///< Start position along the circumference line, 0..1.
        float end;                                  ///< End position along the circumference line, 0..1.
        juce::Colour colour { juce::Colours::black }; ///< Base shadow colour.
        bool isRadial { true };                      ///< `true` for a radial gradient, `false` for linear.

        //==============================================================================
        /**
         * @brief Builds the colour gradient along @p shape's circumference line.
         * @param shape  Shape supplying the circumference line to gradient along.
         */
        juce::ColourGradient getGradient (const Shape& shape) const noexcept
        {
            auto line { shape.getCircumference() };
            auto startPoint { line.getPointAlongLineProportionally (start) };
            auto endPoint { line.getPointAlongLineProportionally (end) };
            auto startColour { colour.withAlpha (startAlpha * opacity) };
            auto endColour { colour.withAlpha (endAlpha * opacity) };

            return juce::ColourGradient (startColour, startPoint, endColour, endPoint, isRadial);
        }

        //==============================================================================
    } gradient;

    jam::Blur blur;     ///< Blur applied when rendering `image`.
    juce::Image image;  ///< Backing image the shape/gradient are rasterized into, then blurred.
};

/**_____________________________END OF NAMESPACE______________________________*/
} /** namespace jam */
