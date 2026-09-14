/**
 * @file jam_Selector.h
 * @brief Custom-painted combo-box-alike backed by an id/text item list, with optional prev/next navigation.
 */

namespace jam
{
/*____________________________________________________________________________*/

/**
 * @class Selector
 * @brief Combo-box-like component maintaining its own id/text item list and a
 * bound `juce::Value` mirroring the selected item's text, with an optional
 * hierarchical popup menu and prev/next navigation buttons.
 */
class Selector
    : public juce::Component
    , public Model::ValueComponent<Selector>
    , private juce::Value::Listener
{
public:
    //==============================================================================

    /** @brief One selectable entry — an integer id paired with its display text. */
    struct Item
    {
        int id;           ///< Item id.
        juce::String text; ///< Display text.
    };

    /** @brief Constructs an empty selector, wiring selected-value attachment to setSelectedIdFromString(). */
    Selector()
        : Model::ValueComponent<Selector> (juce::String())
    {
        selected.addListener (this);

        onAttachment = [this]()
        {
            setSelectedIdFromString (selected.toString());
        };
    }

    /** @brief Default destructor. */
    virtual ~Selector() = default;

    //==============================================================================
    // Item management

    /**
     * @brief Adds a single item.
     * @param newItemText  Display text.
     * @param newItemId    Item id.
     */
    void addItem (const juce::String& newItemText, int newItemId)
    {
        items.push_back ({ newItemId, newItemText });
    }

    /**
     * @brief Adds a list of items, assigning sequential ids starting at an offset.
     * @param itemList          Display texts, in order.
     * @param firstItemIdOffset  Id assigned to the first item; subsequent items increment.
     */
    void addItemList (const juce::StringArray& itemList, int firstItemIdOffset)
    {
        int id { firstItemIdOffset };

        for (const auto& text : itemList)
            items.push_back ({ id++, text });
    }

    /**
     * @brief Adds items from a one-based id/label map.
     * @param map  Map from one-based item key to display label.
     */
    void addItemList (const jam::HashMap<int, juce::String>& map)
    {
        for (int key { 1 }; key <= (int) map.size(); ++key)
            addItem (map.at (key), key);
    }

    /**
     * @brief Adopts a pre-built hierarchical popup menu, deriving the flat item list from it.
     * @param menu  Popup menu (possibly with sub-menus) to adopt.
     */
    void addItems (juce::PopupMenu menu)
    {
        hierarchicalMenu = std::move (menu);
        rebuildItemsFromMenu();
    }

    /** Clears all items, the hierarchical menu, and the current selection. */
    void clear()
    {
        items.clear();
        hierarchicalMenu = juce::PopupMenu();
        selectedId = 0;
        displayText.clear();
        repaint();
    }

    //==============================================================================
    // Selection

    /**
     * @brief Sets the selected item by id.
     * @param newItemId     Id of the item to select.
     * @param notification  When not `dontSendNotification`, calls selectionChanged().
     */
    void setSelectedId (int newItemId, juce::NotificationType notification = juce::dontSendNotification)
    {
        selectedId = newItemId;
        displayText.clear();

        for (const auto& item : items)
        {
            if (item.id == newItemId)
            {
                displayText = item.text;
                break;
            }
        }

        if (displayText.isNotEmpty())
            selected = displayText;

        repaint();

        if (notification != juce::dontSendNotification)
            selectionChanged();
    }

    /**
     * @brief Sets the selected item by its zero-based position in the item list.
     * @param index         Zero-based item index.
     * @param notification  When not `dontSendNotification`, calls selectionChanged().
     */
    void setSelectedItemIndex (int index, juce::NotificationType notification = juce::dontSendNotification)
    {
        if (index >= 0 and index < (int) items.size())
        {
            setSelectedId (items.at ((size_t) index).id, notification);
        }
    }

    /**
     * @brief Sets the displayed text directly, without changing the selected id.
     * @param text  New display text.
     */
    void setText (const juce::String& text)
    {
        displayText = text;
        repaint();
    }

    /**
     * @brief Sets the placeholder text shown when no item is selected.
     * @param newText  Placeholder text.
     */
    void setTextWhenNothingSelected (const juce::String& newText)
    {
        placeholderText = newText;
        repaint();
    }

    /** @return Zero-based index of the currently selected item, or -1 when none is selected. */
    int getSelectedItemIndex() const noexcept
    {
        int result { -1 };

        for (int i { 0 }; i < (int) items.size(); ++i)
        {
            if (items.at ((size_t) i).id == selectedId)
            {
                result = i;
                break;
            }
        }

        return result;
    }

    /** @return Number of items. */
    int getNumItems() const noexcept { return (int) items.size(); }

    /** @return The currently displayed text. */
    juce::String getText() const noexcept { return displayText; }

    /**
     * @brief Finds an item's id by matching its display text, case-insensitively.
     * @param itemToFind  Display text to match.
     * @return Item id, or -1 when not found.
     */
    int getItemId (const juce::String& itemToFind) const noexcept
    {
        int result { -1 };

        for (const auto& item : items)
        {
            if (item.text.equalsIgnoreCase (itemToFind))
            {
                result = item.id;
                break;
            }
        }

        return result;
    }

    /**
     * @brief Returns an item's display text by zero-based index.
     * @param index  Zero-based item index.
     * @return Display text, or an empty string when out of range.
     */
    juce::String getItemText (int index) const noexcept
    {
        juce::String result;

        if (index >= 0 and index < (int) items.size())
            result = items.at ((size_t) index).text;

        return result;
    }

    //==============================================================================
    // Navigation

    /**
     * @brief Adds previous/next navigation buttons, wiring them to step the selection.
     * @param previousButton  Button that selects the previous item.
     * @param nextButton      Button that selects the next item.
     */
    void setNavigationButtons (std::unique_ptr<juce::Button> previousButton,
                               std::unique_ptr<juce::Button> nextButton)
    {
        buttons.add (std::move (previousButton));
        buttons.add (std::move (nextButton));

        for (auto& button : buttons)
            juce::Component::addAndMakeVisible (button.get());

        getButton (navigation::previous)->onClick = [this]()
        {
            refreshItems();

            if (int index { getSelectedItemIndex() }; index > 0)
            {
                setSelectedItemIndex (index - 1);
            }

            selectionChanged();
        };

        getButton (navigation::next)->onClick = [this]()
        {
            refreshItems();

            if (int index { getSelectedItemIndex() }; index < getNumItems() - 1)
            {
                setSelectedItemIndex (index + 1);
            }

            selectionChanged();
        };
    }

    /** @return The navigation button at the given navigation index. */
    juce::Button* getButton (int index) const { return buttons.at (index).get(); }

    //==============================================================================
    // Virtual hooks

    /** Refreshes the item list; base implementation does nothing. */
    virtual void refreshItems() {}

    /** Propagates the current selection to `selected`, fires onChange(), and refreshes navigation buttons. */
    virtual void selectionChanged()
    {
        selected.setValue (displayText);

        if (onChange != nullptr)
            onChange();

        ComboBox::refreshNavigationButtons (this);
    }

    //==============================================================================

    /** @return Reference to the underlying `juce::Value` holding the selected item's text. */
    juce::Value& getValueObject() noexcept override { return selected; }

    //==============================================================================
    // Component overrides

    /** Draws the rounded background, arrow glyph, and current/placeholder text. */
    void paint (juce::Graphics& g) override
    {
        auto bounds { getLocalBounds() };

        if (buttons.size() >= 2)
            bounds.removeFromRight (bounds.getHeight() / 2);

        constexpr auto cornerSize { 3.0f };

        g.setColour (findColour (juce::ComboBox::backgroundColourId));
        g.fillRoundedRectangle (bounds.toFloat(), cornerSize);

        g.setColour (findColour (juce::ComboBox::outlineColourId));
        g.drawRoundedRectangle (bounds.toFloat().reduced (0.5f, 0.5f), cornerSize, 1.0f);

        const auto& arrowPath { menuActive ? SelectorArrows::open() : SelectorArrows::closed() };
        auto inset { menuActive ? 4 : 5 };
        auto offset { menuActive ? -4 : 0 };
        auto arrowArea { bounds.removeFromRight (bounds.getHeight()).translated (offset, 0).reduced (inset).toFloat() };
        juce::Path p { juce::Drawable::parseSVGPath (arrowPath) };
        auto stroke { juce::PathStrokeType (1.5f) };
        auto arrowAlpha { (isEnabled() ? 0.9f : 0.2f) * (isMouseOver() ? 1.0f : 0.5f) };
        g.setColour (findColour (juce::ComboBox::arrowColourId).withAlpha (arrowAlpha));
        g.strokePath (p, stroke, p.getTransformToScaleToFit (arrowArea, true));

        auto textBounds { bounds.reduced (bounds.getHeight() / 2, 0) };
        auto text { displayText.isNotEmpty() ? displayText : placeholderText };
        auto textAlpha { displayText.isNotEmpty() ? 1.0f : 0.5f };

        g.setFont (getLookAndFeel().getPopupMenuFont());

        g.setColour (findColour (juce::ComboBox::textColourId).withMultipliedAlpha (textAlpha));
        g.drawText (text, textBounds, juce::Justification::centred);
    }

    /** Lays the navigation buttons out in a stacked column on the right edge, when present. */
    void resized() override
    {
        auto area { getLocalBounds() };

        if (buttons.size() >= 2)
        {
            int buttonSize { area.getHeight() };
            auto column { area.removeFromRight (buttonSize / 2) };
            getButton (navigation::previous)->setBounds (column.removeFromTop (area.getHeight() / 2));
            getButton (navigation::next)->setBounds (column);
        }
    }

    /** Refreshes items and shows the selection popup menu. */
    void mouseDown (const juce::MouseEvent&) override
    {
        refreshItems();
        showMenu();
    }

    void mouseEnter (const juce::MouseEvent&) override { repaint(); }
    void mouseExit (const juce::MouseEvent&) override { repaint(); }

    //==============================================================================

    /** Called after the selection changes. */
    std::function<void()> onChange;

    /** @brief Navigation button indices. */
    enum navigation
    {
        previous,
        next,
    };

protected:
    Owner<juce::Button> buttons;
    juce::Value selected { juce::var (juce::String()) };

    std::vector<Item> items;
    juce::PopupMenu hierarchicalMenu;
    int selectedId { 0 };
    juce::String displayText;
    juce::String placeholderText;
    bool menuActive { false };

private:
    //==============================================================================

    /** @brief Reconciles a written selected value (by text or numeric id) with the item list. */
    void valueChanged (juce::Value&) override
    {
        auto text { selected.toString() };
        bool found { false };

        for (const auto& item : items)
        {
            if (item.text.equalsIgnoreCase (text))
            {
                selectedId = item.id;
                displayText = item.text;
                found = true;
                break;
            }
        }

        if (not found)
        {
            const int id { juce::roundToInt (static_cast<float> (selected.getValue())) };

            if (id != 0)
            {
                for (const auto& item : items)
                {
                    if (item.id == id)
                    {
                        selectedId = id;
                        displayText = item.text;
                        selected.setValue (displayText);
                        found = true;
                        break;
                    }
                }
            }

            if (not found and text.isNotEmpty())
                displayText = text;
        }

        repaint();
    }

    /** @brief Sets selectedId/displayText from the item whose text matches searchString exactly, without notification. */
    void setSelectedIdFromString (const juce::String& searchString)
    {
        for (const auto& item : items)
        {
            if (item.text == searchString)
            {
                selectedId = item.id;
                displayText = item.text;
                break;
            }
        }

        repaint();
    }

    /** @brief Rebuilds the flat item list from hierarchicalMenu's positive-id entries, recursing sub-menus. */
    void rebuildItemsFromMenu()
    {
        items.clear();
        juce::PopupMenu::MenuItemIterator iter (hierarchicalMenu, true);

        while (iter.next())
        {
            if (iter.getItem().itemID > 0)
            {
                items.push_back ({ iter.getItem().itemID, iter.getItem().text });
            }
        }
    }

    /** @return A flat juce::PopupMenu built from the current item list. */
    juce::PopupMenu buildMenuFromItems() const
    {
        juce::PopupMenu menu;

        for (const auto& item : items)
            menu.addItem (item.id, item.text);

        return menu;
    }

    /** @brief Shows the hierarchical menu (when adopted) or a flat menu built from items, applying the result. */
    void showMenu()
    {
        auto menuToShow { hierarchicalMenu.getNumItems() > 0
                              ? hierarchicalMenu
                              : buildMenuFromItems() };

        if (menuToShow.getNumItems() > 0)
        {
            menuActive = true;
            repaint();

            auto callback = [this] (int result)
            {
                menuActive = false;
                repaint();

                if (result > 0)
                {
                    setSelectedId (result, juce::sendNotificationAsync);
                }
            };

            menuToShow.setLookAndFeel (&getLookAndFeel());

            auto options = juce::PopupMenu::Options()
                               .withTargetComponent (this)
                               .withMinimumWidth (getWidth());

            jam::showAsync (menuToShow, options, callback);
        }
    }

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Selector)
};

/**_____________________________END OF NAMESPACE______________________________*/
}// namespace jam
