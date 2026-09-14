/**
 * @file jam_ModalWindow.h
 * @brief Window subclass with modal dialog semantics.
 *
 * ModalWindow combines Window's wrapper role with modal input blocking and
 * Escape dismiss. Glass is derived automatically from the active StyleCustom
 * LookAndFeel via Window::lookAndFeelChanged(); setupWindow() only inherits
 * centreAround's LookAndFeel for the content and centres the window.
 * Replaces the need to subclass juce::DialogWindow for glass modals.
 *
 * @par Usage
 * Subclass ModalWindow, call setVisible(true), then enterModalState(true).
 * Escape and close button call closeButtonPressed() which exits modal state
 * and invokes the onModalDismissed callback.
 *
 * @see Window
 */

namespace jam
{
/*____________________________________________________________________________*/

/**
 * @class ModalWindow
 * @brief Window with modal semantics: Escape dismiss, input blocking.
 *
 * Provides the common pattern shared by all modal glass windows:
 * - Escape key exits modal state
 * - Close button exits modal state
 * - Clicks outside bring window to front (does not dismiss)
 * - Optional dismiss callback
 *
 * @see Window
 */
class ModalWindow : public Window
{
public:
    /**
     * @brief Constructs a modal glass window.
     *
     * @param mainComponent      Content component; ownership transferred.
     * @param name               Window title (empty for borderless).
     * @param alwaysOnTop        Whether the window floats above others.
     * @param windowButtons      Whether native close/min/max buttons are shown.
     */
    ModalWindow (juce::Component* mainComponent,
                 const juce::String& name,
                 bool alwaysOnTop,
                 bool windowButtons = false);

    /**
     * @brief Caller-ctor — creates, configures, and enters modal state.
     *
     * Glass is derived automatically from the active StyleCustom LookAndFeel
     * via Window::lookAndFeelChanged().
     *
     * @param content              Content component; ownership transferred.
     * @param centreAround         Component to centre the window on; L&F inherited.
     * @param dismissCallback      Called when the modal is dismissed.
     * @param newShouldDismissOnLostFocus  When true, dismisses on focus loss.
     * @note MESSAGE THREAD.
     */
    ModalWindow (std::unique_ptr<juce::Component> content,
                 juce::Component& centreAround,
                 std::function<void()> dismissCallback,
                 bool newShouldDismissOnLostFocus = false);

    /** @brief Default destructor. */
    ~ModalWindow() override = default;

    /** @brief Exits modal state with result 0, then invokes onModalDismissed. */
    void closeButtonPressed() override;
    /** @brief Dismisses via closeButtonPressed() on Escape. @return `true` when the key was Escape. */
    bool keyPressed (const juce::KeyPress& key) override;
    /** @brief Dismisses when shouldDismissOnLostFocus is true; otherwise brings the window to front. */
    void inputAttemptWhenModal() override;

    /** @brief Exits modal state (result 0) and invokes onModalDismissed.
     *  Idempotent — safe to call when no modal state is active. */
    void dismiss();

    /** @brief Returns true while this window is in an active modal state.
     *  On macOS the window itself is a dormant base and the ModalSheet drives
     *  presentation; isActive() reflects the modal-state flag regardless. */
    bool isActive() const noexcept;

    /** @brief Callback invoked when the modal window is dismissed. */
    std::function<void()> onModalDismissed;

    /** When true, inputAttemptWhenModal dismisses the window via closeButtonPressed.
        When false (default), inputAttemptWhenModal brings the window to front. */
    bool shouldDismissOnLostFocus { false };

private:
    /** @brief Inherits centreAround's LAF for the content, disables resizing, and centres
     *  the window. Glass is applied separately via Window::lookAndFeelChanged().
     */
    void setupWindow (juce::Component& centreAround);

#if JUCE_MAC
    std::unique_ptr<ModalSheet> sheet;
#endif

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModalWindow)
};

/**_____________________________END_OF_NAMESPACE______________________________*/
} /** namespace jam */
