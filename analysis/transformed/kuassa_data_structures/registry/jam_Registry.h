/**
 * @file jam_Registry.h
 * @brief Central per-application registry of component factories, style factories,
 * and dispatch tables keyed by component type.
 */

namespace jam
{
/*____________________________________________________________________________*/

/**
 * @class Registry
 * @brief Owns the type-keyed factory and dispatch tables that turn a document's
 * component descriptors into live `juce::Component`/`juce::LookAndFeel` instances.
 *
 * Each `register*` method is called once per component type during application
 * registration (see Registration/registerApplicationsFor), populating one entry
 * per dispatch table for every capability the registered `ObjectType` implements
 * (detected via `jam::Traits`). Consumers then dispatch purely by type string —
 * `make.get (type)` constructs, `bindTo.get (type, ...)` binds, and so on — with
 * no per-type conditional in consuming code.
 */
class Registry : public Instance<Registry>
{
public:
    /** @brief One registration callback per application-defined registration phase,
     *  invoked in field order by registerApplicationsFor(). Any callback left unset
     *  is skipped. */
    struct Registration
    {
        std::function<void (Registry&)> buttons;        ///< Registers button component types.
        std::function<void (Registry&)> components;     ///< Registers non-button component types.
        std::function<void (Registry&)> styles;         ///< Registers LookAndFeel types.
        std::function<void (Registry&)> attachments;    ///< Registers APVTS attachment bindings.
        std::function<void (Registry&)> config;         ///< Registers per-type configuration callbacks.
        std::function<void (Registry&)> events;         ///< Registers event/trigger bindings.
        std::function<void (Registry&)> viewComponents; ///< Registers view-only component bindings.
        std::function<void (Registry&)> viewBindings;   ///< Registers view-to-model bindings.
        std::function<void (Registry&)> makeContent;    ///< Registers content-component factories.
    };

    using Make = Function::MapType<juce::String, std::unique_ptr<juce::Component>>; ///< type -> component factory.
    using Style = Function::Map<juce::String, std::unique_ptr<juce::LookAndFeel>>;  ///< type -> LookAndFeel factory.
    using SetImage = Function::Map<juce::String, void>;                             ///< type -> image setter dispatch.
    using Configure = jam::HashMap<juce::String, Function::MapType<juce::String, void>>; ///< type -> (config key -> configurator).
    using BindTo = Function::Map<juce::String, void>;                               ///< type -> bind dispatch.
    using BindToView = Function::Map<juce::String, void>;                           ///< type -> view-bind dispatch.
    using SetProcessorChain = Function::Map<juce::String, void>;                    ///< type -> processor-chain setter dispatch.
    using SetFont = Function::Map<juce::String, void>;                              ///< type -> font setter dispatch.
    using Attach = Function::Map<juce::String, void>;                               ///< type -> APVTS attachment dispatch.
    using BindEvent = Function::Map<juce::String, void>;                            ///< type -> event-binding dispatch.
    using MakeContent = Function::Map<juce::String, std::unique_ptr<juce::Component>>; ///< content key -> content factory.
    using Callbacks = Function::Map<juce::String, void>;                            ///< type -> callback dispatch.

    Make make;                        ///< Component factories, keyed by type.
    Style style;                      ///< LookAndFeel factories, keyed by type.
    SetImage setImage;                ///< Image setters for types implementing `setImage`.
    SetImage setStyleImage;           ///< Image setters for LookAndFeel types implementing `setImage`.
    SetImage setClipImage;            ///< Clip-image setters for types implementing `setClipImage`.
    SetFont setFont;                  ///< Font setters for types implementing `setFont`.
    BindTo bindTo;                    ///< `bindTo (juce::Component*)` dispatch for types implementing it.
    Attach attach;                    ///< APVTS attachment factories, keyed by type.
    Configure configure;              ///< Per-type configuration callbacks, keyed by type then config key.
    BindTo bindToProcessor;           ///< `bindToProcessor` dispatch for types implementing it.
    BindTo bindToModel;               ///< `bindToModel` dispatch for types implementing it.
    BindToView bindToView;            ///< View-only component bindings, keyed by componentID.
    BindToView bindToPanel;           ///< `bindToPanel (Registry&, Document::Element, AudioModel&)` dispatch.
    BindEvent bindEvent;              ///< Event/trigger bindings, keyed by type.
    MakeContent makeContent;          ///< Content-component factories, keyed by content key.
    Callbacks callbacks;              ///< Miscellaneous callback dispatch, keyed by type.

