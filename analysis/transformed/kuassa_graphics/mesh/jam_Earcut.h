/**
 * @file jam_Earcut.h
 * @brief Single-ring polygon triangulation via ear clipping.
 */

namespace jam
{
/*____________________________________________________________________________*/
/** @brief Single-ring polygon triangulation via ear clipping (fan-free, hole-free).
 *
 *  Clean-room implementation of the textbook ear-clipping algorithm — NOT
 *  transcribed from any vendored/third-party earcut implementation. Replaces the
 *  previously-vendored mapbox earcut.hpp at its one real call site
 *  (LowLevelGraphicsContext::appendEarcutTriangulatedRing(),
 *  jam_vulkan/context/jam_VulkanLowLevelGraphicsContextPath.cpp — jam_vulkan
 *  consumes this type via its jam_graphics module dependency; relocated here
 *  so jam::WavefrontObj's n-gon triangulation in this same directory can
 *  share it), which always wraps a SINGLE ring
 *  per call — multi-ring/holed/self-intersecting paths never reach this type,
 *  they are routed to the winding-rule stencil-then-cover path instead (see
 *  fillPath()'s isComplexFillPath() branch, same file). triangulate() therefore
 *  only ever needs to solve the hole-free, single-boundary case.
 *
 *  @par Algorithm (jam_Earcut.cpp)
 *  1. Orientation is detected once via the shoelace signed-area sum, so the
 *     ear/convexity tests work identically regardless of whether the caller's
 *     ring winds clockwise or counter-clockwise — JUCE's PathFlatteningIterator
 *     gives no winding guarantee across subpaths.
 *  2. The ring is linked into a circular doubly-linked list over ARENA INDICES —
 *     plain std::vector\<int32_t\> next/prev arrays sized to the ring, never
 *     pointers — built once, excluding near-duplicate consecutive vertices from
 *     the cycle: JUCE's bezier flattening tolerance can legitimately emit points
 *     this close together at segment joins.
 *  3. A single pruning pass removes vertices whose perpendicular displacement
 *     from the line through their neighbors is below that same closeness
 *     threshold — a near-zero-area ear would otherwise stall the ear test at
 *     that vertex indefinitely.
 *  4. The remaining cycle is repeatedly scanned for a vertex that is both convex
 *     and has no other remaining vertex strictly inside its candidate triangle;
 *     each ear found is emitted and spliced out — classic O(n) ears-per-scan,
 *     O(n) vertices, O(n^2) total. Accepted: every ring reaching this type is a
 *     UI-scale path flattening (terminal glyphs, box drawing, window chrome) —
 *     genuinely complex/large paths are the multi-ring case above, which never
 *     calls triangulate() at all.
 *  5. Termination is unconditional: if a full scan of the remaining cycle finds
 *     no ear, clipping stops and returns whatever triangles were already
 *     produced — jassert flags this as the degenerate all-collinear-remainder
 *     case in debug builds, but the function never loops forever and never
 *     crashes on it.
 *
 *  @par Ownership
 *  This type carries no instance state (BLESSED Stateless) — its one public
 *  method is a pure function bundled under one name, not an object with a
 *  lifecycle.
 */
class Earcut
{
public:
    /** @brief Triangulates a single closed polygon ring by ear clipping.
     *  @param ring  Closed polygon boundary in any consistent winding — at least
     *               3 points (asserted; see class doc's termination note for
     *               what happens if internal duplicate/collinear culling
     *               reduces the ring below 3 usable vertices after entry).
     *  @return Triangle list as flat index triples into @p ring itself (0-based,
     *          same indexing space as the input — the caller appends @p ring
     *          verbatim as its own vertex buffer and offsets these indices).
     *          Empty if @p ring has fewer than 3 points, or fewer than 3
     *          vertices survive duplicate/collinear culling. */
    static jam::Array<uint32_t> triangulate (const jam::Array<juce::Point<float>>& ring) noexcept;
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
