/**
 * @file jam_DynamicRow.h
 * @brief ValueTree-mirrored list of removable rows, with add/close SVG buttons.
 */

namespace jam
{
/*____________________________________________________________________________*/

/** @return A newly constructed add button (`AddOneButton`). */
std::unique_ptr<juce::Button> createAddButton();

//==============================================================================
/** @brief SVG-backed close/remove button for a single row. */
class CloseOneButton final : public juce::Button
{
public:
    /** @brief Default constructor. Button name is empty. */
    CloseOneButton()
        : juce::Button ({})
    {
    }

    /** Draws the close glyph, alternating fill style when highlighted. */
    void paintButton (juce::Graphics& g,
                      bool shouldDrawButtonAsHighlighted,
                      bool shouldDrawButtonAsDown) override
    {
        const auto pathStyle { shouldDrawButtonAsHighlighted ? jam::Svg::PathStyle::alternate
                                                             : jam::Svg::PathStyle::stroke };
        jam::ButtonSVG::paint (g,
                               shouldDrawButtonAsHighlighted,
                               shouldDrawButtonAsDown,
                               *this,
                               closeOneRawImages,
                               closeOneImageCount,
                               jam::ButtonSVG::ColourMode::toggle,
                               pathStyle);
    }

private:
    static constexpr size_t closeOneImageCount { 4 };

    const juce::String closeOneImages[closeOneImageCount] {
        BinaryData::getString (files::closeNormal),
        BinaryData::getString (files::closeOver),
        BinaryData::getString (files::closeDown),
        BinaryData::getString (files::closeDisabled),
    };
    const char* const closeOneRawImages[closeOneImageCount] {
        closeOneImages[0].toRawUTF8(), closeOneImages[1].toRawUTF8(),
        closeOneImages[2].toRawUTF8(), closeOneImages[3].toRawUTF8(),
    };

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CloseOneButton)
};

/** @brief SVG-backed add button for appending a new row. */
class AddOneButton final : public juce::Button
{
public:
    /** @brief Default constructor. Button name is empty. */
    AddOneButton()
        : juce::Button ({})
    {
    }

    /** Draws the add glyph, alternating fill style when highlighted. */
    void paintButton (juce::Graphics& g,
                      bool shouldDrawButtonAsHighlighted,
                      bool shouldDrawButtonAsDown) override
    {
        const auto pathStyle { shouldDrawButtonAsHighlighted ? jam::Svg::PathStyle::alternate
                                                             : jam::Svg::PathStyle::stroke };
        jam::ButtonSVG::paint (g,
                               shouldDrawButtonAsHighlighted,
                               shouldDrawButtonAsDown,
                               *this,
                               addOneRawImages,
                               addOneImageCount,
                               jam::ButtonSVG::ColourMode::toggle,
                               pathStyle);
    }

private:
    static constexpr size_t addOneImageCount { 4 };

    const juce::String addOneImages[addOneImageCount] {
        BinaryData::getString (files::addNormal),
        BinaryData::getString (files::addOver),
        BinaryData::getString (files::addDown),
        BinaryData::getString (files::addDisabled),
    };
    const char* const addOneRawImages[addOneImageCount] {
        addOneImages[0].toRawUTF8(), addOneImages[1].toRawUTF8(),
        addOneImages[2].toRawUTF8(), addOneImages[3].toRawUTF8(),
    };
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AddOneButton)
};

//==============================================================================
/**
 * @class DynamicRow
 * @brief Mirrors a `juce::ValueTree` subtree as a vertical stack of `RowType`
 * rows, adding/removing rows as children are added to/removed from the subtree.
 *
 * @tparam RowType  Row component type; constructed via the supplied RowFactory.
 */
