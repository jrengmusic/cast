namespace jam
{
/*____________________________________________________________________________*/

void ViewManager::buildContent (juce::Component* view, AudioModel& model, Owner<juce::Component>& children, const Document& layout)
{
    if (layout.root->firstChild != nullptr)
    {
        if (auto* body { layout.root->getChildByID (Id::body) })
        {
            const auto width { body->isType<double> (Id::width) ? static_cast<int> (*body->get<double> (Id::width)) : 0 };
            const auto height { body->isType<double> (Id::height) ? static_cast<int> (*body->get<double> (Id::height)) : 0 };

            view->getProperties().set (Id::body, true);

            if (body->contains (Id::display))
                view->getProperties().set (Id::display, *body->get<juce::String> (Id::display));

            if (body->contains (Id::flexDirection))
                view->getProperties().set (Id::flexDirection, *body->get<juce::String> (Id::flexDirection));

            if (body->contains (Id::padding))
                view->getProperties().set (Id::padding, *body->get<double> (Id::padding));

            if (body->contains (Id::flexGap))
                view->getProperties().set (Id::flexGap, *body->get<double> (Id::flexGap));

            if (body->contains (Id::justifyContent))
                view->getProperties().set (Id::justifyContent, *body->get<juce::String> (Id::justifyContent));

            if (auto graphics { getGraphics (*body) })
            {
                graphics->setBounds (0, 0, width, height);
                view->addAndMakeVisible (graphics.get());
                children.add (std::move (graphics));
            }

            makeImages (view, children, *body, width, height);

            makeFromHTML (view, model, children, *body);
            applyConfig (view);
            applyLayout (view);
            setContentText (view, model, width);

            if (width > 0 and height > 0)
                view->setSize (width, height);
        }
    }
}

void ViewManager::attachParameter (juce::Component* child,
                                   jam::SettingsModel& state,
                                   juce::ValueTree& top,
                                   const juce::String& parameterID,
                                   const juce::String& componentType,
                                   Registry& registry)
{
    if (registry.configure.contains (componentType)
        and registry.configure.at (componentType).contains (parameterID)
        and Model::isNonVoid (child))
    {
        auto& value { Model::getFrom (child) };
        const juce::Identifier parameterKey { parameterID };
        auto parameterTree { top.getChildWithName (parameterKey) };

        if (not parameterTree.isValid())
        {
            parameterTree = juce::ValueTree (parameterKey);
            parameterTree.setProperty (Id::value, value.getValue(), nullptr);
            top.appendChild (parameterTree, nullptr);
        }

        value.addListener (&state);
        value.referTo (parameterTree.getPropertyAsValue (Id::value, nullptr));
    }

    if (auto* obj { dynamic_cast<Model::Object*> (child) })
    {
        if (obj->onAttachment)
            obj->onAttachment();
    }
}

void ViewManager::attachSettings (jam::SettingsModel& state, juce::Component* view)
{
    if (auto parentID { view->getComponentID() }; parentID.isNotEmpty())
    {
        auto top { state.get().getOrCreateChildWithName (juce::Identifier { Format::toValidID (parentID) }, nullptr) };

        if (top.isValid())
        {
            auto* registry { Registry::getInstance() };
            jassert (registry != nullptr);

            const auto attachChild { [&top, &state, &registry = *registry] (juce::Component* child)
            {
                const auto parameterID { child->getProperties()[Id::parameter].toString() };
                const auto componentType { child->getProperties()[Id::type].toString() };

                if (parameterID.isNotEmpty())
                    attachParameter (child, state, top, parameterID, componentType, registry);
            } };

            jam::Component::applyFunctionRecursively (view, attachChild);
        }
    }
}

jam::HashMap<juce::String, juce::String> ViewManager::buildDynamicValues (AudioModel& model)
{
    return jam::HashMap<juce::String, juce::String>
    {
        { Id::daw.toString(),           model.getHostDescription() },
        { Id::type.toString(),          model.getPluginFormatName() },
#if JAM_USING_AQUATIC_PRIME
        { Id::user.toString(),          model.getUserName() },
#endif
        { Id::versionString.toString(), ProjectInfo::versionString },
        { Id::product.toString(),       ProjectInfo::projectName },
        { Id::pluginWrapper.toString(), map::PluginWrapper::getInstance()->get (static_cast<int> (model.getWrapperType())) },
        { Id::trademark.toString(),     Format::toTrademark (Id::trademarkText, ProjectInfo::projectName) },
        { Id::copyright.toString(),     Id::copyrightSymbol.toString() + Chars::space + Format::getBuildYear()
                                            + Chars::space + ProjectInfo::legalCompanyName
                                            + juce::String::charToString (Chars::dot) }
    };
}

void ViewManager::applyTextTransform (juce::Label* label, const juce::String& contentValue)
{
    if (jam::Component::hasProperty (label->getProperties(), Id::textTransform, Id::uppercase))
        label->setText (contentValue.toUpperCase(), juce::dontSendNotification);
    else
        label->setText (contentValue, juce::dontSendNotification);
}

void ViewManager::applyMaxContentBounds (juce::Label* label, int width)
{
    auto& properties { label->getProperties() };

    if (jam::Component::hasProperty (properties, Id::width, Id::maxContent))
    {
        const auto font { label->getLookAndFeel().getLabelFont (*label) };
        const auto text { label->getText() };

        const auto measuredWidth { juce::roundToInt (std::ceil (juce::TextLayout::getStringWidth (font, text))) };
        const auto right { properties.contains (Id::right) ? juce::roundToInt (static_cast<double> (properties[Id::right])) : 0 };
        const auto top { properties.contains (Id::top) ? juce::roundToInt (static_cast<double> (properties[Id::top])) : 0 };
        const auto height { properties.contains (Id::height) ? juce::roundToInt (static_cast<double> (properties[Id::height])) : 0 };

        const auto labelBounds {
            juce::Rectangle<int> (measuredWidth, height)
                .withRightX (width - right)
                .withY (top)
        };

        label->setBounds (labelBounds);
    }
}

void ViewManager::setContentText (juce::Component* view, AudioModel& model, int width)
{
    const auto dynamicValues { buildDynamicValues (model) };

    jam::Component::applyFunctionRecursively (view,
        [&dynamicValues, width] (juce::Component* component)
        {
            const auto contentKey { component->getProperties()[Id::dataContent].toString() };

            if (contentKey.isNotEmpty())
            {
                if (auto* label { dynamic_cast<juce::Label*> (component) })
                {
                    if (dynamicValues.contains (contentKey))
                    {
                        applyTextTransform (label, dynamicValues.at (contentKey));
                        applyMaxContentBounds (label, width);
                    }
                    else
                    {
#if ! JAM_USING_AQUATIC_PRIME
                        if (jam::Component::hasProperty (component->getProperties(), Id::dataContent, Id::user))
                            label->setVisible (false);
#endif
                    }
                }
            }
        });
}

void ViewManager::makeImages (juce::Component* view, Owner<juce::Component>& children, const Document::Element& body, int width, int height)
{
    for (auto* child : body)
    {
        if (child->id == Id::img)
        {
            if (child->contains (Id::src) and child->get<juce::String> (Id::src)->isNotEmpty())
            {
                const auto src { *child->get<juce::String> (Id::src) };
                const auto artBounds { Registry::getBounds (*child) };
                std::unique_ptr<juce::Component> artComponent;

                if (auto svg { Xml::getFromBinary (src) })
                {
                    artComponent = juce::Drawable::createFromSVG (*svg);
                }
                else
                {
                    auto artImage { ImageLoader::getFromBinary (src) };

                    if (artImage.isValid())
                    {
                        auto imageComponent { std::make_unique<juce::ImageComponent>() };
                        imageComponent->setImage (artImage, juce::RectanglePlacement::stretchToFit);
                        artComponent = std::move (imageComponent);
                    }
                }

                if (artComponent != nullptr)
                {
                    if (not artBounds.isEmpty())
                        artComponent->setBounds (artBounds);
                    else
                        artComponent->setBounds (0, 0, width, height);

                    view->addAndMakeVisible (artComponent.get());
                    children.add (std::move (artComponent));
                }
            }
        }
    }
}

std::unique_ptr<juce::Drawable> ViewManager::getGraphics (const Document::Element& body)
{
    if (const auto* graphicsElement { body.getChildByID (Id::svg) })
    {
        if (graphicsElement->contains (Id::svg))
        {
            if (const auto svg { juce::parseXML (*graphicsElement->get<juce::String> (Id::svg)) })
                return juce::Drawable::createFromSVG (*svg);
        }
    }

    return nullptr;
}

/**_____________________________END_OF_NAMESPACE______________________________*/
} /** namespace jam */
