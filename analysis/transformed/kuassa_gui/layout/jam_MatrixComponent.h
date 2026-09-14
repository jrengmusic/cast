/**
 * @file jam_MatrixComponent.h
 * @brief Binary-space-partition pane graph — split, resize, swap, and join panes.
 */
#pragma once

namespace jam
{ /*____________________________________________________________________________*/

/** @brief Binary-space-graph owner strategy. Every child SPACE is either a
 *  PANE row (an OwnedComponent leaf, minted by createChild) or reachable
 *  through an EDGE row (jam::PaneEdge) whose head/tail name the two SPACEs
 *  either side of the seam. The graph is held flat as sibling rows on the
 *  owner's own state — there is no nested tree structure, only head/tail
 *  UUID references between rows. The root is whichever row no EDGE
 *  references from either its head or its tail.
 */
class MatrixComponent : public OwnerComponent
{
public:
    /**
     * @brief Build-or-adopt: forwards to OwnerComponent's build-or-adopt constructor.
     * @param model       Owning model.
     * @param parentState Tree to search/append the row under.
     * @param type        Row type identifier to match or create.
     * @param uuid        Identity to match or assign to the row.
     */
    MatrixComponent (Model& model,
                     juce::ValueTree parentState,
                     const juce::Identifier& type,
                     UUID uuid);

    /**
     * @brief Adopt: binds directly to an already-existing row.
     * @param model          Owning model.
     * @param existingState  Row to bind to.
     */
    MatrixComponent (Model& model, juce::ValueTree existingState);
    ~MatrixComponent() override = default;

    /** @brief Splits the focused pane along edge's axis: mints a new pane,
     *  mounts it as a child, constructs the EDGE row between the new pane
     *  and the focused pane, and rewrites whichever slot referenced the
     *  focused pane (its parent EDGE's head or tail, when one exists) to
     *  the new EDGE instead.
     *  @param edge     Split side — determines axis and which of the new
     *                  pane / focused pane leads the EDGE's head slot.
     *  @param position Initial cut position along the split axis, 0..1.
     *  @return Identity of the newly minted pane.
     */
    UUID split (const juce::Identifier& edge, float position = 0.5f);

    /** @brief Shrinks pane along axis by proportions, distributed across the
     *  nearest bounding EDGE rows found by walking the ancestor chain: the
     *  nearest EDGE on the axis reached via a tail link (low seam) and the
     *  nearest reached via a head link (high seam) each absorb an equal
     *  share of the shift.
     *  @param pane        Pane whose extent is being reduced.
     *  @param axis        Axis to reduce along — Id::width or Id::height.
     *  @param proportions Total shift to distribute across the bounding seams.
     */
    void reducePane (UUID pane, const juce::Identifier& axis, float proportions);

    /** @brief Grows pane along axis by proportions — the inverse shift of
     *  reducePane(), same ancestor-walk distribution.
     *  @param pane        Pane whose extent is being expanded.
     *  @param axis        Axis to expand along — Id::width or Id::height.
     *  @param proportions Total shift to distribute across the bounding seams.
     */
    void expandPane (UUID pane, const juce::Identifier& axis, float proportions);

    /**
     * @brief Finds the pane adjacent to @p pane in the given direction.
     * @param pane       Pane to search from.
     * @param direction  Direction to search in (map::Position value).
     * @return Identity of the neighboring pane, or UUID::none() when there is none.
     */
    UUID getNeighbor (UUID pane, const juce::Identifier& direction);

    /**
     * @brief Swaps the positions of two panes in the graph.
     * @param a  First pane.
     * @param b  Second pane.
     */
    void swap (UUID a, UUID b);

    /**
     * @brief Removes @p pane's neighbor in the given direction, giving its
     *        space to @p pane.
     * @param pane       Pane absorbing the neighbor's space.
     * @param direction  Direction of the neighbor to remove (map::Position value).
     * @return Identity of the removed neighbor, or UUID::none() when there is
     *         no neighbor in that direction or the graph shape is unsupported.
     */
    UUID join (UUID pane, const juce::Identifier& direction);

protected:
    void childAdded (UUID uuid) override;

    /** @brief Graph collapse on child removal: the child's parent EDGE row
     *  dies, and the sibling SPACE across that EDGE inherits the slot the
     *  dead EDGE itself occupied on the grandparent EDGE (head or tail,
     *  whichever referenced the dead EDGE).
     */
    void childRemoved (UUID uuid) override;

