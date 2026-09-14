/**
 * @file jam_DebugWidget.h
 * @brief DebugWidget — aggregates DebugConsole, DebugModelMonitor, and DebugBuildInfo
 *        around a host editor/component.
 */

#pragma once

#if JUCE_DEBUG

namespace jam
{
/*____________________________________________________________________________*/

/**
 * @brief Owns and positions the debug console, value tree monitor, and
 *        build-info overlay around a host editor or main component.
 *
 * Tracks the host component via ComponentListener so the build-info overlay
 * repositions itself on resize.
 */
class DebugWidget : public juce::ComponentListener
{
public:
    /**
     * @brief Constructs console + optional value tree monitor + optional build-info overlay
     *        anchored to a plugin editor.
     *
     * @param editorToAdd      Editor component this widget attaches to and listens on.
     * @param state            ValueTree shown in the DebugModelMonitor when enabled.
     * @param isUsingValueTreeMonitor Whether to construct and show a DebugModelMonitor.
     * @param alwaysOnTop      Whether the console/monitor windows stay always-on-top.
     * @param isUsingConsole   Whether to construct and show the DebugConsole.
     */
    DebugWidget (juce::Component& editorToAdd,
                 juce::ValueTree& state,
                 bool isUsingValueTreeMonitor = true,
                 bool alwaysOnTop = true,
                 bool isUsingConsole = false)
        : editor (editorToAdd)
        , isUsingMonitor (isUsingValueTreeMonitor)
    {
        auto desktop { juce::Desktop::getInstance().getDisplays().getPrimaryDisplay()->userBounds.toNearestInt() };

        if (isUsingConsole)
        {
            console = std::make_unique<DebugConsole>();
            console->setTopLeftPosition (
                desktop.getWidth() - console->getWidth(), desktop.getHeight() - console->getHeight());
            console->setAlwaysOnTop (alwaysOnTop);
        }

        if (isUsingMonitor)
        {
            monitor = std::make_unique<DebugModelMonitor> (state);
            monitor->setAlwaysOnTop (alwaysOnTop);
            auto x { desktop.getWidth() - monitor->getWidth() };
#if JUCE_MAC
            monitor->setTopLeftPosition (x, 0);
#elif JUCE_WINDOWS
            monitor->setTopLeftPosition (x, windowsMonitorYOffset);
#endif
        }
#if JAM_USING_BUILD_INFO
        buildInfo = std::make_unique<DebugBuildInfo> (&editor);
        buildInfo->setAlpha (0.5f);
#endif// JAM_USING_BUILD_INFO

        editor.addComponentListener (this);
    }

    ~DebugWidget() override = default;

    /**
     * @brief Repositions the build-info overlay to track the host component's resize.
     * @param component   The listened-to host component.
     * @param wasMoved    Unused; JUCE ComponentListener contract parameter.
     * @param wasResized  Whether the component was resized; overlay repositions only then.
     */
    void componentMovedOrResized (juce::Component& component, bool wasMoved, bool wasResized) override
    {
#if JAM_USING_BUILD_INFO
        if (buildInfo != nullptr and wasResized)
        {
            auto editorBounds { component.getLocalBounds() };

            int y { 0 };

            if (auto panel { component.findChildWithID (Id::panel) })
            {
                for (auto* container : panel->getChildren())
                {
                    if (container->getProperties()[Id::id] == Id::panelTop.toString())
                    {
                        y = container->getHeight();
                        break;
                    }
                }
            }

            buildInfo->setBounds (0, y, editorBounds.getWidth(), buildInfoHeight);
        }
#endif// JAM_USING_BUILD_INFO
    }

    /**
     * @brief Points the DebugModelMonitor, if present, at a new ValueTree.
     * @param newTree  The ValueTree to display.
     */
    void setTree (juce::ValueTree& newTree)
    {
        if (monitor != nullptr)
            monitor->setSource (newTree);
    }

    /**
     * @brief Forwards property-value formatters to the DebugModelMonitor, if present.
     * @tparam ValidatorsType  Nested-map validators container type.
     * @param  validators      Per-tag, per-property validators supplying display formatters.
     */
    template<typename ValidatorsType>
    void setFormats (const ValidatorsType& validators)
    {
        if (monitor != nullptr)
            monitor->setFormats (validators);
    }

    /** @brief Returns the owned DebugConsole window, or nullptr if none was constructed. */
    juce::ResizableWindow* getConsoleWindow() { return console.get(); }

private:
#if JAM_USING_BUILD_INFO
    std::unique_ptr<DebugBuildInfo> buildInfo;
#endif// JAM_USING_BUILD_INFO

    juce::Component& editor;

    std::unique_ptr<DebugConsole> console;
    std::unique_ptr<DebugModelMonitor> monitor;

    bool isUsingMonitor { false };

    /** @brief Console window height as a fraction of screen height when no DebugModelMonitor is showing. */
    static constexpr float consoleOnlyHeightFraction { 0.67f };

    /** @brief DebugModelMonitor vertical position offset on Windows (leaves room for the title bar). */
    static constexpr int windowsMonitorYOffset { 28 };

    /** @brief DebugBuildInfo overlay height in pixels. */
    static constexpr int buildInfoHeight { 35 };

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DebugWidget)
};

/**_____________________________END OF NAMESPACE______________________________*/
}// namespace jam

#endif// JUCE_DEBUG
