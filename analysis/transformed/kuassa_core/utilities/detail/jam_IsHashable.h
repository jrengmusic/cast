/**
 * @file jam_IsHashable.h
 * @brief SFINAE detector for a nested T::Hash functor.
 */

#pragma once

namespace jam
{
/*____________________________________________________________________________*/

/**
 * @brief SFINAE trait detecting whether T defines a nested Hash functor.
 *
 * Evaluates to std::true_type when T::Hash exists as a type,
 * std::false_type otherwise. Used by Owner to select O(1) hash-based dedup
 * vs O(n) linear fallback at compile time.
 */
template <typename ObjectClass, typename = void>
struct IsHashable : std::false_type
{
};

template <typename ObjectClass>
struct IsHashable<ObjectClass, std::void_t<typename ObjectClass::Hash>> : std::true_type
{
};

/**_____________________________END OF NAMESPACE______________________________*/
}// namespace jam
