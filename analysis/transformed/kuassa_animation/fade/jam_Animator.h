/**
 * @file jam_Animator.h
 * @brief Static helpers wrapping `juce::ComponentAnimator` fade/cross-fade transitions.
 */

namespace jam
{
/*__________________________________________________________________________________________*/

/**
 * @struct Animator
 * @brief Namespace-like collection of static fade and cross-fade transition helpers.
 *
 * All members are static; the struct is never instantiated.
 */
struct Animator
{
    /** @struct Duration
     * @brief Default fade timing constants, in milliseconds.
     */
    struct Duration
    {
        static const int fadeIn { 150 };          ///< Default fade-in duration, in milliseconds.
        static const int fadeOut { 2 * fadeIn };   ///< Default fade-out duration, in milliseconds.
    };

    /**
     * @brief Fades a component in or out and updates its visibility.
     * @param component      Component to fade. Must not be null.
     * @param shouldBeVisible  `true` to fade in, `false` to fade out.
     * @param fadeInTimeMs   Fade-in duration, in milliseconds.
     * @param fadeOutTimeMs  Fade-out duration, in milliseconds.
     */
    static void toggleFade (juce::Component* component,
                            bool shouldBeVisible,
                            int fadeInTimeMs = Duration::fadeIn,
                            int fadeOutTimeMs = Duration::fadeOut)
    {
        if (shouldBeVisible)
        {
            juce::Desktop::getInstance().getAnimator().fadeIn (component, fadeInTimeMs);
            component->setVisible (shouldBeVisible);
        }
        else
        {
            juce::Desktop::getInstance().getAnimator().fadeOut (component, fadeOutTimeMs);
            component->setVisible (shouldBeVisible);
        }
    }

    /**
     * @brief Fades an owned component in or out and updates its visibility.
     * @tparam ComponentType  Component type held by @p component.
     * @param component      Owning pointer to the component to fade.
     * @param shouldBeVisible  `true` to fade in, `false` to fade out.
     * @param fadeInTimeMs   Fade-in duration, in milliseconds.
     * @param fadeOutTimeMs  Fade-out duration, in milliseconds.
     */
    template <typename ComponentType>
    static void toggleFade (const std::unique_ptr<ComponentType>& component,
                            bool shouldBeVisible,
                            int fadeInTimeMs = Duration::fadeIn,
                            int fadeOutTimeMs = Duration::fadeOut)
    {
        toggleFade (component.get(), shouldBeVisible, fadeInTimeMs, fadeOutTimeMs);
    }

    /**
     * @brief Cross-fades between two components, fading one out and the other in.
     *
     * The incoming component only starts fading in once the outgoing component's
     * animation has finished.
     *
     * @param firstComponent   Component shown when @p toggle is `true`.
     * @param secondComponent  Component shown when @p toggle is `false`.
     * @param toggle           `true` to show @p firstComponent, `false` to show @p secondComponent.
     * @param durationMs       Duration of each fade, in milliseconds.
     */
    static void toggleCrossFade (juce::Component* firstComponent,
                                 juce::Component* secondComponent,
                                 bool toggle,
                                 int durationMs = Duration::fadeOut)
    {
        if (toggle)
        {
            juce::Desktop::getInstance().getAnimator().fadeOut (secondComponent, durationMs);
            secondComponent->setVisible (not toggle);

            if (not juce::Desktop::getInstance().getAnimator().isAnimating())
            {
                juce::Desktop::getInstance().getAnimator().fadeIn (firstComponent, durationMs);
                firstComponent->setVisible (toggle);
            }
        }
        else
        {
            juce::Desktop::getInstance().getAnimator().fadeOut (firstComponent, durationMs);
            firstComponent->setVisible (not toggle);

            if (not juce::Desktop::getInstance().getAnimator().isAnimating())
            {
                juce::Desktop::getInstance().getAnimator().fadeIn (secondComponent, durationMs);
                secondComponent->setVisible (toggle);
            }
        }
    }

    /**
     * @brief Cross-fades a parent component through a snapshot overlay while applying a mutation.
     *
     * Captures a snapshot of @p parent, shows it in @p overlay, applies @p apply asynchronously,
     * then fades the snapshot out to reveal the mutated parent underneath.
     *
     * @param parent      Component to snapshot and mutate.
     * @param overlay     Owning pointer that receives the snapshot `juce::ImageComponent`.
     * @param apply       Mutation applied to @p parent after the snapshot is taken.
     * @param durationMs  Fade-out duration of the snapshot overlay, in milliseconds.
     */
    static void crossFadeWith (juce::Component* parent,
                               std::unique_ptr<juce::ImageComponent>& overlay,
                               std::function<void()> apply,
                               int durationMs = Duration::fadeOut)
    {
        overlay.reset();
        auto image { parent->createComponentSnapshot (parent->getLocalBounds()) };

        if (image.isValid())
        {
            overlay = std::make_unique<juce::ImageComponent>();
            overlay->setImage (image);
            parent->addAndMakeVisible (overlay.get());
            overlay->setBounds (parent->getLocalBounds());
            overlay->toFront (false);
        }

        juce::Component::SafePointer<juce::Component> safeParent { parent };
        juce::Component::SafePointer<juce::ImageComponent> safeOverlay { overlay.get() };

        juce::MessageManager::callAsync (
            [safeParent, safeOverlay, apply = std::move (apply), durationMs]
            {
                if (safeParent != nullptr)
                {
                    apply();

                    if (safeOverlay != nullptr)
                        toggleFade (safeOverlay.getComponent(), false, Duration::fadeIn, durationMs);
                }
            });
    }
};

/**____________________________________END OF NAMESPACE____________________________________*/
} /** namespace jam */
