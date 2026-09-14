namespace jam
{
/*____________________________________________________________________________*/
// Distance threshold (device-space pixels) below which two ring vertices are
// treated as duplicates and excluded from the working cycle before clipping
// (buildInitialLinkedList()), and reused (as a perpendicular-displacement
// threshold) by isVertexCollinear()'s pruning pass below. JUCE's
// PathFlatteningIterator can legitimately emit points this close together at
// bezier segment joins; at END's terminal-cell UI scale (>= 8 physical px per
// cell) no two DISTINCT vertices of a real glyph/box-drawing/UI path are ever
// this close, so the threshold never merges or discards genuine geometry.
static constexpr float duplicateVertexEpsilon { 0.01f };

// duplicateVertexEpsilon squared — arePointsNearlyCoincident() compares squared
// distances directly, avoiding a sqrt per vertex pair.
static constexpr float duplicateVertexEpsilonSquared { duplicateVertexEpsilon * duplicateVertexEpsilon };

/*____________________________________________________________________________*/
// orientation — twice the signed area of triangle (a, b, c): positive when
// (a, b, c) winds counter-clockwise, negative when clockwise. Only the SIGN is
// ever meaningful at any call site below (convexity test, point-in-triangle
// test); magnitude is unused except by isVertexCollinear()'s scale-corrected
// comparison. Textbook 2D cross product — the one kernel every
// orientation-dependent test in this file shares (Single Source of Truth).
static float orientation (const juce::Point<float>& a, const juce::Point<float>& b, const juce::Point<float>& c) noexcept
{
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

/*____________________________________________________________________________*/
// computeSignedArea — twice the signed area of the whole ring (shoelace
// formula). Only its SIGN is used, to normalize triangulate()'s ear/convexity
// tests against whichever winding direction the caller's ring happens to use —
// JUCE's PathFlatteningIterator gives no winding guarantee across subpaths.
static float computeSignedArea (const jam::Array<juce::Point<float>>& ring) noexcept
{
    float area { 0.0f };
    const auto pointCount { ring.size() };

    for (int i = 0; i < pointCount; ++i)
    {
        const auto& current { ring.at (i) };
        const auto& next { ring.at ((i + 1) % pointCount) };

        area += current.x * next.y - next.x * current.y;
    }

    return area;
}

/*____________________________________________________________________________*/
// arePointsNearlyCoincident — true if a and b are closer than duplicateVertexEpsilon.
static bool arePointsNearlyCoincident (const juce::Point<float>& a, const juce::Point<float>& b) noexcept
{
    return a.getDistanceSquaredFrom (b) < duplicateVertexEpsilonSquared;
}

/*____________________________________________________________________________*/
// isVertexCollinear — true if current's perpendicular displacement from the
// line through previous and next is below duplicateVertexEpsilon. orientation()
// equals that displacement times |previous - next| (twice the triangle's area
// = base x height), so squaring both sides of the displacement comparison and
// multiplying through by the squared edge length keeps the test scale-correct
// without a second, separately-named area-unit constant.
static bool isVertexCollinear (const juce::Point<float>& previous, const juce::Point<float>& current,
                                const juce::Point<float>& next) noexcept
{
    const float turn { orientation (previous, current, next) };
    const float edgeLengthSquared { previous.getDistanceSquaredFrom (next) };

    return (turn * turn) < (duplicateVertexEpsilonSquared * edgeLengthSquared);
}

/*____________________________________________________________________________*/
// buildInitialLinkedList — links every non-duplicate vertex of ring into a
// circular doubly-linked list over ARENA INDICES (original 0-based positions
// into ring) — nextOf/prevOf sized to ring.size(), pre-allocated by the caller.
// Index 0 is always kept as the cycle's permanent entry point, so triangulate()
// never needs a separate "find a valid start" step.
static int32_t buildInitialLinkedList (const jam::Array<juce::Point<float>>& ring,
                                        std::vector<int32_t>& nextOf, std::vector<int32_t>& prevOf) noexcept
{
    const auto pointCount { ring.size() };

    std::vector<int32_t> keptIndices;
    keptIndices.push_back (0);

    for (int32_t i = 1; i < pointCount; ++i)
        if (not arePointsNearlyCoincident (ring.at (keptIndices.back()), ring.at (i)))
            keptIndices.push_back (i);

    if (keptIndices.size() > 1 and arePointsNearlyCoincident (ring.at (keptIndices.back()), ring.at (0)))
        keptIndices.pop_back();

    const auto keptCount { static_cast<int32_t> (keptIndices.size()) };

    for (int32_t k = 0; k < keptCount; ++k)
    {
        const auto currentIndex { keptIndices.at (static_cast<size_t> (k)) };
        const auto nextIndex { keptIndices.at (static_cast<size_t> ((k + 1) % keptCount)) };

        nextOf.at (static_cast<size_t> (currentIndex)) = nextIndex;
        prevOf.at (static_cast<size_t> (nextIndex)) = currentIndex;
    }

    return keptCount;
}

/*____________________________________________________________________________*/
// pruneCollinearVertices — single pass over the remainingCount-vertex cycle
// starting at startIndex, splicing out every vertex isVertexCollinear() flags —
// stops early once only 3 vertices remain (the minimum triangle). startIndex is
// updated in place if the vertex it names is itself spliced out.
static int32_t pruneCollinearVertices (const jam::Array<juce::Point<float>>& ring,
                                        std::vector<int32_t>& nextOf, std::vector<int32_t>& prevOf,
                                        int32_t& startIndex, int32_t remainingCount) noexcept
{
    int32_t current { startIndex };
    int32_t stepsRemaining { remainingCount };
    int32_t survivingCount { remainingCount };

    while (stepsRemaining > 0 and survivingCount > 3)
    {
        const auto previousIndex { prevOf.at (static_cast<size_t> (current)) };
        const auto nextIndex { nextOf.at (static_cast<size_t> (current)) };

        if (isVertexCollinear (ring.at (previousIndex),
                                ring.at (current),
                                ring.at (nextIndex)))
        {
            nextOf.at (static_cast<size_t> (previousIndex)) = nextIndex;
            prevOf.at (static_cast<size_t> (nextIndex)) = previousIndex;
            --survivingCount;

            if (current == startIndex)
                startIndex = nextIndex;
        }

        current = nextIndex;
        --stepsRemaining;
    }

    return survivingCount;
}

/*____________________________________________________________________________*/
// isPointStrictlyInsideTriangle — true if point lies strictly inside triangle
// (a, b, c); boundary points (exactly on an edge) are NOT inside. Used only by
// isEar()'s containment scan below.
static bool isPointStrictlyInsideTriangle (const juce::Point<float>& point, const juce::Point<float>& a,
                                            const juce::Point<float>& b, const juce::Point<float>& c,
                                            float orientationSign) noexcept
{
    const float sideAB { orientation (a, b, point) * orientationSign };
    const float sideBC { orientation (b, c, point) * orientationSign };
    const float sideCA { orientation (c, a, point) * orientationSign };

    return sideAB > 0.0f and sideBC > 0.0f and sideCA > 0.0f;
}

/*____________________________________________________________________________*/
// isEar — true if the vertex at candidate (sitting between previousIndex and
// nextIndex in the cycle) is a valid ear right now: convex under
// orientationSign, and no other vertex still in the cycle lies strictly inside
// triangle (previousIndex, candidate, nextIndex). O(n) scan over the remaining
// cycle — see jam_Earcut.h's class doc for why the resulting O(n^2) cost across
// triangulate() is accepted (rings reaching this type are UI-scale path
// flattenings; genuinely complex paths never reach earcut at all — see
// fillPath()'s isComplexFillPath() branch, jam_VulkanLowLevelGraphicsContextPath.cpp).
static bool isEar (const jam::Array<juce::Point<float>>& ring, const std::vector<int32_t>& nextOf,
                    int32_t previousIndex, int32_t candidate, int32_t nextIndex, float orientationSign) noexcept
{
    const auto& a { ring.at (previousIndex) };
    const auto& b { ring.at (candidate) };
    const auto& c { ring.at (nextIndex) };

    bool isValidEar { orientation (a, b, c) * orientationSign > 0.0f };

    int32_t probe { nextOf.at (static_cast<size_t> (nextIndex)) };

    while (isValidEar and probe != previousIndex)
    {
        if (isPointStrictlyInsideTriangle (ring.at (probe), a, b, c, orientationSign))
            isValidEar = false;

        probe = nextOf.at (static_cast<size_t> (probe));
    }

    return isValidEar;
}

/*____________________________________________________________________________*/
// clipEarIfPossible — tests isEar() at current and, if it is a valid ear,
// appends its triangle to triangleIndices and splices current out of the cycle.
static bool clipEarIfPossible (const jam::Array<juce::Point<float>>& ring,
                                std::vector<int32_t>& nextOf, std::vector<int32_t>& prevOf,
                                int32_t current, float orientationSign, jam::Array<uint32_t>& triangleIndices) noexcept
{
    const auto previousIndex { prevOf.at (static_cast<size_t> (current)) };
    const auto nextIndex { nextOf.at (static_cast<size_t> (current)) };

    const bool clipped { isEar (ring, nextOf, previousIndex, current, nextIndex, orientationSign) };

    if (clipped)
    {
        triangleIndices.add (static_cast<uint32_t> (previousIndex));
        triangleIndices.add (static_cast<uint32_t> (current));
        triangleIndices.add (static_cast<uint32_t> (nextIndex));

        nextOf.at (static_cast<size_t> (previousIndex)) = nextIndex;
        prevOf.at (static_cast<size_t> (nextIndex)) = previousIndex;
    }

    return clipped;
}

/*____________________________________________________________________________*/
// clipEars — repeatedly clips ears from the remainingCount-vertex cycle
// (starting at startIndex) until exactly 3 vertices remain — emitted as the
// final triangle — or a full scan finds no ear (degenerate all-collinear
// remainder). Termination is unconditional: stepsWithoutEar reaching
// remainingCount means a full pass found nothing left to clip, so the loop
// stops and whatever triangles were already produced are returned — jassert
// flags this in debug builds, but the function never loops forever and never
// crashes on it.
static void clipEars (const jam::Array<juce::Point<float>>& ring,
                       std::vector<int32_t>& nextOf, std::vector<int32_t>& prevOf,
                       int32_t startIndex, int32_t remainingCount, float orientationSign,
                       jam::Array<uint32_t>& triangleIndices) noexcept
{
    int32_t current { startIndex };
    int32_t stepsWithoutEar { 0 };

    while (remainingCount > 3 and stepsWithoutEar < remainingCount)
    {
        if (clipEarIfPossible (ring, nextOf, prevOf, current, orientationSign, triangleIndices))
        {
            current = nextOf.at (static_cast<size_t> (current));
            --remainingCount;
            stepsWithoutEar = 0;
        }
        else
        {
            current = nextOf.at (static_cast<size_t> (current));
            ++stepsWithoutEar;
        }
    }

    // Degenerate all-collinear remainder — see this function's own doc comment.
    jassert (remainingCount == 3);

    if (remainingCount == 3)
    {
        triangleIndices.add (static_cast<uint32_t> (prevOf.at (static_cast<size_t> (current))));
        triangleIndices.add (static_cast<uint32_t> (current));
        triangleIndices.add (static_cast<uint32_t> (nextOf.at (static_cast<size_t> (current))));
    }
}

/*____________________________________________________________________________*/
jam::Array<uint32_t> Earcut::triangulate (const jam::Array<juce::Point<float>>& ring) noexcept
{
    // Precondition — a polygon ring needs at least 3 points to enclose an area.
    jassert (ring.size() >= 3);

    jam::Array<uint32_t> triangleIndices;

    if (ring.size() >= 3)
    {
        const auto pointCount { ring.size() };
        const float orientationSign { computeSignedArea (ring) >= 0.0f ? 1.0f : -1.0f };

        std::vector<int32_t> nextOf (static_cast<size_t> (pointCount), -1);
        std::vector<int32_t> prevOf (static_cast<size_t> (pointCount), -1);

        int32_t startIndex { 0 };
        int32_t remainingCount { buildInitialLinkedList (ring, nextOf, prevOf) };

        if (remainingCount >= 3)
            remainingCount = pruneCollinearVertices (ring, nextOf, prevOf, startIndex, remainingCount);

        if (remainingCount >= 3)
            clipEars (ring, nextOf, prevOf, startIndex, remainingCount, orientationSign, triangleIndices);
    }

    return triangleIndices;
}

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
