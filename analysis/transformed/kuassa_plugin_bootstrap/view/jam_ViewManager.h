/**
 * @file jam_ViewManager.h
 * @brief Builds and binds the editor and HTML view component trees from
 *        component.md and the parsed layout documents.
 */

#pragma once

namespace jam
{
/*____________________________________________________________________________*/

/**
 * @class ViewManager
 * @brief Static builder/binder for the plugin's view component trees.
 *
 * component.md's `component` table is the single source of truth for the
 * editor lane: row order is creation order, and creation order is the
 * resulting juce::Component child index (`z`). Each created component's
 * componentID is the SVG rect id verbatim; a created component carries only
 * `Id::type` and `Id::parameter` as component properties. A row's `primary`
 * cell holds another row's `z`, so re-entrant operations (bindComponent,
 * setAppearance, applyConfig) address components by that number, never by
 * searching the tree.
 *
 * The HTML lane (Panel + Settings) reads PanelLayout.html / SettingsLayout.html
 * and copies each element's flex/layout properties onto the created component
 * through applyLayoutProperties(), so applyFlexLayout() can lay the tree out
 * later without re-reading the document.
 */
struct ViewManager final
{
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    /** Processor getter callbacks, keyed first by component type, then by parameter ID. */
    using Getters = jam::HashMap<juce::Identifier, Function::Map<juce::Identifier, void>>;

    /** Callback arrays contributed by bound components, keyed by event name (the row's `event` cell). */
    using Events = jam::HashMap<juce::Identifier, Function::Array<void>>;

    //==============================================================================
    // Editor
    //==============================================================================
    /**
     * @brief Builds the full editor component tree, then binds every row's
     *        runtime behaviour and applies the current bypass state.
     * @param view              Root component the editor tree is built into.
     * @param model             Audio model providing appearance and bypass state.
     * @param children          Owning store the created components are moved into.
     * @param lafs              Owning store for LookAndFeel instances applied to components.
     * @param attachments       Owning store for APVTS attachments created during binding.
     * @param processorGetters  Processor getter callbacks, populated per bound component.
     * @param chainEvents       Event callback arrays, populated per bound component.
     */
    static void build (juce::Component* view,
                       AudioModel& model,
                       Owner<juce::Component>& children,
                       Owner<juce::LookAndFeel>& lafs,
                       AnyOwner& attachments,
                       Getters& processorGetters,
                       Events& chainEvents);

    /**
     * @brief Re-applies style, image, and font appearance to every component
     *        in the tree without rebuilding it.
     * @param view          Root of the previously-built component tree.
     * @param lightOrDark   Appearance key to apply (light or dark).
     */
    static void setAppearance (juce::Component* view, juce::StringRef lightOrDark);

    /**
     * @brief Enables or disables every parameter-bound component in the tree
     *        to reflect the model's bypass state. The bypass toggle itself
     *        always stays enabled.
     * @param view   Root of the previously-built component tree.
     * @param model  Audio model providing the bypass state.
     */
    static void setBypassed (juce::Component* view, AudioModel& model);

    //==============================================================================
    // HTML (Panel + Settings)
    //==============================================================================
    /**
     * @brief Builds a component tree from a parsed HTML body element (Panel
     *        or Settings layout), styling and binding each created component
     *        through the registry.
     * @param parent    Root component the HTML tree is built into.
     * @param model     Audio model passed to registry bind callbacks.
     * @param children  Owning store the created components are moved into.
     * @param body      Parsed `\<body\>` element of the HTML layout document.
     */
    static void makeFromHTML (juce::Component* parent, AudioModel& model, Owner<juce::Component>& children, const Document::Element& body);

    /**
     * @brief Lays a component's direct children out as a single flexbox row
     *        or column, then recurses into any flex-typed descendant that
     *        carries an `id`.
     * @param view     Component whose children are laid out.
     * @param emUnit   Pixel size of one em, used to resolve em-relative flex-basis and margin-top.
     */
    static void applyFlexLayout (juce::Component* view, float emUnit = 0.0f);

