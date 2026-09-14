namespace jam
{
/*____________________________________________________________________________*/
/** @brief Saved transform/fill state, pushed by saveState() and popped by
 *  restoreState() to keep this class's own copy in lockstep with the base
 *  class's own state stack.
 *
 *  fillColour already carries opacity in its own alpha channel (see
 *  setOpacity()), so no separate opacity field is kept.
 */
struct CachedState
{
    /** @brief Accumulated user-to-device transform at the point this state
     *  was saved. */
    juce::AffineTransform transform;

    /** @brief Current fill colour, alpha-inclusive, at the point this state
     *  was saved. */
    juce::Colour fillColour;

    /** @brief Device-space clip rectangle at the point this state was saved —
     *  populated only by the non-mac LowLevelGraphicsGlyphRenderer's own
     *  clipToRectangle()/clipToRectangleList() mirror. */
    juce::Rectangle<int> clip;
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
