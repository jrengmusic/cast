/**
 * @file        jam_ButtonSVG.h
 * @brief       SVG-backed paint-delegate button.
 */
#pragma once

namespace jam
{
/*____________________________________________________________________________*/

/**
 * @brief SVG-backed button. Default-constructed instances delegate all painting
 *        to the consuming LookAndFeel; constructed from a StringArray of SVG
 *        images, the button owns and self-paints its own per-state bank.
 *
 * Default-constructed: ButtonSVG owns no graphics bank and stores no state.
 * Parsed SVG Segments are owned by the consuming LookAndFeel, keyed by state identifier.
 * State selection uses jam::ButtonSVG::getState against map::ButtonState indices;
 * the consuming project declares the Bimap.
 * Paint is forwarded verbatim to StyleCustom::drawTabButton.
 *
 * Constructed from a StringArray: ButtonSVG owns the image bank directly and
 * paints itself via jam::ButtonSVG::paintSingleImage, resolving the active
 * state index from the same map::ButtonState ordering.
 */
class ButtonSVG : public juce::Button
{
public:
    /** @brief Colour tinting mode applied when painting SVG paths. */
    enum class ColourMode { toggle, forceOff, forceOn };

    /** @brief Builds an 8-entry StringArray of SVG images from a BinaryData asset
     *  name prefix, appending the map::ButtonState suffixes
     *  (_normal, _over, _down, _disabled, _normalOn, _overOn, _downOn, _disabledOn).
     *  @param name BinaryData asset name prefix.
     */
    static juce::StringArray fromBinary (juce::StringRef name)
    {
        static constexpr std::array<const char*, 8> suffixes {
            "normal", "over", "down", "disabled",
            "normalOn", "overOn", "downOn", "disabledOn"
        };

        juce::StringArray result;

        for (int i { 0 }; i < 8; ++i)
            result.add (BinaryData::getString (juce::String (name) + "_" + suffixes.at (i) + ".svg"));

        return result;
    }

    /**
     * @brief Parses and paints a single SVG image into a button's bounds.
     * @param g                              Graphics context to draw into.
     * @param shouldDrawButtonAsHighlighted  Hover state; affects colour via @p colourMode.
     * @param shouldDrawButtonAsDown         Pressed state; affects colour via @p colourMode.
     * @param button                         Button supplying bounds, enabled/toggle state, and colours.
     * @param image                          SVG XML source to parse and paint.
     * @param colourMode                     Colour tinting mode applied to the parsed paths.
     * @param style                          Path rendering style — stroke, fill, or alternate
     *                                       (fill using the even-odd winding rule).
     * @param strokeType                     Stroke parameters used when @p style is stroke.
     * @param edgeIndent                     Pixel inset applied to the paint bounds.
     */
    static void
    paintSingleImage (juce::Graphics& g,
                      bool shouldDrawButtonAsHighlighted,
                      bool shouldDrawButtonAsDown,
                      const juce::Button& button,
                      const char* const image,
                      ColourMode colourMode = ColourMode::toggle,
                      jam::Svg::PathStyle style = jam::Svg::PathStyle::stroke,
                      juce::PathStrokeType strokeType = juce::PathStrokeType (1.2f,
                                                                              juce::PathStrokeType::beveled,
                                                                              juce::PathStrokeType::rounded),
                      int edgeIndent = 0);

    /** @brief Returns the state index for a button's current state, applying
     *  the count-fallback rules used by paint(): count must be 1, 3, 4, 6, or 8;
     *  fewer slots fall back toward map::ButtonState::normal.
     *  State indices are map::ButtonState values (normal=0 … disabledOn=7);
     *  the consuming project declares the Bimap.
     *  @param button  The button whose enabled/toggle state is read.
     *  @param isOver  Highlight flag (mirrors paint's shouldDrawButtonAsHighlighted).
     *  @param isDown  Pressed flag (mirrors paint's shouldDrawButtonAsDown).
     *  @param count   Number of available state slots.
     */
    static int getState (const juce::Button& button, bool isOver, bool isDown, size_t count);

