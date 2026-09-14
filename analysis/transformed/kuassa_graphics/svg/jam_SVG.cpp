namespace jam
{
/*____________________________________________________________________________*/
/** -------------------- FUNCTIONS FOR READING Svg ---------------------------*/

juce::String Svg::getElementId (const Document::Element& element)
{
    const juce::String serif { element.contains (Id::serifId)
                                    ? *element.get<juce::String> (Id::serifId)
                                    : juce::String() };

    if (serif.isNotEmpty())
        return serif;

    const juce::String id { element.contains (Id::id)
                                 ? *element.get<juce::String> (Id::id)
                                 : juce::String() };

    int trimmedLength { id.length() };
    while (trimmedLength > 0 and Chars::isNumeric (id[trimmedLength - 1]))
        --trimmedLength;

    return id.substring (0, trimmedLength);
}

/*____________________________________________________________________________*/

juce::Path Svg::getElementPath (const Document::Element& element)
{
    static const jam::Function::Map<juce::String, juce::Path> tags { [] {
        jam::Function::Map<juce::String, juce::Path> tags;

        tags.add<const Document::Element&> (Id::path.toString(),
            [] (const Document::Element& element)
            {
                const juce::String d { element.contains (Id::d)
                                            ? *element.get<juce::String> (Id::d)
                                            : juce::String() };

                juce::Path path;
                path.addPath (juce::DrawableImage::parseSVGPath (d));
                return path;
            });

        tags.add<const Document::Element&> (Id::rect.toString(),
            [] (const Document::Element& element)
            {
                const float x { element.contains (Id::x)
                                    ? element.get<juce::String> (Id::x)->getFloatValue()
                                    : 0.0f };
                const float y { element.contains (Id::y)
                                    ? element.get<juce::String> (Id::y)->getFloatValue()
                                    : 0.0f };
                const float w { element.contains (Id::width)
                                    ? element.get<juce::String> (Id::width)->getFloatValue()
                                    : 0.0f };
                const float h { element.contains (Id::height)
                                    ? element.get<juce::String> (Id::height)->getFloatValue()
                                    : 0.0f };

                juce::Path path;
                path.addRectangle (juce::Rectangle<float> (x, y, w, h));
                return path;
            });

        tags.add<const Document::Element&> (Id::ellipse.toString(),
            [] (const Document::Element& element)
            {
                const float cx { element.contains (Id::cx)
                                    ? element.get<juce::String> (Id::cx)->getFloatValue()
                                    : 0.0f };
                const float cy { element.contains (Id::cy)
                                    ? element.get<juce::String> (Id::cy)->getFloatValue()
                                    : 0.0f };
                const float rx { element.contains (Id::rx)
                                    ? element.get<juce::String> (Id::rx)->getFloatValue()
                                    : 0.0f };
                const float ry { element.contains (Id::ry)
                                    ? element.get<juce::String> (Id::ry)->getFloatValue()
                                    : 0.0f };

                juce::Path path;
                path.addEllipse (cx - rx, cy - ry, rx * 2.0f, ry * 2.0f);
                return path;
            });

        tags.add<const Document::Element&> (Id::circle.toString(),
            [] (const Document::Element& element)
            {
                const float cx { element.contains (Id::cx)
                                    ? element.get<juce::String> (Id::cx)->getFloatValue()
                                    : 0.0f };
                const float cy { element.contains (Id::cy)
                                    ? element.get<juce::String> (Id::cy)->getFloatValue()
                                    : 0.0f };
                const float r  { element.contains (Id::r)
                                    ? element.get<juce::String> (Id::r)->getFloatValue()
                                    : 0.0f };

                juce::Path path;
                path.addEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f);
                return path;
            });

        tags.add<const Document::Element&> (Id::line.toString(),
            [] (const Document::Element& element)
            {
                const float x1 { element.contains (Id::x1)
                                    ? element.get<juce::String> (Id::x1)->getFloatValue()
                                    : 0.0f };
                const float y1 { element.contains (Id::y1)
                                    ? element.get<juce::String> (Id::y1)->getFloatValue()
                                    : 0.0f };
                const float x2 { element.contains (Id::x2)
                                    ? element.get<juce::String> (Id::x2)->getFloatValue()
                                    : 0.0f };
                const float y2 { element.contains (Id::y2)
                                    ? element.get<juce::String> (Id::y2)->getFloatValue()
                                    : 0.0f };

                juce::Path path;
                path.startNewSubPath (x1, y1);
                path.lineTo (x2, y2);
                return path;
            });

        return tags;
    } () };

    const juce::String tag { element.id.toString() };

    if (tags.contains (tag))
        return tags.get (tag, element);

    return {};
}

