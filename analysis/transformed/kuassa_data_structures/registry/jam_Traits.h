namespace jam
{
/*____________________________________________________________________________*/

struct Traits
{
    template <typename ComponentTypeOrLookAndFeelType, typename = void>
    struct HasSetImage : std::false_type
    {
    };

    template <typename ComponentTypeOrLookAndFeelType>
    struct HasSetImage<ComponentTypeOrLookAndFeelType,
                       std::void_t<decltype (std::declval<ComponentTypeOrLookAndFeelType>().setImage (std::declval<juce::Image>()))>>
        : std::true_type
    {
    };

    template <typename ComponentType, typename = void>
    struct HasSetImageRect : std::false_type
    {
    };

    template <typename ComponentType>
    struct HasSetImageRect<ComponentType,
                           std::void_t<decltype (std::declval<ComponentType>().setImage (std::declval<juce::Image>(), std::declval<juce::RectanglePlacement>()))>>
        : std::true_type
    {
    };

    template <typename ComponentTypeOrLookAndFeelType, typename = void>
    struct HasSetClipImage : std::false_type
    {
    };

    template <typename ComponentTypeOrLookAndFeelType>
    struct HasSetClipImage<ComponentTypeOrLookAndFeelType,
                           std::void_t<decltype (std::declval<ComponentTypeOrLookAndFeelType>().setClipImage (std::declval<juce::Image>()))>>
        : std::true_type
    {
    };

    template <typename ComponentTypeOrLookAndFeelType>
    using HasImage = std::disjunction<HasSetImage<ComponentTypeOrLookAndFeelType>, HasSetImageRect<ComponentTypeOrLookAndFeelType>>;

    template <typename ComponentType, typename = void>
    struct HasBindTo : std::false_type
    {
    };

    template <typename ComponentType>
    struct HasBindTo<ComponentType,
                     std::void_t<decltype (&ComponentType::bindTo)>> : std::true_type
    {
    };

    template <typename ComponentType, typename = void>
    struct HasBindToProcessor : std::false_type
    {
    };

    template <typename ComponentType>
    struct HasBindToProcessor<ComponentType,
                     std::void_t<decltype (&ComponentType::bindToProcessor)>> : std::true_type
    {
    };

    //==============================================================================
    template <typename ComponentType, typename = void>
    struct HasBindToModel : std::false_type
    {
    };

    template <typename ComponentType>
    struct HasBindToModel<ComponentType,
                          std::void_t<decltype (std::declval<ComponentType>().bindToModel (
                              std::declval<const juce::String&> (), std::declval<AudioModel&> ()))>>
        : std::true_type
    {
    };

    template <typename ComponentType, typename = void>
    struct HasAddItemList : std::false_type
    {
    };

    template <typename ComponentType>
    struct HasAddItemList<ComponentType,
                          std::void_t<decltype (std::declval<ComponentType>().addItemList (
                              std::declval<const std::map<int, juce::String>&>()))>>
        : std::true_type
    {
    };

    template <typename ComponentType, typename = void>
    struct HasValueObject : std::false_type
    {
    };

    template <typename ComponentType>
    struct HasValueObject<ComponentType,
                          std::void_t<decltype (std::declval<ComponentType>().getValueObject())>>
        : std::true_type
    {
    };

    template <typename ComponentType, typename RegistryType, typename = void>
    struct HasBindToPanel : std::false_type
    {
    };

    template <typename ComponentType, typename RegistryType>
    struct HasBindToPanel<ComponentType, RegistryType,
                          std::void_t<decltype (std::declval<ComponentType>().bindToPanel (
                              std::declval<RegistryType&> (), std::declval<const Document::Element&> (), std::declval<AudioModel&> ()))>>
        : std::true_type
    {
    };

    template <typename ObjectType, typename = std::void_t<>>
    struct HasSetFont : std::false_type
    {
    };

    template <typename ObjectType>
    struct HasSetFont<ObjectType,
                      std::void_t<decltype (std::declval<ObjectType>().setFont (
                          std::declval<juce::StringRef>(),
                          std::declval<const juce::FontOptions&>()))>>
        : std::true_type
    {
    };

};

/**_____________________________END OF NAMESPACE______________________________*/
}// namespace jam
