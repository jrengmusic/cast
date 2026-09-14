/**
 * @file jam_Model.h
 * @brief Model — APVTS-analog state owner, owns the juce::ValueTree by value.
 *
 * jam::Model wraps juce::ValueTree and adds:
 * - Parameter<T> (atomic value holder) + ParameterAdapter (internal VT↔atomic bridge)
 * - AnyMap-based parameter groups for hierarchical organization
 * - Adaptive-rate flush timer (60/120 Hz) for atomic-to-VT synchronization
 * - createAndAddParameter<T>() — creates and owns a parameter, seeds VT, registers adapter
 * - ParameterAttachment — listener bridge delivering parameter changes to a callback on the message thread
 * - Static tree utility methods
 *
 * @par Thread ownership
 * - Parameter store()/load(): any thread, lock-free
 * - flush(), timerCallback(), get(): MESSAGE THREAD only
 * - createAndAddParameter(), ParameterAttachment ctor/dtor: MESSAGE THREAD, construction/destruction only
 *
 * @see jam::Parameter<int>
 * @see jam::ParameterBase
 */
namespace jam
{
/*____________________________________________________________________________*/

/**
 * @class Model
 * @brief APVTS-analog state owner. Owns a juce::ValueTree by value and bridges
 *        it to a flat store of atomic Parameter<T> instances.
 *
 * Combines a juce::ValueTree (message-thread state, undo/redo, XML persistence)
 * with Parameter<T> (lock-free atomic transport for any thread) via
 * ParameterAdapter, kept in sync by an adaptive-rate flush timer.
 *
 * @see jam::Parameter
 * @see jam::ParameterBase
 */
class Model
    : public juce::Timer
    , public juce::ValueTree::Listener
{
public:
    //==========================================================================
    /** @brief Cross-thread parameter change listener.
     *  Fires on the calling thread (same thread that called setValue).
     *  JUCE analog: AudioProcessorValueTreeState::Listener.
     */
    struct Listener
    {
        virtual ~Listener() = default;

        /** @brief Called when any parameter's atomic value changes.
         *  @param id        Property identifier of the changed parameter.
         *  @param newValue  The new value (type-erased).
         *  @note Fires on the calling thread — may be audio/reader/GL thread.
         */
        virtual void parameterChanged (const juce::Identifier& id, const juce::var& newValue) = 0;
    };

    //==========================================================================
    /**
     * @brief Base interface for any Component that exposes a juce::Value.
     *
     * Classes inheriting this must implement getValueObject() to return
     * a reference to their bound juce::Value. This allows generic traversal
     * code to attach component state to a ValueTree.
     *
     * Optionally, onAttachment can be set to a callback that will be invoked
     * when the component's Value is attached to the ValueTree state.
     */
    struct Object
    {
        virtual ~Object() = default;

        /**
         * @brief Return the component's bound juce::Value.
         *
         * @return Reference to the juce::Value owned by the component.
         */
        virtual juce::Value& getValueObject() noexcept = 0;

        /**
         * @brief Optional callback invoked when this component's Value
         *        is attached to a ValueTree state.
         */
        std::function<void()> onAttachment;
    };

    //==========================================================================
    /**
     * @brief CRTP mix-in that enforces ComponentID assignment.
     *
     * Inherit from `ValueComponent<Derived>` alongside juce::Component to ensure
     * that every component has a non-empty ComponentID. The constructor
     * requires an ID string and sets it on the underlying Component.
     *
     * This makes the contract explicit:
     *   - The derived type must be a juce::Component.
     *   - Every instance must declare its ComponentID up front.
     *
     * @tparam Derived The concrete component type, which must inherit juce::Component.
     *
     * @code
     * // Example usage:
     * class MyComponent : public juce::Component,
     *                     public ValueComponent<MyComponent>
     * {
     * public:
     *     MyComponent() : ValueComponent("MyComponentID") {}
     *
     *     juce::Value& getValueObject() noexcept override { return value; }
     *
     * private:
     *     juce::Value value;
     * };
     * @endcode
     */
    template <typename Derived>
    struct ValueComponent : public Object
    {
        /**
         * @brief Construct a ValueObject with a required ComponentID.
         *
         * @param newID The ComponentID string to assign.
         */
        explicit ValueComponent (juce::StringRef newID)
        {
            static_assert (std::is_base_of<juce::Component, Derived>::value,
                           "Component can only be mixed into juce::Component subclasses");

            auto* comp = static_cast<Derived*> (this);
            comp->setComponentID (newID);
        }
    };

    //==========================================================================
    /**
     * @struct Component
     * @brief CRTP mixin binding a juce::Component subclass to a ValueTree node,
     *        finding or creating that node by UUID under a parent.
     * @tparam Derived  The juce::Component subclass mixing this in. Must set
     *                  its componentID from the bound node's @c Id::id.
     */
    template<typename Derived>
    struct Component
    {
        /**
         * @brief Finds the child of @p parentState matching @p type and @p uuid,
         *        or creates and appends one if absent.
         * @param model       Owning Model.
         * @param parentState Parent node to search/append under.
         * @param type        Node type identifier to match/create.
         * @param uuid        Identity of the node, compared against @c Id::id.
         */
        Component (Model& model,
                   juce::ValueTree parentState,
                   const juce::Identifier& type,
                   jam::UUID uuid)
            : model (model)
        {
            static_assert (std::is_base_of_v<juce::Component, Derived>,
                           "Model::Component requires a juce::Component subclass");

            for (auto child : parentState)
            {
                if (child.getType() == type
                    and child.getProperty (Id::id) == juce::var { uuid.value })
                {
                    state = child;
                }
            }

            if (not state.isValid())
            {
                state = juce::ValueTree { type };
                state.setProperty (Id::id, uuid.value, nullptr);
                parentState.appendChild (state, nullptr);
            }

            static_cast<Derived*> (this)->setComponentID (uuid.toString());
        }

        /**
         * @brief Adopts an already-existing bound node directly (no search/create).
         * @param model         Owning Model.
         * @param existingState The node to adopt as @c state.
         */
        Component (Model& model, juce::ValueTree existingState)
            : model (model), state (existingState)
        {
            static_assert (std::is_base_of_v<juce::Component, Derived>,
                           "Model::Component requires a juce::Component subclass");

            static_cast<Derived*> (this)->setComponentID (state.getProperty (Id::id).toString());
        }

        virtual ~Component() = default;

        /** @brief Returns the bound ValueTree node. */
        juce::ValueTree getValueTree() noexcept { return state; }

        /** @brief Owning Model. */
        Model& model;

        /** @brief The bound ValueTree node. */
        juce::ValueTree state;
    };

    //==========================================================================
    /**
     * @class Attachment
     * @brief Fires an initial resized()/repaint() pass on an attached juce::Component.
     */
    class Attachment
    {
    public:
        /**
         * @brief Attaches to @p child and asserts it is already parented in the ValueTree.
         * @tparam Derived  A juce::Component subclass.
         * @param child     The component to attach to.
         */
        template<typename Derived>
        explicit Attachment (Derived& child) noexcept
            : self (child)
        {
            static_assert (std::is_base_of_v<juce::Component, Derived>,
                           "Attachment requires a juce::Component subclass");

            jassert (child.getValueTree().getParent().isValid());

            sendInitialUpdate();
        }

        ~Attachment() = default;

        /** @brief Calls resized() then repaint() on the attached component. */
        void sendInitialUpdate()
        {
            self.resized();
            self.repaint();
        }

    private:
        juce::Component& self;

        //==============================================================================
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Attachment)
    };