    //==============================================================================
    // Content
    //==============================================================================
    /**
     * @brief Builds a content view (background graphics, images, HTML tree,
     *        config, layout, and data-bound text) from a parsed layout document.
     * @param view      Root component the content is built into.
     * @param model     Audio model passed through to bound components.
     * @param children  Owning store the created components are moved into.
     * @param layout    Parsed layout document (PanelLayout.html / SettingsLayout.html / AboutLayout.html).
     */
    static void
    buildContent (juce::Component* view, AudioModel& model, Owner<juce::Component>& children, const Document& layout);

    /**
     * @brief Binds every already-built settings component under `view` to
     *        its persisted node in the settings state tree.
     * @param state  Settings model owning the persisted value tree.
     * @param view   Root of the previously-built settings component tree.
     */
    static void attachSettings (jam::SettingsModel& state, juce::Component* view);

    //==============================================================================
    // Shared
    //==============================================================================
    /**
     * @brief Reads the UI size stored in a markdown view-layout document's
     *        `UISize` table.
     * @param viewLayout  Parsed markdown view-layout document.
     * @return Width and height, in pixels, read from the `UISize` table.
     */
    static Size<int> getUISize (const MarkdownDocument& viewLayout)
    {
        auto* uiSizeTable { viewLayout.root->getChildByID (Id::UISize) };
        jassert (uiSizeTable != nullptr);

        return { uiSizeTable->getChildByID (Id::width)->getChildByID (Id::value)->getAllSubText().getIntValue(),
                 uiSizeTable->getChildByID (Id::height)->getChildByID (Id::value)->getAllSubText().getIntValue() };
    }

    /**
     * @brief Reads the UI size from an SVG view-layout document's root
     *        `width`/`height` attributes.
     * @param viewLayout  Parsed SVG view-layout document.
     * @return Width and height, in pixels, read from the SVG root element.
     */
    static Size<int> getUISize (const XmlDocument& viewLayout)
    {
        return { Svg::getSVGWidth (*viewLayout.root), Svg::getSVGHeight (*viewLayout.root) };
    }

    /**
     * @brief Loads or creates a view-layout document by identifier, then
     *        reads its UI size.
     * @tparam DocumentType  Document type to load (MarkdownDocument or XmlDocument).
     * @param viewLayout  Identifier of the view-layout document to load.
     * @return Width and height, in pixels, of the loaded document.
     */
    template <typename DocumentType>
    static Size<int> getUISize (const juce::Identifier& viewLayout)
    {
        return getUISize (DocumentType::getOrCreate (viewLayout));
    }

    /**
     * @brief Applies flexbox layout to a body-tagged view, either laying the
     *        view itself out as a flex container or recursing to find one.
     * @param view     Component to lay out.
     * @param emUnit   Pixel size of one em, used to resolve em-relative flex-basis and margin-top.
     */
    static void applyLayout (juce::Component* view, float emUnit = 0.0f);

    //==============================================================================
private:
    /**
     * @brief Creates every top-level component described by component.md's
     *        rows, in row order.
     * @param view      Root component the created components are added to.
     * @param model     Audio model providing the current appearance.
     * @param children  Owning store the created components are moved into.
     * @param lafs      Owning store for LookAndFeel instances applied to components.
     */
    static void make (juce::Component* view,
                      AudioModel& model,
                      Owner<juce::Component>& children,
                      Owner<juce::LookAndFeel>& lafs);