    /**
     * @brief Selects and paints the state-appropriate image from a bank of SVG images.
     * @param g                              Graphics context to draw into.
     * @param shouldDrawButtonAsHighlighted  Hover state, forwarded to getState() and paintSingleImage().
     * @param shouldDrawButtonAsDown         Pressed state, forwarded to getState() and paintSingleImage().
     * @param button                         Button supplying bounds, enabled/toggle state, and colours.
     * @param images                         Bank of SVG XML sources, ordered by map::ButtonState index.
     * @param count                          Number of images in @p images.
     * @param colourMode                     Colour tinting mode applied to the parsed paths.
     * @param style                          Path rendering style — stroke, fill, or alternate
     *                                       (fill using the even-odd winding rule).
     * @param strokeType                     Stroke parameters used when @p style is stroke.
     * @param edgeIndent                     Pixel inset applied to the paint bounds.
     */
    static void paint (juce::Graphics& g,
                       bool shouldDrawButtonAsHighlighted,
                       bool shouldDrawButtonAsDown,
                       const juce::Button& button,
                       const char* const* images,
                       size_t count,
                       ColourMode colourMode = ColourMode::toggle,
                       jam::Svg::PathStyle style = jam::Svg::PathStyle::stroke,
                       juce::PathStrokeType strokeType = juce::PathStrokeType (1.2f,
                                                                               juce::PathStrokeType::beveled,
                                                                               juce::PathStrokeType::rounded),
                       int edgeIndent = 0);

    /** @brief Default constructor. Button name is empty — set via setButtonText. */
    ButtonSVG() : juce::Button ({}) {}

    /**
     * @brief Constructs from a BinaryData asset-name prefix. Over-state style defaults to the same as normal.
     * @param name          BinaryData asset name prefix (suffixes _normal, _over, etc. are appended).
     * @param newColourMode Colour tinting mode for path painting.
     * @param pathStyle     Path rendering style for normal and non-hover states.
     * @param newEdgeIndent Pixel inset applied to the paint bounds.
     */
    /** @brief Default pixel inset applied to the paint bounds when no edgeIndent is given. */
    static constexpr int defaultEdgeIndent { 2 };

    ButtonSVG (juce::StringRef name,
               ColourMode newColourMode = ColourMode::toggle,
               jam::Svg::PathStyle pathStyle = jam::Svg::PathStyle::stroke,
               int newEdgeIndent = defaultEdgeIndent)
        : ButtonSVG (fromBinary (name), newColourMode, pathStyle, newEdgeIndent, pathStyle)
    {
    }

    /**
     * @brief Constructs from a BinaryData asset-name prefix with an explicit hover style override.
     * @param name          BinaryData asset name prefix (suffixes _normal, _over, etc. are appended).
     * @param newColourMode Colour tinting mode for path painting.
     * @param pathStyle     Path rendering style for normal and non-hover states.
     * @param newEdgeIndent Pixel inset applied to the paint bounds.
     * @param overPathStyle Path rendering style used when the button is highlighted (hovered).
     */
    ButtonSVG (juce::StringRef name,
               ColourMode newColourMode,
               jam::Svg::PathStyle pathStyle,
               int newEdgeIndent,
               jam::Svg::PathStyle overPathStyle)
        : ButtonSVG (fromBinary (name), newColourMode, pathStyle, newEdgeIndent, overPathStyle)
    {
    }

    /**
     * @brief Constructs from a StringArray of SVG images. Over-state style defaults to the same as normal.
     * @param images        Pre-built array of SVG XML strings ordered by map::ButtonState index.
     * @param newColourMode Colour tinting mode for path painting.
     * @param pathStyle     Path rendering style for normal and non-hover states.
     * @param newEdgeIndent Pixel inset applied to the paint bounds.
     */
    ButtonSVG (const juce::StringArray& images,
               ColourMode newColourMode = ColourMode::toggle,
               jam::Svg::PathStyle pathStyle = jam::Svg::PathStyle::stroke,
               int newEdgeIndent = defaultEdgeIndent)
        : ButtonSVG (images, newColourMode, pathStyle, newEdgeIndent, pathStyle)
    {
    }