/*____________________________________________________________________________*/

AttributedGraphics Svg::getAttributedGraphics (juce::XmlElement* group, const ColourScheme& colourScheme)
{
    assert (group != nullptr);

    return getAttributedGraphics (*Xml::toDocument (*group).root, colourScheme);
}

AttributedGraphics Svg::getAttributedGraphics (juce::XmlElement* group)
{
    static const ColourScheme empty;
    return getAttributedGraphics (group, empty);
}

AttributedGraphics Svg::getAttributedGraphics (const Document::Element& group, const ColourScheme& colourScheme)
{
    AttributedGraphics raw;

    group.applyFunctionRecursively ([&raw, &colourScheme] (const Document::Element& e)
    {
        const juce::String elementName { getElementId (e) };
        juce::Path geometry { Id::bounds != juce::StringRef (elementName) ? getElementPath (e) : juce::Path {} };

        if (not geometry.isEmpty())
        {
            // Unnamed elements (empty id) and elements absent from colourScheme
            // are self-styled — constructing juce::Identifier from an empty
            // string would assert, so the empty check gates the lookup.
            int colourId { 0 };

            if (elementName.isNotEmpty())
            {
                const juce::Identifier elementKey { elementName };

                if (colourScheme.isType<int> (elementKey))
                    colourId = *colourScheme.get<int> (elementKey);
            }

            const juce::String fillValue { parseStyle (e, Id::fill) };

            if (fillValue.isNotEmpty() and Id::none != juce::StringRef (fillValue))
            {
                juce::Colour colour;

                if (colourId == 0)
                    colour = parseColour (fillValue);

                raw.shapes.add (AttributedShape {
                    geometry,
                    juce::PathStrokeType { 0.0f },
                    colour,
                    colourId });
            }

            const juce::String strokeValue { parseStyle (e, Id::stroke) };

            if (strokeValue.isNotEmpty() and Id::none != juce::StringRef (strokeValue))
            {
                const juce::String widthStr { parseStyle (e, Id::strokeWidth) };
                const float strokeWidth { widthStr.isNotEmpty()
                                              ? widthStr.getFloatValue()
                                              : 1.0f };

                const juce::String joinStr { parseStyle (e, Id::strokeLinejoin) };
                juce::PathStrokeType::JointStyle jointStyle { juce::PathStrokeType::mitered };

                if (Id::round == juce::StringRef (joinStr))  jointStyle = juce::PathStrokeType::curved;
                if (Id::bevel == juce::StringRef (joinStr))  jointStyle = juce::PathStrokeType::beveled;

                const juce::String capStr { parseStyle (e, Id::strokeLinecap) };
                juce::PathStrokeType::EndCapStyle endCapStyle { juce::PathStrokeType::butt };

                if (Id::round == juce::StringRef (capStr))   endCapStyle = juce::PathStrokeType::rounded;
                if (Id::square == juce::StringRef (capStr))  endCapStyle = juce::PathStrokeType::square;

                juce::Colour colour;

                if (colourId == 0)
                    colour = parseColour (strokeValue);

                raw.shapes.add (AttributedShape {
                    geometry,
                    juce::PathStrokeType { strokeWidth, jointStyle, endCapStyle },
                    colour,
                    colourId });
            }
        }
    });

    return raw.flatten();
}

