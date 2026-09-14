/**
 * @file        jam_PaneEdge.h
 * @brief       EDGE row component — the seam between two SPACEs in a binary
 *              space graph.
 */
#pragma once

namespace jam
{ /*____________________________________________________________________________*/

/** @brief EDGE row component. head and tail are UUIDs, each naming a SPACE —
 *  either a PANE row or another EDGE row — so the binary space graph is held
 *  flat in the ValueTree rather than nested. Registers head, tail,
 *  orientation, and proportions on its own row. Component identity equals
 *  the EDGE row's UUID, which is also the seam's identity.
 */
class PaneEdge : public OwnedComponent
{
public:
    /** @brief Builds a new EDGE row under parentState and registers its
     *  head/tail/orientation/proportions parameters.
     *  @param model          Owning model; source of the registered parameters.
     *  @param parentState    Tree to append the new EDGE row under.
     *  @param newHead        Identity of the SPACE occupying the head slot.
     *  @param newTail        Identity of the SPACE occupying the tail slot.
     *  @param newOrientation Split axis — vertical or horizontal.
     *  @param newProportions Initial cut position along the split axis, 0..1.
     */
    PaneEdge (Model& model,
             juce::ValueTree parentState,
             UUID newHead,
             UUID newTail,
             const juce::String& newOrientation,
             float newProportions)
        : OwnedComponent (model, parentState, Id::toType (Id::edge), UUID {})
    {
        model.createAndAddParameter<Parameter<UUID>> (state, Id::head, newHead);
        model.createAndAddParameter<Parameter<UUID>> (state, Id::tail, newTail);
        model.createAndAddParameter<ParameterText> (state, Id::orientation, newOrientation);
        model.createAndAddParameter<Parameter<float>> (state, Id::proportions, newProportions);

        setRepaintsOnMouseActivity (true);
        setAlwaysOnTop (true);
    }

    /** @brief Derives the hit region from proportions × bounds, expanded to
     *  the active LookAndFeel's edge thickness along the split axis.
     *  @return Local-space rectangle the mouse must land in to drag the seam.
     */
    juce::Rectangle<int> getSeam() const
    {
        const auto thickness { static_cast<StyleCustom&> (getLookAndFeel()).getPaneEdgeSize() };
        const auto line { juce::Point<int> { juce::roundToInt (getProportions() * static_cast<float> (getWidth())),
                                             juce::roundToInt (getProportions() * static_cast<float> (getHeight())) } };

        return isVertical() ? getLocalBounds().withX (line.getX()).withWidth (0).expanded (thickness / 2, 0)
                            : getLocalBounds().withY (line.getY()).withHeight (0).expanded (0, thickness / 2);
    }

    /** @brief Narrows hit-testing to the seam strip, per getSeam(). */
    bool hitTest (int x, int y) override { return getSeam().contains (x, y); }

    /** @brief Delegates seam rendering to the active LookAndFeel. */
    void paint (juce::Graphics& g) override
    {
        static_cast<StyleCustom&> (getLookAndFeel()).drawPaneEdge (g, *this);
    }

    /** @brief Identity of the SPACE occupying the head slot. */
    UUID getHead() const { return UUID { state.getProperty (Id::head) }; }
    /** @brief Identity of the SPACE occupying the tail slot. */
    UUID getTail() const { return UUID { state.getProperty (Id::tail) }; }
    /** @brief Current cut position along the split axis, 0..1. */
    float getProportions() const { return static_cast<float> (state.getProperty (Id::proportions)); }
    /** @brief True when the split axis is vertical (head/tail sit left/right of the seam). */
    bool isVertical() const { return state.getProperty (Id::orientation) == map::Orientation::getInstance()->get (map::Orientation::vertical); }

    /** @brief Re-derives the resize cursor from orientation after every
     *  bounds publish.
     */
    void resized() override
    {
        OwnedComponent::resized();
        setMouseCursor (isVertical() ? juce::MouseCursor::LeftRightResizeCursor
                                     : juce::MouseCursor::UpDownResizeCursor);
    }

    /** @brief Anchors the drag reference point to the seam's current centre. */
    void mouseDown (const juce::MouseEvent&) override
    {
        mouseDownPos = getSeam().getCentre();
    }

    /** @brief Constrains the drag offset within minimumPaneExtent of both
     *  edges, then writes only Id::proportions — the owner's listener
     *  reacts and re-runs layout; this component never lays itself out.
     */
    void mouseDrag (const juce::MouseEvent& event) override
    {
        const auto line { getLocalBounds().reduced (PaneComponent::minimumPaneExtent)
                              .getConstrainedPoint (mouseDownPos + event.getOffsetFromDragStart()) };
        const auto proportions { isVertical() ? static_cast<float> (line.getX()) / static_cast<float> (getWidth())
                                              : static_cast<float> (line.getY()) / static_cast<float> (getHeight()) };

        state.setProperty (Id::proportions, proportions, nullptr);
    }

private:
    /** @brief Seam centre captured on mouseDown — drag offset origin. */
    juce::Point<int> mouseDownPos {};

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PaneEdge)
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
