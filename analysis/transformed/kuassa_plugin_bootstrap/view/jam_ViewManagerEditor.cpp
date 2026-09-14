namespace jam
{
/*____________________________________________________________________________*/

juce::String ViewManager::getComponentType (const MarkdownDocument& components, Document::Element& row)
{
    const auto typeView { components.getTableValueView (row, Id::type) };
    return juce::String::fromUTF8 (typeView.data(), static_cast<int> (typeView.size()));
}

juce::String ViewManager::getComponentParameter (const MarkdownDocument& components, Document::Element& row)
{
    const auto parameterView { components.getTableValueView (row, Id::parameter) };
    return juce::String::fromUTF8 (parameterView.data(), static_cast<int> (parameterView.size()));
}

juce::String ViewManager::getComponentParameterID (const MarkdownDocument& components, Document::Element& row)
{
    return Format::toScreamingSnakeCase (getComponentParameter (components, row));
}

juce::String ViewManager::getComponentParameter (const MarkdownDocument& components, Document::Element& row, const juce::Identifier& itemId)
{
    const auto parameterView { components.getTableValueView (row, Id::parameter, itemId) };
    return juce::String::fromUTF8 (parameterView.data(), static_cast<int> (parameterView.size()));
}

juce::Rectangle<int> ViewManager::getComponentBounds (const Document::Element& rect)
{
    return Svg::getElementPath (rect).getBounds().toNearestInt();
}

juce::String ViewManager::getComponentStyle (const MarkdownDocument& components, Document::Element& row)
{
    const auto styleView { components.getTableValueView (row, Id::style) };
    return juce::String::fromUTF8 (styleView.data(), static_cast<int> (styleView.size()));
}

juce::String ViewManager::getComponentImage (const MarkdownDocument& components, Document::Element& row)
{
    const auto imageView { components.getTableValueView (row, Id::image) };
    return juce::String::fromUTF8 (imageView.data(), static_cast<int> (imageView.size()));
}

juce::String ViewManager::getComponentDark (const MarkdownDocument& components, Document::Element& row)
{
    const auto darkView { components.getTableValueView (row, Id::dark) };
    return juce::String::fromUTF8 (darkView.data(), static_cast<int> (darkView.size()));
}

juce::String ViewManager::getComponentStyle (const MarkdownDocument& components, Document::Element& row, const juce::Identifier& itemId)
{
    const auto styleView { components.getTableValueView (row, Id::style, itemId) };
    return juce::String::fromUTF8 (styleView.data(), static_cast<int> (styleView.size()));
}

juce::String ViewManager::getComponentImage (const MarkdownDocument& components, Document::Element& row, const juce::Identifier& itemId)
{
    const auto imageView { components.getTableValueView (row, Id::image, itemId) };
    return juce::String::fromUTF8 (imageView.data(), static_cast<int> (imageView.size()));
}

juce::String ViewManager::getComponentDark (const MarkdownDocument& components, Document::Element& row, const juce::Identifier& itemId)
{
    const auto darkView { components.getTableValueView (row, Id::dark, itemId) };
    return juce::String::fromUTF8 (darkView.data(), static_cast<int> (darkView.size()));
}

juce::String ViewManager::getComponentEvent (const MarkdownDocument& components, Document::Element& row)
{
    const auto eventView { components.getTableValueView (row, Id::event) };
    return juce::String::fromUTF8 (eventView.data(), static_cast<int> (eventView.size()));
}

//==============================================================================

void ViewManager::build (juce::Component* view,
                         AudioModel& model,
                         Owner<juce::Component>& children,
                         Owner<juce::LookAndFeel>& lafs,
                         AnyOwner& attachments,
                         Getters& getters,
                         Events& chainEvents)
{
    if (auto r { Registry::getInstance() })
    {
        const auto& components { MarkdownDocument::getOrCreate (files::componentLayout) };

        make (view, model, children, lafs);

        for (auto* row : components.getTableRows (Id::component))
            bindComponent (view, r, components, *row, model, attachments, getters, chainEvents);

        setBypassed (view, model);
    }
}

//==============================================================================

void ViewManager::make (juce::Component* view,
                        AudioModel& model,
                        Owner<juce::Component>& children,
                        Owner<juce::LookAndFeel>& lafs)
{
    if (auto r { Registry::getInstance() })
    {
        const auto& components { MarkdownDocument::getOrCreate (files::componentLayout) };
        const auto lightOrDark { model.getAppearance() };

        for (auto* row : components.getTableRows (Id::component))
            makeComponent (view, r, components, *row, children, lafs, lightOrDark);
    }
}

static void cacheAppearanceImages (const juce::String& imageName, const juce::String& darkImageName)
{
    jassert (VulkanEngine::getInstance() != nullptr);

    if (VulkanEngine::getInstance()->isGpuAvailable())
    {
        auto* cache { VulkanTextureCache::getInstance() };
        jassert (cache != nullptr);

        if (imageName.isNotEmpty())
        {
            auto lightImage { ImageLoader::getFromBinary (imageName) };
            cache->cacheImageTexture (lightImage, lightImage.getWidth(), lightImage.getHeight());
        }

        if (darkImageName.isNotEmpty())
        {
            auto darkImage { ImageLoader::getFromBinary (darkImageName) };
            cache->cacheImageTexture (darkImage, darkImage.getWidth(), darkImage.getHeight());
        }
    }
}

void ViewManager::makeComponent (juce::Component* view,
                                 Registry* r,
                                 const MarkdownDocument& components,
                                 Document::Element& row,
                                 Owner<juce::Component>& children,
                                 Owner<juce::LookAndFeel>& lafs,
                                 juce::StringRef lightOrDark)
{
    const auto type { getComponentType (components, row) };

    const auto dParam { getComponentParameterID (components, row) };
    auto* rect { *row.get<Document::Element*> (Id::rect) };

    auto component { r->make.get (type) };

    component->setComponentID (Svg::getElementId (*rect));
    component->getProperties().set (Id::parameter, dParam);
    component->getProperties().set (Id::type, type);

    const auto styleName { getComponentStyle (components, row) };

    if (styleName.isNotEmpty())
        if (auto laf { r->style.get (styleName) })
        {
            component->setLookAndFeel (laf.get());
            lafs.add (std::move (laf));
        }

    const auto imageName { getComponentImage (components, row) };
    const auto darkImageName { getComponentDark (components, row) };

    cacheAppearanceImages (imageName, darkImageName);

    applyAppearance (r, component.get(), type, styleName, imageName, darkImageName, lightOrDark);

    view->addAndMakeVisible (component.get());

    applyConfig (r, component.get(), type, dParam);

    auto* parentComponent { component.get() };
    children.add (std::move (component));
    makeChildren (parentComponent, r, components, row, children, lafs, dParam, lightOrDark);
    parentComponent->setBounds (getComponentBounds (*rect));
}

void ViewManager::makeChildren (juce::Component* parentComponent,
                                Registry* r,
                                const MarkdownDocument& components,
                                Document::Element& row,
                                Owner<juce::Component>& children,
                                Owner<juce::LookAndFeel>& lafs,
                                const juce::String& parentParameterID,
                                juce::StringRef lightOrDark)
{
    if (auto* list { components.getTableList (row, Id::type) })
        for (auto* item : *list)
        {
            const auto childType { item->id.toString() };
            const auto childParameter { getComponentParameter (components, row, item->id) };
            const auto dParam { childParameter.isNotEmpty() ? Format::toScreamingSnakeCase (childParameter) : parentParameterID };

            auto component { r->make.get (childType) };

            component->setComponentID (childType);
            component->getProperties().set (Id::parameter, dParam);
            component->getProperties().set (Id::type, childType);

            const auto childStyle { getComponentStyle (components, row, item->id) };

            if (childStyle.isNotEmpty())
                if (auto laf { r->style.get (childStyle) })
                {
                    component->setLookAndFeel (laf.get());
                    lafs.add (std::move (laf));
                }

            const auto childImage { getComponentImage (components, row, item->id) };
            const auto childDark { getComponentDark (components, row, item->id) };

            cacheAppearanceImages (childImage, childDark);

            applyAppearance (r, component.get(), childType, childStyle, childImage, childDark, lightOrDark);

            parentComponent->addChildComponent (component.get());

            applyConfig (r, component.get(), childType, dParam);

            children.add (std::move (component));
        }
}

void ViewManager::bindComponent (Registry* r,
                                 juce::Component* c,
                                 const juce::String& type,
                                 const juce::String& dParam,
                                 const juce::String& dEvent,
                                 AudioModel& model,
                                 AnyOwner& attachments,
                                 Getters& getters,
                                 Events& chainEvents)
{
    if (dParam.isNotEmpty() and type.compare (Id::popupTextBox.toString()) == 0)
        if (auto* parameter { ParameterManager::getInstance() })
            if (const auto& unitMap { parameter->getUnitMap() }; unitMap.contains (dParam))
                static_cast<juce::Slider*> (c)->setTextValueSuffix (unitMap.at (dParam).toString());

    if (dParam.isNotEmpty() and r->attach.contains (type))
        r->attach.get (type, &attachments, c, &model, dParam);

    if (dParam.isNotEmpty() and r->bindToProcessor.contains (type))
        r->bindToProcessor.get (type, c, &getters, dParam);

    if (r->bindToModel.contains (type))
        r->bindToModel.get (type, c, dParam, model);

    if (dEvent.isNotEmpty() and r->bindEvent.contains (type))
        r->bindEvent.get (type, c, &chainEvents, dEvent, dParam);
}

void ViewManager::bindComponent (juce::Component* view,
                                 Registry* r,
                                 const MarkdownDocument& components,
                                 Document::Element& row,
                                 AudioModel& model,
                                 AnyOwner& attachments,
                                 Getters& getters,
                                 Events& chainEvents)
{
    auto* c { view->getChildComponent (Format::getNumber (components.getTableValueView (row, Id::z))) };
    const auto type { getComponentType (components, row) };

    const auto dParam { getComponentParameterID (components, row) };
    const auto dEvent { getComponentEvent (components, row) };
    const auto primaryView { components.getTableValueView (row, Id::primary) };

    if (not primaryView.empty())
    {
        auto* primaryComponent { view->getChildComponent (Format::getNumber (primaryView)) };

        if (r->bindTo.contains (type))
            r->bindTo.get (type, c, primaryComponent);
    }

    bindComponent (r, c, type, dParam, dEvent, model, attachments, getters, chainEvents);

    int ordinal { 0 };

    if (auto* list { components.getTableList (row, Id::type) })
        for (auto* item : *list)
        {
            auto* child { c->getChildComponent (ordinal) };
            const auto childType { item->id.toString() };
            const auto childParameter { getComponentParameter (components, row, item->id) };
            const auto childParameterID { childParameter.isNotEmpty() ? Format::toScreamingSnakeCase (childParameter) : dParam };
            const juce::String childEvent;

            bindComponent (r, child, childType, childParameterID, childEvent, model, attachments, getters, chainEvents);

            ++ordinal;
        }
}

void ViewManager::applyAppearance (Registry* r,
                                   juce::Component* c,
                                   const juce::String& type,
                                   const juce::String& styleName,
                                   const juce::String& imageName,
                                   const juce::String& darkImageName,
                                   juce::StringRef lightOrDark)
{
    if (auto* style { StyleManager::getInstance() })
    {
        const bool isDarkActive { StyleManager::isDark (lightOrDark) };
        juce::Image image;

        if (imageName.isNotEmpty())
        {
            juce::String resourceName;

            map::Appearance::setAppearance (
                imageName,
                darkImageName,
                isDarkActive,
                [&resourceName] (const juce::String& slot)
                {
                    resourceName = slot;
                });

            image = ImageLoader::getFromBinary (resourceName);
        }

        if (styleName.isNotEmpty())
        {
            auto& laf { c->getLookAndFeel() };
            style->setAppearance (laf, lightOrDark);
            style->setColourForStyle (styleName, &laf);

            if (r->setFont.contains (styleName))
                style->applyFontsTo (&laf, styleName, r->setFont);
        }

        if (imageName.isNotEmpty())
        {
            if (r->setImage.contains (type))
                r->setImage.get (type, c, image);

            if (r->setStyleImage.contains (styleName))
            {
                auto* laf { &c->getLookAndFeel() };
                r->setStyleImage.get (styleName, laf, image);
            }

            if (r->setClipImage.contains (type))
                r->setClipImage.get (type, c, image);
        }
    }
}

void ViewManager::setAppearance (juce::Component* view, juce::StringRef lightOrDark)
{
    if (auto r { Registry::getInstance() })
    {
        const auto& components { MarkdownDocument::getOrCreate (files::componentLayout) };

        for (auto* row : components.getTableRows (Id::component))
        {
            auto* c { view->getChildComponent (Format::getNumber (components.getTableValueView (*row, Id::z))) };

            const auto type { getComponentType (components, *row) };
            const auto styleName { getComponentStyle (components, *row) };
            const auto imageName { getComponentImage (components, *row) };
            const auto darkImageName { getComponentDark (components, *row) };

            applyAppearance (r, c, type, styleName, imageName, darkImageName, lightOrDark);

            int ordinal { 0 };

            if (auto* list { components.getTableList (*row, Id::type) })
                for (auto* item : *list)
                {
                    auto* child { c->getChildComponent (ordinal) };

                    applyAppearance (r, child, item->id.toString(),
                                     getComponentStyle (components, *row, item->id),
                                     getComponentImage (components, *row, item->id),
                                     getComponentDark (components, *row, item->id),
                                     lightOrDark);

                    ++ordinal;
                }
        }
    }
}

void ViewManager::setBypassed (juce::Component* view, AudioModel& model)
{
    const bool enabled { not static_cast<bool> (model.getBypassStateValue().getValue()) };
    const auto bypassParameter { Format::toScreamingSnakeCase (Id::bypass.toString()) };

    jam::Component::applyFunctionRecursively (view,
        [enabled, &bypassParameter] (juce::Component* c)
        {
            const auto parameter { c->getProperties()[Id::parameter].toString() };

            if (parameter.isNotEmpty())
            {
                const bool isBypassToggle { parameter == bypassParameter };
                c->setEnabled (isBypassToggle or enabled);
            }
        });
}

void ViewManager::applyConfig (juce::Component* view)
{
    if (auto r { Registry::getInstance() })
    {
        jam::Component::applyFunctionRecursively (view,
            [&r] (juce::Component* child)
            {
                const auto type { child->getProperties()[Id::type].toString() };

                if (type.isNotEmpty())
                {
                    const auto parameter { child->getProperties()[Id::parameter].toString() };
                    applyConfig (r, child, type, parameter);
                }
            });
    }
}

void ViewManager::applyConfig (Registry* r, juce::Component* c, const juce::String& type, const juce::String& parameter)
{
    if (r->configure.contains (type))
    {
        auto& typeConfig { r->configure.at (type) };

        if (typeConfig.contains (Id::all))
            typeConfig.get (Id::all, static_cast<juce::Component*> (c));

        if (typeConfig.contains (parameter))
            typeConfig.get (parameter, static_cast<juce::Component*> (c));
    }
}

/**_____________________________END_OF_NAMESPACE______________________________*/
} /** namespace jam */