    /**
     * @brief Creates the component described by one component.md row: resolves
     *        it from the registry, stamps its identity properties, applies
     *        style and appearance, creates its declared nested children, and
     *        sets its bounds from the row's SVG rect. Both the row's light-
     *        and dark-appearance images are cached GPU-resident at creation,
     *        ahead of any later appearance toggle.
     * @param view          Parent component the created component is added to.
     * @param r             Registry supplying component factories and bind callbacks.
     * @param components    Parsed component.md document.
     * @param row           component.md row describing the component to create.
     * @param children      Owning store the created component is moved into.
     * @param lafs          Owning store for LookAndFeel instances applied to the component.
     * @param lightOrDark   Appearance key used to resolve the component's initial image.
     */
    static void makeComponent (juce::Component* view,
                               Registry* r,
                               const MarkdownDocument& components,
                               Document::Element& row,
                               Owner<juce::Component>& children,
                               Owner<juce::LookAndFeel>& lafs,
                               juce::StringRef lightOrDark);

    /**
     * @brief Creates the nested children declared in a row's `type` list,
     *        in list order, beneath the row's parent component. Each child's
     *        light- and dark-appearance images are cached GPU-resident at
     *        creation, ahead of any later appearance toggle.
     * @param parentComponent      Component the created children are added to.
     * @param r                    Registry supplying component factories and bind callbacks.
     * @param components           Parsed component.md document.
     * @param row                  component.md row declaring the nested children.
     * @param children             Owning store the created children are moved into.
     * @param lafs                 Owning store for LookAndFeel instances applied to the children.
     * @param parentParameterID    Parent component's parameter ID, used when a child declares no parameter of its own.
     * @param lightOrDark          Appearance key used to resolve each child's initial image.
     */
    static void makeChildren (juce::Component* parentComponent,
                              Registry* r,
                              const MarkdownDocument& components,
                              Document::Element& row,
                              Owner<juce::Component>& children,
                              Owner<juce::LookAndFeel>& lafs,
                              const juce::String& parentParameterID,
                              juce::StringRef lightOrDark);

    /**
     * @brief Resolves a row's component by its `z` index and, when present,
     *        its `primary` reference, then binds the component and its
     *        nested children in list order.
     * @param view          Root component the row's component is a child of.
     * @param r             Registry supplying bind callbacks.
     * @param components    Parsed component.md document.
     * @param row           component.md row describing the component to bind.
     * @param model         Audio model passed to model-binding callbacks.
     * @param attachments   Owning store for APVTS attachments created during binding.
     * @param getters       Processor getter callbacks, populated for this component.
     * @param chainEvents   Event callback arrays, populated for this component.
     */
    static void bindComponent (juce::Component* view,
                               Registry* r,
                               const MarkdownDocument& components,
                               Document::Element& row,
                               AudioModel& model,
                               AnyOwner& attachments,
                               Getters& getters,
                               Events& chainEvents);

    /**
     * @brief Runs the registry's attachment, processor-getter, model-binding,
     *        and event callbacks registered for one component's type.
     * @param r             Registry supplying bind callbacks.
     * @param c             Component being bound.
     * @param type          Component's type.
     * @param dParam        Component's parameter ID.
     * @param dEvent        Component's event name, or empty when the row declares none.
     * @param model         Audio model passed to model-binding callbacks.
     * @param attachments   Owning store for APVTS attachments created during binding.
     * @param getters       Processor getter callbacks, populated for this component.
     * @param chainEvents   Event callback arrays, populated for this component.
     */
    static void bindComponent (Registry* r,
                               juce::Component* c,
                               const juce::String& type,
                               const juce::String& dParam,
                               const juce::String& dEvent,
                               AudioModel& model,
                               AnyOwner& attachments,
                               Getters& getters,
                               Events& chainEvents);

    /**
     * @brief Applies the resolved style and light/dark image to a component
     *        through the registry's font, image, style-image, and clip-image
     *        callbacks registered for its type or style.
     * @param r             Registry supplying appearance callbacks.
     * @param c             Component to apply appearance to.
     * @param type          Component's type.
     * @param styleName     Component's style name, or empty when none is declared.
     * @param imageName     Component's light-appearance image name, or empty when none is declared.
     * @param darkImageName Component's dark-appearance image name, or empty when none is declared.
     * @param lightOrDark   Appearance key selecting between `imageName` and `darkImageName`.
     */
    static void applyAppearance (Registry* r,
                                 juce::Component* c,
                                 const juce::String& type,
                                 const juce::String& styleName,
                                 const juce::String& imageName,
                                 const juce::String& darkImageName,
                                 juce::StringRef lightOrDark);