juce::String Svg::parseStyle (const Document::Element& element, juce::StringRef key)
{
    const juce::String style { element.contains (Id::style)
                                    ? *element.get<juce::String> (Id::style)
                                    : juce::String() };
    juce::StringArray pairs;
    pairs.addTokens (style, juce::String::charToString (Chars::semicolon), "");

    for (const auto& pair : pairs)
    {
        const juce::String trimmed { pair.trim() };
        if (trimmed.startsWith (key) and trimmed.length() > key.length()
            and trimmed[key.length()] == Chars::colon)
            return jam::Format::getPostColon (trimmed).trim();
    }

    return {};
}

juce::Colour Svg::parseColour (juce::StringRef value)
{
    // Nibble accumulator — hex[6]/hex[7] default to 15 (alpha opaque) so a
    // 6-digit RRGGBB input produces a fully-opaque colour without a branch.
    uint32_t hex[8] = { 0 };
    hex[6] = hex[7] = 15;

    int numChars { 0 };
    auto s { value.text };

    // Skip optional leading '#'; collect up to 8 hex nibbles.
    if (*s == '#')
        ++s;

    while (numChars < 8 and not s.isEmpty())
    {
        const int nibble { juce::CharacterFunctions::getHexDigitValue (s.getAndAdvance()) };

        if (nibble >= 0)
            hex[numChars++] = static_cast<uint32_t> (nibble);
        else
            break;
    }

    // CSS shorthand #RGB — each nibble expanded by ×0x11 (e.g. 'f' → 0xff).
    if (numChars <= 3)
        return juce::Colour { static_cast<uint8_t> (hex[0] * 0x11),
                              static_cast<uint8_t> (hex[1] * 0x11),
                              static_cast<uint8_t> (hex[2] * 0x11) };

    // #RRGGBB (alpha defaults opaque via hex[6]=hex[7]=15) or #RRGGBBAA.
    return juce::Colour { static_cast<uint8_t> ((hex[0] << 4) + hex[1]),
                          static_cast<uint8_t> ((hex[2] << 4) + hex[3]),
                          static_cast<uint8_t> ((hex[4] << 4) + hex[5]),
                          static_cast<uint8_t> ((hex[6] << 4) + hex[7]) };
}

juce::String Svg::Filter::group (juce::XmlElement* svg)
{
    juce::String filtered { svg->toString().replace ("</g>", "") };

    while (filtered.contains ("<g"))
    {
        auto toBeRemoved = filtered.fromFirstOccurrenceOf ("<g", true, true)
                               .upToFirstOccurrenceOf (">", true, true);

        filtered = filtered.replaceFirstOccurrenceOf (toBeRemoved, "", true);
    }

    return filtered;
}

juce::String Svg::Filter::declaration (juce::XmlElement* svg)
{
    juce::String filtered { svg->toString() };
    juce::String remove { filtered.fromFirstOccurrenceOf ("<?xml", true, true)
                              .upToFirstOccurrenceOf ("<svg", false, true) };

    filtered = filtered.replaceFirstOccurrenceOf (remove, "", true);
    remove = filtered.fromFirstOccurrenceOf (" version", true, true)
                 .upToFirstOccurrenceOf (" style", false, true);

    filtered = filtered.replaceFirstOccurrenceOf (remove, "", true);

    return filtered.trim();
}

/*____________________________________________________________________________*/
/** -------------------- FUNCTIONS FOR DRAWING Svg ---------------------------*/

void Svg::draw (juce::Graphics& g, juce::Rectangle<int> area, const juce::File& file)
{
    juce::DrawableImage::createFromSVG (*juce::parseXML (juce::File (file)))
        ->drawWithin (g, area.toFloat(), juce::RectanglePlacement::centred, 1.0f);
}

void Svg::draw (juce::Graphics& g, juce::Rectangle<int> area, juce::XmlElement* e)
{
    juce::DrawableImage::createFromSVG (*e)->drawWithin (
        g, area.toFloat(), juce::RectanglePlacement::centred, 1.0f);
}

