/**
 * @file jam_Hash.h
 * @brief std::hash specialisation for juce::Identifier.
 */

#pragma once

#include <juce_core/juce_core.h>

namespace std
{
    /**
     * @brief Hashes a juce::Identifier by its interned string pool address.
     *
     * juce::Identifier interns each distinct name to a single shared buffer,
     * so equal identifiers always share the same character pointer — hashing
     * the address is both correct and O(1).
     */
    template <> struct hash<juce::Identifier>
    {
        /**
         * @brief Computes the hash of @p id.
         * @param id  The identifier to hash.
         * @return Hash of @p id's interned character pointer.
         */
        size_t operator() (const juce::Identifier& id) const noexcept
        {
            return std::hash<const void*>{} (id.getCharPointer().getAddress());
        }
    };
}
