/**
 * @file jam_ViewContent.h
 * @brief Scaled content view holding the padding band and framework close
 *        button shared by the Settings and About windows.
 */

#pragma once

namespace jam
{
/*____________________________________________________________________________*/

/**
 * @class ViewContent
 * @brief Scaled content view that wraps a built layout body in a padding
 *        band and the framework's close button.
 *
 * The padding and close-button sizes come from the active StyleCustom
 * LookAndFeel's `--padding` and `--close` CSS registry entries (the
 * `.window` selector); on a LookAndFeel without those entries both default
 * to zero.
 */
class ViewContent : public ScaledContent
{
public:
    //==============================================================================
    /**
     * @brief Builds a ViewContent from a parsed layout document and sizes it
     *        to the built content plus the padding band.
     * @param model          Audio model passed to ViewManager::buildContent().
     * @param contentLayout  Parsed layout document (e.g. AboutLayout.html).
     * @return The built content view, or a content view with no children when
     *         the document declares no body.
     */
    static std::unique_ptr<ViewContent> create (AudioModel& model, ViewPanel::Layout contentLayout)
    {
        std::unique_ptr<ViewContent> content { new ViewContent (contentLayout) };

        if (content->layout.root->firstChild != nullptr)
        {
            if (const auto* body { content->layout.root->getChildByID (Id::body) })
            {
                jassert (body->contains (Id::width) and body->contains (Id::height));

                ViewManager::buildContent (content->getTargetComponent(), model, content->children, content->layout);

                content->setScaledSize();
            }
        }

        return content;
    }

    ~ViewContent() = default;

    /** @brief Fills the background -- opaque with the window opacity on macOS, or solid when the Windows blur is disabled. */
    void paint (juce::Graphics& g) override
    {
#if JUCE_MAC
        auto* styleLaf { dynamic_cast<jam::StyleCustom*> (&getLookAndFeel()) };
        const auto opacity { styleLaf != nullptr ? styleLaf->getWindowOpacity() : 1.0f };
        g.fillAll (findColour (juce::ResizableWindow::backgroundColourId).withAlpha (opacity));
#elif JUCE_WINDOWS
        if (not jam::BackgroundBlur::isEnabled())
            g.fillAll (findColour (juce::ResizableWindow::backgroundColourId));
#endif
    }

    /** @brief Re-applies the target component's flex/absolute layout. */
    void resized() override { ViewManager::applyLayout (getTargetComponent(), emUnit); }

    //==============================================================================
protected:
    /**
     * @brief Positions the target component below the padding band and the
     *        close button, then sizes this view to the target's scaled
     *        bounds plus the padding band, and positions the close button.
     */
    void setScaledSize()
    {
        auto* styleLaf { dynamic_cast<jam::StyleCustom*> (&getLookAndFeel()) };
        const auto padding { styleLaf != nullptr ? styleLaf->getWindowPadding() : 0 };
        const auto close { styleLaf != nullptr ? styleLaf->getWindowClose() : 0 };

        auto* target { getTargetComponent() };
        target->setTopLeftPosition (padding, padding + close);

        const auto scaled { target->getBoundsInParent() };
        setSize (scaled.getRight() + padding, scaled.getBottom() + padding);

        closeButton.setBounds (getWidth() - padding - close, padding, close, close);
    }

    /**
     * @brief Adds the framework close button, stamped with the close
     *        parameter tag, and reads the root font size for `em` layout units.
     * @param contentLayout  Parsed layout document owning this view's content.
     */
    ViewContent (ViewPanel::Layout contentLayout)
        : layout (contentLayout)
    {
        setOpaque (false);

        closeButton.getProperties().set (Id::parameter, Id::toTag (Id::close));
        addAndMakeVisible (closeButton);

        if (layout.root->contains (Id::fontSize))
            emUnit = static_cast<float> (*layout.root->get<double> (Id::fontSize));
    }

    /** Parsed layout document this view's content was built from. */
    ViewPanel::Layout layout;
    /** Owning store for the components built into this view's content. */
    Owner<juce::Component> children;
    /** Pixel size of one em, used to resolve em-relative layout properties. */
    float emUnit { 0.0f };

private:
    //==============================================================================
    /** Framework-provided close button, drawn from the `close` SVG binary resource. */
    jam::ButtonSVG closeButton { jam::ButtonSVG::fromBinary (Id::close.toString()) };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ViewContent)
};

/**_____________________________END_OF_NAMESPACE______________________________*/
} /** namespace jam */