void Svg::draw (juce::Graphics& g, juce::Rectangle<int> area, juce::Drawable* drawable)
{
    drawable->drawWithin (g, area.toFloat(), juce::RectanglePlacement::centred, 1.0f);
}

juce::Path Svg::getEllipsePath (const Document::Element& xml)
{
    juce::Path path;

    for (auto* child : xml)
        if (child->isTag (Id::ellipse))
            path.addPath (getElementPath (*child));

    return path;
}

juce::Path Svg::getCirclePath (const Document::Element& xml)
{
    juce::Path path;

    for (auto* child : xml)
        if (child->isTag (Id::circle))
            path.addPath (getElementPath (*child));

    return path;
}

juce::Path Svg::getRectPath (const Document::Element& xml)
{
    juce::Path path;

    for (auto* child : xml)
        if (child->isTag (Id::rect))
            path.addPath (getElementPath (*child));

    return path;
}

juce::Path Svg::getAllFoundPath (const Document::Element& xml, ElementType elementToAdd)
{
    juce::Path path;

    switch (elementToAdd)
    {
        case ElementType::path:
            for (auto* child : xml)
                if (child->isTag (Id::path))
                    path.addPath (getElementPath (*child));
            break;

        case ElementType::ellipse:
            path.addPath (getEllipsePath (xml));
            break;

        case ElementType::circle:
            path.addPath (getCirclePath (xml));
            break;

        case ElementType::rect:
            path.addPath (getRectPath (xml));
            break;

        case ElementType::all:
            for (auto* child : xml)
                if (child->isTag (Id::path))
                    path.addPath (getElementPath (*child));
            path.addPath (getEllipsePath (xml));
            path.addPath (getCirclePath (xml));
            path.addPath (getRectPath (xml));
            break;
    }

    return path;
}

juce::Path Svg::getPath (juce::XmlElement* svg, ElementType elementToAdd)
{
    assert (svg != nullptr);

    return getPath (*Xml::toDocument (*svg).root, elementToAdd);
}

juce::Path Svg::getPath (const Document::Element& svg, ElementType elementToAdd)
{
    juce::Path path;

    for (auto* child : svg)
        if (child->isTag (Id::g))
            path.addPath (getAllFoundPath (*child, elementToAdd));

    path.addPath (getAllFoundPath (svg, elementToAdd));

    return path;
}

juce::Path Svg::getPath (const juce::String& svgString, ElementType elementToAdd)
{
    juce::Path path;

    if (const auto document { Xml::parse (svgString) }; not document.root->id.isNull())
        path = getPath (*document.root, elementToAdd);

    return path;
}

juce::Path Svg::getPath (const Document::Element& svg,
                         const juce::Rectangle<float>& areaToFit,
                         ElementType elementToAdd)
{
    juce::Path path { getPath (svg, elementToAdd) };

    juce::Rectangle<float> source;

    if (svg.contains (Id::viewBox))
    {
        source = juce::Rectangle<float>::fromString (*svg.get<juce::String> (Id::viewBox));
    }
    else
    {
        const float width { svg.contains (Id::width)
                                ? svg.get<juce::String> (Id::width)->getFloatValue()
                                : 0.0f };
        const float height { svg.contains (Id::height)
                                 ? svg.get<juce::String> (Id::height)->getFloatValue()
                                 : 0.0f };
        source = juce::Rectangle<float> { 0.0f, 0.0f, width, height };
    }

    path.applyTransform (juce::RectanglePlacement().getTransformToFit (source, areaToFit));

    return path;
}

juce::Path Svg::getPath (const juce::String& svg,
                         const juce::Rectangle<float>& areaToFit,
                         ElementType elementToAdd)
{
    juce::Path path;

    if (const auto document { Xml::parse (svg) }; not document.root->id.isNull())
        path = getPath (*document.root, areaToFit, elementToAdd);

    return path;
}

juce::Path Svg::getPath (const juce::String& svg,
                         const juce::Rectangle<int>& areaToFit,
                         ElementType elementToAdd)
{
    return getPath (svg, areaToFit.toFloat(), elementToAdd);
}

