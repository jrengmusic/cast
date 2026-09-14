namespace jam
{

class Logo
{
public:
    using Points = std::vector<std::vector<juce::Point<float>>>;

    using Colours = std::vector<juce::Colour>;

    struct ColouredShape
    {
        std::vector<juce::Point<float>> points;

        juce::Colour colour;

        ColouredShape (std::vector<juce::Point<float>> newPoints, juce::Colour newColour)
            : points (std::move (newPoints)), colour (newColour) {}
    };

    Logo()
    {
        const auto& svg { jam::Xml::getOrCreate (juce::Identifier { files::logo }) };

        lines = loadLines (svg);
        shapes = loadShapes (svg);
        bounds8 = loadBounds (svg, "bounds8_", 8);
        bounds3 = loadBounds (svg, "bounds3_", 3);
    }

    void drawStroke (juce::Graphics& g,
                     const juce::Rectangle<float>& area,
                     const Colours& colourScheme = default3(),
                     float lineThickness = 1.0f,
                     int index = 3,
                     float normalisedValue = 1.0f)
    {
        juce::DrawableComposite comp;

        if (index < lines.size())
        {
            addLinesPerimeter (comp, colourScheme, lineThickness, index, normalisedValue, index != 1);
        }
        else
        {
            for (int idx { 0 }; idx <= (index - 1); ++idx)
            {
                addLinesPerimeter (comp, colourScheme, lineThickness, idx, normalisedValue, idx != 1);
            }
        }

        comp.drawWithin (g, area, juce::RectanglePlacement::centred, 1.0f);
    }

    void addLinesPerimeter (juce::DrawableComposite& comp,
                            const Colours& colours,
                            float lineThickness,
                            int index,
                            float normalisedValue,
                            bool shouldBeCounterClockwise = false)
    {
        Perimeter shape { lines.at (index), normalisedValue, shouldBeCounterClockwise, lineThickness };
        auto drawable { new juce::DrawablePath (shape.getDrawablePath()) };
        drawable->setStrokeFill (colours.at (index));
        drawable->setStrokeType (juce::PathStrokeType (lineThickness));
        comp.addAndMakeVisible (drawable);
    }

    void drawFill (juce::Graphics& g,
                   const juce::Rectangle<float>& area,
                   float normalisedValue = 1.0f,
                   bool shouldBeHold = true,
                   bool shouldBeReversed = false)
    {
        juce::DrawableComposite comp;
        Owner<juce::Drawable> drawables;

        for (auto& [points, colour] : shapes)
        {
            juce::Path path;
            path.startNewSubPath (points.at (0));
            for (auto& p : points)
            {
                path.lineTo (p);
            }
            path.closeSubPath();

            auto drawable { std::make_unique<juce::DrawablePath>() };
            drawable->setPath (path);
            drawable->setFill (colour);
            comp.addChildComponent (*drawable);
            drawables.add (std::move (drawable));
        }

        int index { shouldBeReversed
                        ? toInt (Value::map (normalisedValue, static_cast<float> (drawables.size() - 1), 0.0f))
                        : toInt (Value::map (normalisedValue, 0.0f, static_cast<float> (drawables.size() - 1))) };

        if (shouldBeHold)
            for (int idx { 0 }; idx <= index; ++idx)
            {
                drawables.at (idx)->setVisible (true);
            }
        else
            drawables.at (index)->setVisible (true);

        comp.drawWithin (g, area, juce::RectanglePlacement::centred, 1.0f);
    }

    static const Colours& default3()
    {
        static const Colours colours { loadDefault3() };
        return colours;
    }

private:
    static const Document::Element* findElementById (const Document& root, juce::StringRef elementId)
    {
        for (auto* child : root)
            if (child->contains (Id::id) and *child->get<juce::String> (Id::id) == elementId)
                return child;

        return nullptr;
    }

    static std::vector<juce::Point<float>> pathToPoints (const juce::Path& path)
    {
        std::vector<juce::Point<float>> points;

        for (juce::Path::Iterator it (path); it.next();)
            if (it.elementType == juce::Path::Iterator::startNewSubPath
                or it.elementType == juce::Path::Iterator::lineTo)
                points.push_back ({ it.x1, it.y1 });

        return points;
    }

    static Points loadLines (const Document& svg)
    {
        Points result;

        for (int index { 0 }; index < 3; ++index)
        {
            const auto* element { findElementById (svg, "line" + juce::String (index)) };
            result.push_back (pathToPoints (jam::Svg::getElementPath (*element)));
        }

        return result;
    }

    static std::vector<ColouredShape> loadShapes (const Document& svg)
    {
        std::vector<ColouredShape> result;
        result.reserve (8);

        for (int index { 0 }; index < 8; ++index)
        {
            const auto* element { findElementById (svg, "shape" + juce::String (index)) };
            result.emplace_back (pathToPoints (jam::Svg::getElementPath (*element)),
                                 jam::Svg::parseColour (element->contains (Id::fill)
                                                             ? *element->get<juce::String> (Id::fill)
                                                             : juce::String()));
        }

        return result;
    }

    static std::vector<juce::Rectangle<float>> loadBounds (const Document& svg, juce::StringRef idPrefix, int count)
    {
        std::vector<juce::Rectangle<float>> result;
        result.reserve (static_cast<size_t> (count));

        for (int index { 0 }; index < count; ++index)
        {
            const auto* element { findElementById (svg, juce::String (idPrefix) + juce::String (index)) };
            result.push_back (jam::Svg::getElementPath (*element).getBounds());
        }

        return result;
    }

    static Colours loadDefault3()
    {
        Colours result;
        result.reserve (3);

        const auto& svg { jam::Xml::getOrCreate (juce::Identifier { files::logo }) };

        for (int index { 0 }; index < 3; ++index)
        {
            const auto* element { findElementById (svg, "default3_" + juce::String (index)) };
            result.push_back (jam::Svg::parseColour (element->contains (Id::fill)
                                                          ? *element->get<juce::String> (Id::fill)
                                                          : juce::String()));
        }

        return result;
    }

    Points lines;
    std::vector<ColouredShape> shapes;
    std::vector<juce::Rectangle<float>> bounds8;
    std::vector<juce::Rectangle<float>> bounds3;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Logo)
};

} // namespace jam