template <typename RowType>
class DynamicRow
    : public juce::Component
    , public juce::ValueTree::Listener
{
public:
    /** @brief Factory constructing a `RowType` row for a given ValueTree child. */
    using RowFactory = std::function<std::unique_ptr<RowType> (juce::ValueTree child)>;

    /**
     * @brief Constructs, creating one row per existing child of @p subtreeToMirror.
     * @param newID             Component ID.
     * @param subtreeToMirror   ValueTree subtree whose children are mirrored as rows.
     * @param factory           Factory constructing a row for a given child.
     * @param addButton         Add-row trigger button; ownership transfers.
     */
    DynamicRow (const juce::String& newID,
                juce::ValueTree subtreeToMirror,
                RowFactory factory,
                std::unique_ptr<juce::Button> addButton = createAddButton())
        : factory (std::move (factory))
        , addButton (std::move (addButton))
        , subtree (subtreeToMirror)
    {
        setComponentID (newID);

        this->addButton->onClick = [this]
        {
            if (onAddButtonClicked)
                onAddButtonClicked();
        };

        addAndMakeVisible (this->addButton.get());
        addAndMakeVisible (separator);

        for (auto child : subtree)
        {
            rows.push_back (createRow (child));
            mirroredChildren.add (child);
        }

        subtree.addListener (this);

        resized();
    }

    ~DynamicRow() override { subtree.removeListener (this); }

    /** Lays rows out top to bottom, followed by the separator and add button. */
    void resized() override
    {
        auto area { getLocalBounds() };

        addButton->setBounds (area.removeFromBottom (rowHeight));
        separator.setBounds (area.removeFromBottom (rowHeight));

        for (auto& r : rows)
            r->setBounds (area.removeFromTop (rowHeight));
    }

    /**
     * @brief Sets the inset applied to every row, and relays out.
     * @param newInset  New inset, in pixels.
     */
    void setInset (int newInset)
    {
        inset = newInset;

        for (auto& r : rows)
            r->setInset (inset);

        resized();
    }

    /**
     * @brief Sets the row height applied to every row, and relays out.
     * @param newRowHeight  New row height, in pixels.
     */
    void setRowHeight (int newRowHeight)
    {
        rowHeight = newRowHeight;

        for (auto& r : rows)
            r->setRowHeight (rowHeight);

        resized();
    }

    /** @return Number of mirrored rows. */
    int getNumRows() const noexcept { return static_cast<int> (rows.size()); }

    /** @return Number of rows plus the separator and add-button slots. */
    int getNumRowsForSection() const noexcept { return static_cast<int> (rows.size()) + 2; }

    /** @return Total preferred height, in pixels, for all rows plus separator and add button. */
    int getPreferredHeight() const noexcept { return getNumRowsForSection() * rowHeight; }

    /** Cycles keyboard focus between child `juce::TextEditor`s on Tab/Shift+Tab. */
    bool keyPressed (const juce::KeyPress& key) override
    {
        if (key == juce::KeyPress::tabKey
            or key == juce::KeyPress (juce::KeyPress::tabKey, juce::ModifierKeys::shiftModifier, 0))
        {
            juce::Array<juce::TextEditor*> editors;

            jam::Component::applyFunctionRecursively (
                this,
                [&] (juce::Component* c)
                {
                    if (auto* e { dynamic_cast<juce::TextEditor*> (c) })
                        editors.add (e);
                });

            if (editors.isEmpty())
                return false;

            auto* focused { juce::Component::getCurrentlyFocusedComponent() };
            int idx { editors.indexOf (dynamic_cast<juce::TextEditor*> (focused)) };

            if (idx < 0)
                idx = 0;
            else
                idx =
                    (key.getModifiers().isShiftDown() ? (idx - 1 + editors.size()) % editors.size()
                                                      : (idx + 1) % editors.size());

            editors[idx]->grabKeyboardFocus();
            return true;
        }

        return juce::Component::keyPressed (key);
    }

    //==============================================================================
    /** Adds a row mirroring the newly added child, then resizes and relays out. */
    void valueTreeChildAdded (juce::ValueTree&, juce::ValueTree& childWhichHasBeenAdded) override
    {
        rows.push_back (createRow (childWhichHasBeenAdded));
        mirroredChildren.add (childWhichHasBeenAdded);

        setSize (getWidth(), getPreferredHeight());
        resized();
    }

    /** Removes the row mirroring the removed child, then resizes and relays out. */
    void valueTreeChildRemoved (juce::ValueTree&,
                                juce::ValueTree& childWhichHasBeenRemoved,
                                int) override
    {
        const int index { mirroredChildren.indexOf (childWhichHasBeenRemoved) };
        jassert (index >= 0);

        mirroredChildren.remove (index);
        rows.erase (rows.begin() + index);

        setSize (getWidth(), getPreferredHeight());
        resized();
    }

    //==============================================================================
    /** Called with the row's mirrored ValueTree child when its remove button is clicked. */
    std::function<void (juce::ValueTree)> onRemoveButtonClicked;

    /** Called when the add-row button is clicked. */
    std::function<void()> onAddButtonClicked;

private:
    /** @brief Constructs a row for @p child via factory, wiring its remove-button callback and LAF. */
    std::unique_ptr<RowType> createRow (juce::ValueTree child)
    {
        auto row { factory (child) };
        row->setLookAndFeel (&getLookAndFeel());

        row->onRemoveButtonClicked = [this, child]
        {
            if (onRemoveButtonClicked)
                onRemoveButtonClicked (child);
        };

        addAndMakeVisible (row.get());
        return row;
    }

    RowFactory factory;                             ///< Constructs a RowType row for a given ValueTree child.
    std::unique_ptr<juce::Button> addButton;         ///< Owned add-row trigger button.
    juce::ValueTree subtree;                         ///< Mirrored ValueTree subtree.
    Owner<RowType> rows;                             ///< Owned rows, in mirrored-child order.
    juce::Array<juce::ValueTree> mirroredChildren;   ///< Children currently mirrored, parallel to rows.

    /** @brief Horizontal divider line drawn between the rows and the add button. */
    class ComponentSeparator : public juce::Component
    {
    public:
        /** @brief Constructs, setting the inset applied when painting the line. */
        ComponentSeparator (int newInset = 4)
            : inset (newInset)
        {
        }

        /** @brief Default destructor. */
        ~ComponentSeparator() = default;

        /** @brief Draws a horizontal line across the inset area. */
        void paint (juce::Graphics& g) override
        {
            auto area { getLocalBounds().reduced (2 * inset) };
            auto left { juce::Point<float> (area.getX(), area.getCentreY()) };
            auto right { juce::Point<float> (getX() + getWidth(), area.getCentreY()) };
            g.setColour (findColour (juce::Label::textColourId).withAlpha (0.2f));
            g.drawLine (juce::Line<float> (left, right));
        }

        /** @brief Repaints on resize (the line spans the new bounds). */
        void resized() override { repaint(); }

    private:
        const int inset; ///< Inset applied to the drawn line's bounds.
        //==============================================================================
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ComponentSeparator)
    };
    ComponentSeparator separator; ///< Divider drawn between the rows and the add button.

    int inset { 4 };      ///< Inset applied to every row.
    int rowHeight { 32 }; ///< Height applied to every row, the separator, and the add button.

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DynamicRow)
};