    /** @brief Full re-layout from the root: derives root as the one row no
     *  EDGE references, then recurses — an EDGE row receives the current
     *  region as its own bounds and cuts it at its proportions along its
     *  orientation, recursing into head with the cut piece and tail with
     *  the rest; a PANE row receives the current region as setBounds().
     */
    void layout() override;

    /**
     * @brief Factory hook minting a new PANE leaf component.
     * @param uuid  Identity to assign to the new pane.
     * @return The newly constructed pane component.
     */
    virtual std::unique_ptr<OwnedComponent> createChild (UUID uuid) = 0;

private:
    /** @brief The EDGE row whose head or tail names uuid, or an invalid
     *  tree when uuid is the root (no EDGE references it).
     */
    juce::ValueTree getParent (UUID uuid);

    /** @brief Walks the ancestor chain from pane, returning the nearest EDGE
     *  row whose orientation matches direction's axis and whose head/tail
     *  slot places pane on the low/high side matching direction — i.e. the
     *  EDGE across which the neighbor in that direction is found.
     *  @param pane       Pane to search from.
     *  @param direction  Direction to search in (map::Position value).
     *  @return The bounding EDGE row, or an invalid tree when there is none.
     */
    juce::ValueTree getParent (UUID pane, const juce::Identifier& direction);

    /** @return Id::head or Id::tail — whichever property of row names uuid. */
    juce::Identifier getPropertyId (const juce::ValueTree& row, UUID uuid);
    /** @return The identity on row's other slot (head/tail) from the one naming uuid. */
    UUID sibling (const juce::ValueTree& row, UUID uuid);

    /** @return Identity of the row no EDGE references from either its head or tail. */
    UUID getRoot();

    /** @return uuid unchanged if it names a PANE row; otherwise recurses into
     *  the EDGE row's head until a PANE row is reached.
     */
    UUID getFirstPane (UUID uuid);

protected:
    /** @brief True when key's own low endpoint ("left" for a vertical key,
     *  "top" for a horizontal key) equals key's own map::Position property —
     *  i.e. key already names the low endpoint of its axis. Protected so
     *  derived owners (jam::TabView) can reuse it against a Position key
     *  read from their own state.
     */
    static bool isLow (int position) noexcept;

private:
    /** @brief Recursive descent used by layout(): dispatches on row type —
     *  EDGE rows set their own bounds to region, cut region at proportions
     *  along orientation, and recurse into head/tail with the two pieces;
     *  PANE rows terminate the recursion with setBounds (region).
     */
    void layout (UUID uuid, juce::Rectangle<int> region);

    /** @brief Promotes child into parent's slot on the grandparent EDGE, then
     *  demotes parent into child's other (non-shared) slot — a tree rotation
     *  used by join() when both sides of the removed target's EDGE are
     *  themselves EDGE rows of the same shape.
     *  @param parent  EDGE row being replaced by child.
     *  @param child   EDGE row promoted to take parent's place.
     */
    void rotate (juce::ValueTree parent, juce::ValueTree child);

    /** @brief Clamps and writes proportions on the given EDGE row, keeping
     *  both sides at or above PaneComponent::minimumPaneExtent.
     *  @param row   EDGE row whose proportions is being shifted.
     *  @param delta Signed shift applied to the current proportions.
     */
    void shiftSeam (juce::ValueTree row, float delta);

    /** @brief Live jam::PaneEdge components, one per EDGE row. */
    juce::OwnedArray<PaneEdge> bars;
    /** @brief Dispatches, by map::Position axis key, the region-cut function used by layout(). */
    Function::Map<int, juce::Rectangle<float>> splits {
        []
        {
            Function::Map<int, juce::Rectangle<float>> s;
            s.add<juce::Rectangle<float>&, float> (map::Position::left,
                                                   [] (juce::Rectangle<float>& region, float position)
                                                   {
                                                       return region.removeFromLeft (position * region.getWidth());
                                                   });
            s.add<juce::Rectangle<float>&, float> (map::Position::top,
                                                   [] (juce::Rectangle<float>& region, float position)
                                                   {
                                                       return region.removeFromTop (position * region.getHeight());
                                                   });
            return s;
        }()
    };

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MatrixComponent)
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
