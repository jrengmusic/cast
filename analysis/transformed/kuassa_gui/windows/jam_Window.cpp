/**
 * @file jam_Window.cpp
 * @brief Implementation of Window — juce::DocumentWindow wrapper with LookAndFeel-owned glass.
 *
 * lookAndFeelChanged() recreates title-bar buttons via the base DocumentWindow
 * behaviour, then dispatches to the active StyleCustom LookAndFeel's
 * prepareWindow() to derive and apply chrome + tint + blur, once the native
 * peer exists; visibilityChanged() dispatches the same way once the window
 * becomes visible with a peer in place. setStyle() applies the resolved
 * colour/blur/backend values to this window. setWindowButtons() toggles
 * native traffic-light buttons (macOS) or native title bar (Windows).
 *
 * @see jam_window.h
 * @see BackgroundBlur
 * @see StyleWindow
 */

namespace jam
{
/*____________________________________________________________________________*/

Window::Window (juce::Component* mainComponent, juce::String const& name, bool alwaysOnTop, bool windowButtons)
    : juce::DocumentWindow (name,
                            juce::Colours::transparentBlack,
#if JUCE_WINDOWS
                            windowButtons ? juce::DocumentWindow::allButtons : 0)
#else
                            juce::DocumentWindow::allButtons)
#endif
    , windowButtons (windowButtons)
{
#if JUCE_WINDOWS
    setUsingNativeTitleBar (false);
    setDropShadowEnabled (false);
    setTitleBarHeight (windowButtons ? defaultTitleBarHeight : 0);
#else
    setUsingNativeTitleBar (true);
#endif
    setOpaque (false);
    setContentOwned (std::move (mainComponent), true);
    setAlwaysOnTop (alwaysOnTop);
#if JUCE_IOS || JUCE_ANDROID
    setFullScreen (true);
#elif JUCE_WINDOWS
    setResizable (true, false);
#else
    setResizable (true, true);
#endif
    setConstrainer (this);
    centreWithSize (getWidth(), getHeight());

#if JUCE_WINDOWS
    addMouseListener (this, true);
#endif

    lookAndFeelChanged();
}

Window::~Window()
{
    vulkanEngine.removePeer (*this);

    setLookAndFeel (nullptr);
}

void Window::closeButtonPressed() { juce::JUCEApplication::getInstance()->systemRequestedQuit(); }

// =============================================================================
// Style API
// =============================================================================

void Window::lookAndFeelChanged()
{
    DocumentWindow::lookAndFeelChanged();

    if (getPeer() != nullptr)
        if (auto* styleLaf { dynamic_cast<StyleCustom*> (&getLookAndFeel()) })
            styleLaf->prepareWindow (*this);
}

void Window::visibilityChanged()
{
    DocumentWindow::visibilityChanged();

    if (isVisible() and getPeer() != nullptr)
        if (auto* styleLaf { dynamic_cast<StyleCustom*> (&getLookAndFeel()) })
            styleLaf->prepareWindow (*this);
}

void Window::setStyle (juce::Colour colour, float blur, BackgroundBlur::WindowFX style)
{
    if (BackgroundBlur::shouldTintComponent (style))
        jam::StyleWindow::apply (this, juce::Colours::transparentBlack);
    else
        jam::StyleWindow::apply (this, colour);

    if (colour.getFloatAlpha() < 1.0f)
    {
        setOpaque (false);
        setBackgroundColour (BackgroundBlur::shouldTintComponent (style) ? colour : juce::Colours::transparentBlack);
    }
    else
    {
        setOpaque (true);
        setBackgroundColour (colour);
    }

    BackgroundBlur::enable (this, style, blur, colour);
}

void Window::setWindowButtons (bool shouldShow)
{
    windowButtons = shouldShow;

#if JUCE_MAC
    if (auto* peer { getPeer() })
        jam::StyleWindow::setButtons (*peer, shouldShow);
#elif JUCE_WINDOWS
    setTitleBarHeight (shouldShow ? defaultTitleBarHeight : 0);
#endif
}

// =============================================================================
// macOS: content inset
// =============================================================================
#if JUCE_MAC
juce::BorderSize<int> Window::getContentComponentBorder() const
{
    auto border { DocumentWindow::getContentComponentBorder() };

    if (windowButtons and getPeer() != nullptr)
        border.setTop (border.getTop() + StyleWindow::getTitleBarHeight (*getPeer()));

    return border;
}
#endif

// =============================================================================
// Windows: middle-click drag
// =============================================================================
#if JUCE_WINDOWS
void Window::mouseDown (const juce::MouseEvent& event)
{
    if (event.mods.isMiddleButtonDown())
        windowDragger.startDraggingComponent (this, event);
}

void Window::mouseDrag (const juce::MouseEvent& event)
{
    if (event.mods.isMiddleButtonDown())
        windowDragger.dragComponent (this, event, nullptr);
}
#endif

/**_____________________________END_OF_NAMESPACE______________________________*/
} /** namespace jam */