    /**
     * @brief Registers a configuration callback under @p key, applied to every
     * component type (`Id::all` bucket).
     * @tparam ObjectType     Concrete component type the callback receives.
     * @param key             Configuration key the callback is stored under.
     * @param configFunction  Callback invoked with the component cast to @p ObjectType.
     */
    template <typename ObjectType>
    void registerConfig (const juce::Identifier& key, std::function<void (ObjectType*)> configFunction)
    {
        auto setter = [configFunction] (juce::Component* c)
        {
            configFunction (static_cast<ObjectType*> (c));
        };

        configure[key.toString()].template add<ObjectType, juce::Component*> (Id::all, std::move (setter));
    }

    /**
     * @brief Registers a configuration callback under @p key, scoped to @p typeString.
     * @tparam ObjectType     Concrete component type the callback receives.
     * @param typeString      Component type this callback applies to.
     * @param key             Configuration key the callback is stored under.
     * @param configFunction  Callback invoked with the component cast to @p ObjectType.
     */
    template <typename ObjectType>
    void registerConfig (const juce::Identifier& typeString,
                         const juce::Identifier& key,
                         std::function<void (ObjectType*)> configFunction)
    {
        auto setter = [configFunction] (juce::Component* c)
        {
            configFunction (static_cast<ObjectType*> (c));
        };

        configure[typeString.toString()].template add<ObjectType, juce::Component*> (key, std::move (setter));
    }

    /** @brief Constructs the registry and immediately registers every application
     *  supplied via @p r (see registerApplicationsFor()). */
    explicit Registry (const Registration& r);

    ~Registry() = default;

    /**
     * @brief Registers a component type: its factory, and its image/bindTo/
     * bindToProcessor/bindToModel/bindToPanel/clip-image dispatch entries for
     * every capability @p ObjectType implements.
     * @tparam ObjectType  Concrete `juce::Component` subtype to register.
     * @tparam Args        Constructor argument types forwarded to @p ObjectType
     *                     on every factory invocation; empty for a default-constructed factory.
     * @param type         Component type key.
     * @param args         Constructor arguments captured by the factory.
     */
    template <typename ObjectType, typename... Args>
    void registerComponent (juce::StringRef type, Args&&... args)
    {
        if constexpr (sizeof...(Args) == 0)
        {
            make.template add<ObjectType> (type, makeFactory<ObjectType, juce::Component> (make, type));
        }
        else
        {
            make.template add<ObjectType> (type,
                      [capturedArgs = std::tuple<Args...> (std::forward<Args> (args)...)]() mutable
                      {
                          return std::apply (
                              [] (auto&&... constructorArgs)
                              {
                                  return std::make_unique<ObjectType> (
                                      std::forward<decltype (constructorArgs)> (constructorArgs)...);
                              },
                              capturedArgs);
                      });
        }

        registerTypeWithImage<ObjectType, juce::Component> (setImage, type);
        registerTypeWithBindTo<ObjectType, juce::Component> (bindTo, type);
        registerTypeWithBindToProcessor<ObjectType, juce::Component> (bindToProcessor, type);
        registerTypeWithBindToModel<ObjectType, juce::Component> (bindToModel, type);
        registerTypeWithBindToPanel<ObjectType, juce::Component> (bindToPanel, type);
        registerTypeWithClipImage<ObjectType, juce::Component> (setClipImage, type);
    }

    /**
     * @brief Registers a LookAndFeel type: its factory and its image/font
     * dispatch entries for every capability @p ObjectType implements.
     * @tparam ObjectType  Concrete `juce::LookAndFeel` subtype to register.
     * @param type         Style type key.
     */
    template <typename ObjectType>
    void registerStyle (juce::StringRef type)
    {
        style.add (type, makeFactory<ObjectType, juce::LookAndFeel> (style, type));
        registerTypeWithImage<ObjectType, juce::LookAndFeel> (setStyleImage, type);
        registerTypeWithSetFont<ObjectType, juce::LookAndFeel> (setFont, type);
    }

    /**
     * @brief Registers a view-only binding under @p componentID.
     * @tparam Func         Callable matching `void (juce::Component*&, const Document::Element&, Registry&, AudioModel&)`.
     * @param componentID   Component identifier this binding applies to.
     * @param func          Binding callback.
     */
    template <typename Func>
    void registerViewComponent (juce::StringRef componentID, Func&& func)
    {
        bindToView.add<juce::Component*&, const Document::Element&, Registry&, AudioModel&> (
            componentID, std::forward<Func> (func));
    }