    /**
     * @brief Runs the registry's configuration callbacks registered for one
     *        component's type -- first the type-wide callback, then the
     *        callback registered for its specific parameter.
     * @param r          Registry supplying configuration callbacks.
     * @param c          Component to configure.
     * @param type       Component's type.
     * @param parameter  Component's parameter ID.
     */
    static void applyConfig (Registry* r, juce::Component* c, const juce::String& type, const juce::String& parameter);

    /**
     * @brief Runs applyConfig() for every typed component in the tree.
     * @param view  Root of the component tree to configure.
     */
    static void applyConfig (juce::Component* view);

    /**
     * @brief Replaces the text of every data-bound label under `view` with
     *        its resolved dynamic value, clamping labels sized `max-content`.
     * @param view   Root of the component tree to update.
     * @param model  Audio model supplying the dynamic values.
     * @param width  Content width, used to clamp `max-content` labels.
     */
    static void setContentText (juce::Component* view, AudioModel& model, int width);

    //==============================================================================
    // Row readers -- component.md `component` table
    //==============================================================================
    /** @return The row's `type` cell. */
    static juce::String getComponentType (const MarkdownDocument& components, Document::Element& row);
    /** @return The row's `parameter` cell, verbatim. */
    static juce::String getComponentParameter (const MarkdownDocument& components, Document::Element& row);
    /** @return The nested child identified by `itemId`'s `parameter` cell, verbatim. */
    static juce::String getComponentParameter (const MarkdownDocument& components, Document::Element& row, const juce::Identifier& itemId);
    /** @return The row's `parameter` cell, normalised to a SCREAMING_SNAKE_CASE parameter ID. */
    static juce::String getComponentParameterID (const MarkdownDocument& components, Document::Element& row);
    /** @return The pixel bounds of the row's SVG rect element. */
    static juce::Rectangle<int> getComponentBounds (const Document::Element& rect);
    /** @return The row's `style` cell. */
    static juce::String getComponentStyle (const MarkdownDocument& components, Document::Element& row);
    /** @return The nested child identified by `itemId`'s `style` cell. */
    static juce::String getComponentStyle (const MarkdownDocument& components, Document::Element& row, const juce::Identifier& itemId);
    /** @return The row's `image` cell (light-appearance image name). */
    static juce::String getComponentImage (const MarkdownDocument& components, Document::Element& row);
    /** @return The nested child identified by `itemId`'s `image` cell (light-appearance image name). */
    static juce::String getComponentImage (const MarkdownDocument& components, Document::Element& row, const juce::Identifier& itemId);
    /** @return The row's `dark` cell (dark-appearance image name). */
    static juce::String getComponentDark (const MarkdownDocument& components, Document::Element& row);
    /** @return The nested child identified by `itemId`'s `dark` cell (dark-appearance image name). */
    static juce::String getComponentDark (const MarkdownDocument& components, Document::Element& row, const juce::Identifier& itemId);
    /** @return The row's `event` cell. */
    static juce::String getComponentEvent (const MarkdownDocument& components, Document::Element& row);

    /**
     * @brief Builds the drawable described by a body element's inline `svg` child, if present.
     * @param body  Parsed `\<body\>` element of a layout document.
     * @return The built drawable, or `nullptr` when the body declares none.
     */
    static std::unique_ptr<juce::Drawable> getGraphics (const Document::Element& body);

