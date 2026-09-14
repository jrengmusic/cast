namespace jam
{
/*____________________________________________________________________________*/

static const juce::Identifier flexBasisUnit { Format::appendWithDash (Id::flexBasis.toString(), Id::unit.toString()) };
static const juce::Identifier marginTopUnit { Format::appendWithDash (Id::marginTop.toString(), Id::unit.toString()) };

static const jam::Array<juce::Identifier> layoutProperties { Id::flexGrow,       Id::flexShrink,     Id::flexBasis,
                                                              flexBasisUnit,     Id::fontSize,       Id::marginTop,
                                                              marginTopUnit,     Id::display,        Id::flexDirection,
                                                              Id::padding,       Id::flexGap,        Id::justifyContent,
                                                              Id::position,      Id::left,           Id::top,
                                                              Id::right,         Id::height,         Id::width };

void ViewManager::applyLayoutProperties (const Document::Element& element, juce::Component* component)
{
    for (const auto& key : layoutProperties)
    {
        if (element.isType<double> (key))
            component->getProperties().set (key, *element.get<double> (key));
        else if (element.isType<juce::String> (key))
            component->getProperties().set (key, *element.get<juce::String> (key));
    }
}

juce::FontOptions ViewManager::getFontOptions (const Document::Element& element)
{
    auto* styleManager { StyleManager::getInstance() };
    jassert (styleManager != nullptr);

    auto options { styleManager->getFont (element.contains (Id::fontFamily)
                                              ? *element.get<juce::String> (Id::fontFamily)
                                              : juce::String {}) };

    if (element.contains (Id::fontSize))
        options = options.withPointHeight (static_cast<float> (*element.get<double> (Id::fontSize)));

    return options;
}

void ViewManager::applyLabelStyle (juce::Label* label, const Document::Element& element)
{
    const auto text { element.contains (Id::text) ? element.get<juce::String> (Id::text)->trim()
                                                  : juce::String {} };

    const auto textTransform { element.contains (Id::textTransform)
                                   ? *element.get<juce::String> (Id::textTransform)
                                   : juce::String {} };

    label->getProperties().set (Id::textTransform, textTransform);

    if (text.isNotEmpty())
        applyTextTransform (label, text);

    if (element.hasProperty (Id::textAlign, Id::center))
        label->setJustificationType (juce::Justification::centred);

    if (element.contains (Id::fontFamily))
    {
        const auto options { getFontOptions (element) };

        label->setFont (juce::Font { options });
        label->getProperties().set (Id::fontFamily, *element.get<juce::String> (Id::fontFamily));
    }

    if (element.contains (Id::color))
    {
        if (auto* styleManager { StyleManager::getInstance() })
            label->setColour (juce::Label::textColourId,
                              styleManager->getColour (juce::Identifier { *element.get<juce::String> (Id::color) }));
    }
}

void ViewManager::makeFromHTML (juce::Component* view, AudioModel& model, Owner<juce::Component>& children, const Document::Element& body)
{
    if (auto registry { Registry::getInstance() })
    {
        static const jam::Function::Map<juce::String, void> elementStyles {
            []
            {
                jam::Function::Map<juce::String, void> styles;

                styles.add<juce::Component*&, const Document::Element&> (
                    Id::label,
                    [] (juce::Component* component, const Document::Element& element)
                    {
                        applyLabelStyle (static_cast<juce::Label*> (component), element);
                    });

                styles.add<juce::Component*&, const Document::Element&> (
                    Id::scrambledText,
                    [] (juce::Component* component, const Document::Element& element)
                    {
                        if (element.contains (Id::fontFamily))
                            static_cast<jam::AnimationScrambledText*> (component)->setFont (getFontOptions (element));
                    });

                return styles;
            }()
        };

        auto make = [&registry, &model, &children, &view] (auto& self, juce::Component* parent, const Document::Element& element) -> void
        {
            for (const auto* child : element)
            {
                if (Registry::isStandardTag (*child))
                {
                    if (Registry::isValid (*child))
                    {
                        const auto componentType { Registry::getType (*child) };

                        auto component { registry->make.get (componentType) };

                        component->getProperties().set (Id::parameter, Format::toScreamingSnakeCase (Registry::getParameter (*child)));
                        component->getProperties().set (Id::type, componentType);
                        component->setComponentID (jam::UUID {}.toString());
                        component->setBounds (Registry::getBounds (*child));

                        if (child->contains (Id::dataContent))
                            component->getProperties().set (Id::dataContent, *child->get<juce::String> (Id::dataContent));

                        applyLayoutProperties (*child, component.get());

                        if (elementStyles.contains (componentType))
                        {
                            auto* styledComponent { component.get() };
                            elementStyles.get (componentType, styledComponent, *child);
                        }

                        if (registry->bindToPanel.contains (componentType))
                        {
                            auto* boundComponent { component.get() };
                            registry->bindToPanel.get (componentType, boundComponent, *child, *registry, model);
                        }

                        const auto componentParameter { Registry::getParameter (*child) };

                        if (registry->bindToView.contains (componentParameter))
                        {
                            auto* boundComponent { component.get() };
                            registry->bindToView.get (componentParameter, boundComponent, *child, *registry, model);
                        }

                        parent->addAndMakeVisible (component.get());
                        children.add (std::move (component));
                    }
                    else if (child->id == Id::htmlTagDiv)
                    {
                        if (child->contains (Id::id))
                        {
                            const auto id { *child->get<juce::String> (Id::id) };

                            auto container { view->getComponentID().equalsIgnoreCase (Id::toTag (Id::panel))
                                                 ? std::make_unique<ViewPanel::Background>()
                                                 : std::make_unique<juce::Component>() };

                            container->setComponentID (jam::UUID {}.toString());
                            container->getProperties().set (Id::id, id);

                            applyLayoutProperties (*child, container.get());

                            if (child->hasProperty (Id::position, Id::absolute)
                                and not child->hasProperty (Id::width, Id::maxContent))
                                container->setBounds (Registry::getBounds (*child));

                            parent->addAndMakeVisible (container.get());
                            self (self, container.get(), *child);
                            children.add (std::move (container));
                        }
                    }
                }
            }
        };

        make (make, view, body);
    }
}

void ViewManager::addFlexItems (juce::FlexBox& flexBox, juce::Component* parent, float emUnit)
{
    auto getFlexBasis = [&emUnit] (juce::Component* child) -> float
    {
        auto& properties { child->getProperties() };

        if (properties.contains (Id::flexBasis))
        {
            auto basis { static_cast<float> (properties[Id::flexBasis]) };

            if (jam::Component::hasProperty (properties, flexBasisUnit, Id::em))
                basis *= emUnit;

            return basis;
        }

        if (properties.contains (Id::fontSize))
            return static_cast<float> (properties[Id::fontSize]);

        return 0.0f;
    };

    auto getMarginTop = [&emUnit] (juce::Component* child) -> float
    {
        auto& properties { child->getProperties() };

        auto marginTop { static_cast<float> (properties[Id::marginTop]) };

        if (jam::Component::hasProperty (properties, marginTopUnit, Id::em))
            marginTop *= emUnit;

        return marginTop;
    };

    for (auto* child : parent->getChildren())
    {
        auto& properties { child->getProperties() };

        const float grow { properties.contains (Id::flexGrow) ? static_cast<float> (properties[Id::flexGrow]) : 0.0f };
        const float shrink { properties.contains (Id::flexShrink) ? static_cast<float> (properties[Id::flexShrink]) : 1.0f };
        const float basis { getFlexBasis (child) };

        auto item { juce::FlexItem { *child } };
        item.flexGrow = grow;
        item.flexShrink = shrink;
        item.flexBasis = basis;

        if (properties.contains (Id::marginTop))
            item.margin.top = getMarginTop (child);

        flexBox.items.add (item);
    }
}

void ViewManager::applyFlexGap (juce::FlexBox& flexBox, float gap)
{
    if (gap > 0.0f)
    {
        const auto isRow { flexBox.flexDirection == juce::FlexBox::Direction::row };

        for (int i { 1 }; i < flexBox.items.size(); ++i)
        {
            if (isRow)
                flexBox.items.getReference (i).margin.left += gap;
            else
                flexBox.items.getReference (i).margin.top += gap;
        }
    }
}

void ViewManager::applyFlexLayout (juce::Component* view, float emUnit)
{
    auto applyFlex = [&emUnit] (auto& self, juce::Component* parent) -> void
    {
        auto& properties { parent->getProperties() };

        const auto padding { properties.contains (Id::padding) ? static_cast<float> (properties[Id::padding]) : 0.0f };
        const auto gap { properties.contains (Id::flexGap) ? static_cast<float> (properties[Id::flexGap]) : 0.0f };

        juce::FlexBox flexBox;
        flexBox.flexDirection = jam::Component::hasProperty (properties, Id::flexDirection, Id::row)
                                     ? juce::FlexBox::Direction::row
                                     : juce::FlexBox::Direction::column;

        const auto justifyStr { properties[Id::justifyContent].toString() };

        if (justifyStr.isNotEmpty())
        {
            static const jam::HashMap<juce::String, juce::FlexBox::JustifyContent> justify
            {
                { Id::flexEnd.toString(),      juce::FlexBox::JustifyContent::flexEnd      },
                { Id::center.toString(),       juce::FlexBox::JustifyContent::center       },
                { Id::spaceBetween.toString(), juce::FlexBox::JustifyContent::spaceBetween },
                { Id::spaceAround.toString(),  juce::FlexBox::JustifyContent::spaceAround  }
            };

            if (justify.contains (justifyStr))
                flexBox.justifyContent = justify.at (justifyStr);
        }

        addFlexItems (flexBox, parent, emUnit);
        applyFlexGap (flexBox, gap);

        flexBox.performLayout (parent->getLocalBounds().toFloat().reduced (padding));

        for (auto* child : parent->getChildren())
            if (child->getProperties().contains (Id::id))
                self (self, child);
    };

    applyFlex (applyFlex, view);
}

/**_____________________________END_OF_NAMESPACE______________________________*/
} /** namespace jam */
