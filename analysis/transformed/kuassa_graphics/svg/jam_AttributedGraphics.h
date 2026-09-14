/**
 * @file jam_AttributedGraphics.h
 * @brief Paint-ready draw-call collections — flattened Svg Shape owner
 *        (AttributedGraphics) and 9-slice flex-stretch layout (Svg::Flex).
 */

namespace jam
{
/*____________________________________________________________________________*/

/** @brief Smallest drawable unit: geometry + drawing attributes.
 *
 *  Produced by Svg::getAttributedGraphics and consumed via AttributedGraphics.
 *  Pure draw-call data — no layout knowledge. Bounds and segment scoping
 *  belong to Flex::Segment; AttributedShape carries only what one GPU draw
 *  call needs.
 *
 *  A thickness of 0 in stroke signals a fill op; non-zero signals a stroke op.
 *  One GPU draw call: setColour (LAF-resolved when colourId is non-zero, else
 *  colour directly) followed by fillPath (thickness==0) or strokePath.
 *
 *  When colourId is non-zero the shape is LAF-coloured — colour is normalised
 *  to a default-constructed juce::Colour{} and must not be read directly.
 *  When colourId is zero the shape is self-styled and colour is used directly.
 *
 *  gradientEndColourId, when non-zero, turns the shape's paint from flat
 *  (colourId/colour) into a linear juce::ColourGradient running from
 *  colourId (LAF-resolved start stop) at gradientStart to
 *  gradientEndColourId (always LAF-resolved — the gradient primitive has
 *  no self-styled end stop) at gradientEnd (AttributedGraphics::paint,
 *  Svg::Format::linearGradient). Zero (default) preserves the flat
 *  behaviour above unchanged.
 *
 *  operator== defines draw-call identity: colourId, colour, stroke,
 *  dashLength, gradientEndColourId, gradientStart, and gradientEnd
 *  determine whether two shapes can be collapsed into one draw call —
 *  a dashed and a solid stroke sharing every other attribute must never
 *  merge in flatten() (AttributedGraphics::flatten() doc comment), nor may
 *  two gradients with differing axes or end stops. path is geometry
 *  payload — excluded from equality.
 */
struct AttributedShape
{
    /** @brief Geometry payload — excluded from operator== draw-call identity. */
    juce::Path path;

    /** @brief Stroke thickness; 0 signals a fill op, non-zero a stroke op. */
    juce::PathStrokeType stroke { 0.0f };

    /** @brief Self-styled paint colour; used only when colourId == 0. */
    juce::Colour colour;

    /** @brief LAF colour id; 0 = self-styled (use colour directly), non-zero = LAF-resolved. */
    int colourId { 0 };
    /** @brief Dash length in path units, 0 = solid (default). A stroke
     *  shape with dashLength > 0 paints as alternating on/off segments
     *  of this length (AttributedGraphics::paint's `juce::PathStrokeType::
     *  createDashedStroke` branch); ignored for fill shapes
     *  (stroke.getStrokeThickness() == 0). */
    float dashLength { 0.0f };
    /** @brief Second gradient stop's LAF colour id, 0 = no gradient
     *  (default — shape paints flat via colourId/colour, unchanged
     *  behaviour). Non-zero enables a linear gradient (see this
     *  struct's own doc comment above). */
    int gradientEndColourId { 0 };
    /** @brief Gradient axis start point, meaningful only when
     *  gradientEndColourId != 0. */
    juce::Point<float> gradientStart;
    /** @brief Gradient axis end point, meaningful only when
     *  gradientEndColourId != 0. */
    juce::Point<float> gradientEnd;

    bool operator== (const AttributedShape& other) const noexcept
    {
        return colourId == other.colourId and colour == other.colour and stroke == other.stroke
               and dashLength == other.dashLength
               and gradientEndColourId == other.gradientEndColourId
               and gradientStart == other.gradientStart and gradientEnd == other.gradientEnd;
    }
};

/*____________________________________________________________________________*/

/** @brief Paint-ready text draw-call: a jam::AttributedString plus its
 *  target bounds and LAF colour id.
 */
struct AttributedText
{
    /** @brief Shaped, attribute-carrying text content. */
    jam::AttributedString text;

    /** @brief Exact draw rectangle, stamped at creation. */
    juce::Rectangle<float> bounds;

    juce::TextLayout layout;