/*____________________________________________________________________________*/
#if JUCE_MODULE_AVAILABLE_juce_gui_basics
std::unique_ptr<juce::DrawablePath> Svg::getDrawablePath (juce::XmlElement* svg,
                                                          const juce::Colour& colour,
                                                          ElementType elementToAdd,
                                                          PathStyle style,
                                                          float strokeWidth)
{
    auto drawable { std::make_unique<juce::DrawablePath>() };

    juce::Path path { getPath (svg, elementToAdd) };

    switch (style)
    {
        case PathStyle::alternate:
            path.setUsingNonZeroWinding (false);
        case PathStyle::stroke:
            drawable->setPath (path);
            drawable->setFill (juce::Colour());
            drawable->setStrokeType (
                juce::PathStrokeType (strokeWidth, juce::PathStrokeType::JointStyle::mitered));
            drawable->setStrokeFill (colour);
            break;

        case PathStyle::fill:
            drawable->setPath (path);
            drawable->setFill (colour);
            break;
    }

    return drawable;
}

std::unique_ptr<juce::DrawablePath> Svg::getDrawablePath (const char* svgString,
                                                          const juce::Colour& colour,
                                                          ElementType elementToAdd,
                                                          PathStyle style,
                                                          float strokeWidth)
{
    return Svg::getDrawablePath (
        juce::parseXML (svgString).get(), colour, elementToAdd, style, strokeWidth);
}

std::unique_ptr<juce::Drawable> Svg::getDrawable (const char* svgString)
{
    return juce::DrawableImage::createFromSVG (*juce::parseXML (svgString));
}
#endif// JUCE_MODULE_AVAILABLE_juce_gui_basics
/*____________________________________________________________________________*/
/** -------------------- FUNCTIONS FOR WRITING Svg ---------------------------*/

const juce::String Svg::Template::declaration {
    R"***(<?xml version="1.0" encoding="UTF-8" standalone="no"?>
<!DOCTYPE svg PUBLIC "-//W3C//DTD SVG 1.1//EN" "http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd">
<svg width="@svgWidth@px" height="@svgHeight@px" version="1.1" xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" xml:space="preserve" xmlns:serif="http://www.serif.com/" style="fill-rule:evenodd;clip-rule:evenodd;stroke-linecap:round;stroke-linejoin:round;stroke-miterlimit:1.5;">
@svgString@
</svg>)***"
};

const juce::String Svg::Template::parseSVGPath {
    "\t\tjuce::DrawableImage::parseSVGPath (@path@),"
};

const juce::String Svg::Template::arrayPath {
    R"***(
    const juce::Array<juce::Path> @varName@
    {
@paths@
    };)***"
};

const juce::String Svg::Template::Ptr::chars {
    R"***(
    constexpr static const char* const images[]
    {@pointers@
    };)***"
};

const juce::String Svg::Template::Ptr::literal {
    R"***(
        R"SVGTEMPLATE(
@filtered@
        )SVGTEMPLATE")***"
};

const juce::String Svg::Template::Ptr::createFromSVG {
    "juce::DrawableImage::createFromSVG (*juce::parseXML (@chars@))"
};

/*____________________________________________________________________________*/

juce::String Svg::Format::Ptr::chars (const juce::String& images)
{
    return Template::Ptr::chars.replace ("@pointers@", images);
}

juce::String Svg::Format::Ptr::literal (juce::XmlElement* parsedXML)
{
    return Template::Ptr::literal.replace ("@filtered@", Filter::declaration (parsedXML));
}

juce::String Svg::Format::Ptr::createFromSVG (const juce::String& chars)
{
    return Template::Ptr::createFromSVG.replace ("@chars@", chars);
}

/*____________________________________________________________________________*/

