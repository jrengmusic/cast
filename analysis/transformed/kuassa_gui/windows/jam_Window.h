/**
 * @file jam_Window.h
 * @brief JUCE DocumentWindow wrapper — no built-in glass surface.
 *
 * @par Overview
 * Window wraps juce::DocumentWindow. Glass (chrome tint, opacity, blur backend)
 * is derived from the active jam::StyleCustom LookAndFeel and applied via
 * lookAndFeelChanged() and visibilityChanged(), both of which dispatch to
 * StyleCustom::prepareWindow() once the native peer exists.
 *
 * @par Usage
 * @code
 * auto* win = new jam::Window (
 *     std::make_unique<MainComponent>().release(),
 *     "My App",
 *     false,  // alwaysOnTop
 *     true);  // windowButtons
 * win->setVisible (true);
 * @endcode
 *
 * @note Glass is LookAndFeel-owned — a window relying on the process default
 *       LookAndFeel is styled once at the end of the constructor; a window
 *       given a specific LookAndFeel via setLookAndFeel() is restyled by
 *       JUCE's own broadcast. A one-off restyle is requested by calling
 *       lookAndFeelChanged() directly.
 *
 * @see BackgroundBlur
 * @see StyleWindow
 * @see StyleCustom::prepareWindow
 */

namespace jam
{

/*____________________________________________________________________________*/

/**
 * @class Window
 * @brief juce::DocumentWindow wrapper whose glass is LookAndFeel-owned.
 *
 * Glass (chrome + tint + blur) is derived from the active StyleCustom
 * LookAndFeel and applied via lookAndFeelChanged() / visibilityChanged() ->
 * StyleCustom::prepareWindow(). Traffic-light button visibility is applied
 * by the caller via setWindowButtons().
 *
 * @par Usage
 * Construct the window, call setWindowButtons(visible), then call setVisible(true).
 * A window using a LookAndFeel other than the process default calls setLookAndFeel()
 * itself, which triggers the same glass derivation.
 *
 * @see BackgroundBlur
 * @see StyleWindow
 * @see StyleCustom::prepareWindow
 */
class Window
    : public juce::DocumentWindow
    , public juce::ComponentBoundsConstrainer
{
public:
    /**
     * @brief Constructs, taking ownership of the content and applying a transparent,
     *        resizable DocumentWindow with the given traffic-light button visibility.
     * @param mainComponent  Content component; ownership transferred.
     * @param name           Window title.
     * @param alwaysOnTop    Whether the window floats above others.
     * @param windowButtons  Whether native close/min/max buttons are shown.
     */
    Window (juce::Component* mainComponent, juce::String const& name, bool alwaysOnTop, bool windowButtons = true);

    /** @brief Removes this window's peer from the shared VulkanEngine, then clears the LAF. */
    ~Window() override;

    /** @brief Requests application quit via juce::JUCEApplication::systemRequestedQuit(). */
    void closeButtonPressed() override;

    /**
     * @brief Recreates title-bar buttons via the base DocumentWindow behaviour, then
     *        re-derives and applies window glass from the active StyleCustom LookAndFeel.
     *
     * Called by JUCE on setLookAndFeel()/default-LookAndFeel changes, and once explicitly
     * at the end of the constructor so windows relying on the process default LookAndFeel
     * receive one application even though construction predates any such broadcast.
     * Dispatch only occurs once the native peer exists.
     */
    void lookAndFeelChanged() override;

    /**
     * @brief Re-derives and applies window glass from the active StyleCustom LookAndFeel
     *        once this window becomes visible with a native peer in place.
     */
    void visibilityChanged() override;

    /**
     * @brief Applies chrome tint, native transparency, opacity, and blur backend to this
     *        window from resolved theme values.
     * @param colour  Base tint colour (with alpha) for the window.
     * @param blur    Blur radius passed to BackgroundBlur::enable().
     * @param style   Blur/tint backend selecting native vs Component-level tinting.
     */
    void setStyle (juce::Colour colour, float blur, BackgroundBlur::WindowFX style);

    /**
     * @brief Toggles traffic-light button visibility (macOS) or native title bar (Windows).
     *
     * - macOS: delegates to @c StyleWindow::setButtons, which hides/shows the
     *   close/min/max traffic-light buttons on the native NSWindow.
     * - Windows: toggles @c setUsingNativeTitleBar and @c setTitleBarHeight to
     *   show/hide the native title bar.
     *
     * @par Windows limitation
     * The DocumentWindow ctor's @c allButtons flag is fixed at construction
     * time and cannot be undone at runtime.  Toggling this method on Windows
     * shows or hides the title bar, but cannot add or remove the close/min/max
     * buttons that were configured in the ctor.
     *
     * @param shouldShow  When @c true, show native chrome; when @c false, hide it.
     */
    void setWindowButtons (bool shouldShow);

    /** @brief Asserts a jam::VulkanEngine already exists and returns it — every
     *  Window or ModalSheet requires one alive at construction time (see
     *  vulkanEngine's own doc comment). */
    static jam::VulkanEngine& getVulkanEngine() noexcept
    {
        auto* engine { jam::VulkanEngine::getInstance() };
        jassert (engine != nullptr
                 and "jam::Window requires a jam::VulkanEngine instance to already exist");
        return *engine;
    }

#if JUCE_MAC
    /** @brief Returns the border inset required to keep the content component
     *  below the native title bar. Adds the runtime title-bar height
     *  (StyleWindow::getTitleBarHeight()) as the top border when windowButtons
     *  is true and a live native peer exists; returns zero border otherwise.
     */
    juce::BorderSize<int> getContentComponentBorder() const override;
#endif

#if JUCE_WINDOWS
    /** @brief Middle-click anywhere initiates a window drag (Windows). */
    void mouseDown (const juce::MouseEvent& event) override;
    /** @brief Continues the middle-click window drag (Windows). */
    void mouseDrag (const juce::MouseEvent& event) override;
#endif

private:
    /** @brief When @c false, traffic-light buttons are hidden. */
    bool windowButtons { true };

    /** @brief Shared Vulkan engine — removes this window's peer's Graphics/swapchain
     *  entry on destruction so no window consumer must call removePeer() itself. */
    jam::VulkanEngine& vulkanEngine { getVulkanEngine() };

#if JUCE_WINDOWS
    /** @brief Handles middle-click window dragging. */
    juce::ComponentDragger windowDragger;

    /** @brief JUCE DocumentWindow default title bar height — restored by setWindowButtons(true). */
    static constexpr int defaultTitleBarHeight { 26 };
#endif

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Window)
};

/**_____________________________END_OF_NAMESPACE______________________________*/
} /** namespace jam */
