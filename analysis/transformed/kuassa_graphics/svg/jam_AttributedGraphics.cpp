namespace jam
{
/*____________________________________________________________________________*/

AttributedGraphics AttributedGraphics::flatten()
{
    AttributedGraphics flat;

    for (auto& shape : shapes)
    {
        const auto index { flat.shapes.find (shape) };

        if (index >= 0)
            flat.shapes.at (index).path.addPath (shape.path);
        else
            flat.shapes.add (std::move (shape));
    }

    flat.texts = std::move (texts);

    return flat;
}

void AttributedGraphics::paint (juce::Graphics& g,
                                const juce::LookAndFeel& laf,
                                const juce::AffineTransform& transform) const
{
    for (const auto& shape : shapes)
    {
        if (shape.gradientEndColourId != 0)
        {
            const auto startColour { shape.colourId != 0 ? laf.findColour (shape.colourId)
                                                          : shape.colour };
            const auto endColour { laf.findColour (shape.gradientEndColourId) };
            g.setGradientFill (juce::ColourGradient {
                startColour, shape.gradientStart.transformedBy (transform),
                endColour, shape.gradientEnd.transformedBy (transform), false });
        }
        else
        {
            g.setColour (shape.colourId != 0 ? laf.findColour (shape.colourId) : shape.colour);
        }

        if (shape.stroke.getStrokeThickness() > 0.0f and shape.dashLength > 0.0f)
        {
            const float dashLengths[] { shape.dashLength, shape.dashLength };
            juce::Path dashedPath;
            shape.stroke.createDashedStroke (dashedPath, shape.path, dashLengths, 2, transform);
            g.fillPath (dashedPath);
        }
        else if (shape.stroke.getStrokeThickness() > 0.0f)
        {
            juce::Path stretched { shape.path };
            stretched.applyTransform (transform);
            g.strokePath (stretched, shape.stroke);
        }
        else
        {
            g.fillPath (shape.path, transform);
        }
    }

    if (not texts.isEmpty())
    {
        const juce::Graphics::ScopedSaveState saveState (g);
        g.addTransform (transform);

        for (const auto& text : texts)
        {
            if (text.backgroundColourId != 0)
            {
                g.setColour (laf.findColour (text.backgroundColourId));
                g.fillRect (text.bounds);
            }

            text.layout.draw (g, text.bounds);
        }
    }
}

void AttributedGraphics::paint (juce::Graphics& g,
                                const juce::LookAndFeel& laf,
                                juce::Rectangle<float> bounds) const
{
    const auto contentBounds { getContentBounds() };
    const juce::RectanglePlacement placement { juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize };
    const juce::Graphics::ScopedSaveState saveState (g);

    if (not contentBounds.isEmpty())
        g.addTransform (placement.getTransformToFit (contentBounds, bounds));

    paint (g, laf, juce::AffineTransform{});
}

juce::Rectangle<float> AttributedGraphics::getContentBounds() const
{
    juce::Rectangle<float> bounds;

    for (const auto& shape : shapes)
        bounds = bounds.getUnion (shape.path.getBounds());

    for (const auto& text : texts)
        bounds = bounds.getUnion (text.bounds);

    return bounds;
}

/*____________________________________________________________________________*/

Svg::Flex::Segments Svg::Flex::getSegments (const juce::String& svg, const ColourScheme& colourScheme)
{
    Segments segments;

    if (const auto document { Xml::parse (svg) }; not document.root->id.isNull())
    {
        for (auto* group : *document.root)
            if (group->isTag (Id::g))
            {
                const juce::String name { Svg::getElementId (*group) };

                if (map::Segment::getInstance()->contains (name))
                {
                    const auto slot { map::Segment::getInstance()->get (name) };
                    auto& cell { segments.at (static_cast<size_t> (slot)) };

                    cell.id = juce::Identifier { name };
                    cell.graphics = Svg::getAttributedGraphics (*group, colourScheme);

                    group->applyFunctionRecursively ([&cell] (const Document::Element& e)
                    {
                        if (Id::bounds == juce::StringRef (Svg::getElementId (e)))
                            cell.bounds = Svg::getElementPath (e).getBounds();
                    });
                }
            }
    }

    return segments;
}

/*____________________________________________________________________________*/

Svg::Flex::Layout Svg::Flex::getLayout (const Segments& segments, juce::Rectangle<float> area)
{
    using position = map::Segment;

    Layout layout {};
    juce::Rectangle<float> source;

    for (const auto& segment : segments)
        source = source.getUnion (segment.bounds);

    if (not source.isEmpty())
    {
        const float scale { area.getHeight() / source.getHeight() };

        auto topRow { area.removeFromTop (segments.at (position::topLeft).bounds.getHeight()
                                          * scale) };
        auto bottomRow { area.removeFromBottom (
            segments.at (position::bottomLeft).bounds.getHeight() * scale) };

        auto layoutRow = [&layout, &segments, scale] (
                             juce::Rectangle<float> row, size_t left, size_t centre, size_t right)
        {
            layout.at (left) = row.removeFromLeft (segments.at (left).bounds.getWidth() * scale);
            layout.at (right) = row.removeFromRight (segments.at (right).bounds.getWidth() * scale);
            layout.at (centre) = row;
        };

        layoutRow (topRow, position::topLeft, position::top, position::topRight);
        layoutRow (area, position::left, position::centre, position::right);
        layoutRow (bottomRow, position::bottomLeft, position::bottom, position::bottomRight);
    }

    return layout;
}

void Svg::Flex::paint (juce::Graphics& g,
                       const juce::LookAndFeel& laf,
                       const Segments& segments,
                       juce::Rectangle<float> bounds)
{
    const juce::RectanglePlacement placement { juce::RectanglePlacement::stretchToFit };
    const auto layout { getLayout (segments, bounds) };

    for (size_t slot { 0 }; slot < segments.size(); ++slot)
    {
        const auto& segment { segments.at (slot) };
        auto areaToFit { placement.getTransformToFit (segment.bounds, layout.at (slot)) };

        if (not segment.bounds.isEmpty())
            segment.graphics.paint (g, laf, areaToFit);
    }
}

/**_____________________________END OF NAMESPACE______________________________*/
}// namespace jam