    //==========================================================================
    /** @brief Per-parameter listener bridge with message-thread callback delivery.
     *
     *  JUCE analog: ParameterAttachment — connects to an existing parameter,
     *  delivers changes to a callback on the message thread via AsyncUpdater.
     *  Does NOT create or destroy the parameter.
     *
     *  Create the parameter first via createAndAddParameter(), then construct
     *  this only when a Component needs to react to changes.
     */
    class ParameterAttachment
        : private ParameterBase::Listener
        , private juce::AsyncUpdater
    {
    public:
        /** @brief Connects to an existing parameter.
         *  @param parameter  The parameter to listen to (must already exist on Model).
         *  @param callback   Called on the message thread when the parameter changes.
         */
        ParameterAttachment (ParameterBase& parameter,
                             std::function<void (const juce::var&)> callback);

        ~ParameterAttachment() override;

        /** @brief Fires the callback with the parameter's current value.
         *  Call after setup to sync initial state.
         */
        void sendInitialUpdate();

    private:
        void parameterValueChanged (const juce::Identifier& id, const juce::var& newValue) override;
        void handleAsyncUpdate() override;

        ParameterBase& parameter;
        juce::var lastValue;
        std::function<void (const juce::var&)> setValue;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParameterAttachment)
    };

#if JUCE_MODULE_AVAILABLE_juce_audio_processors
    //==========================================================================
    /**
     * @brief A class to attach a juce::Parameter to a component's Value.
     *
     * This class allows the value of a parameter in an AudioProcessorValueTreeState
     * to be linked to a component's Value. The ValueAttachment ensures that changes
     * in the parameter are reflected in the component's Value, and vice versa.
     */
    class ValueAttachment
        : public juce::ParameterAttachment
        , private juce::Value::Listener
    {
    public:
        /**
         * @brief Construct a ValueAttachment with an AudioProcessorValueTreeState,
         *        parameter ID, associated component value, and optional UndoManager.
         *
         * @param stateToUse Reference to the AudioProcessorValueTreeState.
         * @param parameterID The unique identifier for the parameter to attach.
         * @param componentWithValue Reference to a component that implements Object
         *                         and provides its Value via getValueObject().
         * @param undoManager Optional pointer to an UndoManager for handling value changes.
         */
        ValueAttachment (juce::AudioProcessorValueTreeState& stateToUse,
                         const juce::String& parameterID,
                         Object& componentWithValue,
                         juce::UndoManager* undoManager = nullptr)
            : juce::ParameterAttachment (*stateToUse.getParameter (parameterID), [this] (float newValue)
                                         {
                                             value.setValue (newValue);
                                         },
                                         undoManager)
            , value (componentWithValue.getValueObject())
        {
            // Assert that the parameter was found. If this fails, the parameterID is incorrect.
            jassert (stateToUse.getParameter (parameterID) != nullptr);

            value.addListener (this);
            sendInitialUpdate();
        }

        /**
         * @brief Destructor for ValueAttachment.
         */
        ~ValueAttachment() override
        {
            value.removeListener (this);
        }

    private:
        /**
         * @brief Callback when the associated Value changes.
         *
         * @param newValue The Value that has changed.
         */
        void valueChanged (juce::Value& newValue) override
        {
            if (newValue.refersToSameSourceAs (value))
                setValueAsCompleteGesture (newValue.getValue());
        }

        /**
         * @brief Reference to the component's associated Value.
         */
        juce::Value& value;

        //==============================================================================
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ValueAttachment)
    };