/*____________________________________________________________________________*/

/**
 * @class RemovableRow
 * @brief Base for a single `DynamicRow` row — hosts a remove button and delegates
 * the remaining content area layout to layoutContentArea().
 */
class RemovableRow : public juce::Component
{
public:
    /**
     * @brief Constructs and assigns the row's component ID.
     * @param newID  Component ID.
     */
    RemovableRow (juce::StringRef newID);
    ~RemovableRow() = default;

    /** Positions the remove button, then delegates the remaining area to layoutContentArea(). */
    void resized() override;

    /**
     * @brief Sets the inset applied to the remove button and content area.
     * @param newInset  New inset, in pixels.
     */
    void setInset (int newInset);

    /**
     * @brief Sets the row's height.
     * @param newRowHeight  New row height, in pixels.
     */
    void setRowHeight (int newRowHeight);

    /** Called when this row's remove button is clicked. */
    std::function<void()> onRemoveButtonClicked;

protected:
    int inset { 4 };                              ///< Inset applied to the remove button and content area.
    int rowHeight { 32 };                          ///< This row's height.
    std::unique_ptr<juce::Button> removeButton;    ///< Owned remove-row trigger button.

    /**
     * @brief Lays out the row's own content within the area remaining after the remove button.
     * @param bounds  Bounds remaining after the remove button has been positioned.
     * @return Unused return area (implementation-defined by subclasses).
     */
    virtual juce::Rectangle<int> layoutContentArea (juce::Rectangle<int> bounds) = 0;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RemovableRow)
};

/**_____________________________END OF NAMESPACE______________________________*/
}// namespace jam
