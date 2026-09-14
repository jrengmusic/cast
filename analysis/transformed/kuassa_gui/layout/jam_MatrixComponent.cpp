namespace jam
{ /*____________________________________________________________________________*/

MatrixComponent::MatrixComponent (Model& model,
                                  juce::ValueTree parentState,
                                  const juce::Identifier& type,
                                  UUID uuid)
    : OwnerComponent (model, parentState, type, uuid, Id::focusedPane)
{
}

MatrixComponent::MatrixComponent (Model& model, juce::ValueTree existingState)
    : OwnerComponent (model, existingState, Id::focusedPane)
{
}

//==============================================================================
juce::ValueTree MatrixComponent::getParent (UUID uuid)
{
    auto parent { juce::ValueTree {} };

    for (auto row : state)
        if (row.getType() == Id::toType (Id::edge)
            and (UUID { row.getProperty (Id::head) } == uuid
                 or UUID { row.getProperty (Id::tail) } == uuid))
            parent = row;

    return parent;
}

juce::ValueTree MatrixComponent::getParent (UUID pane, const juce::Identifier& direction)
{
    const auto side { map::Position::getInstance()->get (direction.toString()) };
    auto current { pane };

    for (auto parent { getParent (current) }; parent.isValid(); parent = getParent (current))
    {
        if ((parent.getProperty (Id::orientation) == map::Orientation::getInstance()->get (map::Orientation::vertical))
                == (side == map::Position::right or side == map::Position::left)
            and (getPropertyId (parent, current) == Id::head) != isLow (side))
            return parent;

        current = UUID { parent.getProperty (Id::id) };
    }

    return {};
}

//==============================================================================
juce::Identifier MatrixComponent::getPropertyId (const juce::ValueTree& row, UUID uuid)
{
    return UUID { row.getProperty (Id::head) } == uuid ? Id::head : Id::tail;
}

bool MatrixComponent::isLow (int position) noexcept
{
    const bool vertical { position == map::Position::right or position == map::Position::left };
    return map::Position::getInstance()->get (position) == map::Position::getInstance()->get (vertical ? map::Position::left : map::Position::top);
}

UUID MatrixComponent::sibling (const juce::ValueTree& row, UUID uuid)
{
    const auto own { getPropertyId (row, uuid) };
    return UUID { row.getProperty (own == Id::head ? Id::tail : Id::head) };
}

//==============================================================================
UUID MatrixComponent::split (const juce::Identifier& edge, float position)
{
    const auto pane { getFocusedChild() };
    const auto side { map::Position::getInstance()->get (edge.toString()) };
    const UUID newPane {};

    add (newPane, createChild (newPane));

    auto parent { getParent (pane) };

    const bool newLeads { isLow (side) };

    bars.add (std::make_unique<PaneEdge> (
        model,
        state,
        newLeads ? newPane : pane,
        newLeads ? pane : newPane,
        map::Orientation::getInstance()->get ((side == map::Position::right or side == map::Position::left) ? map::Orientation::vertical
                                                                                           : map::Orientation::horizontal),
        position));
    addAndMakeVisible (*bars.getLast());

    if (parent.isValid())
    {
        const UUID newEdge { bars.getLast()->getValueTree().getProperty (Id::id) };

        parent.setProperty (getPropertyId (parent, pane), newEdge.value, nullptr);
    }

    layout();

    return newPane;
}

//==============================================================================
void MatrixComponent::reducePane (UUID pane, const juce::Identifier& axis, float proportions)
{
    const bool vertical { axis == Id::width };
    auto current { pane };
    juce::ValueTree low {};
    juce::ValueTree high {};

    for (auto parent { getParent (current) }; parent.isValid(); parent = getParent (current))
    {
        const bool edgeVertical { parent.getProperty (Id::orientation) == map::Orientation::getInstance()->get (map::Orientation::vertical) };
        const bool fromTail { UUID { parent.getProperty (Id::tail) } == current };

        if (edgeVertical == vertical)
        {
            auto& bound { fromTail ? low : high };

            if (not bound.isValid())
                bound = parent;
        }

        current = UUID { parent.getProperty (Id::id) };
    }

    const auto count { static_cast<int> (low.isValid()) + static_cast<int> (high.isValid()) };

    if (low.isValid())
        shiftSeam (low, proportions / static_cast<float> (count));

    if (high.isValid())
        shiftSeam (high, -proportions / static_cast<float> (count));
}

void MatrixComponent::shiftSeam (juce::ValueTree row, float delta)
{
    const UUID id { row.getProperty (::Id::id) };
    auto* bar { findChildWithID (id.toString()) };

    const bool vertical { row.getProperty (::Id::orientation) == map::Orientation::getInstance()->get (map::Orientation::vertical) };
    const auto extent { static_cast<float> (vertical ? bar->getWidth() : bar->getHeight()) };
    const auto minimum { static_cast<float> (PaneComponent::minimumPaneExtent) / extent };

    row.setProperty (
        ::Id::proportions,
        juce::jlimit (minimum,
                      1.0f - minimum,
                      static_cast<float> (row.getProperty (::Id::proportions)) + delta),
        nullptr);
}

void MatrixComponent::expandPane (UUID pane, const juce::Identifier& axis, float proportions)
{
    reducePane (pane, axis, -proportions);
}

//==============================================================================
UUID MatrixComponent::getNeighbor (UUID pane, const juce::Identifier& direction)
{
    const bool paneInHead { not isLow (map::Position::getInstance()->get (direction.toString())) };
    const auto parent { getParent (pane, direction) };

    if (not parent.isValid())
        return UUID::none();

    auto current { pane };
    float position { 0.5f };

    for (auto row { getParent (current) }; row.isValid() and row != parent; row = getParent (current))
    {
        if (row.getProperty (Id::orientation) != parent.getProperty (Id::orientation))
        {
            const auto proportions { static_cast<float> (row.getProperty (Id::proportions)) };

            position = getPropertyId (row, current) == Id::head
                           ? position * proportions
                           : proportions + position * (1.0f - proportions);
        }

        current = UUID { row.getProperty (Id::id) };
    }

    auto space { UUID { parent.getProperty (paneInHead ? Id::tail : Id::head) } };

    while (not getChildren().contains (space))
    {
        auto row { state.getChildWithProperty (Id::id, space.value) };
        const bool sameOrientation { row.getProperty (Id::orientation) == parent.getProperty (Id::orientation) };

        if (sameOrientation)
        {
            space = UUID { row.getProperty (paneInHead ? Id::head : Id::tail) };
        }
        else
        {
            const auto proportions { static_cast<float> (row.getProperty (Id::proportions)) };

            if (position < proportions)
            {
                space = UUID { row.getProperty (Id::head) };
                position = position / proportions;
            }
            else
            {
                space = UUID { row.getProperty (Id::tail) };
                position = (position - proportions) / (1.0f - proportions);
            }
        }
    }

    return space;
}

//==============================================================================
void MatrixComponent::swap (UUID a, UUID b)
{
    auto parentA { getParent (a) };
    auto parentB { getParent (b) };

    const auto propertyA { getPropertyId (parentA, a) };
    const auto propertyB { getPropertyId (parentB, b) };

    parentA.setProperty (propertyA, b.value, nullptr);
    parentB.setProperty (propertyB, a.value, nullptr);

    layout();
}

//==============================================================================
UUID MatrixComponent::join (UUID pane, const juce::Identifier& direction)
{
    const auto target { getNeighbor (pane, direction) };

    if (target == UUID::none())
        return UUID::none();

    auto parent { getParent (pane, direction) };
    const bool paneInHead { not isLow (map::Position::getInstance()->get (direction.toString())) };
    const UUID own { parent.getProperty (paneInHead ? Id::head : Id::tail) };
    const UUID other { parent.getProperty (paneInHead ? Id::tail : Id::head) };

    auto current { other };
    auto row { state.getChildWithProperty (Id::id, current.value) };
    float proportions { 1.0f };
    bool sameOrientation { true };

    while (sameOrientation and not getChildren().contains (current))
    {
        sameOrientation = row.getProperty (Id::orientation) == parent.getProperty (Id::orientation);

        if (sameOrientation)
        {
            proportions *= paneInHead ? static_cast<float> (row.getProperty (Id::proportions))
                                       : 1.0f - static_cast<float> (row.getProperty (Id::proportions));
            current = UUID { row.getProperty (paneInHead ? Id::head : Id::tail) };
            row = state.getChildWithProperty (Id::id, current.value);
        }
    }

    auto ownRow { state.getChildWithProperty (Id::id, own.value) };
    auto otherRow { state.getChildWithProperty (Id::id, other.value) };

    if (getParent (target) == parent)
    {
        remove (target);
    }
    else if (own == pane and sameOrientation and current == target)
    {
        const auto p { static_cast<float> (parent.getProperty (Id::proportions)) };
        float region { paneInHead ? 1.0f - p : p };

        proportions *= region;

        for (auto row { otherRow };
             UUID { row.getProperty (paneInHead ? Id::head : Id::tail) } != target;
             row = state.getChildWithProperty (Id::id, row.getProperty (paneInHead ? Id::head : Id::tail)))
        {
            const float head { region * static_cast<float> (row.getProperty (Id::proportions)) };
            const float tail { region - head };

            row.setProperty (Id::proportions,
                             paneInHead ? (head - proportions) / (region - proportions)
                                        : 1.0f - (tail - proportions) / (region - proportions),
                             nullptr);

            region = paneInHead ? head : tail;
        }

        remove (target);
        parent.setProperty (Id::proportions, paneInHead ? p + proportions : p - proportions, nullptr);
    }
    else if (not getChildren().contains (own)
             and ownRow.getProperty (Id::orientation) != parent.getProperty (Id::orientation)
             and not getChildren().contains (other)
             and otherRow.getProperty (Id::orientation) != parent.getProperty (Id::orientation)
             and UUID { otherRow.getProperty (getPropertyId (ownRow, pane)) } == target
             and ownRow.getProperty (Id::proportions) == otherRow.getProperty (Id::proportions))
    {
        remove (target);
        rotate (parent, ownRow);
    }
    else
    {
        return UUID::none();
    }

    layout();

    return target;
}

//==============================================================================
void MatrixComponent::rotate (juce::ValueTree parent, juce::ValueTree child)
{
    const UUID parentId { parent.getProperty (Id::id) };
    const UUID childId { child.getProperty (Id::id) };
    const auto own { getPropertyId (parent, childId) };
    const auto other { own == Id::head ? Id::tail : Id::head };
    auto grandparent { getParent (parentId) };

    if (grandparent.isValid())
        grandparent.setProperty (getPropertyId (grandparent, parentId), childId.value, nullptr);

    parent.setProperty (own, UUID { child.getProperty (other) }.value, nullptr);
    child.setProperty (other, parentId.value, nullptr);
}

//==============================================================================
void MatrixComponent::childAdded (UUID uuid)
{
    get (uuid).setVisible (true);
    resized();
}

void MatrixComponent::childRemoved (UUID uuid)
{
    auto parent { getParent (uuid) };

    if (parent.isValid())
    {
        const UUID sib { sibling (parent, uuid) };
        const UUID parentId { parent.getProperty (Id::id) };
        auto grandparent { getParent (parentId) };

        if (grandparent.isValid())
            grandparent.setProperty (getPropertyId (grandparent, parentId), sib.value, nullptr);

        if (auto* bar { findChildWithID (parentId.toString()) })
            bars.removeObject (static_cast<PaneEdge*> (bar));

        state.removeChild (parent, nullptr);

        if (uuid == getFocusedChild())
            setFocusedChild (getFirstPane (sib));
    }

    resized();
}

//==============================================================================
UUID MatrixComponent::getRoot()
{
    UUID root { UUID::none() };

    for (auto row : state)
    {
        const UUID id { row.getProperty (::Id::id) };

        if (not getParent (id).isValid())
            root = id;
    }

    return root;
}

UUID MatrixComponent::getFirstPane (UUID uuid)
{
    auto row { state.getChildWithProperty (Id::id, uuid.value) };

    return row.getType() == Id::toType (Id::edge) ? getFirstPane (UUID { row.getProperty (Id::head) }) : uuid;
}

//==============================================================================
void MatrixComponent::layout()
{
    const auto root { getRoot() };

    if (root != UUID::none())
        layout (root, getLocalBounds());
}

void MatrixComponent::layout (UUID uuid, juce::Rectangle<int> region)
{
    auto row { state.getChildWithProperty (Id::id, uuid.value) };

    if (row.getType() == Id::toType (Id::edge))
    {
        if (auto* bar { findChildWithID (uuid.toString()) })
        {
            bar->setBounds (region);
            bar->repaint();
        }

        const bool vertical { row.getProperty (Id::orientation) == map::Orientation::getInstance()->get (map::Orientation::vertical) };
        auto tail { region.toFloat() };
        const auto head { splits.get (
            vertical ? map::Position::left : map::Position::top,
            tail,
            static_cast<float> (row.getProperty (Id::proportions))) };

        layout (UUID { row.getProperty (Id::head) }, head.toNearestInt());
        layout (UUID { row.getProperty (Id::tail) }, tail.toNearestInt());
    }
    else
    {
        get (uuid).setBounds (region);
    }
}

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
