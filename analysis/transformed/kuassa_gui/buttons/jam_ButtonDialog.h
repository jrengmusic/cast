/**
 * @file jam_ButtonDialog.h
 * @brief Icon button that launches a modal glass dialog hosting arbitrary content.
 */
#pragma once

namespace jam
{
/*____________________________________________________________________________*/

/**
 * @class ButtonDialog
 * @brief Hosts an icon button whose click launches a modal glass dialog built from
 * a content factory via a ModalWindow on all platforms.
 */
class ButtonDialog : public juce::Component
{
public:
    ButtonDialog() = default;

    ~ButtonDialog() { popup.dismiss(); }

    void resized() override
    {
        if (button != nullptr)
            button->setBounds (getLocalBounds());
    }

    /**
     * @brief Binds an icon button and dialog content factory to a panel model.
     * @param iconButton  Icon button that launches the dialog.
     * @param factory     Factory producing the dialog's content component.
     */
    void bindToPanel (std::unique_ptr<juce::Button> iconButton,
                      std::function<std::unique_ptr<juce::Component>()> factory)
    {
        contentFactory = std::move (factory);
        button = std::move (iconButton);
        juce::Component::addAndMakeVisible (button.get());
        button->onClick = launchDialog();
    }

    /**
     * @brief Binds the registered icon button and a content factory to a panel model.
     *
     * Takes the registered button via `reg.make`, casts it to `juce::Button`, and
     * builds a content factory that produces the element's registered content
     * component via `reg.makeContent`. No guards — the button is trusted to exist.
     *
     * @param reg         Registry supplying the registered button and content factory.
     * @param element     Component descriptor providing the button and content keys.
     * @param modelToUse  Audio model passed to the content factory.
     */
    void bindToPanel (Registry& reg, const Document::Element& element, AudioModel& modelToUse)
    {
        const auto descButton { Registry::getButton (element) };

        if (reg.make.contains (descButton))
        {
            if (auto iconButton { reg.make.get (descButton) })
            {
                const auto content { Registry::getContent (element) };

                bindToPanel (std::unique_ptr<juce::Button> (dynamic_cast<juce::Button*> (iconButton.release())),
                            [&reg, &modelToUse, content]()
                            {
                                return reg.makeContent.get (content, modelToUse);
                            });
            }
        }
    }

    /**
     * @brief Sets whether the dialog dismisses itself when it loses focus.
     * @param value  `true` to dismiss on lost focus.
     */
    void setShouldBeDismissedWhenOutOfFocus (bool value) noexcept { shouldBeDismissed = value; }

    //==============================================================================
    /** @return A callback that constructs the dialog content via contentFactory and shows the popup. */
    std::function<void()> launchDialog()
    {
        return [this]()
        {
            if (contentFactory != nullptr)
            {
                auto newContent { contentFactory() };

                if (newContent != nullptr)
                {
                    const int contentWidth { newContent->getWidth() };
                    const int contentHeight { newContent->getHeight() };

                    popup.onDismiss = onDismiss;

                    popup.show (*getTopLevelComponent(),
                                std::move (newContent),
                                contentWidth,
                                contentHeight,
                                shouldBeDismissed);

                    if (onLaunch != nullptr)
                        onLaunch();
                }
            }
        };
    }

    /** @brief Callback invoked after the dialog is launched. */
    std::function<void()> onLaunch;

    /** @brief Callback invoked after the dialog is dismissed. */
    std::function<void()> onDismiss;

    //==============================================================================
private:
    std::function<std::unique_ptr<juce::Component>()> contentFactory;
    std::unique_ptr<juce::Button> button;

    bool shouldBeDismissed { true };

    /** @brief Owns and drives the modal dialog window (ModalWindow on all platforms). */
    class Popup
    {
    public:
        /** @brief Default constructor. No dialog is active until show() is called. */
        Popup() = default;
        /** @brief Default destructor. */
        ~Popup() = default;

        /**
     * @brief Shows a modal glass dialog centred on @p caller with @p content inside.
     *
     * Sets the given pixel @p width and @p height on the content, then
     * creates a ModalWindow, centres it on @p caller, and enters modal state.
     * Ownership of @p content transfers to the dialog.
     *
     * Walks the content tree via `jam::Component::applyFunctionRecursively` and
     * wires the click handler of every child whose `Id::parameter` property is
     * `CLOSE` to dismiss the dialog — this includes the framework close button
     * that `ViewContent` itself owns, not only application-declared close buttons.
     *
     * @param caller                   The component to centre the dialog around.
     * @param content                  The component to host; ownership is transferred.
     * @param width                    Popup width in logical pixels.
     * @param height                   Popup height in logical pixels.
     * @param shouldDismissOnLostFocus Whether the dialog closes when it loses focus.
     * @note MESSAGE THREAD.
     * @see dismiss
     */
        void show (juce::Component& caller,
                   std::unique_ptr<juce::Component> content,
                   int width,
                   int height,
                   bool shouldDismissOnLostFocus)
        {
            dismiss();

            auto* rawContent { content.get() };

            window = std::make_unique<jam::ModalWindow> (
                std::move (content),
                caller,
                [this]
                {
                    if (onDismiss != nullptr)
                        onDismiss();

                    dismiss();
                },
                shouldDismissOnLostFocus);

            jam::Component::applyFunctionRecursively (
                rawContent,
                [this] (juce::Component* child)
                {
                    if (jam::Component::hasProperty (child->getProperties(), Id::parameter, Id::toType (Id::close)))
                    {
                        static_cast<juce::Button*> (child)->onClick = [this]
                        {
                            if (onDismiss != nullptr)
                                onDismiss();

                            dismiss();
                        };
                    }
                });
        }

        /**
     * @brief Dismisses the dialog if active.
     *
     * Calls exitModalState (0) on the dialog window and releases it.
     *
     * @note MESSAGE THREAD.
     * @see show
     */
        void dismiss()
        {
            if (window != nullptr)
            {
                window->dismiss();
                auto owned { std::move (window) };
                juce::MessageManager::callAsync ([owned = std::move (owned)] {});
            }
        }

        /**
     * @brief Returns whether the dialog is currently active.
     *
     * @return true if the dialog window exists and is visible.
     * @note MESSAGE THREAD.
     */

        bool isActive() const noexcept
        {
            return window != nullptr;
        }
        /** @brief Callback invoked when the dialog is dismissed (Escape or close button). */
        std::function<void()> onDismiss;

    private:
        std::unique_ptr<jam::ModalWindow> window;

        //==========================================================================
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Popup)
    };

    Popup popup;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ButtonDialog)
};

/**_____________________________END OF NAMESPACE______________________________*/
} /** namespace jam */
