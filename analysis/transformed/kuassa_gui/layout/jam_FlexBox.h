/**
 * @file jam_FlexBox.h
 * @brief Convenience helpers for laying out JUCE Components using juce::FlexBox.
 */
namespace jam
{
/*____________________________________________________________________________*/

/**
 * @brief Utility struct providing convenience helpers for laying out
 *        JUCE Components using FlexBox.
 *
 * Contains factory methods for creating row/column FlexBoxes and
 * static helpers to arrange components in rows or columns with equal
 * sizing or label/slider pair layouts.
 */
struct FlexBox
{
    /**
     * @brief Create a FlexBox configured as a horizontal row.
     * @return A FlexBox with row direction and stretch alignment.
     */
    static inline juce::FlexBox makeRowBox()
    {
        auto flexBox = juce::FlexBox {};
        flexBox.flexDirection = juce::FlexBox::Direction::row;
        flexBox.flexWrap = juce::FlexBox::Wrap::noWrap;
        flexBox.alignContent = juce::FlexBox::AlignContent::stretch;
        flexBox.alignItems = juce::FlexBox::AlignItems::stretch;
        flexBox.justifyContent = juce::FlexBox::JustifyContent::spaceBetween;
        return flexBox;
    }

    /**
     * @brief Create a FlexBox configured as a vertical column.
     * @return A FlexBox with column direction and stretch alignment.
     */
    static inline juce::FlexBox makeColumnBox()
    {
        auto flexBox = juce::FlexBox {};
        flexBox.flexDirection = juce::FlexBox::Direction::column;
        flexBox.flexWrap = juce::FlexBox::Wrap::noWrap;
        flexBox.alignContent = juce::FlexBox::AlignContent::stretch;
        flexBox.alignItems = juce::FlexBox::AlignItems::stretch;
        flexBox.justifyContent = juce::FlexBox::JustifyContent::spaceBetween;
        return flexBox;
    }

    //==============================================================================
    /**
     * @brief Layout a row of components inside a given area.
     *
     * @param items List of component pointers to arrange.
     * @param area  Rectangle area to perform layout in.
     */
    template <typename RectangleType>
    static inline void
    makeRow (const std::initializer_list<juce::Component*>& items, const RectangleType& area)
    {
        auto flexBox = makeRowBox();
        const int itemWidth = area.getWidth() / (int) items.size();

        for (auto* item : items)
        {
            if (item != nullptr)
                flexBox.items.add (
                    juce::FlexItem (*item).withFlex (1.0f).withMinWidth (itemWidth).withMinHeight (
                        area.getHeight()));
        }

        flexBox.performLayout (area.toFloat());
    }

    /**
     * @brief Layout a row of components from a juce::Array.
     *
     * @param items Array of component pointers.
     * @param area  Rectangle area to perform layout in.
     */
    template <typename RectangleType>
    static inline void
    makeRow (const juce::Array<juce::Component*>& items, const RectangleType& area)
    {
        auto flexBox = makeRowBox();
        const int itemWidth = area.getWidth() / items.size();

        for (auto* item : items)
        {
            if (item != nullptr)
                flexBox.items.add (
                    juce::FlexItem (*item).withFlex (1.0f).withMinWidth (itemWidth).withMinHeight (
                        area.getHeight()));
        }

        flexBox.performLayout (area.toFloat());
    }

    /**
     * @brief Layout a row of components from an Owner container.
     *
     * @param items Owner container of component pointers.
     * @param area  Rectangle area to perform layout in.
     */
    template <typename ComponentType, typename RectangleType>
    static inline void makeRow (const Owner<ComponentType>& items, const RectangleType& area)
    {
        auto flexBox = makeRowBox();
        const int itemWidth = area.getWidth() / items.size();

        for (auto& item : items)
        {
            if (item != nullptr)
                flexBox.items.add (
                    juce::FlexItem (*item).withFlex (1.0f).withMinWidth (itemWidth).withMinHeight (
                        area.getHeight()));
        }

        flexBox.performLayout (area.toFloat());
    }

    /**
     * @brief Layout a row of alternating label/slider pairs.
     *
     * Each even index is treated as a label with fixed width,
     * each odd index as a slider sharing remaining space.
     *
     * @param items      Owner container of components (label/slider pairs).
     * @param area       Rectangle area to perform layout in.
     * @param labelWidth Fixed width for label components.
     */
    template <typename ComponentType, typename RectangleType>
    static inline void makeLabelSliderRow (const Owner<ComponentType>& items,
                                           const RectangleType& area,
                                           int labelWidth)
    {
        auto flexBox = makeRowBox();
        const int halfSize = (int) items.size() / 2;
        const int sliderWidth = (area.getWidth() - (halfSize * labelWidth)) / halfSize;

        for (int i = 0; i < items.size(); ++i)
        {
            juce::Component* item = items.at (i).get();
            if (item == nullptr)
                continue;

            const bool isSlider = (i % 2 == 1);
            const int itemWidth = isSlider ? sliderWidth : labelWidth;

            flexBox.items.add (
                juce::FlexItem (*item).withMinWidth (itemWidth).withMinHeight (area.getHeight()));
        }

        flexBox.performLayout (area.toFloat());
    }

    //==============================================================================
    /**
     * @brief Layout a column of components inside a given area.
     *
     * @param items List of component pointers to arrange.
     * @param area  Rectangle area to perform layout in.
     */
    template <typename RectangleType>
    static inline void
    makeColumn (const std::initializer_list<juce::Component*>& items, const RectangleType& area)
    {
        auto flexBox = makeColumnBox();
        const int itemHeight = area.getHeight() / (int) items.size();

        for (auto* item : items)
        {
            if (item != nullptr)
                flexBox.items.add (juce::FlexItem (*item)
                                       .withFlex (1.0f)
                                       .withMinWidth (area.getWidth())
                                       .withMinHeight (itemHeight));
        }

        flexBox.performLayout (area.toFloat());
    }

    /**
     * @brief Layout a column of components from an Owner container.
     *
     * @param items Owner container of component pointers.
     * @param area  Rectangle area to perform layout in.
     */
    template <typename ComponentType, typename RectangleType>
    static inline void makeColumn (const Owner<ComponentType>& items, const RectangleType& area)
    {
        auto flexBox = makeColumnBox();
        const int itemHeight = area.getHeight() / items.size();

        for (auto& item : items)
        {
            if (item != nullptr)
                flexBox.items.add (juce::FlexItem (*item)
                                       .withFlex (1.0f)
                                       .withMinWidth (area.getWidth())
                                       .withMinHeight (itemHeight));
        }

        flexBox.performLayout (area.toFloat());
    }
};

/**_____________________________END OF NAMESPACE______________________________*/
}// namespace jam
