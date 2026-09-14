/**
 * @file jam_CollapsibleList.h
 * @brief Scrollable vertical list of collapsible sections.
 */

namespace jam
{
/*____________________________________________________________________________*/

/**
 * @class CollapsibleList
 * @brief Scrollable, vertically-stacked list of `CollapsibleSectionBase` sections —
 * each section's expanded/collapsed height determines its slot in the layout.
 */
class CollapsibleList : public juce::Component
{
public:
    /**
     * @brief Constructs the list and assigns its component ID.
     * @param componentID  Component ID.
     */
    explicit CollapsibleList (const juce::String& componentID);

    ~CollapsibleList() override = default;

    /**
     * @brief Adds a pre-constructed section to the list.
     * @param section  Section to add; ownership transfers to the list.
     */
    void addSection (std::unique_ptr<CollapsibleSectionBase> section);

    /**
     * @brief Constructs a `CollapsibleSection<ComponentType>` and adds it to the list.
     * @tparam ComponentType  Content component type hosted by the new section.
     * @param model        Model that owns the section's persisted open-state parameter.
     * @param sectionNode  ValueTree node the open-state parameter is created under.
     * @param title        Header title and content component ID.
     * @param rowHeight    Collapsed (header-only) height, in pixels.
     * @param numRows      Number of content rows.
     * @param args         Arguments forwarded to `ComponentType`'s constructor.
     * @return Raw pointer to the newly added section, owned by the list.
     */
    template <typename ComponentType, typename... Args>
    CollapsibleSection<ComponentType>* addNewSection (jam::Model& model,
                                                       juce::ValueTree sectionNode,
                                                       const juce::String& title,
                                                       int rowHeight,
                                                       int numRows,
                                                       Args&&... args)
    {
        auto section { std::make_unique<CollapsibleSection<ComponentType>> (
            model, sectionNode, title, rowHeight, numRows, std::forward<Args> (args)...) };

        auto* raw { section.get() };
        addSection (std::move (section));
        return raw;
    }

    /**
     * @brief Returns a section's content component by index.
     * @tparam ComponentType  Expected content component type.
     * @param index  Zero-based section index.
     * @return Content component, or `nullptr` when out of range or the type does not match.
     */
    template <typename ComponentType>
    ComponentType* getContentOf (int index)
    {
        try
        {
            if (auto* typed { dynamic_cast<CollapsibleSection<ComponentType>*> (sections.at (index).get()) })
                return typed->getContent();
        }
        catch (const std::out_of_range&)
        {
        }

        return nullptr;
    }

    /**
     * @brief Returns a section by index.
     * @tparam ComponentType  Expected content component type.
     * @param index  Zero-based section index.
     * @return Section, or `nullptr` when out of range or the type does not match.
     */
    template <typename ComponentType>
    CollapsibleSection<ComponentType>* getSection (int index)
    {
        try
        {
            return dynamic_cast<CollapsibleSection<ComponentType>*> (sections.at (index).get());
        }
        catch (const std::out_of_range&)
        {
            return nullptr;
        }
    }

    /**
     * @brief Returns a section by component ID.
     * @tparam ComponentType  Expected content component type.
     * @param id  Section component ID.
     * @return Section, or `nullptr` when no section matches or the type does not match.
     */
    template <typename ComponentType>
    CollapsibleSection<ComponentType>* getSectionByID (const juce::String& id)
    {
        for (auto& sec : sections)
        {
            if (sec->getComponentID().compare (id) == 0)
                return dynamic_cast<CollapsibleSection<ComponentType>*> (sec.get());
        }
        return nullptr;
    }

    /**
     * @brief Returns a section's content component by component ID.
     * @tparam ComponentType  Expected content component type.
     * @param id  Section component ID.
     * @return Content component, or `nullptr` when no section matches or the type does not match.
     */
    template <typename ComponentType>
    ComponentType* getContentByID (const juce::String& id)
    {
        for (auto& sec : sections)
        {
            if (sec->getComponentID().compare (id) == 0)
            {
                if (auto* typed { dynamic_cast<CollapsibleSection<ComponentType>*> (sec.get()) })
                    return typed->getContent();
            }
        }
        return nullptr;
    }

    /** Removes all sections from the list. */
    void clearSections();

    /** @brief Fits the viewport to this component's bounds and recomputes section layout. */
    void resized() override;

    /** Recomputes every section's bounds from top to bottom and resizes the scrollable content. */
    void updateLayout();

private:
    juce::Viewport viewport;               ///< Scrolling container for content.
    juce::Component content;               ///< Viewed component hosting the stacked sections.
    Owner<CollapsibleSectionBase> sections; ///< Owned sections, in display order.

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CollapsibleList)
};

/**_____________________________END OF NAMESPACE______________________________*/
}// namespace jam
