namespace jam
{
/*____________________________________________________________________________*/

bool
Model::applyFunctionRecursively (const juce::ValueTree& root,
                                 const std::function<bool (const juce::ValueTree&)>& function)
{
    if (function (root))
        return true;

    for (auto&& child : root)
    {
        if (Model::applyFunctionRecursively (child, function))
            return true;
    }

    return false;
}

bool Model::applyFunctionRecursively (
    juce::ValueTree& target,
    const juce::ValueTree& source,
    const std::function<bool (juce::ValueTree&, const juce::ValueTree&)>& function)
{
    if (function (target, source))
        return true;

    for (auto&& parsedChild : source)
    {
        if (auto liveChild { target.getChildWithName (parsedChild.getType()) }; liveChild.isValid())
        {
            if (Model::applyFunctionRecursively (liveChild, parsedChild, function))
                return true;
        }
    }

    return false;
}

juce::ValueTree Model::getChildWithName (const juce::ValueTree& root, const juce::Identifier& type)
{
    juce::ValueTree foundChild;

    auto findChildWithName = [&foundChild, &type] (const juce::ValueTree& tree) -> bool
    {
        if (tree.getType() == type)
        {
            foundChild = tree;
            return true;
        }
        return false;
    };

    Model::applyFunctionRecursively (root, findChildWithName);

    return foundChild;
}

bool Model::findAndRemoveChild (juce::ValueTree& root,
                                const juce::Identifier& type,
                                juce::UndoManager* undoManager)
{
    auto target = Model::getChildWithName (root, type);

    if (target.isValid())
    {
        auto parent = target.getParent();

        if (parent.isValid())
        {
            parent.removeChild (target, undoManager);
            return true;
        }
    }

    return false;
}

juce::ValueTree Model::getOrCreateChildWithName (juce::ValueTree& root,
                                                 const juce::Identifier& type,
                                                 juce::UndoManager* undoManager)
{
    juce::ValueTree foundChild;

    auto findChildWithName = [&foundChild, &type] (const juce::ValueTree& tree) -> bool
    {
        if (tree.getType() == type)
        {
            foundChild = tree;
            return true;
        }
        return false;
    };

    Model::applyFunctionRecursively (root, findChildWithName);

    if (foundChild.isValid())
        return foundChild;

    return root.getOrCreateChildWithName (type, undoManager);
}

juce::ValueTree Model::getChildWithID (const juce::ValueTree& root, const juce::var& parameterID)
{
    juce::ValueTree childWithID;

    auto findChildWithID = [&childWithID, &parameterID] (const juce::ValueTree& tree) -> bool
    {
        if (tree.hasProperty (Id::id) and tree.getProperty (Id::id) == parameterID)
        {
            childWithID = tree;
            return true;
        }
        return false;
    };

    Model::applyFunctionRecursively (root, findChildWithID);

    return childWithID;
}

juce::Value Model::getValueFromChildWithID (const juce::ValueTree& root,
                                            const juce::Identifier& parameterID,
                                            const juce::Identifier& name,
                                            juce::UndoManager* undoManager)
{
    return Model::getChildWithID (root, parameterID.toString())
        .getPropertyAsValue (name, undoManager);
}

juce::Value Model::getValueFromChildWithProperty (const juce::ValueTree& root,
                                                  const juce::Identifier& type,
                                                  const juce::Identifier& name,
                                                  juce::UndoManager* undoManager)
{
    auto tree { Model::getChildWithName (root, type) };

    jassert (tree.isValid());
    jassert (tree.hasProperty (name));

    return tree.getPropertyAsValue (name, undoManager);
}

void Model::forEachProperty (
    const juce::ValueTree& tree,
    const std::function<void (const juce::Identifier&, const juce::var&)>& function)
{
    for (int i = 0; i < tree.getNumProperties(); ++i)
    {
        const auto id { tree.getPropertyName (i) };
        function (id, tree[id]);
    }
}

//==============================================================================
juce::ValueTree
Model::getRoot (juce::ValueTree& taproot, juce::Component* component, juce::UndoManager* undoManager)
{
    if (auto componentID { component->getComponentID() }; componentID.isNotEmpty())
    {
        return juce::Identifier (componentID) == taproot.getType()
                   ? taproot
                   : Model::getOrCreateChildWithName (taproot, componentID, undoManager);
    }

    return juce::ValueTree();
}

juce::ValueTree
Model::getParent (juce::ValueTree& taproot, juce::Component* child, juce::UndoManager* undoManager)
{
    if (auto childID { child->getComponentID() }; childID.isNotEmpty())
    {
        if (auto parent { Model::getChildWithName (taproot, childID) }; parent.isValid())
            return parent;

        if (auto root { Model::getRoot (taproot, child->getParentComponent(), undoManager) }; root.isValid())
            return Model::getOrCreateChildWithName (root, childID, undoManager);
    }

    return juce::ValueTree();
}

//==============================================================================
void Model::attach (juce::ValueTree& taproot,
                    juce::Component* component,
                    juce::Value::Listener* listener,
                    juce::UndoManager* undoManager)
{
    if (auto root { Model::getRoot (taproot, component, undoManager) }; root.isValid())
        for (auto& child : component->getChildren())
            Model::attachChild (taproot, child, listener, undoManager);
}

void Model::attachChild (juce::ValueTree& taproot,
                         juce::Component* child,
                         juce::Value::Listener* listener,
                         juce::UndoManager* undoManager)
{
    if (auto parent { Model::getParent (taproot, child, undoManager) }; parent.isValid())
    {
        const auto& hasValidChildID = [] (auto& c)
        {
            return std::any_of (c->getChildren().begin(),
                                c->getChildren().end(),
                                [] (auto& grandChild)
                                {
                                    return grandChild->getComponentID().isNotEmpty();
                                });
        };

        for (auto& grandchild : child->getChildren())
        {
            if (hasValidChildID (grandchild))
            {
                Model::attachChild (taproot, grandchild, listener, undoManager);
            }
            else
            {
                if (auto grandchildID { grandchild->getComponentID() }; grandchildID.isNotEmpty())
                {
                    auto& value { Model::getFrom (grandchild) };

                    if (not parent.hasProperty (grandchildID))
                        parent.setProperty (grandchildID, value, undoManager);

                    value.referTo (parent.getPropertyAsValue (grandchildID, undoManager));

                    if (listener != nullptr)
                        value.addListener (listener);
                }
            }
        }
    }
}

//==============================================================================
void Model::attach (juce::ValueTree& root,
                    juce::Value& value,
                    const juce::Identifier& parameterId,
                    const juce::Identifier& valueId,
                    juce::Value::Listener* listener,
                    juce::UndoManager* undoManager)
{
    if (auto tree { root.getChildWithProperty (Id::id, parameterId.toString()) }; tree.isValid())
    {
        if (not tree.hasProperty (valueId))
            tree.setProperty (valueId, value.getValue(), undoManager);

        if (listener != nullptr)
            value.addListener (listener);

        value.referTo (tree.getPropertyAsValue (valueId, undoManager));
    }
}

//==============================================================================
Model::UniqueNodeMap Model::buildUniqueNodeMap (const juce::ValueTree& root)
{
    jam::HashMap<juce::String, juce::ValueTree> map;

    std::function<void (const juce::ValueTree&)> visit = [&] (const juce::ValueTree& node)
    {
        if (node.getNumProperties() > 0)
            map.emplace (node.getType().toString(), node);

        for (auto&& child : node)
            visit (child);
    };

    visit (root);
    return map;
}

//==============================================================================
bool Model::loadState (juce::ValueTree& target, const juce::File& xmlFile)
{
    if (auto xml { juce::parseXML (xmlFile) })
    {
        if (auto source { juce::ValueTree::fromXml (*xml) }; source.isValid())
            return Model::loadState (target, source);
    }

    return false;
}

bool Model::loadState (juce::ValueTree& target, const juce::ValueTree& source)
{
    Model::applyFunctionRecursively (
        target,
        source,
        [] (juce::ValueTree& live, const juce::ValueTree& parsed)
        {
            for (int i { 0 }; i < parsed.getNumProperties(); ++i)
            {
                const auto name { parsed.getPropertyName (i) };
                live.setProperty (name, parsed[name], nullptr);
            }

            for (auto&& parsedChild : parsed)
            {
                if (not live.getChildWithName (parsedChild.getType()).isValid())
                    live.addChild (parsedChild.createCopy(), -1, nullptr);
            }

            return false;
        });

    return true;
}

/**_____________________________END OF NAMESPACE______________________________*/
}// namespace jam
