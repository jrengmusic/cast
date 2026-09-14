/**
 * @file jam_ComponentUtils.h
 * @brief Free-function helpers for constructing, finding, and traversing juce::Component trees.
 */
namespace jam
{
/*____________________________________________________________________________*/
/**
 * @namespace jam::Component
 * @brief Free-function helpers for constructing, finding, and traversing `juce::Component` trees.
 */
namespace Component
{
/*____________________________________________________________________________*/

/** @brief Shared timer rate, in Hz, for components that poll their model on a repeating timer. */
static constexpr int refreshRateHz { 60 };

/**---------------------------- STATIC FUNCTIONS -----------------------------*/
//==============================================================================
/**
 * @brief Constructs a child component and adds it as visible.
 * @tparam ComponentType  Component type to construct.
 * @tparam PointerType    Smart pointer type receiving the constructed component.
 * @param parent   Component to add the child to.
 * @param pointer  Owning pointer that receives the new component.
 * @param args     Arguments forwarded to `ComponentType`'s constructor.
 */
template <typename ComponentType, typename PointerType, typename... Args>
static void addAndMakeVisible (juce::Component* parent, PointerType& pointer, Args&&... args)
{
    pointer = std::make_unique<ComponentType> (std::forward<Args> (args)...);
    parent->addAndMakeVisible (*pointer);
}

/**
 * @brief Constructs a child component and adds it as visible, given an owning-pointer parent.
 * @tparam ComponentType        Component type to construct.
 * @tparam ParentComponentType  Owning pointer type of the parent component.
 * @tparam PointerType          Smart pointer type receiving the constructed component.
 * @param parent   Owning pointer to the parent component.
 * @param pointer  Owning pointer that receives the new component.
 * @param args     Arguments forwarded to `ComponentType`'s constructor.
 */
template <typename ComponentType, typename ParentComponentType, typename PointerType, typename... Args>
static void addAndMakeVisible (ParentComponentType& parent, PointerType& pointer, Args&&... args)
{
    pointer = std::make_unique<ComponentType> (std::forward<Args> (args)...);
    parent->addAndMakeVisible (*pointer);
}

/**
 * @brief Exits every currently active modal component's modal state.
 * @param component  Unused.
 */
static inline void dismissModal ([[maybe_unused]] const juce::Component* component)
{
    for (int index { juce::Component::getNumCurrentlyModalComponents() }; --index >= 0;)
    {
        if (auto* modal { juce::Component::getCurrentlyModalComponent (index) })
            modal->exitModalState (0);
    }
}

/**
 * @brief Applies a function to a component and all of its descendants, depth-first.
 * @tparam Function  Callable accepting a `juce::Component*`.
 * @param component  Root component to start from.
 * @param function   Function applied to @p component and every descendant.
 */
template <typename Function>
static void applyFunctionRecursively (juce::Component* component, const Function& function)
{
    function (component);

    for (auto& child : component->getChildren())
        applyFunctionRecursively (child, function);
}

/** @brief Test whether the property stored under @p key in @p properties is a string equal to @p value.
 *  @param properties  The component's property set.
 *  @param key         The property key to look up.
 *  @param value       The text to match.
 *  @return True if the property is present, is a string, and equals @p value's text; false otherwise. */
static bool hasProperty (const juce::NamedValueSet& properties, const juce::Identifier& key, const juce::Identifier& value) noexcept
{
    const auto& property { properties[key] };
    return property.isString() and property.toString().compare (value.toString()) == 0;
}

/**
 * @brief Sizes the host AudioProcessorEditor to the view's UI size and applies its scale transform.
 * @tparam ViewType   The view component type, supplying getUISize (ModelType&) and getUIScaleFactor (ModelType&).
 * @tparam ModelType  The model type forwarded to @p view's sizing queries.
 * @param view The view whose parent AudioProcessorEditor is resized and which receives the scale transform.
 * @param model The model supplying the UI size and scale factor.
 */
template <typename ViewType, typename ModelType>
static void resizeEditor (ViewType& view, ModelType& model)
{
    auto* editor { view.template findParentComponentOfClass<juce::AudioProcessorEditor>() };
    jassert (editor != nullptr);

    auto [width, height] { view.getUISize (model) };
    editor->setSize (width, height);

    auto scaleFactor { view.getUIScaleFactor (model) };
    view.setTransform (juce::AffineTransform::scale (scaleFactor));
}

/**_____________________________END OF NAMESPACE______________________________*/
}// namespace Component

/**
 * @struct TextEditorUtils
 * @brief Static helpers for `juce::TextEditor` layout.
 */
struct TextEditorUtils
{
    /**
     * @brief Centers a text editor's text vertically by setting its border insets.
     * @param editor          Text editor to adjust.
     * @param multiLineInset  Border inset used when the editor is tall enough for multiple lines.
     */
    static void setTextCentered (juce::TextEditor& editor, int multiLineInset = 4)
    {
        auto fontHeight = editor.getFont().getHeight();
        const bool shouldUseMultilineInset { editor.getHeight() >= 2 * fontHeight };
        const int singleLineInset { (editor.getHeight() - toInt (fontHeight)) / 2 };

        editor.setBorder (juce::BorderSize<int> (shouldUseMultilineInset ? multiLineInset : singleLineInset));
        editor.setIndents (0, 0);
    }
};

/**_____________________________END OF NAMESPACE______________________________*/
}// namespace jam
