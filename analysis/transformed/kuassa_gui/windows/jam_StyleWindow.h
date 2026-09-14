/**
 * @file jam_StyleWindow.h
 * @brief Native window chrome/tint/rounding helpers, applied per-platform.
 */
#pragma once

namespace jam::StyleWindow
{
/*____________________________________________________________________________*/

#if JUCE_MAC
inline constexpr float cornerRadius { 8.0f }; ///< Base CA layer corner radius applied by setMenu().
#endif


/**
 * @brief Applies window-level chrome and tint to component's native window.
 *
 * macOS: hides the title text, makes the title bar transparent, extends the
 * content view to full size, and sets the window opaque/background colour
 * from @p colour. Windows: sets the DWM window-corner-preference to rounded;
 * @p colour is unused.
 *
 * @param component  Component whose native window is affected.
 * @param colour     Tint colour with alpha (macOS only).
 * @return `true` on success; `false` if the peer or native window/handle could not be obtained.
 */
bool apply (juce::Component* component, juce::Colour colour);

/**
 * @brief Applies rounding and tint to component's native window or view, for popup menus.
 *
 * macOS: sets the view's CALayer corner radius (golden-ratio-scaled when
 * @p colour is translucent) and tints via the layer's background colour.
 * Windows: sets the DWM window-corner-preference to rounded, then applies
 * the accentEnableGradient accent policy tinted with @p colour.
 *
 * @param component  Component whose native window/view is affected.
 * @param colour     Tint colour with alpha.
 * @return `true` on success; `false` if the peer or native window/handle could not be obtained.
 */
bool setMenu (juce::Component* component, juce::Colour colour);

#if JUCE_MAC
/** @brief Toggles traffic-light button visibility on the native window. */
void setButtons (juce::ComponentPeer& peer, bool visible);

/** @brief Returns the title-bar height for the given peer's NSWindow in logical
 *  points, measured as the difference between the window frame height and the
 *  content layout rect height (frame.size.height - contentLayoutRect.size.height).
 *  @param peer  The component peer whose native NSWindow is queried.
 */
int getTitleBarHeight (const juce::ComponentPeer& peer);
#endif

/*____________________________END_OF_NAMESPACE________________________________*/
}