juce::String Svg::Format::pathToString (const juce::Path& path)
{
    if (path.toString().isNotEmpty())
    {
        auto tokenize { juce::StringArray::fromTokens (path.toString().toUpperCase(), true) };

        for (auto& token : tokenize)
        {
            if (token.containsAnyOf ("ABCDEFGHIJKLMNOPQRSTUVWXYZ"))
            {
                token = '*' + token + '*';
            }
            else if (token.containsAnyOf ("0123456789"))
            {
                token = '|' + token + '|';
            }
        }

        return tokenize.joinIntoString ("")
            .replace ("*", "")
            .replace ("|*", "")
            .replace ("*|", "")
            .replace ("||", ", ")
            .replace ("|", "");
    }

    return juce::String();
}

juce::String Svg::Format::stroke (const juce::Path& path,
                                  const juce::Colour& colour,
                                  float strokeWidth,
                                  const juce::String& pathId,
                                  float dashLength,
                                  const juce::String& gradientId)
{
    auto d = pathToString (path);
    if (d.isEmpty())
        return {};

    // Extract RGB hex (#RRGGBB) and alpha separately
    const juce::String rgb = juce::String::formatted (
        "#%02x%02x%02x", colour.getRed(), colour.getGreen(), colour.getBlue());
    const juce::String alpha = juce::String (colour.getFloatAlpha(), 1).replaceCharacter (',', '.');
    const juce::String dashArray = dashLength > 0.0f
                                       ? " stroke-dasharray=\"" + juce::String (dashLength) + " "
                                             + juce::String (dashLength) + "\""
                                       : juce::String();
    const juce::String strokeValue = gradientId.isNotEmpty()
                                         ? juce::String (Id::url.toString()) + "(#" + gradientId + ")"
                                         : rgb;
    const juce::String opacityAttr = gradientId.isNotEmpty()
                                         ? juce::String()
                                         : " stroke-opacity=\"" + alpha + "\"";

    juce::String xml;
    xml << "\n\t\t<path" << (pathId.isNotEmpty() ? " id=" + pathId.quoted() : juce::String())
        << " d=\"" << d << "\""
        << " fill=\"none\""
        << " stroke=\"" << strokeValue << "\""
        << " stroke-width=\"" << juce::String (strokeWidth) << "px\""
        << opacityAttr
        << dashArray
        << "/>\n";

    return xml;
}

juce::String Svg::Format::fill (const juce::Path& path,
                                const juce::Colour& colour,
                                const juce::String& pathId,
                                const juce::String& gradientId)
{
    auto d = pathToString (path);
    if (d.isEmpty())
        return {};

    const juce::String rgb = juce::String::formatted (
        "#%02x%02x%02x", colour.getRed(), colour.getGreen(), colour.getBlue());
    const juce::String alpha = juce::String (colour.getFloatAlpha(), 1).replaceCharacter (',', '.');
    const juce::String fillValue = gradientId.isNotEmpty()
                                       ? juce::String (Id::url.toString()) + "(#" + gradientId + ")"
                                       : rgb;
    const juce::String opacityAttr = gradientId.isNotEmpty()
                                         ? juce::String()
                                         : " fill-opacity=\"" + alpha + "\"";

    juce::String xml;
    xml << "\n\t\t<path" << (pathId.isNotEmpty() ? " id=" + pathId.quoted() : juce::String())
        << " d=\"" << d << "\""
        << " fill=\"" << fillValue << "\""
        << opacityAttr
        << " stroke=\"none\""
        << "/>\n";

    return xml;
}