    /** @brief LAF colour id; 0 = self-styled (attributes carry their own colour), non-zero = LAF-resolved. */
    int colourId { 0 };
    int backgroundColourId { 0 };
};

/*____________________________________________________________________________*/

/** @brief Flat composition of paint-ready draw-call Shapes and Texts.
 *
 *  Produced by Svg::getAttributedGraphics and composed by Svg::Flex::getSegments.
 *  shapes and texts are jam::Array value containers — move-only (jam::Array's
 *  own contract), no per-element heap allocation.
 *
 *  flatten() merges the raw walk output into the final paint sequence. Fills
 *  (AttributedShape::stroke.getStrokeThickness()==0) are emitted before strokes,
 *  in encounter order within each category. Shapes sharing the same draw-call
 *  key (AttributedShape::operator==) have their geometry merged via
 *  juce::Path::addPath — one entry per key, one setColour + one
 *  fillPath/strokePath at paint time. texts pass through flatten() untouched.
 *
 *  Per-segment scoping is provided by Flex::Segment storage, not by Shape
 *  identity. Shape carries no segment or bounds members.
 *
 *  @note z-reorder authoring contract: flatten departs from Svg painter order.
 *        z within mixed colours is controlled by the designer through colour
 *        discipline — fills paint before strokes.
 */
struct AttributedGraphics
{
    /** @brief Paint-ready draw-call Shapes, fills-then-strokes ordered after flatten(). */
    jam::Array<AttributedShape> shapes;

    /** @brief Paint-ready text draw calls, passed through flatten() untouched. */
    jam::Array<AttributedText> texts;

    /** @brief Collapse raw walk output into the final paint-ordered sequence.
     *
     *  Consumes this — moves shapes out. Returns a new AttributedGraphics with
     *  fills (stroke thickness==0) in encounter order, then strokes in
     *  encounter order. Shapes with the same draw-call key
     *  (AttributedShape::operator==) are merged: first occurrence keeps its slot,
     *  subsequent matches accumulate geometry via path.addPath. texts move
     *  across unchanged.
     *
     *  @return Fill-before-stroke AttributedGraphics ready for painting.
     */
    AttributedGraphics flatten();

    /** @brief Emit the collapsed draw calls under @p transform.
     *
     *  Iterates shapes in order (fills then strokes after flatten()). Per
     *  shape: gradientEndColourId != 0 sets a juce::ColourGradient brush
     *  (colourId/colour-resolved start stop, laf.findColour-resolved end
     *  stop, axis gradientStart/gradientEnd carried through @p transform via
     *  juce::Point::transformedBy — same coordinate space the path itself
     *  paints into); else resolves flat colour (colourId != 0 →
     *  laf.findColour (colourId); else @p colour). Either brush then feeds
     *  the same geometry dispatch: if stroke thickness > 0 and dashLength >
     *  0, builds a dashed outline via juce::PathStrokeType::
     *  createDashedStroke (equal on/off segments at dashLength, @p transform
     *  applied through the same call) and strokes that; else if stroke
     *  thickness > 0 copies the path, applies @p transform, and calls
     *  g.strokePath at authored width (geometry-then-stroke); else calls
     *  g.fillPath (path, transform).
     *
     *  Then, when texts is non-empty, one juce::Graphics::ScopedSaveState
     *  pushes @p transform onto the context via g.addTransform so every
     *  text's local-space coordinates paint correctly, and each text draws
     *  via its own jam::AttributedString::toJuce(), centred-justified, then
     *  juce::AttributedString::draw against its bounds — resolved colour
     *  (same colourId/colour resolution rule as shapes) applied via
     *  juce::AttributedString::setColour on the converted copy when
     *  colourId != 0. Empty texts incurs no ScopedSaveState construction
     *  and no per-text work.
     *
     *  flatten() and paint() are the two halves of AttributedGraphics' job —
     *  collapse draw calls, emit draw calls.
     *
     *  @param g         Graphics context.
     *  @param laf       Provides colour resolution via findColour(colourId).
     *  @param transform Affine transform applied to each shape/text at paint time.
     */
    void paint (juce::Graphics& g,
                const juce::LookAndFeel& laf,
                const juce::AffineTransform& transform) const;

    void paint (juce::Graphics& g,
                const juce::LookAndFeel& laf,
                juce::Rectangle<float> bounds) const;

    juce::Rectangle<float> getContentBounds() const;
};

/*____________________________________________________________________________*/
/** @brief 9-slice flex-stretch Svg layout — static parser and painter.
 *
 *  Stateless. The parser turns Svg markup into a Segments array (one Segment
 *  per occupied 9-slice cell; parser yields Segments, painter consumes
 *  Segments). The painter stretches the Segments into a component's bounds
 *  using a uniform scale derived from the target height: corners scale
 *  proportionally preserving aspect ratio, edges stretch along their adjacent
 *  axis at their scaled thickness, centre absorbs the remainder.
 *
 *  ## Svg authoring contract
 *
 *  - 3×3 grid of named top-level groups: top-left, top, top-right, left,
 *    centre, right, bottom-left, bottom, bottom-right. All optional.
 *  - Always authored in top orientation — the caller passes the target bounds,
 *    swapping width/height for vertical layouts.
 *  - Each group must contain exactly one element named @c bounds (resolved via
 *    Svg::getElementId). That element is layout metadata (cell extent), never
 *    painted. Its path bounds become Segment::bounds.
 *  - Other elements in the group are painted shapes. Element name resolved via
 *    Svg::getElementId matching a key in the colourIds registry → LAF-coloured
 *    (resolved at paint time via LookAndFeel::findColour). No match → self-styled
 *    from the element's own style attribute.
 *  - Same colour + same style + same stroke width within the same cell →
 *    geometry collapsed into one Shape (one setColour + one draw call).
 *  - Output order: fills before strokes, in encounter order.
 */
struct Svg::Flex
{
    /** @brief One 9-slice cell — id, source bounds, and flattened draw calls.
     *
     *  Exactly one canonical group name (@c id), one source-space cell rectangle
     *  (@c bounds, from the group's element named @c Id::bounds), and one
     *  flattened AttributedGraphics (@c graphics, per-colour/per-stroke merged draw
     *  calls produced by Svg::getAttributedGraphics).
     *
     *  Slots are indexed by the map::Segment enum (topLeft=0 …
     *  bottomRight=8). Unauthored slots stay default — null id, empty bounds,
     *  empty graphics.
     *
     *  @note Move-only: AttributedGraphics holds jam::Array members (move-only).
     */
    struct Segment
    {
        /** @brief Canonical group name (matches an map::Segment slot); null when unauthored. */
        juce::Identifier id;