    /**
     * @brief Registers an APVTS attachment factory for @p type.
     *
     * The registered entry casts the component to @p ObjectType and, when the
     * cast succeeds, constructs an @p AttachmentType bound to the given
     * parameter and hands ownership to @p owner.
     * @tparam ObjectType      Concrete component type the attachment targets.
     * @tparam AttachmentType  Attachment type constructed as `AttachmentType (AudioModel&, juce::String, ObjectType&)`.
     * @param type             Component type key.
     */
    template <typename ObjectType, typename AttachmentType>
    void registerAttachment (juce::StringRef type)
    {
        auto a = [] (AnyOwner* owner,
                     juce::Component* c,
                     AudioModel* state,
                     const juce::String& paramID)
        {
            if (auto* target { dynamic_cast<ObjectType*> (c) })
            {
                auto attachment { std::unique_ptr<AttachmentType> (new AttachmentType (*state, paramID, *target)) };

                owner->add (std::move (attachment));
            }
        };

        attach.template add<AnyOwner*, juce::Component*&, AudioModel*, const juce::String&> (
            type, std::move (a));
    }

    /** @brief Invokes every set callback in @p r, in Registration field order, then
     *  asserts (debug only) that every registered configure type is also a make type. */
    void registerApplicationsFor (const Registration& r);

    /** @return The component type key for @p element — its `Id::codeClass` when
     *  present and non-empty, the `Id::type` attribute for `Id::input` elements,
     *  otherwise the element's own tag identifier. */
    static juce::String getType (const Document::Element& element);

    /** @return The `Id::dataParameter` attribute of @p element, or an empty string. */
    static juce::String getParameter (const Document::Element& element);

    /** @return The `Id::dataContent` attribute of @p element, or an empty string. */
    static juce::String getContent (const Document::Element& element);

    /** @return The `Id::dataButton` attribute of @p element, or an empty string. */
    static juce::String getButton (const Document::Element& element);

    /** @brief Downcasts a registry-made component to `juce::Button`, transferring ownership.
     *  @param component  Component known to be a `juce::Button` at the call site.
     *  @return The same object, retyped as a `juce::Button`. */
    static std::unique_ptr<juce::Button> toButton (std::unique_ptr<juce::Component> component);

    /** @return The absolute pixel bounds declared on @p element via `Id::position` =
     *  `Id::absolute` and numeric `Id::left`/`Id::top`/`Id::width`/`Id::height`
     *  attributes, or an empty rectangle when any of those is absent. */
    static juce::Rectangle<int> getBounds (const Document::Element& element);

    /** @return `true` when @p tag is a known HTML tag, per `map::HtmlStandardTag`. */
    static bool isStandardTag (const juce::String& tag);

    /** @return `true` when @p element's tag is a known HTML tag, per `map::HtmlStandardTag`. */
    static bool isStandardTag (const Document::Element& element);

    /** @return `true` when @p element is a `div` with non-empty `Id::id` and
     *  `Id::codeClass`, or a standard HTML tag with a non-empty `Id::id`. */
    static bool isValid (const Document::Element& element);

    //==============================================================================
private:
    /** @brief Adds a `type -> setImage` entry to @p typesWithImageMap when
     *  @p ObjectType implements `setImage`; no-op otherwise. */
    template <typename ObjectType, typename BaseType>
    void registerTypeWithImage (SetImage& typesWithImageMap, juce::StringRef type)
    {
        if constexpr (Traits::HasImage<ObjectType>::value)
        {
            auto set = [] (BaseType* base, const juce::Image& image)
            {
                if (auto* p { dynamic_cast<ObjectType*> (base) })
                    p->setImage (image);
            };

            typesWithImageMap.template add<BaseType*&, juce::Image&> (type, set);
        }
    }

    /** @brief Adds a `type -> setClipImage` entry to @p typesWithClipImageMap when
     *  @p ObjectType implements `setClipImage`; no-op otherwise. */
    template <typename ObjectType, typename BaseType>
    void registerTypeWithClipImage (SetImage& typesWithClipImageMap, juce::StringRef type)
    {
        if constexpr (Traits::HasSetClipImage<ObjectType>::value)
        {
            auto set = [] (BaseType* base, const juce::Image& image)
            {
                if (auto* p { dynamic_cast<ObjectType*> (base) })
                    p->setClipImage (image);
            };

            typesWithClipImageMap.template add<BaseType*&, juce::Image&> (type, set);
        }
    }

    /** @brief Adds a `type -> bindTo (juce::Component*)` entry to @p typesWithBindToMap
     *  when @p ObjectType implements `bindTo`; no-op otherwise. */
    template <typename ObjectType, typename BaseType>
    void registerTypeWithBindTo (BindTo& typesWithBindToMap, juce::StringRef type)
    {
        if constexpr (Traits::HasBindTo<ObjectType>::value)
        {
            auto set = [] (BaseType* base, juce::Component* primary)
            {
                if (auto* p { dynamic_cast<ObjectType*> (base) })
                    p->bindTo (primary);
            };

            typesWithBindToMap.template add<BaseType*&, juce::Component*&> (type, set);
        }
    }

