/**
 * @file jam_ModalWindow.cpp
 * @brief Implementation of ModalWindow — modal Window.
 *
 * @see jam_modal_window.h
 * @see Window
 */

namespace jam
{
/*____________________________________________________________________________*/

ModalWindow::ModalWindow (juce::Component* mainComponent,
                          const juce::String& name,
                          bool alwaysOnTop,
                          bool windowButtons)
    : Window (mainComponent, name, alwaysOnTop, windowButtons)
{
}

ModalWindow::ModalWindow (std::unique_ptr<juce::Component> content,
                          juce::Component& centreAround,
                          std::function<void()> dismissCallback,
                          bool newShouldDismissOnLostFocus)
#if JUCE_MAC
    : ModalWindow (nullptr, {}, true, false)
#else
    : ModalWindow (
          [&content]
          {
              const int w { content->getWidth() };
              const int h { content->getHeight() };
              auto* raw { content.release() };
              raw->setSize (w, h);
              return raw;
          }(),
          {},
          true,
          false)
#endif
{
#if JUCE_MAC
    const int w { content->getWidth() };
    const int h { content->getHeight() };

    auto* styleLaf { dynamic_cast<jam::StyleCustom*> (&centreAround.getLookAndFeel()) };
    const float opacity { styleLaf != nullptr ? styleLaf->getWindowOpacity() : 1.0f };
    const float blur    { styleLaf != nullptr ? styleLaf->getWindowBlur()    : 0.0f  };
    const juce::Colour baseColour { centreAround.findColour (juce::ResizableWindow::backgroundColourId) };

    sheet = std::make_unique<ModalSheet> (
        centreAround,
        std::move (content),
        w, h,
        baseColour.withAlpha (opacity),
        blur,
        jam::BackgroundBlur::isGlassFXAvailable()
            ? BackgroundBlur::WindowFX::glassFXClear
            : BackgroundBlur::WindowFX::visualFXWindowBackground,
        newShouldDismissOnLostFocus,
        std::move (dismissCallback));
#else
    onModalDismissed = std::move (dismissCallback);
    shouldDismissOnLostFocus = newShouldDismissOnLostFocus;
    setupWindow (centreAround);
    setVisible (true);
    enterModalState (true);
#endif
}

void ModalWindow::setupWindow (juce::Component& centreAround)
{
    if (auto* content { getContentComponent() })
        content->setLookAndFeel (&centreAround.getLookAndFeel());

    setResizable (false, false);
    centreAroundComponent (&centreAround, getWidth(), getHeight());
}

void ModalWindow::closeButtonPressed()
{
    exitModalState (0);

    if (onModalDismissed != nullptr)
        onModalDismissed();
}

bool ModalWindow::keyPressed (const juce::KeyPress& key)
{
    bool handled { false };

    if (key == juce::KeyPress::escapeKey)
    {
        closeButtonPressed();
        handled = true;
    }

    return handled;
}

void ModalWindow::inputAttemptWhenModal()
{
    if (shouldDismissOnLostFocus)
        closeButtonPressed();
    else
        toFront (true);
}

void ModalWindow::dismiss()
{
#if JUCE_MAC
    if (sheet != nullptr)
        sheet->dismiss();
#else
    if (isCurrentlyModal())
    {
        if (auto* content { getContentComponent() })
            content->setLookAndFeel (nullptr);

        exitModalState (0);
    }
#endif
}

bool ModalWindow::isActive() const noexcept
{
#if JUCE_MAC
    return sheet != nullptr and sheet->isActive();
#else
    return isVisible();
#endif
}

/**_____________________________END_OF_NAMESPACE______________________________*/
} /** namespace jam */