    /**
     * @brief Constructs from a StringArray of SVG images with an explicit hover style override.
     * @param images        Pre-built array of SVG XML strings ordered by map::ButtonState index.
     * @param newColourMode Colour tinting mode for path painting.
     * @param pathStyle     Path rendering style for normal and non-hover states.
     * @param newEdgeIndent Pixel inset applied to the paint bounds.
     * @param overPathStyle Path rendering style used when the button is highlighted (hovered).
     */
    ButtonSVG (const juce::StringArray& images,
               ColourMode newColourMode,
               jam::Svg::PathStyle pathStyle,
               int newEdgeIndent,
               jam::Svg::PathStyle overPathStyle)
        : juce::Button ({})
        , imageStorage (images)
        , colourMode (newColourMode)
        , style (pathStyle)
        , overStyle (overPathStyle)
        , edgeIndent (newEdgeIndent)
    {
    }

    /** @brief Replaces the owned image bank and repaints. */
    void setImages (juce::StringArray images)
    {
        imageStorage = std::move (images);
        repaint();
    }

    /**
     * @brief Paints from the owned bank when constructed with one, otherwise
     *        forwards to StyleCustom::drawTabButton.
     *
     * @param g            JUCE graphics context.
     * @param isMouseOver  Hover state; forwarded to the LAF or self-paint.
     * @param isMouseDown  Pressed state; forwarded to the LAF or self-paint.
     */
    void paintButton (juce::Graphics& g, bool isMouseOver, bool isMouseDown) override
    {
        if (imageStorage.isEmpty())
        {
            static_cast<jam::StyleCustom&> (getLookAndFeel())
                .drawTabButton (g, *this, isMouseOver, isMouseDown);
        }
        else
        {
            const int numImages { imageStorage.size() };
            int index { map::ButtonState::normal };

            if (numImages >= 8)
            {
                if (not isEnabled())
                    index = map::ButtonState::disabled;
                else if (isMouseDown)
                    index = map::ButtonState::down;
                else if (isMouseOver)
                    index = map::ButtonState::over;
                if (getToggleState())
                    index += map::ButtonState::normalOn;
            }
            else if (numImages >= 6)
            {
                if (isMouseDown)
                    index = map::ButtonState::down;
                else if (isMouseOver)
                    index = map::ButtonState::over;
                if (getToggleState())
                    index += map::ButtonState::normalOn;
            }
            else if (numImages >= 4)
            {
                if (not isEnabled())
                    index = map::ButtonState::disabled;
                else if (isMouseDown)
                    index = map::ButtonState::down;
                else if (isMouseOver)
                    index = map::ButtonState::over;
            }
            else if (numImages >= 3)
            {
                if (isMouseDown)
                    index = map::ButtonState::down;
                else if (isMouseOver)
                    index = map::ButtonState::over;
            }

            if (index >= numImages or imageStorage.getReference (index).isEmpty())
                index = map::ButtonState::normal;

            const auto activeStyle { isMouseOver ? overStyle : style };

            paintSingleImage (g,
                              isMouseOver,
                              isMouseDown,
                              *this,
                              imageStorage.getReference (index).toRawUTF8(),
                              colourMode,
                              activeStyle,
                              juce::PathStrokeType (1.2f, juce::PathStrokeType::beveled, juce::PathStrokeType::rounded),
                              edgeIndent);
        }
    }

private:
    juce::StringArray imageStorage;
    ColourMode colourMode { ColourMode::toggle };
    jam::Svg::PathStyle style { jam::Svg::PathStyle::stroke };
    jam::Svg::PathStyle overStyle { jam::Svg::PathStyle::stroke };
    int edgeIndent { 2 };
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ButtonSVG)
};

/**______________________________END OF NAMESPACE______________________________*/
} // namespace jam