#endif// JUCE_MODULE_AVAILABLE_juce_audio_processors

    //==========================================================================
    /**
     * @brief Check if a Component exposes a non-void juce::Value.
     *
     * Uses getFrom() internally to retrieve the Value.
     *
     * @param c Pointer to the Component.
     * @return true if the Component has a non-void Value, false otherwise.
     */
    static bool isNonVoid (juce::Component* c) noexcept
    {
        return not getFrom (c).getValue().isVoid();
    }

    /**
     * @brief Retrieve the juce::Value associated with a Component.
     *
     * Supports JUCE built-in widgets (Slider, Button, Label, TextEditor, ComboBox)
     * and any custom component inheriting Object/Component.
     *
     * @param c Pointer to the Component. Must be non-null — asserted on entry.
     * @return Reference to the associated juce::Value, or a dummy Value if the
     *         Component's runtime type is not one of the supported widgets.
     */
    static juce::Value& getFrom (juce::Component* c)
    {
        static juce::Value dummy;
        assert (c != nullptr and "getFrom requires a non-null Component");

        // JUCE built-ins
        if (auto* s = dynamic_cast<juce::Slider*> (c))
            return s->getValueObject();
        if (auto* b = dynamic_cast<juce::Button*> (c))
            return b->getToggleStateValue();
        if (auto* l = dynamic_cast<juce::Label*> (c))
            return l->getTextValue();
        if (auto* e = dynamic_cast<juce::TextEditor*> (c))
            return e->getTextValue();
        if (auto* cb = dynamic_cast<juce::ComboBox*> (c))
            return cb->getSelectedIdAsValue();

        // Object-based components
        if (auto* vo = dynamic_cast<Object*> (c))
            return vo->getValueObject();

        return dummy;
    }

    //==========================================================================

    explicit Model (const juce::Identifier& newTreeID);

    /** @brief Constructs the model by adopting a pre-built ValueTree as its state.
     *
     *  Use when the full tree is built before the Model exists (e.g. via the
     *  bimap @c jam::lua::fromLua / @c fromFiles aggregators). The listener is attached to
     *  the adopted tree directly — no copy, no content transfer.
     *
     *  @param initialState  The tree to adopt as @c state (moved in).
     */
    explicit Model (juce::ValueTree initialState);

    /** @brief Constructs the model with an empty, invalid @c state ValueTree. */
    Model();

    ~Model();

    //==========================================================================
    // Static tree utilities — operate on arbitrary juce::ValueTree handles.
    //==========================================================================

    /** @brief Walks @p root and its descendants depth-first, calling function(node) at each.
     *  @param root      Tree to walk.
     *  @param function  Called once per node. Return true to stop early.
     *  @return true if function returned true (early stop), false if walk completed.
     */
    static bool
    applyFunctionRecursively (const juce::ValueTree& root,
                              const std::function<bool (const juce::ValueTree&)>& function);

    /** @brief Walks two trees in lockstep, calling function(live, parsed) at each matching node.
     *
     *  For each node in parsed, finds the child with matching type in live.
     *  Recurses only into matched pairs. Unmatched children are skipped.
     *
     *  @param live      Mutable tree — the target of mutations.
     *  @param parsed    Source tree — structure drives the walk.
     *  @param function  Called once per matched node pair. Return true to stop early.
     *  @return true if function returned true (early stop), false if walk completed.
     */
    static bool applyFunctionRecursively (
        juce::ValueTree& live,
        const juce::ValueTree& parsed,
        const std::function<bool (juce::ValueTree&, const juce::ValueTree&)>& function);

    /** @brief Recursively finds the first descendant of @p root with type @p name.
     *  @param root  Tree to search.
     *  @param name  Node type identifier to match.
     *  @return The matching node, or an invalid ValueTree if none found.
     */
    static juce::ValueTree
    getChildWithName (const juce::ValueTree& root, const juce::Identifier& name);

    /** @brief Recursively finds the first descendant of @p root with type @p name
     *         and removes it from its actual parent.
     *  @param root         Tree to search.
     *  @param name         Node type identifier to match.
     *  @param undoManager  Optional undo manager for undo/redo support.
     *  @return true if a matching descendant was found and removed.
     */
    static bool findAndRemoveChild (juce::ValueTree& root,
                                    const juce::Identifier& name,
                                    juce::UndoManager* undoManager = nullptr);

    /** @brief Recursively finds the first descendant of @p root with type @p name;
     *         if none exists, appends a direct child of @p root with that type.
     *  @param root         Tree to search/append to.
     *  @param name         Node type identifier to match/create.
     *  @param undoManager  Optional undo manager for undo/redo support.
     *  @return The existing descendant, or the newly created direct child.
     */
    static juce::ValueTree getOrCreateChildWithName (juce::ValueTree& root,
                                                     const juce::Identifier& name,
                                                     juce::UndoManager* undoManager = nullptr);

    /** @brief Recursively finds the first descendant of @p root whose @c Id::id
     *         property equals @p parameterID.
     *  @param root         Tree to search.
     *  @param parameterID  Value to match against each node's @c Id::id property.
     *  @return The matching node, or an invalid ValueTree if none found.
     */
    static juce::ValueTree
    getChildWithID (const juce::ValueTree& root, const juce::var& parameterID);

    /** @brief Recursively finds the descendant with @c Id::id equal to @p parameterID
     *         and returns a live juce::Value bound to one of its properties.
     *  @param root          Tree to search.
     *  @param parameterID   Value to match against each node's @c Id::id property.
     *  @param propertyName  Property to bind the returned Value to.
     *  @param undoManager   Optional undo manager for undo/redo support.
     *  @return A juce::Value bound to the matched node's property.
     */
    static juce::Value getValueFromChildWithID (const juce::ValueTree& root,
                                                const juce::Identifier& parameterID,
                                                const juce::Identifier& propertyName = Id::value,
                                                juce::UndoManager* undoManager = nullptr);

    /** @brief Recursively finds the descendant of type @p name and returns a live
     *         juce::Value bound to one of its properties.
     *  @param root          Tree to search.
     *  @param name          Node type identifier to match.
     *  @param propertyName  Property to bind the returned Value to.
     *  @param undoManager   Optional undo manager for undo/redo support.
     *  @return A juce::Value bound to the matched node's property.
     */
    static juce::Value
    getValueFromChildWithProperty (const juce::ValueTree& root,
                                   const juce::Identifier& name,
                                   const juce::Identifier& propertyName = Id::value,
                                   juce::UndoManager* undoManager = nullptr);

    /** @brief Calls function(name, value) for every property directly on @p tree.
     *  @param tree      Tree whose properties to iterate.
     *  @param function  Called once per property.
     */
    static void forEachProperty (
        const juce::ValueTree& tree,
        const std::function<void (const juce::Identifier&, const juce::var&)>& function);

    //==============================================================================
    /**
     * @brief Retrieve a value from a child node in the ValueTree.
     *
     * This function retrieves a value of the specified type from a child node within the ValueTree.
     *
     * @tparam ValueType The type of the value to retrieve.
     * @param valueTree The ValueTree to search within.
     * @param childId The identifier of the child node to retrieve the value from.
     * @return The value of the specified type from the child node.
     */
    template<typename ValueType>
    static const ValueType getValue (const juce::ValueTree& valueTree, const juce::Identifier& childId) noexcept
    {
        if constexpr (std::is_same_v<ValueType, juce::String>)
            return valueTree.getChildWithName (childId).getPropertyAsValue (Id::value, nullptr).getValue().toString();

        return valueTree.getChildWithName (childId).getPropertyAsValue (Id::value, nullptr).getValue();
    }

    /**
     * @brief Finds or creates the ValueTree node bound to the given component's ID.
     *
     * Returns @p tapRoot itself when its type matches @p component's ComponentID;
     * otherwise recursively finds a descendant of @p tapRoot matching that ID,
     * creating and appending one if absent.
     *
     * @param tapRoot The taproot node to start the search from.
     * @param component The component for which to find or create the root node.
     * @param undoManager An optional undo manager for undo/redo support.
     * @return The matching or newly created node, or an invalid ValueTree if
     *         @p component has no ComponentID.
     */
    static juce::ValueTree
    getRoot (juce::ValueTree& tapRoot, juce::Component* component, juce::UndoManager* undoManager);

    /**
     * @brief Finds or creates the ValueTree node bound to the given child component's ID.
     *
     * Recursively finds a descendant of @p tapRoot matching @p child's ComponentID;
     * if none exists, resolves (finding or creating) the parent component's node via
     * getRoot() and creates the child node under it.
     *
     * @param tapRoot The taproot node to start the search from.
     * @param child The child component for which to find or create the parent node.
     * @param undoManager An optional undo manager for undo/redo support.
     * @return The matching or newly created node, or an invalid ValueTree if
     *         @p child has no ComponentID.
     */
    static juce::ValueTree
    getParent (juce::ValueTree& tapRoot, juce::Component* child, juce::UndoManager* undoManager);

    //==============================================================================
    /**
     * @brief Attach the ValueTree to the component and its children.
     *
     * This function attaches the ValueTree to the provided component and its children, adding a listener to the value changes if specified.
     *
     * This is used to build standalone app such as PABRIK.
     *
     * @param taproot The taproot node to attach to.
     * @param component The component to attach.
     * @param listener The listener to add for value changes.
     * @param undoManager An optional undo manager for undo/redo support.
     */
    static void attach (juce::ValueTree& taproot,
                        juce::Component* component,
                        juce::Value::Listener* listener,
                        juce::UndoManager* undoManager = nullptr);

    /**
     * @brief Attach the ValueTree to the child component and its children.
     *
     * This function attaches the ValueTree to the provided child component and its children, adding a listener to the value changes if specified.
     *
     * @param taproot The taproot node to attach to.
     * @param child The child component to attach.
     * @param listener The listener to add for value changes.
     * @param undoManager An optional undo manager for undo/redo support.
     */
    static void attachChild (juce::ValueTree& taproot,
                             juce::Component* child,
                             juce::Value::Listener* listener,
                             juce::UndoManager* undoManager);

    /**
     * @brief Attach the ValueTree to the specified value for APVTS parameters.
     *
     * This function searches @p root for a child with the given @p parameterId and
     * binds @p value to that child's @p valueId property, adding a listener to the
     * value changes if specified.
     *
     * @param root The root node to attach to.
     * @param value The value to attach.
     * @param parameterId The ID of the parameter to find in the ValueTree.
     * @param valueId The ID of the value to attach.
     * @param listener The listener to add for value changes.
     * @param undoManager An optional undo manager for undo/redo support.
     */
    static void attach (juce::ValueTree& root,
                        juce::Value& value,
                        const juce::Identifier& parameterId,
                        const juce::Identifier& valueId,
                        juce::Value::Listener* listener = nullptr,
                        juce::UndoManager* undoManager = nullptr);

    /**
     * @brief Attaches every descriptor-mapped component's juce::Value under @p component
     *        to a persisted parameter node keyed by its parent's ComponentID.
     *
     * @tparam ManagerType  Disambiguates this overload from the ComponentID-keyed
     *                      attach() above; unused in the body.
     * @param state         The ValueTree the parameter node is created under.
     * @param component     The component whose children are walked.
     * @param undoManager   An optional undo manager for undo/redo support.
     */
    template <typename ManagerType>
    static void attach (juce::ValueTree& state, juce::Component* component, juce::UndoManager* undoManager = nullptr)
    {
        const auto& attachValue = [&state, &undoManager] (auto& c)
        {
            if (juce::String childId { c->getProperties()[Id::parameter].toString() }; childId.isNotEmpty())
            {
                if (Model::isNonVoid (c))
                {
                    juce::Value& value { Model::getFrom (c) };
                    juce::String parentId { c->getParentComponent()->getComponentID() };

                    if (auto parameter { state.getOrCreateChildWithName (Id::toTag (Id::parameter), undoManager) };
                        parameter.isValid())
                    {
                        juce::ValueTree param;
                        auto parentChild { parameter.getChildWithProperty (Id::id, parentId) };

                        if (parentChild.isValid())
                        {
                            param = parentChild;

                            if (parentChild.hasProperty (childId))
                            {
                                value.referTo (param.getPropertyAsValue (childId, undoManager));
                            }
                            else
                            {
                                param.setProperty (childId, value, undoManager);
                                value.referTo (param.getPropertyAsValue (childId, undoManager));
                            }
                        }
                        else
                        {
                            auto child { parameter.getChildWithProperty (Id::id, childId) };

                            if (child.isValid())
                            {
                                param = child;
                                value.referTo (param.getPropertyAsValue (Id::value, undoManager));
                            }
                            else
                            {
                                param = juce::ValueTree (Id::toType (Id::param));
                                param.setProperty (Id::id, childId, undoManager);
                                param.setProperty (Id::value, value, undoManager);
                                value.referTo (param.getPropertyAsValue (Id::value, undoManager));
                                parameter.appendChild (param, undoManager);
                            }
                        }
                    }
                }

                if (auto comp { dynamic_cast<Model::Object*> (c) })
                {
                    if (comp->onAttachment != nullptr)
                        comp->onAttachment();
                }
            }
        };

        attachValue (component);

        for (auto& child : component->getChildren())
            attachValue (child);
    }

    //==============================================================================
    /**
     * @brief Build a lookup map from a ValueTree hierarchy for use in callbacks.
     *
     * Traverses the given ValueTree recursively and collects all nodes
     * that have at least one property. Each node is inserted into the map
     * keyed by its type identifier converted to a juce::String.
     *
     * @note This function assumes that each node type identifier is unique
     *       within the tree. If multiple nodes share the same identifier,
     *       only the first occurrence is stored; later duplicates are ignored.
     *
     * @details The sole purpose of this function is to provide an efficient
     *          lookup table for use inside ValueTree::Listener callbacks
     *          (e.g. valueTreePropertyChanged). Instead of re-traversing
     *          the tree on every callback, you can build this map once and
     *          perform O(1) lookups by identifier string.
     *
     * @param root The root ValueTree (taproot) to traverse.
     * @return jam::HashMap<juce::String, juce::ValueTree>
     *         A map of node type names (as strings) to ValueTree handles.
     */
    using UniqueNodeMap = jam::HashMap<juce::String, juce::ValueTree>;
    static UniqueNodeMap buildUniqueNodeMap (const juce::ValueTree& root);

    //==============================================================================
    /** @brief Aggregates a registry of whole files into one @p rootTag-typed tree of properties.
     *
     *  Companion to the bimap @c jam::lua::fromLua overload for content that is not lua —
     *  each entry's whole file content becomes one raw-string property keyed by its
     *  stem. Iterates @p map, fetches each entry's content via @p read, and sets it
     *  verbatim as a property on a fresh @p rootTag tree. No parsing: bytes are
     *  stored as-is (e.g. svg or glsl source). The registry holds names only —
     *  @p read supplies the bytes.
     *
     *  @param rootTag  Type identifier for the returned tree.
     *  @param map      Registry of int keys to stem strings (e.g. a jam::Bimap map).
     *  @param read     Returns the whole file content for a given map key.
     *  @return         @p rootTag tree with one property per map entry (key = stem).
     */
    static juce::ValueTree fromFiles (const juce::Identifier& rootTag,
                                      const HashMap<int, juce::String>& map,
                                      const std::function<juce::String (int key)>& read)
    {
        juce::ValueTree root { rootTag };

        for (auto& [key, stem] : map)
            root.setProperty (juce::Identifier { stem }, read (key), nullptr);

        return root;
    }

    /** @brief Splits a CSV-encoded juce::var into a juce::StringArray.
     *  Inverse of jam::lua's flat-array CSV encoding.
     *  @param v  var holding a comma-separated string.
     *  @return   StringArray of trimmed entries. Empty when @p v is void or empty.
     */
    static juce::StringArray toStringArray (const juce::var& v) noexcept
    {
        auto csv { v.toString() };
        juce::StringArray result;

        if (csv.isNotEmpty())
        {
            auto tokens { juce::StringArray::fromTokens (csv, ",", "") };

            for (auto& token : tokens)
                result.add (token.trim());
        }

        return result;
    }

    /** @brief Parses @p xmlFile as XML and replaces @p target's content in-place.
     *  @param target   Tree to overwrite.
     *  @param xmlFile  XML file to parse.
     *  @return true if the file parsed and was applied successfully.
     */
    static bool loadState (juce::ValueTree& target, const juce::File& xmlFile);

    /** @brief Replaces @p target's content in-place with @p source's.
     *  @param target  Tree to overwrite.
     *  @param source   Tree to copy from.
     *  @return true if the copy was applied successfully.
     */
    static bool loadState (juce::ValueTree& target, const juce::ValueTree& source);

    //==============================================================================
    /** @brief Serializes @c state to a new XmlElement tree.
     *  @return The XML representation, or nullptr if @c state is invalid.
     */
    std::unique_ptr<juce::XmlElement> getXml() const noexcept;

    /** @brief Replaces @c state's content in-place with @p newState's. */
    void replaceState (const juce::ValueTree& newState);

    /** @brief Serializes @c state to XML and writes it to @p destinationFile.
     *  @param destinationFile  File to write to.
     *  @return true if the write succeeded.
     */
    bool writeToXml (juce::File& destinationFile);

    // Listener proxies
    /** @brief Registers a listener on the owned ValueTree directly. */
    void addListener (juce::ValueTree::Listener* listener) noexcept;

    /** @brief Removes a listener from the owned ValueTree directly. */
    void removeListener (juce::ValueTree::Listener* listener) noexcept;

    /** @brief Registers a cross-thread parameter change listener.
     *  @note Any thread (CriticalSection-guarded). */
    void addListener (Listener* listener) noexcept;

    /** @brief Removes a cross-thread parameter change listener.
     *  @note Any thread (CriticalSection-guarded). */
    void removeListener (Listener* listener) noexcept;

    /** @brief VT→atomic reverse sync. Called by JUCE when any property on the
     *         owned state tree changes on the message thread.
     *
     *  Finds the corresponding Parameter in @c params and updates its atomic
     *  so non-message threads (e.g. GL thread) can read the new value via
     *  @c getParameter<T>()->getValue() without waiting for a flush cycle.
     *
     *  No-op for a given parameter when its own loopback guard blocks re-entry —
     *  prevents flush() from bouncing the atomic→VT write back as a VT→atomic write.
     *  Guard is per-parameter (APVTS pattern: ParameterAdapter::ignoreParameterChangedCallbacks).
     *
     *  @param tree      The child tree whose property changed.
     *  @param property  Identifier of the changed property.
     */
    void
    valueTreePropertyChanged (juce::ValueTree& tree, const juce::Identifier& property) override;

    /** @brief Called by JUCE when a child is added anywhere under the owned state tree.
     *  @param parentTree             Tree the child was added to.
     *  @param childWhichHasBeenAdded The newly added child.
     */
    void valueTreeChildAdded (juce::ValueTree& parentTree,
                              juce::ValueTree& childWhichHasBeenAdded) override;

    /** @brief Called by JUCE when the owned state tree's underlying shared data is
     *         replaced wholesale (e.g. via operator=).
     *  @param treeWhichHasBeenChanged  The tree whose identity changed.
     */
    void valueTreeRedirected (juce::ValueTree& treeWhichHasBeenChanged) override;

    // Child proxies
    /** @brief Appends @p child to @c state.
     *  @param child        Node to append.
     *  @param undoManager  Optional undo manager for undo/redo support.
     */
    void appendChild (juce::ValueTree child, juce::UndoManager* undoManager = nullptr);

    /** @brief Removes @p child from @c state.
     *  @param child        Node to remove.
     *  @param undoManager  Optional undo manager for undo/redo support.
     */
    void removeChild (juce::ValueTree child, juce::UndoManager* undoManager = nullptr);

    /** @brief Recursively finds the first descendant of @c state with type @p name.
     *  @param name  Node type identifier to match.
     *  @return The matching node, or an invalid ValueTree if none found.
     */
    juce::ValueTree getChildWithName (const juce::Identifier& name) const noexcept;

    /** @brief Finds the direct child of @c state with type @p name, creating and
     *         appending one if absent. Delegates to juce::ValueTree::getOrCreateChildWithName —
     *         not recursive, unlike the static jam::Model::getOrCreateChildWithName overload.
     *  @param name         Node type identifier to match/create.
     *  @param undoManager  Optional undo manager for undo/redo support.
     *  @return The existing or newly created direct child.
     */
    juce::ValueTree getOrCreateChildWithName (const juce::Identifier& name,
                                              juce::UndoManager* undoManager = nullptr);

    // Property proxies
    /** @brief Sets a property directly on @c state.
     *  @param name         Property identifier.
     *  @param value        New value.
     *  @param undoManager  Optional undo manager for undo/redo support.
     */
    void setTreeProperty (const juce::Identifier& name,
                          const juce::var& value,
                          juce::UndoManager* undoManager = nullptr);

    /** @brief Reads a property directly from @c state.
     *  @param name  Property identifier.
     *  @return The property's current value.
     */
    juce::var getTreeProperty (const juce::Identifier& name) const noexcept;

    /** @brief Returns a live juce::Value bound to a property on @c state.
     *  @param name         Property identifier.
     *  @param undoManager  Optional undo manager for undo/redo support.
     *  @return A juce::Value bound to the property.
     */
    juce::Value
    getTreePropertyAsValue (const juce::Identifier& name, juce::UndoManager* undoManager = nullptr);

    // Identity
    /** @brief Returns @c state's node type identifier. */
    juce::Identifier getType() const noexcept;

    //==========================================================================
    // APVTS-pattern: atomic parameter store + flush timer
    //==========================================================================

    /**
     * @brief Single-pass flush: iterates adapters and OR-aggregates flushToTree() results.
     * @return true if any adapter updated the ValueTree.
     * @note MESSAGE THREAD only.
     */
    bool flush() noexcept;

    /**
     * @brief One AnyMap — nested for group hierarchy.
     * Public for Layout builders and derived class access.
     */
    mutable jam::AnyMap<> params;

    /** @brief Copies every matching property from @p sourceValueTree onto @c state, recursively.
     *  @param sourceValueTree  Tree to copy property values from.
     */
    void setValuesFrom (const juce::ValueTree& sourceValueTree) noexcept;

    /** @brief Returns a raw pointer to the atomic for a parameter in a named group.
     *  @tparam Type  Atomic value type (int, float, int64_t).
     *  @param tag    Group identifier (node type).
     *  @param id     Property identifier within the group.
     *  @return Pointer to the std::atomic<Type>, or nullptr if not found.
     */
    template<typename Type>
    std::atomic<Type>*
    getRawParameterValue (const juce::Identifier& tag, const juce::Identifier& id) const noexcept
    {
        if (params.contains (tag))
        {
            auto* group { params.get<jam::AnyMap<>> (tag) };

            if (group->contains (id))
                return &group->get<Parameter<Type>> (id)->getRawValue();
        }

        return nullptr;
    }

    /** @brief Returns a typed Parameter pointer from a named group.
     *  @tparam T  Parameter type (Parameter<int>, Parameter<float>, Parameter<int64_t>, ParameterText).
     *  @param tag  Group identifier (node type).
     *  @param id   Property identifier within the group.
     *  @return Pointer to the Parameter, or nullptr if not found.
     */
    template<typename T>
    T* getParameter (const juce::Identifier& tag, const juce::Identifier& id) const noexcept
    {
        if (params.contains (tag))
        {
            auto* group { params.get<jam::AnyMap<>> (tag) };

            if (group->contains (id))
                return group->get<T> (id);
        }

        return nullptr;
    }

    /** @brief Creates a typed Parameter, seeds VT, registers adapter.
     *  Parameter lives on Model forever. Returns reference for attachment.
     *
     *  JUCE analog: APVTS::createAndAddParameter — creates and owns the
     *  parameter. Call ParameterAttachment separately if a callback is needed.
     *
     *  @tparam ParameterType  Parameter\<int\>, Parameter\<float\>, Parameter\<int64_t\>,
     *                         Parameter\<T\> (packed domain type, e.g. jam::UUID,
     *                         jam::Bounds\<int16_t\>), or ParameterText.
     *  @tparam ValueType      Deduced from defaultValue. When juce::var is not
     *                         constructible from ValueType, it is treated as a
     *                         packed domain type and seeded on the tree via the
     *                         same int64 backing Parameter\<T\> uses.
     *  @param tree            ValueTree to bind to.
     *  @param name            Property identifier.
     *  @param defaultValue    Seeded on tree and Parameter.
     *  @param maxTextLength   For ParameterText only (ignored for numeric/packed).
     *  @return Reference to the created ParameterBase.
     */
    template<typename ParameterType, typename ValueType>
    ParameterBase& createAndAddParameter (juce::ValueTree& tree,
                                          const juce::Identifier& name,
                                          ValueType defaultValue,
                                          int maxTextLength = 256)
    {
        if constexpr (std::is_constructible_v<juce::var, ValueType>)
        {
            tree.setProperty (name, juce::var { defaultValue }, nullptr);
        }
        else
        {
            // Packed domain type: juce::var has no constructor for ValueType, so seed
            // the VT property with the same int64 backing Parameter<T> stores in its
            // atomic — 8-byte types bit_cast directly, 4-byte types widen through uint32_t.
            if constexpr (sizeof (ValueType) == 8)
                tree.setProperty (name, juce::var { jam::bit_cast<int64_t> (defaultValue) }, nullptr);
            else
                tree.setProperty (
                    name,
                    juce::var { static_cast<int64_t> (jam::bit_cast<uint32_t> (defaultValue)) },
                    nullptr);
        }

        const auto groupId { getGroupId (tree) };

        if (not params.contains (groupId))
            params.add<jam::AnyMap<>> (groupId);

        auto& group { *params.get<jam::AnyMap<>> (groupId) };

        if constexpr (std::is_same_v<ParameterType, ParameterText>)
        {
            group.add<ParameterType> (
                juce::Identifier { name }, name, name, maxTextLength, juce::String { defaultValue });
        }
        else
        {
            group.add<ParameterType> (
                juce::Identifier { name },
                name,
                static_cast<decltype (std::declval<ParameterType>().getValue())> (defaultValue),
                name);
        }

        auto& param { *group.get<ParameterType> (juce::Identifier { name }) };
        addParameterAdapter (param, tree);

        return param;
    }

    /** @brief Registers a ParameterAdapter for the given parameter and tree.
     *  Called by createAndAddParameter.
     *  @param parameter  The parameter to adapt.
     *  @param tree       The ValueTree node the parameter is bound to.
     *  @note MESSAGE THREAD — called during construction only.
     */
    void addParameterAdapter (ParameterBase& parameter, juce::ValueTree& tree);

    /** @brief Returns the current value of a named property from the
     *         recursively located child tree of the given type.
     *
     *  @details
     *  Delegates to @c getValueFromChildWithProperty(state, type, name) which
     *  calls @c Model::getChildWithName(root, type) — a recursive depth-first
     *  walk that finds the first descendant node whose @c getType() matches
     *  @p type regardless of nesting depth — then returns
     *  @c .getPropertyAsValue(name).getValue().
     *
     *  Example: @c getValue (Id::toType (Id::tab), Id::fontFamily) resolves
     *  @c state > display > tab > fontFamily independent of how many levels
     *  of nesting exist between the root and the @c tab node.
     *
     *  @pre The child tree with type @p type must exist in the tree
     *       (@c jassert on invalid tree).
     *  @pre The child tree must have a property named @p name
     *       (@c jassert on missing property).
     *
     *  @param type  Identifier of the descendant node type to locate.
     *  @param name  Identifier of the property to read from that node.
     *  @return      The property's current @c juce::var value.
     *  @note        Any thread. Lock-free (reads value, no write).
     */
    juce::var getValue (const juce::Identifier& type, const juce::Identifier& name) const noexcept;

    /** @brief Sets the value of a named property on the recursively located
     *         child tree of the given type.
     *
     *  @details
     *  Delegates to @c getValueFromChildWithProperty(state, type, name) — same
     *  recursive lookup as @c getValue — then calls @c .setValue(newValue)
     *  on the returned @c juce::Value handle. The mutation is applied
     *  in-place on the live @c juce::ValueTree and fires any registered
     *  listeners.
     *
     *  @pre The child tree with type @p type must exist in the tree
     *       (@c jassert on invalid tree).
     *  @pre The child tree must have a property named @p name
     *       (@c jassert on missing property).
     *
     *  @param type      Identifier of the descendant node type to locate.
     *  @param name      Identifier of the property to write.
     *  @param newValue  The new value to assign.
     *  @note            MESSAGE THREAD — triggers ValueTree listeners.
     */
    void setValue (const juce::Identifier& type,
                   const juce::Identifier& name,
                   const juce::var& newValue);

    /** @brief Reads an Array\<var\> property and packs up to 4 elements as int16_t.
     *
     *  Returns a 4-slot Union\<int16_t\>. Unused slots (when the array has
     *  fewer than 4 elements) are zero. Consumer destructures via structured
     *  binding — only the slots matching the array size carry values.
     *
     *  @param type  Identifier of the descendant node type (recursive search).
     *  @param name  Identifier of the Array\<var\> property.
     *  @return      Packed Union of up to 4 int16_t values.
     */
    jam::Union<int16_t, int16_t, int16_t, int16_t>
    getInt16 (const juce::Identifier& type, const juce::Identifier& name) const noexcept;

    /** @brief Reads an Array\<var\> property and packs up to 2 elements as int32_t.
     *
     *  Returns a 2-slot Union\<int32_t\>. Consumer destructures via structured
     *  binding.
     *
     *  @param type  Identifier of the descendant node type (recursive search).
     *  @param name  Identifier of the Array\<var\> property.
     *  @return      Packed Union of up to 2 int32_t values.
     */
    jam::Union<int32_t, int32_t>
    getInt (const juce::Identifier& type, const juce::Identifier& name) const noexcept;

    /** @brief The owned ValueTree state. */
    juce::ValueTree state;

