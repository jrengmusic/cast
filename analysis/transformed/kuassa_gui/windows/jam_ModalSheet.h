/**
 * @file jam_ModalSheet.h
 * @brief macOS NSSheet modal with RAII lifecycle and explicit blur backend.
 *
 * @par Overview
 * ModalSheet encapsulates the NSSheet presentation lifecycle: window creation,
 * chrome removal, JUCE content hosting, Escape/blur dismiss, and teardown.
 * Blur backend is selected explicitly via BackgroundBlur::WindowFX.
 *
 * @par Content Lifecycle
 * The content component is owned by ModalSheet from construction until dismiss().
 * On dismiss(), the component is removed from the desktop and deleted, preventing
 * it from lingering in Desktop::desktopComponents past DAW shutdown.
 *
 * @note macOS only — guarded by @c JUCE_MAC.
 *
 * @see ModalWindow
 * @see BackgroundBlur
 */

#if JUCE_MAC

namespace jam
{
/*____________________________________________________________________________*/

/**
 * @class ModalSheet
 * @brief RAII wrapper presenting a JUCE component as a macOS NSSheet.
 */
class ModalSheet
{
public:
    /**
     * @brief Presents content as an NSSheet attached to parent's window.
     *
     * @p colour and @p blurRadius are forwarded to the sheet unmodified; the
     * sheet is made non-opaque and BackgroundBlur::enable() is called whenever
     * @p blurRadius is greater than 0.0f.
     *
     * @param parent                    Component whose NSWindow hosts the sheet.
     * @param content                   Content component; ownership transferred.
     * @param width                     Sheet width in logical pixels.
     * @param height                    Sheet height in logical pixels.
     * @param colour                    Tint colour with alpha.
     * @param blurRadius                Blur radius forwarded to BackgroundBlur::enable().
     * @param backend                   Blur backend to use.
     * @param shouldDismissOnLostFocus  When true, sheet dismisses on key-window loss.
     * @param onDismiss                 Callback invoked on any dismiss path.
     */
    ModalSheet (juce::Component& parent,
                std::unique_ptr<juce::Component> content,
                int width, int height,
                juce::Colour colour, float blurRadius,
                BackgroundBlur::WindowFX backend,
                bool shouldDismissOnLostFocus,
                std::function<void()> onDismiss);

    ~ModalSheet();

    /**
     * @brief Dismisses the sheet if active.
     *
     * Calls endSheet: on the parent window, then releases the NSWindow to balance
     * the +1 retain from alloc (MRC — ARC does not manage this file).  Content
     * component removal is deferred via MessageManager::callAsync to avoid
     * use-after-free when dismiss() is called from within the content itself.
     */
    void dismiss();

    /** @brief Returns whether the sheet is currently presented. */
    bool isActive() const noexcept { return sheetHandle != nullptr; }

private:
    jam::VulkanEngine& vulkanEngine { Window::getVulkanEngine() }; ///< Shared Vulkan engine — removes the content's peer's Graphics entry on dismiss.

    void* sheetHandle { nullptr };              ///< Opaque NSWindow* handle for the presented sheet, or nullptr when inactive.
    juce::Component* parentComponent { nullptr }; ///< Component whose NSWindow hosts the sheet.
    juce::Component* contentComponent { nullptr }; ///< Owned content component; released (deferred) on dismiss().

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModalSheet)
};

/**_____________________________END_OF_NAMESPACE______________________________*/
} /** namespace jam */

#endif // JUCE_MAC