juce::String Svg::Format::linearGradient (const juce::String& gradientId,
                                          juce::Point<float> start,
                                          juce::Point<float> end,
                                          const juce::Colour& startColour,
                                          const juce::Colour& endColour)
{
    juce::XmlElement element { Id::linearGradient };
    element.setAttribute (Id::id, gradientId);
    element.setAttribute (Id::gradientUnits, Id::userSpaceOnUse.toString());
    element.setAttribute (Id::x1, static_cast<double> (start.x));
    element.setAttribute (Id::y1, static_cast<double> (start.y));
    element.setAttribute (Id::x2, static_cast<double> (end.x));
    element.setAttribute (Id::y2, static_cast<double> (end.y));

    auto* startStop { element.createNewChildElement (Id::stop) };
    startStop->setAttribute (Id::offset, "0%");
    startStop->setAttribute (Id::stopColor, juce::String::formatted (
        "#%02x%02x%02x", startColour.getRed(), startColour.getGreen(), startColour.getBlue()));
    startStop->setAttribute (Id::stopOpacity,
                             juce::String (startColour.getFloatAlpha(), 1).replaceCharacter (',', '.'));

    auto* endStop { element.createNewChildElement (Id::stop) };
    endStop->setAttribute (Id::offset, "100%");
    endStop->setAttribute (Id::stopColor, juce::String::formatted (
        "#%02x%02x%02x", endColour.getRed(), endColour.getGreen(), endColour.getBlue()));
    endStop->setAttribute (Id::stopOpacity,
                           juce::String (endColour.getFloatAlpha(), 1).replaceCharacter (',', '.'));

    return "\n\t\t" + element.toString (juce::XmlElement::TextFormat().singleLine().withoutHeader())
           + "\n";
}

juce::String Svg::Format::defs (juce::StringRef svgString)
{
    return "\t<defs>\t" + juce::String (svgString) + "\t</defs>";
}

juce::String Svg::Format::text (const AttributedText& text, const juce::Colour& colour)
{
    if (text.text.isEmpty())
        return {};

    const auto& attributes { text.text.getAttributes() };
    jassert (attributes.size() == 1
             and "Format::text: multi-range AttributedText is not serialized -- deferred to whichever diagram type first needs it");

    const auto fontHeight { attributes.at (0).font.getHeight() };

    // bounds' height already equals fontHeight under the shared measurement
    // convention (jam::MermaidDiagram::measureText) — bounds.getY() +
    // fontHeight is bounds' bottom edge, where the baseline sits (see this
    // function's own doc comment above).
    const auto baselineY { text.bounds.getY() + fontHeight };

    const juce::String rgb = juce::String::formatted (
        "#%02x%02x%02x", colour.getRed(), colour.getGreen(), colour.getBlue());
    const juce::String alpha = juce::String (colour.getFloatAlpha(), 1).replaceCharacter (',', '.');

    juce::XmlElement element { Id::text };
    element.setAttribute (Id::x, static_cast<double> (text.bounds.getX()));
    element.setAttribute (Id::y, static_cast<double> (baselineY));
    element.setAttribute (Id::fontSize, static_cast<double> (fontHeight));
    element.setAttribute (Id::fill, rgb);
    element.setAttribute (Id::fillOpacity, alpha);
    element.addTextElement (text.text.toJuce().getText());

    return "\n\t\t"
           + element.toString (juce::XmlElement::TextFormat().singleLine().withoutHeader())
           + "\n";
}

juce::String Svg::Format::group (juce::StringRef svgString, const juce::String& groupId)
{
    juce::String format { "\t<g@groupId@>\t@svgString@\t</g>" };

    format =
        format.replace ("@groupId@", groupId.isNotEmpty() ? " id=" + groupId.quoted() : groupId);

    return format.replace ("@svgString@", svgString);
}

juce::String Svg::Format::anchor (juce::StringRef svgString, const juce::String& url)
{
    juce::String format { "\t<a href=@url@>\t@svgString@\t</a>" };

    format = format.replace ("@url@", url.quoted());

    return format.replace ("@svgString@", svgString);
}

juce::String Svg::Format::rect (const juce::Rectangle<int>& rectangle,
                                const juce::String& rectId,
                                const juce::Colour& colour)
{
    enum
    {
        x,
        y,
        width,
        height
    };

    juce::StringArray keywords {
        "@x@",
        "@y@",
        "@width@",
        "@height@",
    };

    juce::String formatted {
        "\t<rect id=\"" + jam::Format::toValidID (rectId) + "\" x=\"" + keywords[x] + "\" y=\""
        + keywords[y] + "\" width=\"" + keywords[width] + "\" height=\"" + keywords[height]
        + "\" style=\"fill:none;stroke:" + colour.toString().replaceSection (0, 2, "#") + ";\"/>"
    };

    juce::StringArray bounds;
    bounds.addTokens (rectangle.toString(), false);

    for (int index { x }; index < bounds.size(); ++index)
    {
        formatted = formatted.replace (keywords[index], bounds[index]);
    }

    return formatted;
}

