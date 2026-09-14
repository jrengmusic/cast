/**
 * @file jam_ViewPanel.h
 * @brief Top/bottom bar panel view, built from PanelLayout.html and laid
 *        out as a single flexbox row or column.
 */

#pragma once

namespace jam
{
/*____________________________________________________________________________*/

/**
 * @class ViewPanel
 * @brief Scaled content view for the top and bottom bars, built once from
 *        PanelLayout.html and collapsed to a single flexbox layout pass on resize.
 */
class ViewPanel
    : public ScaledContent
    , public Instance<ViewPanel>
{
public:
    using Layout = const Document&;
    //==============================================================================
    /**
     * @brief Builds the panel's component tree and attaches every child to the model.
     * @param model        Audio model the panel and its children attach to.
     * @param panelLayout  Parsed PanelLayout.html document.
     * @return The built panel view.
     */
    static std::unique_ptr<ViewPanel> create (AudioModel& model, Layout panelLayout)
    {
        std::unique_ptr<ViewPanel> panel { new ViewPanel (model, panelLayout) };

        return panel;
    }

    /**
     * @brief Resizes the target layer to the unscaled panel size and
     *        re-applies its flexbox layout, when the panel declares a body.
     */
    void resized() override
    {
        auto* target { getTargetComponent() };

        if (target->getProperties().contains (Id::body))
        {
            const auto hostScale { getHostScale() };

            target->setSize (juce::roundToInt (getWidth() / hostScale), juce::roundToInt (getHeight() / hostScale));

            const auto emUnit { static_cast<float> (model.getUIPanelHeight()) / hostScale };

            ViewManager::applyFlexLayout (target, emUnit);
        }
    }

    /** @brief Plain background fill for a `\<div\>` container inside the panel's HTML tree. */
    struct Background : public juce::Component
    {
        void paint (juce::Graphics& g) override { g.fillAll (findColour (juce::ResizableWindow::backgroundColourId)); }
    };

private:
    /**
     * @brief Builds the panel's HTML component tree and attaches each of
     *        its direct children to the model's value tree.
     * @param modelToUse   Audio model the panel and its children attach to.
     * @param panelLayout  Parsed PanelLayout.html document.
     */
    ViewPanel (AudioModel& modelToUse, Layout panelLayout)
        : model (modelToUse)
    {
        setComponentID (Id::toTag (Id::panel));

        auto* target { getTargetComponent() };
        target->setComponentID (Id::toTag (Id::panel));

        ViewManager::buildContent (target, model, children, panelLayout);

        for (auto* child : target->getChildren())
            Model::attach<ViewManager> (model.state, child);
    }

    /** Audio model this panel and its children attach to. */
    AudioModel& model;
    //==============================================================================
    /** Owning store for the components built into this panel. */
    Owner<juce::Component> children;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ViewPanel)
};

/**_____________________________END_OF_NAMESPACE______________________________*/
} /** namespace jam */