    /**
     * @brief Builds one component per `\<img\>` child of a body element, using
     *        an inline SVG or a raster image resource, at that child's
     *        declared bounds or the given fallback size.
     * @param view      Parent component the created image components are added to.
     * @param children  Owning store the created components are moved into.
     * @param body      Parsed `\<body\>` element of a layout document.
     * @param width     Fallback width for an `\<img\>` with no declared bounds.
     * @param height    Fallback height for an `\<img\>` with no declared bounds.
     */
    static void makeImages (juce::Component* view, Owner<juce::Component>& children, const Document::Element& body, int width, int height);

    /**
     * @brief Builds the table of data-bound text values available to labels
     *        with a `data-content` key (host description, format, version,
     *        product name, wrapper, trademark, copyright).
     * @param model  Audio model supplying host, format, and wrapper information.
     * @return The resolved value for each supported `data-content` key.
     */
    static jam::HashMap<juce::String, juce::String> buildDynamicValues (AudioModel& model);

    /**
     * @brief Sets a label's text, upper-casing it when the label's stored
     *        `textTransform` property requests it.
     * @param label         Label to set the text on.
     * @param contentValue  Resolved text value to set.
     */
    static void applyTextTransform (juce::Label* label, const juce::String& contentValue);

    /**
     * @brief Resizes a label whose CSS width is `max-content` to its
     *        measured text width, keeping its declared right edge and top.
     * @param label  Label to resize.
     * @param width  Content width the label's right edge is measured against.
     */
    static void applyMaxContentBounds (juce::Label* label, int width);

    /**
     * @brief Resolves a parsed element's font family and size through the
     *        style manager.
     * @param element  Parsed HTML element declaring `fontFamily` and/or `fontSize`.
     * @return The resolved font options.
     */
    static juce::FontOptions getFontOptions (const Document::Element& element);

    /**
     * @brief Applies text, transform, alignment, font, and colour from a
     *        parsed HTML element onto a label.
     * @param label    Label to style.
     * @param element  Parsed HTML element declaring the label's style.
     */
    static void applyLabelStyle (juce::Label* label, const Document::Element& element);

    /**
     * @brief Adds one FlexItem per child of `parent`, computed from each
     *        child's stored flex-grow, flex-shrink, flex-basis, and
     *        margin-top properties.
     * @param flexBox   FlexBox the items are added to.
     * @param parent    Component whose children become flex items.
     * @param emUnit    Pixel size of one em, used to resolve em-relative flex-basis and margin-top.
     */
    static void addFlexItems (juce::FlexBox& flexBox, juce::Component* parent, float emUnit);

    /**
     * @brief Adds the flex-gap spacing between consecutive items along the
     *        box's main axis.
     * @param flexBox  FlexBox whose items receive the gap margin.
     * @param gap      Gap size, in pixels.
     */
    static void applyFlexGap (juce::FlexBox& flexBox, float gap);

    /**
     * @brief Binds one settings component's Value to its persisted node in
     *        the settings state tree, creating the node when it does not
     *        exist yet, then runs the component's onAttachment callback.
     * @param child          Settings component to attach.
     * @param state          Settings model receiving the Value listener.
     * @param top            Settings state subtree the parameter node lives under.
     * @param parameterID    Component's parameter ID.
     * @param componentType  Component's type.
     * @param registry       Registry supplying the component's configuration entry.
     */
    static void attachParameter (juce::Component* child,
                                 jam::SettingsModel& state,
                                 juce::ValueTree& top,
                                 const juce::String& parameterID,
                                 const juce::String& componentType,
                                 Registry& registry);

    /**
     * @brief Copies a parsed element's flex/layout properties (flex-grow,
     *        flex-shrink, flex-basis and its unit, font-size, margin-top and
     *        its unit, display, flex-direction, padding, flex-gap,
     *        justify-content, position, left/top/right, height, width) onto
     *        a component's properties.
     * @param element     Parsed HTML element declaring the layout properties.
     * @param component   Component the properties are copied onto.
     */
    static void applyLayoutProperties (const Document::Element& element, juce::Component* component);
};

/**_____________________________END_OF_NAMESPACE______________________________*/
} /** namespace jam */