    /** @brief Adds a `type -> bindToProcessor` entry to @p typesWithBindToProcessorMap
     *  when @p ObjectType implements `bindToProcessor`; no-op otherwise.
     *
     *  The registered entry closes over @p type as the nested map's own key —
     *  forwarding it as the `type` argument to `bindToProcessor` — so the
     *  consuming component always looks itself up under its own registered
     *  type, and the caller's string parameter ID is forwarded as a
     *  `juce::Identifier`. */
    template <typename ObjectType, typename BaseType>
    void registerTypeWithBindToProcessor (BindTo& typesWithBindToProcessorMap, juce::StringRef type)
    {
        if constexpr (Traits::HasBindToProcessor<ObjectType>::value)
        {
            auto set = [typeKey = juce::Identifier { juce::String { type } }] (BaseType* base, jam::HashMap<juce::Identifier, jam::Function::Map<juce::Identifier, void>>* dataMap, const juce::String& paramID)
            {
                if (auto* p { dynamic_cast<ObjectType*> (base) })
                    p->bindToProcessor (*dataMap, typeKey, juce::Identifier { paramID });
            };

            typesWithBindToProcessorMap
                .template add<BaseType*&, jam::HashMap<juce::Identifier, jam::Function::Map<juce::Identifier, void>>*, const juce::String&> (type, set);
        }
    }

    /** @brief Adds a `type -> bindToModel` entry to @p typesWithBindToModelMap when
     *  @p ObjectType implements `bindToModel`; no-op otherwise. */
    template <typename ObjectType, typename BaseType>
    void registerTypeWithBindToModel (BindTo& typesWithBindToModelMap, juce::StringRef type)
    {
        if constexpr (Traits::HasBindToModel<ObjectType>::value)
        {
            auto set = [] (BaseType* base, const juce::String& parameterID, AudioModel& model)
            {
                if (auto* p { dynamic_cast<ObjectType*> (base) })
                    p->bindToModel (parameterID, model);
            };

            typesWithBindToModelMap.template add<BaseType*&, const juce::String&, AudioModel&> (type, set);
        }
    }

    /** @brief Adds a `type -> bindToPanel` entry to @p typesWithBindToPanelMap when
     *  @p ComponentType implements `bindToPanel (Registry&, const Document::Element&, AudioModel&)`;
     *  no-op otherwise. */
    template <typename ComponentType, typename BaseType>
    void registerTypeWithBindToPanel (BindToView& typesWithBindToPanelMap, juce::StringRef type)
    {
        if constexpr (Traits::HasBindToPanel<ComponentType, Registry>::value)
        {
            auto set = [] (BaseType* base, const Document::Element& element, Registry& reg, AudioModel& model)
            {
                if (auto* p { dynamic_cast<ComponentType*> (base) })
                    p->bindToPanel (reg, element, model);
            };

            typesWithBindToPanelMap.template add<BaseType*&, const Document::Element&, Registry&, AudioModel&> (type, set);
        }
    }

    /** @brief Adds a `type -> setFont` entry to @p typesWithSetFontMap when
     *  @p ObjectType implements `setFont`; no-op otherwise. */
    template <typename ObjectType, typename BaseType>
    void registerTypeWithSetFont (SetFont& typesWithSetFontMap, juce::StringRef type)
    {
        if constexpr (Traits::HasSetFont<ObjectType>::value)
        {
            auto set = [] (BaseType* base, juce::StringRef alias, const juce::FontOptions& font)
            {
                if (auto* p { dynamic_cast<ObjectType*> (base) })
                    p->setFont (alias, font);
            };

            typesWithSetFontMap.template add<BaseType*&, juce::StringRef, const juce::FontOptions&> (type, set);
        }
    }

    /** @return A zero-argument factory that constructs a default `ObjectType` and
     *  returns it as a `std::unique_ptr<BaseType>`, guarded by @p map still
     *  containing @p type at call time — `nullptr` otherwise. */
    template <typename ObjectType, typename BaseType, typename MapType>
    auto makeFactory (MapType& map, juce::String type) noexcept
    {
        return [&map, type]() -> std::unique_ptr<BaseType>
        {
            if (map.contains (type))
                return std::make_unique<ObjectType>();
            return nullptr;
        };
    }

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Registry)
};

/**_____________________________END OF NAMESPACE______________________________*/
}// namespace jam
