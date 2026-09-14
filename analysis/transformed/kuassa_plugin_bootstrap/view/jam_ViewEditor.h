/**
 * @file jam_ViewEditor.h
 * @brief Main editor view, built from the pixel-precise EditorLayout
 *        document (SVG or markdown), sized between the top and bottom panels.
 */

#pragma once

namespace jam
{
/*____________________________________________________________________________*/

/**
 * @class ViewEditor
 * @brief Main plugin editor view -- the knobs, faders, and toggles built
 *        from EditorLayout via ViewManager::build(), sized and positioned
 *        between the plugin's top and bottom panel rows.
 */
class ViewEditor : public juce::Component
{
public:
    //==============================================================================
    /**
     * @brief Loads or creates the view-layout document and builds the editor
     *        component tree into it.
     * @tparam DocumentType         Document type the view layout is parsed as (MarkdownDocument or XmlDocument).
     * @param model                 Audio model the editor tree binds to.
     * @param processorGetters      Processor getter callbacks, populated while building the tree.
     * @param chainEvents           Event callback arrays, populated while building the tree.
     * @param viewLayoutFilename    Filename of the view-layout document to load.
     * @return The built editor view.
     */
    template <typename DocumentType>
    static std::unique_ptr<ViewEditor> create (AudioModel& model, ViewManager::Getters& processorGetters, ViewManager::Events& chainEvents, const juce::String& viewLayoutFilename)
    {
        return std::unique_ptr<ViewEditor> (new ViewEditor (model, processorGetters, chainEvents, DocumentType::getOrCreate (juce::Identifier { viewLayoutFilename })));
    }

    /** @brief Re-applies style and image appearance to the editor tree for the model's current appearance. */
    void setAppearance();

    /** @brief Enables or disables the editor tree's parameter-bound components to reflect the model's bypass state. */
    void setBypassed();

    //==============================================================================
    /**
     * @brief Returns this view's own size, scaled by the given factor.
     * @param scale  Scale factor applied to width and height.
     * @return The scaled size.
     */
    Size<int> getUISize (float scale = 1.0f) const noexcept;

    /**
     * @brief Returns the scale factor that fits this view's unscaled size
     *        inside a bounded proportion of the primary display.
     * @return The screen-proportion scale factor.
     */
    float getUserAreaScale() const noexcept;

    /**
     * @brief Returns the combined UI scale factor for the given model state:
     *        the user's chosen UI scale step, times the screen-proportion
     *        scale, divided by the host's desktop scale.
     * @param audioModel  Audio model providing the UI scale setting and desktop scale.
     * @return The combined UI scale factor.
     */
    float getUIScaleFactor (AudioModel& audioModel) const noexcept;

    /**
     * @brief Returns this view's size at the model's current UI scale
     *        factor, plus the panel row(s) reserved above it.
     * @param audioModel  Audio model providing the UI scale factor and panel height.
     * @return The scaled editor size, including reserved panel rows -- one
     *         row normally, or the sandbox row count under an AU sandbox host.
     */
    Size<int> getUISize (AudioModel& audioModel) const noexcept;

    /**
     * @brief Returns this view's bounds within the plugin editor, offset
     *        below the top panel row by the panel height at the current UI scale.
     * @param audioModel  Audio model providing the UI scale factor and panel height.
     * @return The editor's bounds within the plugin editor.
     */
    juce::Rectangle<int> getViewBounds (AudioModel& audioModel) const noexcept;
    //==============================================================================

private:
    /**
     * @brief Sizes this view to the loaded view layout's UI size and builds
     *        the editor component tree into it.
     * @tparam DocumentType         Document type the view layout was parsed as.
     * @param modelToUse            Audio model the editor tree binds to.
     * @param processorGetters      Processor getter callbacks, populated while building the tree.
     * @param chainEvents           Event callback arrays, populated while building the tree.
     * @param viewLayout            Loaded view-layout document.
     */
    template <typename DocumentType>
    ViewEditor (AudioModel& modelToUse, ViewManager::Getters& processorGetters, ViewManager::Events& chainEvents, const DocumentType& viewLayout)
        : model (modelToUse)
    {
        setComponentID (Id::toTag (Id::viewEditor));

        const auto [width, height] { ViewManager::getUISize (viewLayout) };
        setSize (width, height);

        ViewManager::build (this, model, children, lookAndFeels, attachments, processorGetters, chainEvents);
    }

    /**
     * @brief Computes the scale factor that fits a component of the given
     *        size inside `maxScale` of the primary display's user area.
     * @param componentSize  Unscaled component size to fit.
     * @param maxScale       Upper bound on the returned scale factor.
     * @return The fitting scale factor.
     */
    float getScreenProportionScale (Size<int> componentSize, float maxScale) const noexcept;

    /** @return The UI scale factor for the map::UIScaleMap key stored under `val`. */
    static float getScaleFactor (const juce::String& val) noexcept;
    /** @return The UI scale factor for the given map::UIScaleMap key -- 1.0 at the map's highest key, reduced by one scale step per key below it. */
    static float getScaleFactor (int key) noexcept;
    /** Audio model this editor's component tree binds to. */
    AudioModel& model;

    /** Owning store for LookAndFeel instances applied to the editor tree. */
    Owner<juce::LookAndFeel> lookAndFeels;
    /** Owning store for the components built into the editor tree. */
    Owner<juce::Component> children;
    /** Owning store for APVTS attachments created while building the editor tree. */
    AnyOwner attachments;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ViewEditor)
};

/**_____________________________END_OF_NAMESPACE______________________________*/
} /** namespace jam */