private:
    class ParameterAdapter;

    /** @brief Calls flush() on the adaptive-rate timer (flushHz while dirty, idleHz otherwise). */
    void timerCallback() override;

    // Composite type+id key for instance trees (Id::id present) — each
    // same-typed sibling gets its own group. Type-only key for singleton
    // trees (no id) — identical to the pre-existing type-only grouping.
    static juce::Identifier getGroupId (const juce::ValueTree& tree) noexcept;

    void updateAdapterConnections (const juce::ValueTree& root);

    static constexpr int flushHz { 120 };
    static constexpr int idleHz { 60 };

    juce::CriticalSection
        valueTreeChanging;///< Serializes ValueTree mutations: flush, replaceState, copyState. 1:1 APVTS analog.

    class LockedListeners
    {
    public:
        template<typename Fn>
        void call (Fn&& fn)
        {
            const juce::CriticalSection::ScopedLockType lock (mutex);
            listeners.call (std::forward<Fn> (fn));
        }

        void add (Listener* l)
        {
            const juce::CriticalSection::ScopedLockType lock (mutex);
            listeners.add (l);
        }

        void remove (Listener* l)
        {
            const juce::CriticalSection::ScopedLockType lock (mutex);
            listeners.remove (l);
        }

    private:
        juce::CriticalSection mutex;
        juce::ListenerList<Listener> listeners;
    };

    LockedListeners parameterListeners;
    jam::HashMap<juce::Identifier,
                 jam::HashMap<juce::Identifier, std::unique_ptr<ParameterAdapter>>> adapters;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Model)
};

/**_____________________________END OF NAMESPACE______________________________*/
}// namespace jam
