/**
 * @file jam_CollapsibleSectionBase.h
 * @brief Non-templated toggle contract and templated content-hosting section for CollapsibleList.
 */

namespace jam
{
/*____________________________________________________________________________*/

/**
 * @class CollapsibleSectionBase
 * @brief Non-templated interface implemented by every section hosted in a `CollapsibleList`.
 */
class CollapsibleSectionBase : public juce::Component
{
public:
    /**
     * @brief Constructs and assigns the section's component ID.
     * @param newID  Component ID, also used by `CollapsibleList::getSectionByID()`.
     */
    explicit CollapsibleSectionBase (const juce::String& newID) { setComponentID (newID); }

    virtual ~CollapsibleSectionBase() = default;

    /** @return Total height, in pixels, in the section's current open/closed state. */
    virtual int getTotalHeight() const = 0;

    /**
     * @brief Lays out the header and (when open) content within the given bounds.
     * @param bounds  Bounds to lay the section out within.
     */
    virtual void updateLayout (const juce::Rectangle<int>& bounds) = 0;

    /** Toggles the section between open and collapsed. */
    virtual void toggleOpen() = 0;

    /** @return `true` when the section is open. */
    virtual bool isOpen() const noexcept = 0;

    /** Called after the section's open state changes. */
    std::function<void()> onToggle;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CollapsibleSectionBase)
};

/*____________________________________________________________________________*/

class CollapsibleList;

/**
 * @class CollapsibleSection
 * @brief Hosts a `ComponentType` content component beneath a clickable header,
 * persisting its open/collapsed state to a `jam::Parameter<int>` (`Id::open`)
 * under @p sectionNode.
 *
 * @tparam ComponentType  Content component type, constructed with the trailing
 * constructor arguments forwarded from `CollapsibleSection`'s own constructor.
 */
