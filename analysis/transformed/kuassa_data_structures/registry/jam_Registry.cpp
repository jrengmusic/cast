namespace jam
{
/*____________________________________________________________________________*/

Registry::Registry (const Registration& r) { registerApplicationsFor (r); }

void Registry::registerApplicationsFor (const Registration& r)
{
    if (r.buttons)
        r.buttons (*this);

    if (r.components)
        r.components (*this);

    if (r.styles)
        r.styles (*this);

    if (r.attachments)
        r.attachments (*this);

    if (r.config)
        r.config (*this);

    if (r.events)
        r.events (*this);

    if (r.viewComponents)
        r.viewComponents (*this);

    if (r.viewBindings)
        r.viewBindings (*this);

    if (r.makeContent)
        r.makeContent (*this);

#if JUCE_DEBUG
    for (const auto& [configType, configEntries] : configure)
        jassert (make.contains (configType) and "configure key must be a make (type) key");
#endif
}

juce::String Registry::getType (const Document::Element& element)
{
    if (element.contains (Id::codeClass) and element.get<juce::String> (Id::codeClass)->isNotEmpty())
        return *element.get<juce::String> (Id::codeClass);

    if (element.id == Id::input)
        return element.get (Id::type, juce::String {});

    return element.id.toString();
}

juce::String Registry::getParameter (const Document::Element& element)
{
    return element.get (Id::dataParameter, juce::String {});
}

juce::String Registry::getContent (const Document::Element& element)
{
    return element.get (Id::dataContent, juce::String {});
}

juce::String Registry::getButton (const Document::Element& element)
{
    return element.get (Id::dataButton, juce::String {});
}

std::unique_ptr<juce::Button> Registry::toButton (std::unique_ptr<juce::Component> component)
{
    return std::unique_ptr<juce::Button> (static_cast<juce::Button*> (component.release()));
}

juce::Rectangle<int> Registry::getBounds (const Document::Element& element)
{
    juce::Rectangle<int> result {};

    if (element.hasProperty (Id::position, Id::absolute)
        and element.isType<double> (Id::left) and element.isType<double> (Id::top)
        and element.isType<double> (Id::width) and element.isType<double> (Id::height))
    {
        result = { juce::roundToInt (*element.get<double> (Id::left)),
                   juce::roundToInt (*element.get<double> (Id::top)),
                   juce::roundToInt (*element.get<double> (Id::width)),
                   juce::roundToInt (*element.get<double> (Id::height)) };
    }

    return result;
}

bool Registry::isStandardTag (const juce::String& tag)
{
    return map::HtmlStandardTag::getInstance()->contains (tag);
}

bool Registry::isStandardTag (const Document::Element& element)
{
    return isStandardTag (element.id.toString());
}

bool Registry::isValid (const Document::Element& element)
{
    if (element.id == Id::div)
        return element.contains (Id::id) and element.get<juce::String> (Id::id)->isNotEmpty()
           and element.contains (Id::codeClass) and element.get<juce::String> (Id::codeClass)->isNotEmpty();

    if (isStandardTag (element))
        return element.contains (Id::id) and element.get<juce::String> (Id::id)->isNotEmpty();

    return false;
}

/**_____________________________END OF NAMESPACE______________________________*/
}// namespace jam
