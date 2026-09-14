namespace jam
{
/*____________________________________________________________________________*/

class Model::ParameterAdapter final
    : private ParameterBase::Listener
    , private juce::AsyncUpdater
{
public:
    ParameterAdapter (ParameterBase& param,
                      juce::ValueTree parameterTree,
                      Model::LockedListeners& modelListeners) noexcept
        : parameter (param)
        , tree (parameterTree)
        , listeners (modelListeners)
    {
        parameter.addListener (this);
    }

    ~ParameterAdapter() override
    {
        cancelPendingUpdate();
        parameter.removeListener (this);
    }

    // VT->atomic reverse sync. Called by Model::valueTreePropertyChanged.
    // Positive nesting: equality + loopback guard -> parameter.setValueFromVar.
    void setRawValue (const juce::var& newValue) noexcept
    {
        if (parameter.getValueAsVar() != newValue and not ignoreCallbacks)
            parameter.setValueFromVar (newValue);
    }

    // Restores parameter value from the bound VT tree.
    // Called by replaceState() directly, and by updateAdapterConnections()
    // for every adapter it rebinds -- atomics resync whenever an adapter's
    // tree reference changes, not only on replaceState.
    void restoreFromTree() noexcept { setRawValue (tree.getProperty (parameter.propertyId)); }

    // Flushes the parameter's current value to the VT property. CAS-gated.
    // Equality-against-tree gate. Loopback guard. Returns true if the
    // ValueTree was updated. MESSAGE THREAD only.
    bool flushToTree() noexcept
    {
        bool expected { true };

        if (needsUpdate.compare_exchange_strong (expected, false))
        {
            const auto currentValue { parameter.getValueAsVar() };

            if (auto* existingProperty { tree.getPropertyPointer (parameter.propertyId) })
            {
                if (*existingProperty != currentValue)
                {
                    const juce::ScopedValueSetter<bool> svs { ignoreCallbacks, true };
                    tree.setProperty (parameter.propertyId, currentValue, nullptr);
                }
            }
            else
            {
                tree.setProperty (parameter.propertyId, currentValue, nullptr);
            }

            return true;
        }

        return false;
    }

    ParameterBase& getParameter() noexcept { return parameter; }

    juce::ValueTree tree;

private:
    void parameterValueChanged (const juce::Identifier& id, const juce::var& newValue) override
    {
        if (listenersNeedCalling or lastNotifiedValue != newValue)
        {
            lastNotifiedValue = newValue;
            pendingId = id;
            pendingValue = newValue;
            needsUpdate = true;

            if (juce::MessageManager::getInstance()->isThisTheMessageThread())
            {
                cancelPendingUpdate();
                handleAsyncUpdate();
            }
            else
            {
                triggerAsyncUpdate();
            }
        }
    }

    void handleAsyncUpdate() override
    {
        listeners.call (
            [this] (Model::Listener& l)
            {
                l.parameterChanged (pendingId, pendingValue);
            });

        listenersNeedCalling = false;
    }

    ParameterBase& parameter;
    Model::LockedListeners& listeners;
    juce::var lastNotifiedValue;
    juce::Identifier pendingId;
    juce::var pendingValue;
    std::atomic<bool> needsUpdate { true };
    std::atomic<bool> listenersNeedCalling { true };
    bool ignoreCallbacks { false };
};

/*____________________________________________________________________________*/

Model::Model (const juce::Identifier& newTreeID)
    : state (newTreeID)
{
    startTimerHz (10);
    state.addListener (this);
}

Model::Model (juce::ValueTree initialState)
    : state (std::move (initialState))
{
    startTimerHz (10);
    state.addListener (this);
}

Model::Model()
{
    state = juce::ValueTree (Format::toValidID (ProjectInfo::projectName, true));
    startTimerHz (10);
    state.addListener (this);
}

Model::~Model()
{
    state.removeListener (this);
    stopTimer();
}

//==============================================================================
std::unique_ptr<juce::XmlElement> Model::getXml() const noexcept
{
    const juce::ScopedLock lock { valueTreeChanging };
    return state.createXml();
}

void Model::replaceState (const juce::ValueTree& newState)
{
    const juce::ScopedLock lock { valueTreeChanging };

    state.copyPropertiesAndChildrenFrom (newState, nullptr);
    updateAdapterConnections (state);

    for (auto& [groupId, group] : adapters)
        for (auto& [id, adapter] : group)
            adapter->restoreFromTree();
}

bool Model::writeToXml (juce::File& destinationFile)
{
    return state.createXml()->writeTo (destinationFile);
}

//==============================================================================
// Proxy methods — delegate to the private state member
//==============================================================================

void Model::addListener (juce::ValueTree::Listener* listener) noexcept
{
    state.addListener (listener);
}

void Model::removeListener (juce::ValueTree::Listener* listener) noexcept
{
    state.removeListener (listener);
}

void Model::addListener (Listener* listener) noexcept { parameterListeners.add (listener); }
void Model::removeListener (Listener* listener) noexcept { parameterListeners.remove (listener); }

void Model::appendChild (juce::ValueTree child, juce::UndoManager* undoManager)
{
    state.appendChild (child, undoManager);
}

void Model::removeChild (juce::ValueTree child, juce::UndoManager* undoManager)
{
    state.removeChild (child, undoManager);
}

juce::ValueTree Model::getChildWithName (const juce::Identifier& name) const noexcept
{
    return jam::Model::getChildWithName (state, name);
}

juce::ValueTree
Model::getOrCreateChildWithName (const juce::Identifier& name, juce::UndoManager* undoManager)
{
    return state.getOrCreateChildWithName (name, undoManager);
}

void Model::setTreeProperty (const juce::Identifier& name,
                             const juce::var& value,
                             juce::UndoManager* undoManager)
{
    state.setProperty (name, value, undoManager);
}

juce::var Model::getTreeProperty (const juce::Identifier& name) const noexcept
{
    return state.getProperty (name);
}

juce::Value
Model::getTreePropertyAsValue (const juce::Identifier& name, juce::UndoManager* undoManager)
{
    return state.getPropertyAsValue (name, undoManager);
}

juce::Identifier Model::getType() const noexcept { return state.getType(); }

//==============================================================================

juce::Identifier Model::getGroupId (const juce::ValueTree& tree) noexcept
{
    if (tree.hasProperty (Id::id))
        return juce::Identifier { tree.getType().toString() + "#"
                                  + juce::String (static_cast<int64_t> (tree.getProperty (Id::id))) };

    return tree.getType();
}

void Model::addParameterAdapter (ParameterBase& parameter, juce::ValueTree& tree)
{
    const auto groupId { getGroupId (tree) };

    adapters.try_emplace (groupId);

    adapters.at (groupId).addOrReplace (
        parameter.id, std::make_unique<ParameterAdapter> (parameter, tree, parameterListeners));
}

//==============================================================================
// ParameterAttachment
//==============================================================================

Model::ParameterAttachment::ParameterAttachment (ParameterBase& param,
                                                 std::function<void (const juce::var&)> callback)
    : parameter (param)
    , setValue (std::move (callback))
{
    parameter.addListener (this);
}

Model::ParameterAttachment::~ParameterAttachment()
{
    parameter.removeListener (this);
    cancelPendingUpdate();
}

void Model::ParameterAttachment::sendInitialUpdate()
{
    parameterValueChanged (parameter.id, parameter.getValueAsVar());
}

void Model::ParameterAttachment::parameterValueChanged (const juce::Identifier&,
                                                        const juce::var& newValue)
{
    lastValue = newValue;

    if (juce::MessageManager::getInstance()->isThisTheMessageThread())
    {
        cancelPendingUpdate();
        handleAsyncUpdate();
    }
    else
    {
        triggerAsyncUpdate();
    }
}

void Model::ParameterAttachment::handleAsyncUpdate()
{
    if (setValue)
        setValue (lastValue);
}

//==============================================================================
// Overlay — property-only mutation of the live state tree from a tree source
//==============================================================================

void Model::setValuesFrom (const juce::ValueTree& sourceValueTree) noexcept
{
    JUCE_ASSERT_MESSAGE_THREAD
    const juce::ScopedLock lock { valueTreeChanging };

    Model::applyFunctionRecursively (
        state,
        sourceValueTree,
        [] (juce::ValueTree& live, const juce::ValueTree& tree)
        {
            for (int i { 0 }; i < tree.getNumProperties(); ++i)
            {
                const auto propertyName { tree.getPropertyName (i) };
                const auto newValue { tree.getProperty (propertyName) };

                if (live.getProperty (propertyName) != newValue)
                    live.setProperty (propertyName, newValue, nullptr);
            }

            return false;
        });
}

//==============================================================================
// APVTS-pattern: atomic parameter store + flush timer
//==============================================================================

bool Model::flush() noexcept
{
    JUCE_ASSERT_MESSAGE_THREAD
    const juce::ScopedLock lock { valueTreeChanging };

    bool anyDirty { false };

    for (auto& [groupId, group] : adapters)
        for (auto& [id, adapter] : group)
            anyDirty |= adapter->flushToTree();

    return anyDirty;
}

void Model::timerCallback()
{
    const bool anythingUpdated { flush() };
    const int interval { anythingUpdated ? 1000 / flushHz : 1000 / idleHz };
    startTimer (interval);
}

//==============================================================================
juce::var
Model::getValue (const juce::Identifier& type, const juce::Identifier& name) const noexcept
{
    return getValueFromChildWithProperty (state, type, name).getValue();
}

void Model::setValue (const juce::Identifier& type,
                      const juce::Identifier& name,
                      const juce::var& newValue)
{
    getValueFromChildWithProperty (state, type, name).setValue (newValue);
}

jam::Union<int16_t, int16_t, int16_t, int16_t>
Model::getInt16 (const juce::Identifier& type, const juce::Identifier& name) const noexcept
{
    auto var { getValue (type, name) };

    if (auto* array { var.getArray() })
    {
        const auto n { array->size() };
        return jam::Union<int16_t, int16_t, int16_t, int16_t>::pack (
            n > 0 ? static_cast<int16_t> (static_cast<juce::int64> ((*array)[0])) : int16_t { 0 },
            n > 1 ? static_cast<int16_t> (static_cast<juce::int64> ((*array)[1])) : int16_t { 0 },
            n > 2 ? static_cast<int16_t> (static_cast<juce::int64> ((*array)[2])) : int16_t { 0 },
            n > 3 ? static_cast<int16_t> (static_cast<juce::int64> ((*array)[3])) : int16_t { 0 });
    }

    return {};
}

jam::Union<int32_t, int32_t>
Model::getInt (const juce::Identifier& type, const juce::Identifier& name) const noexcept
{
    auto var { getValue (type, name) };

    if (auto* array { var.getArray() })
    {
        const auto n { array->size() };
        return jam::Union<int32_t, int32_t>::pack (
            n > 0 ? static_cast<int32_t> (static_cast<juce::int64> ((*array)[0])) : int32_t { 0 },
            n > 1 ? static_cast<int32_t> (static_cast<juce::int64> ((*array)[1])) : int32_t { 0 });
    }

    return {};
}

//==============================================================================
// VT→atomic reverse sync
//==============================================================================

void Model::valueTreePropertyChanged (juce::ValueTree& tree, const juce::Identifier& property)
{
    const auto groupId { getGroupId (tree) };

    if (adapters.contains (groupId))
    {
        auto& group { adapters.at (groupId) };

        if (group.contains (property))
        {
            group.at (property)->setRawValue (tree.getProperty (property));
            return;
        }
    }
}

void Model::updateAdapterConnections (const juce::ValueTree& root)
{
    applyFunctionRecursively (root,
                              [this] (const juce::ValueTree& tree)
                              {
                                  const auto groupId { getGroupId (tree) };

                                  if (adapters.contains (groupId))
                                  {
                                      auto target { tree };

                                      for (auto& [id, adapter] : adapters.at (groupId))
                                      {
                                          adapter->tree = target;
                                          adapter->restoreFromTree();
                                      }
                                  }

                                  return false;
                              });
}

void Model::valueTreeChildAdded (juce::ValueTree&, juce::ValueTree& childWhichHasBeenAdded)
{
    updateAdapterConnections (childWhichHasBeenAdded);
}

void Model::valueTreeRedirected (juce::ValueTree&) { updateAdapterConnections (state); }

/**_____________________________END OF NAMESPACE______________________________*/
}// namespace jam