template <typename ComponentType>
class CollapsibleSection
    : public CollapsibleSectionBase
    , public juce::ComponentListener
{
public:
    /**
     * @brief Constructs the section, its header, and its content component.
     * @param model         Model that owns the persisted open-state parameter.
     * @param sectionNode   ValueTree node the open-state parameter is created under.
     * @param title         Header title and content component ID.
     * @param newRowHeight  Collapsed (header-only) height, in pixels.
     * @param newNumRows    Number of content rows; expanded height is `(1 + newNumRows) * newRowHeight`.
     * @param args          Arguments forwarded to `ComponentType`'s constructor.
     */
    template <typename... Args>
    explicit CollapsibleSection (jam::Model& model,
                                 juce::ValueTree sectionNode,
                                 const juce::String& title,
                                 int newRowHeight = 30,
                                 int newNumRows = 1,
                                 Args&&... args)
        : CollapsibleSectionBase (title)
        , rowHeight (newRowHeight)
        , numRows (newNumRows)
        , openParameter (static_cast<jam::Parameter<int>&> (
              model.createAndAddParameter<jam::Parameter<int>> (sectionNode, Id::open, 1)))
        , openAttachment (openParameter,
                          [this] (const juce::var& newValue)
                          {
                              applyOpenState (jam::toBool (static_cast<int> (newValue)));
                          })
    {
        headerComponent = std::make_unique<HeaderComponent> (title,
                                                             [this]
                                                             {
                                                                 toggleOpen();
                                                             });
        addAndMakeVisible (headerComponent.get());

        contentComponent = std::make_unique<ComponentType> (std::forward<Args> (args)...);
        contentComponent->setComponentID (title);
        contentComponent->addComponentListener (this);
        addAndMakeVisible (contentComponent.get());

        openAttachment.sendInitialUpdate();
    }

    ~CollapsibleSection() override = default;

    void resized() override { updateLayout (getLocalBounds()); }

    /** Propagates content resizes to this section's height and its owning `CollapsibleList`'s layout. */
    void componentMovedOrResized (juce::Component& c, bool, bool wasResized) override
    {
        if (&c == contentComponent.get() and wasResized)
        {
            setSize (getWidth(), getTotalHeight());

            if (auto* list { findParentComponentOfClass<CollapsibleList>() })
                list->updateLayout();
        }
    }

    //==============================================================================
    void toggleOpen() override { openParameter.setValue (isOpen() ? 0 : 1); }

    bool isOpen() const noexcept override { return jam::toBool (openParameter.getValue()); }

    int getTotalHeight() const override
    {
        return isOpen() ? getExpandedHeight() : getCollapsedHeight();
    }

    /**
     * @brief Sets the number of content rows, recomputing the expanded height.
     * @param newNumRows  New row count.
     */
    void setNumRows (int newNumRows)
    {
        numRows = newNumRows;
        resized();
    }

    void updateLayout (const juce::Rectangle<int>& bounds) override
    {
        auto area { bounds };

        headerComponent->setBounds (area.removeFromTop (getCollapsedHeight()));

        auto expandedBounds { area.withHeight (getExpandedHeight() - getCollapsedHeight()) };
        expandedBounds.setWidth (area.getWidth());

        contentComponent->setBounds (expandedBounds);
    }

    /** @return The owned content component. */
    ComponentType* getContent() noexcept { return contentComponent.get(); }

    /** @return The owned content component. */
    const ComponentType* getContent() const noexcept { return contentComponent.get(); }

protected:
    /** @return Header-only height, in pixels. */
    int getCollapsedHeight() const { return rowHeight; }
    /** @return Header-plus-content height, in pixels: `(1 + numRows) * rowHeight`. */
    int getExpandedHeight() const { return (1 + numRows) * rowHeight; }

    //==============================================================================
    /** @brief Clickable header row showing the section title and an open/closed arrow. */
    class HeaderComponent : public juce::Component
    {
    public:
        /**
         * @brief Constructs the header.
         * @param title     Header label text.
         * @param callback  Invoked on mouseDown, to toggle the owning section.
         */
        HeaderComponent (const juce::String& title, std::function<void()> callback)
            : onClick (std::move (callback))
        {
            label.setText (title, juce::dontSendNotification);
            label.setJustificationType (juce::Justification::centred);
            label.setInterceptsMouseClicks (false, false);
            addAndMakeVisible (label);
        }

        /** @brief Positions the label, reserving room for the arrow when closed. */
        void resized() override
        {
            auto area { getLocalBounds() };
            area.removeFromLeft (opened ? 0 : area.getHeight());
            label.setBounds (area);
        }

        /** @brief Paints the header background and open/closed arrow. */
        void paint (juce::Graphics& g) override
        {
            auto area { getLocalBounds().reduced (2) };
            g.setColour (findColour (juce::ResizableWindow::backgroundColourId).brighter (0.1f));
            g.fillRect (area);

            auto arrow { getToggleArrow (opened) };
            g.setColour (findColour (juce::TextButton::textColourOffId));
            g.fillPath (arrow);
        }

        /** @brief Invokes the toggle callback. */
        void mouseDown (const juce::MouseEvent&) override
        {
            if (onClick)
                onClick();
        }

        /** @brief Reflects the section's open state onto label justification and the arrow. */
        void setOpened (bool shouldBeOpened)
        {
            opened = shouldBeOpened;
            label.setJustificationType (opened ? juce::Justification::centred
                                               : juce::Justification::centredLeft);
            resized();
            repaint();
        }

    private:
        juce::Label label;              ///< Header title label.
        std::function<void()> onClick;  ///< Toggle callback invoked on mouseDown.
        bool opened { true };           ///< Current open/closed state.

        /** @return An arrow path (pointing down when open, right when closed), scaled to fit. */
        juce::Path getToggleArrow (bool isOpened) const
        {
            static constexpr const char* const right { "M20 12L32 24L20 36V12Z" };
            static constexpr const char* const down { "M36 19L24 31L12 19H36Z" };

            auto area { getLocalBounds().reduced (4) };
            auto arrowArea { area.removeFromLeft (area.getHeight()).reduced (4).toFloat() };

            juce::Path arrow { juce::Drawable::parseSVGPath (isOpened ? down : right) };
            arrow.applyTransform (arrow.getTransformToScaleToFit (arrowArea, true));
            return arrow;
        }

        //==============================================================================
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HeaderComponent)
    };

private:
    /** @brief Reflects a persisted open-state change onto the header and fires onToggle(). */
    void applyOpenState (bool shouldBeOpened)
    {
        headerComponent->setOpened (shouldBeOpened);

        if (onToggle)
            onToggle();

        repaint();
    }

    int rowHeight; ///< Collapsed (header-only) height, in pixels.
    int numRows;   ///< Number of content rows; drives getExpandedHeight().
    jam::Parameter<int>& openParameter;             ///< Persisted open-state parameter (Id::open).
    jam::Model::ParameterAttachment openAttachment; ///< Applies parameter changes via applyOpenState().

    std::unique_ptr<HeaderComponent> headerComponent; ///< Owned clickable header row.
    std::unique_ptr<ComponentType> contentComponent;  ///< Owned content component.

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CollapsibleSection)
};

/**_____________________________END OF NAMESPACE______________________________*/
}// namespace jam