juce::StringArray
Svg::Format::getStringArrayPath (juce::XmlElement* svg)
{
    juce::StringArray paths;

    if (auto xml = juce::parseXML (Filter::group (svg)))
    {
        for (auto* e : xml->getChildWithTagNameIterator (Id::path))
        {
            paths.add (
                Template::parseSVGPath.replace ("@path@", e->getStringAttribute (Id::d)));
        }

        for (auto* e : xml->getChildWithTagNameIterator (Id::ellipse))
        {
            const float cx { static_cast<float> (e->getDoubleAttribute (Id::cx)) };
            const float cy { static_cast<float> (e->getDoubleAttribute (Id::cy)) };
            const float rx { static_cast<float> (e->getDoubleAttribute (Id::rx)) };
            const float ry { static_cast<float> (e->getDoubleAttribute (Id::ry)) };
            const float x { cx - rx };
            const float y { cy - ry };

            juce::Path path;
            path.addEllipse (x, y, 2 * rx, 2 * ry);

            paths.add (Template::parseSVGPath.replace ("@path@", Format::pathToString (path)));
        }

        for (auto* e : xml->getChildWithTagNameIterator (Id::circle))
        {
            const float cx { static_cast<float> (e->getDoubleAttribute (Id::cx)) };
            const float cy { static_cast<float> (e->getDoubleAttribute (Id::cy)) };
            const float r { static_cast<float> (e->getDoubleAttribute (Id::r)) };
            const float x { cx - r };
            const float y { cy - r };

            juce::Path path;
            path.addEllipse (x, y, 2 * r, 2 * r);

            paths.add (Template::parseSVGPath.replace ("@path@", Format::pathToString (path)));
        }

        for (auto* e : xml->getChildWithTagNameIterator (Id::rect))
        {
            const juce::String fillNone { Id::fill.toString() + juce::String::charToString (Chars::colon)
                                          + Id::none.toString() + juce::String::charToString (Chars::semicolon) };

            if (not e->getStringAttribute (Id::style).equalsIgnoreCase (fillNone))
            {
                const float x { static_cast<float> (e->getDoubleAttribute (Id::x)) };
                const float y { static_cast<float> (e->getDoubleAttribute (Id::y)) };
                const float w { static_cast<float> (e->getDoubleAttribute (Id::width)) };
                const float h { static_cast<float> (e->getDoubleAttribute (Id::height)) };

                juce::Path path;
                path.addRectangle (x, y, w, h);

                paths.add (
                    Template::parseSVGPath.replace ("@path@", Format::pathToString (path)));
            }
        }
    }

    return paths;
}
/*____________________________________________________________________________*/

juce::String Svg::File::getStringToWrite (int width, int height, juce::StringRef svgString)
{
    return Template::declaration.replace ("@svgWidth@", juce::String (width))
        .replace ("@svgHeight@", juce::String (height))
        .replace ("@svgString@", svgString);
}

/*____________________________________________________________________________*/

int Svg::getSVGWidth (const Document::Element& svg)
{
    return svg.contains (Id::width)
               ? svg.get<juce::String> (Id::width)->getIntValue()
               : 0;
}

int Svg::getSVGHeight (const Document::Element& svg)
{
    return svg.contains (Id::height)
               ? svg.get<juce::String> (Id::height)->getIntValue()
               : 0;
}

auto Svg::getSVGSize (const Document::Element& svg)
{
    return std::make_pair (getSVGWidth (svg), getSVGHeight (svg));
}

/**_____________________________END OF NAMESPACE______________________________*/
}// namespace jam
