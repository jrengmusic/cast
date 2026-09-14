/**
 * @file jam_Validator.h
 * @brief Per-property validation/formatting/creation callbacks keyed by identifier.
 */

namespace jam
{
/*____________________________________________________________________________*/

/**
 * @brief Bundle of optional callbacks validating, formatting, and reacting to
 *        a single ValueTree property's value.
 *
 * @tparam Model The owning model type passed to @c create.
 */
template <typename Model>
struct Validator
{
    /** @brief Returns true when a candidate value is acceptable for the property. */
    std::function<bool (const juce::var&)> check {};
    /** @brief Formats a value for display; unset when no custom formatting applies. */
    std::function<juce::String (const juce::var&)> format {};
    /** @brief Invoked when the property is created/changed, to react on the owning model. */
    std::function<void (Model&, juce::ValueTree&,
                        const juce::Identifier&, const juce::var&)> create {};

    /** @brief Constructs a Validator with no callbacks set. */
    Validator() = default;

    /**
     * @brief Constructs a Validator with only a check callback.
     * @param f  Validation predicate installed as @c check.
     */
    Validator (std::function<bool (const juce::var&)> f) : check (std::move (f)) {}

    /**
     * @brief Constructs a Validator with a check callback and a create callback.
     * @param c  Validation predicate installed as @c check.
     * @param p  Reaction callback installed as @c create.
     */
    Validator (std::function<bool (const juce::var&)> c,
               std::function<void (Model&, juce::ValueTree&,
                                   const juce::Identifier&, const juce::var&)> p)
        : check (std::move (c)), create (std::move (p)) {}

    /**
     * @brief Invokes @c check on @p v.
     * @param v  Candidate value to validate.
     * @return   The result of @c check(v).
     */
    bool operator() (const juce::var& v) const { return check (v); }
};

/**
 * @brief Nested registry of Validator<Model>, keyed first by owning type
 *        identifier, then by property identifier.
 *
 * @tparam Model The owning model type passed to each Validator's @c create.
 */
template <typename Model>
using Validators = jam::HashMap<juce::Identifier,
                                jam::HashMap<juce::Identifier, Validator<Model>>>;

/**_____________________________END OF NAMESPACE______________________________*/
} // namespace jam
