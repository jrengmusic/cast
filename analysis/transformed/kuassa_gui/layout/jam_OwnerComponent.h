/**
 * @file        jam_OwnerComponent.h
 * @brief       Composite owner contract — an OwnedComponent that owns
 *              UUID-keyed children and aggregates their focus self-reports.
 */
#pragma once

namespace jam
{ /*____________________________________________________________________________*/

/** @brief Composite owner contract: an OwnedComponent that owns UUID-keyed
 *  children (component + Model::Attachment per child), aggregates child
 *  Id::focus self-reports into one focused-child parameter on its own
 *  row (identifier supplied by the derived strategy), and delegates
 *  machinery and geometry to the derived strategy. The owner is the layer's
 *  only ValueTree listener.
 */
class OwnerComponent
    : public OwnedComponent
    , protected juce::ValueTree::Listener
{
public:
    /** @brief Build-or-adopt: forwards to OwnedComponent's build-or-adopt
     *  ctor, then seeds the focused-child parameter and starts listening.
     *  @param model          Owning model; source of the focused-child parameter.
     *  @param parentState    Tree to search/append the row under.
     *  @param type           Row type identifier to match or create.
     *  @param uuid           Identity to match or assign to the row.
     *  @param focusedChildId Property identifier for the aggregated focused-child parameter.
     */
    OwnerComponent (jam::Model& model,
                    juce::ValueTree parentState,
                    const juce::Identifier& type,
                    jam::UUID uuid,
                    const juce::Identifier& focusedChildId)
        : OwnedComponent (model, parentState, type, uuid)
        , focusedChildId (focusedChildId)
    {
        model.createAndAddParameter<jam::Parameter<jam::UUID>> (
            state, focusedChildId, jam::UUID::none());
        state.addListener (this);
    }

    /** @brief Adopt: forwards to OwnedComponent's adopt ctor, then seeds the
     *  focused-child parameter and starts listening.
     *  @param model          Owning model; source of the focused-child parameter.
     *  @param existingState  Row to bind to.
     *  @param focusedChildId Property identifier for the aggregated focused-child parameter.
     */
    OwnerComponent (jam::Model& model,
                    juce::ValueTree existingState,
                    const juce::Identifier& focusedChildId)
        : OwnedComponent (model, existingState)
        , focusedChildId (focusedChildId)
    {
        model.createAndAddParameter<jam::Parameter<jam::UUID>> (
            state, focusedChildId, jam::UUID::none());
        state.addListener (this);
    }

    /** @brief Stops listening to own row. */
    ~OwnerComponent() override { state.removeListener (this); }

    /** @brief Emplaces the child, attaches a Model::Attachment to it, adds
     *  it as a juce child component, stamps the focused-child parameter
     *  with the new child's identity, then runs the childAdded hook.
     *  @param uuid  Identity to key the child under.
     *  @param child Child component to take ownership of.
     */
    void add (jam::UUID uuid, std::unique_ptr<OwnedComponent> child)
    {
        auto [entry, inserted] { children.try_emplace (uuid, std::move (child)) };
        jassert (inserted and "duplicate child uuid in add()");

        auto& [key, owned] { *entry };
        addChildComponent (*owned);
        attachments.try_emplace (uuid, std::make_unique<jam::Model::Attachment> (*owned));

        state.setProperty (focusedChildId, uuid.value, nullptr);

        childAdded (uuid);
    }

    /** @brief Runs the childRemoved hook, then erases the child from both
     *  the component and attachment maps. The state row is NOT removed —
     *  row removal is the consumer's explicit verb.
     *  @param uuid  Identity of the child to remove.
     */
    void remove (jam::UUID uuid)
    {
        jassert (children.contains (uuid) and "no child with this uuid");

        childRemoved (uuid);
        removeChildComponent (children.at (uuid).get());
        attachments.erase (uuid);
        children.erase (uuid);
    }

    /** @brief Asserted O(1) lookup of an owned child.
     *  @param uuid  Identity of the child to retrieve.
     *  @return Reference to the child component.
     */
    OwnedComponent& get (jam::UUID uuid)
    {
        jassert (children.contains (uuid) and "no child with this uuid");
        return *children.at (uuid);
    }

    /** @brief Reads the aggregated focused-child parameter from own row.
     *  @return Identity of the currently focused child.
     */
    jam::UUID getFocusedChild() const
    {
        return jam::UUID { state.getProperty (focusedChildId) };
    }

    /** @brief Writes the aggregated focused-child parameter on own row. The
     *  valueTreePropertyChanged listener reacts to the write and runs
     *  layout — this call does not run layout or touch keyboard focus itself.
     *  @param uuid  Identity of the child to focus.
     */
    void setFocusedChild (jam::UUID uuid)
    {
        state.setProperty (focusedChildId, uuid.value, nullptr);
    }

    /** @brief Number of children currently owned. */
    int getChildCount() const noexcept { return static_cast<int> (children.size()); }

    // Owner IS owned — publishes its own bounds, then projects children.
    void resized() override
    {
        OwnedComponent::resized();
        layout();
    }

protected:
    /** @brief Machinery hook run after a child has been emplaced.
     *  @param uuid  Identity of the newly added child.
     */
    virtual void childAdded (jam::UUID uuid) = 0;
    /** @brief Machinery hook run before a child is erased.
     *  @param uuid  Identity of the child about to be removed.
     */
    virtual void childRemoved (jam::UUID uuid) = 0;
    /** @brief Geometry strategy: full projection of children. */
    virtual void layout() = 0;

    /** @brief Three reactions on own subtree: a direct child row's
     *  Id::focus becoming 1 self-reports that child's identity into the
     *  focused-child parameter; own focused-child parameter changing runs
     *  resized(); a direct child row's Id::proportions changing also
     *  runs resized().
     *  @param tree      Row whose property changed.
     *  @param property  Identifier of the changed property.
     */
    void valueTreePropertyChanged (juce::ValueTree& tree, const juce::Identifier& property) override
    {
        if (property == Id::focus and tree.getParent() == state
            and static_cast<int> (tree.getProperty (Id::focus)) == 1)
        {
            state.setProperty (focusedChildId, tree.getProperty (Id::id), nullptr);
        }

        if (property == focusedChildId and tree == state)
        {
            resized();
        }

        if (property == Id::proportions and tree.getParent() == state)
        {
            resized();
        }
    }

    /** @brief Read-only view of the owned children for derived iteration and
     *  queries — map mutation stays behind add()/remove().
     */
    const jam::HashMap<jam::UUID, std::unique_ptr<OwnedComponent>>& getChildren() const noexcept
    {
        return children;
    }

private:
    /** @brief Owned children, keyed by identity. */
    jam::HashMap<jam::UUID, std::unique_ptr<OwnedComponent>> children;
    /** @brief Per-child Model::Attachment, keyed by identity. */
    jam::HashMap<jam::UUID, std::unique_ptr<jam::Model::Attachment>> attachments;
    /** @brief Property identifier for the aggregated focused-child parameter. */
    const juce::Identifier focusedChildId;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OwnerComponent)
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
