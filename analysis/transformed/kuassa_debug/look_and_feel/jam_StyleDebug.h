/**
 * @file jam_StyleDebug.h
 * @brief StyleDebug — static LookAndFeel shared by jam debug windows.
 */

namespace jam
{
/*____________________________________________________________________________*/
/**
 * @class StyleDebug
 * @brief Static, self-contained LookAndFeel for jam debug windows (DebugConsole, DebugModelMonitor).
 */
class StyleDebug : public jam::StyleCustom
{
public:
    /** @brief jam-debug-private colour identifiers for DebugModelMonitor's property-list
     *  syntax colouring (identifier/number/string), which has no JUCE-native id. */
    enum ColourIds
    {
        propertyIdentifierColourId = map::ColourId::styleDebugPropertyIdentifierColourId,
        propertyNumberColourId = map::ColourId::styleDebugPropertyNumberColourId,
        propertyStringColourId = map::ColourId::styleDebugPropertyStringColourId,
        propertyDisclosureColourId = map::ColourId::styleDebugPropertyDisclosureColourId,
    };

    /** @brief Sets up the debug colour scheme. */
    StyleDebug()
    {
        setColour (juce::ResizableWindow::backgroundColourId, juce::Colour (0xff090d12));
        setColour (juce::TreeView::ColourIds::backgroundColourId, juce::Colours::transparentBlack);
        setColour (
            juce::TreeView::ColourIds::selectedItemBackgroundColourId, juce::Colour (0xff00ddee).withAlpha (0.125f));
        setColour (juce::PropertyComponent::ColourIds::backgroundColourId, juce::Colours::transparentBlack);
        setColour (juce::PropertyComponent::ColourIds::labelTextColourId, juce::Colour (0xffa1d6e5));
        setColour (juce::TextEditor::ColourIds::backgroundColourId, juce::Colours::transparentBlack);
        setColour (juce::TextEditor::ColourIds::textColourId, juce::Colour (0xffa1d6e5));

        setColour (propertyIdentifierColourId, juce::Colour (0xff00ddee));
        setColour (propertyNumberColourId, juce::Colour (0xffc5f0e9));
        setColour (propertyStringColourId, juce::Colour (0xfffc704c));
        setColour (propertyDisclosureColourId, juce::Colour (0xff33535b));
    }

    /**
     * @brief Draws the TreeView expand/collapse disclosure triangle using
     *        propertyDisclosureColourId instead of JUCE's hardcoded white/black box;
     *        the passed background colour is unused — colour is sourced from
     *        propertyDisclosureColourId.
     * @param g                Graphics context to draw into.
     * @param area             Bounds of the disclosure control.
     * @param isOpen           Whether the tree item is currently expanded.
     */
    void drawTreeviewPlusMinusBox (juce::Graphics& g,
                                   const juce::Rectangle<float>& area,
                                   juce::Colour,
                                   bool isOpen,
                                   bool) override
    {
        auto const bounds { area.reduced (area.getWidth() * 0.25f, area.getHeight() * 0.25f) };
        juce::Path triangle;

        if (isOpen)
            triangle.addTriangle (
                bounds.getX(), bounds.getY(), bounds.getRight(), bounds.getY(), bounds.getCentreX(), bounds.getBottom());
        else
            triangle.addTriangle (
                bounds.getX(), bounds.getY(), bounds.getX(), bounds.getBottom(), bounds.getRight(), bounds.getCentreY());

        g.setColour (findColour (propertyDisclosureColourId));
        g.fillPath (triangle);
    }

    /**
     * @brief Derives and applies debug-window glass onto @p window, synchronously.
     *
     * When the GPU renderer is available: macOS gets a 0.1-alpha 20pt backgroundBlur
     * tint; Windows gets a 0.1-alpha 20pt acrylic tint. When unavailable, the window
     * is fully opaque with no blur on either platform.
     */
    void prepareWindow (juce::Component& window) override
    {
        const auto gpuAvailable { jam::VulkanEngine::getInstance() != nullptr and jam::VulkanEngine::getInstance()->isGpuAvailable() };
        const auto colour { this->LookAndFeel::findColour (juce::ResizableWindow::backgroundColourId)
                                .withAlpha (gpuAvailable ? 0.1f : 1.0f) };
        const auto blur { gpuAvailable ? 20.0f : 0.0f };
#if JUCE_MAC
        const auto backend { jam::BackgroundBlur::WindowFX::backgroundBlur };
#elif JUCE_WINDOWS
        const auto backend { jam::BackgroundBlur::WindowFX::acrylic };
#endif

        auto* jamWindow { dynamic_cast<jam::Window*> (&window) };
        jassert (jamWindow != nullptr);

        jamWindow->setStyle (colour, blur, backend);
    }

    /** @brief Returns the shared instance, constructed on first call. */
    static StyleDebug& getShared()
    {
        static StyleDebug instance;
        return instance;
    }

private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StyleDebug)
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
