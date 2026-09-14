/**
 * @file        jam_OwnedComponent.h
 * @brief       Ownable child contract — a state-bound component that
 *              self-reports keyboard focus onto its own row.
 */
#pragma once

namespace jam
{ /*____________________________________________________________________________*/

/** @brief Ownable child contract: a state-bound component
 *  (Model::Component<OwnedComponent> CRTP mixin) that self-reports keyboard
 *  focus onto its own row's Id::focus. Owners aggregate; the child
 *  stays dumb.
 */
class OwnedComponent
    : public juce::Component
    , public jam::Model::Component<OwnedComponent>
{
public:
    /** @brief Build-or-adopt: finds an existing row of type/uuid under
     *  parentState, or creates and appends one when absent.
     *  @param model       Owning model; source of the Id::focus parameter.
     *  @param parentState Tree to search/append the row under.
     *  @param type        Row type identifier to match or create.
     *  @param uuid        Identity to match or assign to the row.
     */
    OwnedComponent (jam::Model& model,
                    juce::ValueTree parentState,
                    const juce::Identifier& type,
                    jam::UUID uuid)
        : jam::Model::Component<OwnedComponent> (model, parentState, type, uuid)
    {
        setOpaque (false);

        model.createAndAddParameter<jam::Parameter<int>> (state, Id::focus, 0);
        model.createAndAddParameter<jam::Parameter<jam::Bounds>> (state, Id::bounds, jam::Bounds { 0, 0, 0, 0 });
    }

    /** @brief Adopt: binds directly to an already-existing row.
     *  @param model         Owning model; source of the Id::focus parameter.
     *  @param existingState Row to bind to.
     */
    OwnedComponent (jam::Model& model, juce::ValueTree existingState)
        : jam::Model::Component<OwnedComponent> (model, existingState)
    {
        setOpaque (false);

        model.createAndAddParameter<jam::Parameter<int>> (state, Id::focus, 0);
        model.createAndAddParameter<jam::Parameter<jam::Bounds>> (state, Id::bounds, jam::Bounds { 0, 0, 0, 0 });
    }

    ~OwnedComponent() override = default;

    // Bounds self-report on own row. resized/moved are paired because juce
    // fires them independently (a swap can move without resizing) — derived
    // overrides MUST call this base or the publish dies silently.
    void resized() override { state.setProperty (Id::bounds, jam::bit_cast<int64_t> (jam::Bounds { getBounds() }), nullptr); }
    void moved() override { state.setProperty (Id::bounds, jam::bit_cast<int64_t> (jam::Bounds { getBounds() }), nullptr); }

private:
    /** @brief Writes 1 to own row's Id::focus and repaints. */
    void focusGained (FocusChangeType) override
    {
        state.setProperty (Id::focus, 1, nullptr);
        repaint();
    }
    /** @brief Writes 0 to own row's Id::focus and repaints. */
    void focusLost (FocusChangeType) override
    {
        state.setProperty (Id::focus, 0, nullptr);
        repaint();
    }

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OwnedComponent)
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
