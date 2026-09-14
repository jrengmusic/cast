/**
 * @file jam_UUID.h
 * @brief Lean 64-bit per-process unique identifier.
 */

#pragma once

namespace jam
{
/*____________________________________________________________________________*/

/**
 * @brief Lean 64-bit universally unique identifier — trivially copyable, atomic-compatible.
 *
 * 64-bit random value from juce::Random::getSystemRandom(). Sufficient uniqueness
 * for per-process identity (tabs, panes, sessions). Trivially copyable —
 * works with std::atomic<int64_t> and jam::Parameter<int64_t>.
 * toString() is for display and juce::Component::setComponentID() only.
 */
struct UUID
{
    /** @brief The underlying 64-bit identity value. */
    int64_t value;

    /** @brief Generates a positive non-zero random 64-bit UUID. */
    UUID() noexcept
        : value { std::abs (juce::Random::getSystemRandom().nextInt64()) }
    {
    }

    /** @brief Constructs from an existing int64 value (e.g. restored from ValueTree). */
    explicit UUID (int64_t v) noexcept
        : value { v }
    {
    }

    /** @brief Constructs from a juce::var (e.g. restored from a ValueTree property). */
    explicit UUID (const juce::var& v) noexcept
        : value { static_cast<int64_t> (v) }
    {
    }

    /** @brief Returns a zero-valued UUID representing "no identity." */
    static UUID none() noexcept { return UUID { int64_t { 0 } }; }

    /** @brief Returns true when both UUIDs hold the same value. */
    bool operator== (const UUID& other) const noexcept { return value == other.value; }
    /** @brief Returns true when the UUIDs hold different values. */
    bool operator!= (const UUID& other) const noexcept { return value != other.value; }

    /** @brief String representation for display and componentID. */
    juce::String toString() const noexcept { return juce::String (value); }
};

static_assert (std::is_trivially_copyable_v<UUID>);
static_assert (sizeof (UUID) == sizeof (int64_t));

/**_____________________________END OF NAMESPACE______________________________*/
} // namespace jam

template <>
struct std::hash<jam::UUID>
{
    std::size_t operator() (const jam::UUID& uuid) const noexcept
    {
        return std::hash<int64_t> {} (uuid.value);
    }
};
