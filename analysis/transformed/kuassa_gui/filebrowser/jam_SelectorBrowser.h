/**
 * @file jam_SelectorBrowser.h
 * @brief Composed directory-browse trigger and item selector combo box.
 */

namespace jam
{
/*____________________________________________________________________________*/

/**
 * @class SelectorBrowser
 * @brief Composes a `Browser` trigger and a `Selector` combo box side by side —
 * the browser resolves a path, and the selector picks from a populated item list.
 */
class SelectorBrowser : public juce::Component
{
public:
    /**
     * @brief Constructs, wiring the selector's and browser's change callbacks.
     * @param componentID  Identifier propagated to `juce::Component::setComponentID`.
     * @param selectorID   Component ID assigned to the internal selector.
     */
    SelectorBrowser (juce::StringRef componentID,
                     juce::StringRef selectorID = Id::value)
    {
        setComponentID (componentID);

        jam::Component::addAndMakeVisible<Selector> (this, selector);
        selector->setComponentID (selectorID);

        selector->onChange = [this]
        {
            if (onChange != nullptr)
                onChange();
        };

        jam::Component::addAndMakeVisible<Browser> (this, browser, Id::path);

        browser->onValueChanged = [this]
        {
            if (onValueChanged != nullptr)
                onValueChanged();
        };
    }

    ~SelectorBrowser() {}

    /** Lays the browser trigger and selector out side by side. */
    void resized() override
    {
        auto area { getLocalBounds() };
        browser->setBounds (area.removeFromLeft (area.getHeight()));
        int gap { 4 };
        area.removeFromLeft (gap);
        selector->setBounds (area);
    }

    //==============================================================================

    /**
     * @brief Sets the selector's currently selected item.
     * @param selectedId  Item ID to select.
     */
    void setSelectedId (int selectedId)
    {
        selector->setSelectedId (selectedId);
    }

    /**
     * @brief Populates the selector from an id/label map.
     * @param map  Map from item ID to display label.
     */
    void addItemList (const jam::HashMap<int, juce::String>& map)
    {
        selector->addItemList (map);
    }

    /**
     * @brief Adds a single item to the selector.
     * @param newItemText  Display label.
     * @param newItemId    Item ID.
     */
    void addItem (const juce::String& newItemText,
                  int newItemId)
    {
        selector->addItem (newItemText, newItemId);
    }

    /** Called when the selector's selection changes. */
    std::function<void()> onChange;

    /** Called when the browser's resolved value changes (legacy — see `jam::Browser`). */
    std::function<void()> onValueChanged;

    /**
     * @brief Sets the browser's path (legacy — see `jam::Browser::setPath`).
     * @param newPath  New path.
     */
    void setPath (juce::StringRef newPath)
    {
        browser->setPath (newPath);
    }

    /** @return The browser's path (legacy — see `jam::Browser::getPath`). */
    juce::String getPath() const noexcept
    {
        return browser->getPath();
    }

    //    Browser* getBrowser() const noexcept
    //    {
    //        return browser.get();
    //    }
    //==============================================================================
private:
    juce::Value path;
    std::unique_ptr<Selector> selector;
    std::unique_ptr<Browser> browser;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SelectorBrowser)
};

/**_____________________________END OF NAMESPACE______________________________*/
}// namespace jam