        /** @brief Source-space cell rectangle, from the group's Id::bounds element. */
        juce::Rectangle<float> bounds;

        /** @brief Flattened, paint-ready draw calls for this cell. */
        AttributedGraphics graphics;
    };

    /** @brief Fixed-size 9-slot bank of Segments, indexed by map::Segment. */
    using Segments = std::array<Segment, 9>;

    /** @brief Target rectangles for one 9-slice paint pass.
     *
     *  Indexed by the map::Segment slot enum (topLeft=0 … bottomRight=8),
     *  parallel to Segments. Produced by getLayout(), consumed by paint() —
     *  each rect is where the matching Segment stretches into the component.
     *  Slots whose segment is unauthored collapse to empty rects.
     */
    using Layout = std::array<juce::Rectangle<float>, 9>;

    /** @brief Parse @p svg into a paint-ready Segments array.
     *
     *  Walks top-level \<g\> groups in the Svg document. Per group:
     *  - The group name is resolved via Svg::getElementId. Unknown names (not
     *    present in map::Segment) are skipped.
     *  - The canonical slot index is obtained via
     *    map::Segment::getInstance()->get (name).
     *  - Segment::id = juce::Identifier { name }.
     *  - Segment::graphics = Svg::getAttributedGraphics (group, colourScheme).
     *  - Segment::bounds = path bounds of the group's element whose
     *    Svg::getElementId == Id::bounds.toString(), found by
     *    Xml::applyFunctionRecursively.
     *
     *  @param svg          Valid Svg markup string.
     *  @param colourScheme Node-scoped colour registry; int entries keyed by
     *                      canonical element name map to LAF colour ids.
     *  @return Segments array; unauthored slots are default-initialised.
     *          Empty when @p svg is empty or unparseable.
     */
    static Segments getSegments (const juce::String& svg, const ColourScheme& colourScheme);

    /** @brief Compute the 9 target rectangles for @p segments inside @p area.
     *
     *  Pure and deterministic — same segments + same area always yields the
     *  same array. Indexed by map::Segment slot (topLeft=0 … bottomRight=8).
     *
     *  Layout contract (uniform-scale 9-slice):
     *  - A single scale factor is derived once: scale = area.getHeight() /
     *    source.getHeight(), where source is the union of all segment bounds.
     *  - Corner cells (topLeft, topRight, bottomLeft, bottomRight) are carved
     *    at (sourceWidth × scale, sourceHeight × scale) — aspect ratio
     *    preserved.
     *  - Top and bottom edge cells span the remaining horizontal width at
     *    their source height × scale.
     *  - Left and right edge cells span the remaining vertical height at
     *    their source width × scale.
     *  - Centre absorbs all remaining area in both axes.
     *  - Empty source union (all segments unauthored) → all-empty layout.
     *
     *  Unauthored slots (empty source bounds) produce empty layout rects;
     *  paint() already skips segments whose bounds are empty.
     *
     *  @param segments  The parsed Segments produced by getSegments.
     *  @param area      Target rectangle to carve into 9 cells.
     *  @return std::array of 9 rectangles.
     */
    static Layout getLayout (const Segments& segments, juce::Rectangle<float> area);

    /** @brief Orchestrate layout + per-segment fit transform + AttributedGraphics::paint.
     *
     *  Calls getLayout to compute the 9 target rectangles, then for each
     *  non-empty segment slot calls AttributedGraphics::paint with the
     *  stretchToFit transform from the segment's source bounds to its layout
     *  rectangle.
     *
     *  Empty source (no authored asset): nothing is painted.
     *  Empty segment slots: skipped positively.
     *
     *  @param g        Graphics context.
     *  @param laf      Colour resolution — findColour(colourId) for LAF-coloured shapes.
     *  @param segments The parsed Segments to paint.
     *  @param bounds   Target rectangle for the 9-slice layout.
     */
    static void paint (juce::Graphics& g,
                       const juce::LookAndFeel& laf,
                       const Segments& segments,
                       juce::Rectangle<float> bounds);
};

/**_____________________________END OF NAMESPACE______________________________*/
}// namespace jam
