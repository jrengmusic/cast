/**
 * @file jam_Svg.h
 * @brief Svg read/draw/write pipeline: parsing Svg markup into JUCE paths and
 *        drawables, extracting paint-ready Shape/Text geometry, and emitting
 *        Svg markup or C++ source back out.
 */

namespace jam
{
/*____________________________________________________________________________*/

/** @brief Forward declaration — defined in jam_AttributedGraphics.h. */
struct AttributedGraphics;

/** @brief Forward declaration — defined in jam_AttributedGraphics.h. */
struct AttributedText;

/**
 * @struct Svg
 * @brief Namespace-like container for the Svg read/draw/write pipeline.
 *
 * Organised in three sections: reading (parsing Svg markup and XmlElement
 * trees into juce::Path/Shape/Text data), drawing (rendering parsed Svg
 * directly into a juce::Graphics context), and writing (Template/Format pairs
 * that emit Svg markup or C++ source from juce::Path data).
 */
struct Svg
{
    /** -------------------- FUNCTIONS FOR READING Svg ---------------------------*/

    /**
     * @brief Svg string cleanup helpers for stripping group/header noise.
     *
     * Pure string transforms over a juce::XmlElement's serialised form;
     * used when a downstream consumer needs the raw inner content
     * (no \c \<g\> wrappers, no \c <?xml?> declaration, no \c version attr).
     */
    struct Filter
    {
        /**
         * @brief Remove outer \<g\> group tags from an Svg element.
         *
         * This function takes a juce::XmlElement representing an Svg fragment,
         * converts it to a string, and strips away all \<g\> opening tags and the
         * final closing \</g\> tag. The result is the raw inner content of the
         * group, without any group wrappers.
         *
         * @param svg Pointer to a juce::XmlElement containing the Svg fragment.
         *            Must not be nullptr.
         *
         * @return A juce::String containing the Svg markup with all \<g\> tags removed.
         *
         * @note This is useful when you want to flatten grouped Svg content into
         *       a single layer, or when embedding fragments into a larger Svg
         *       document without redundant grouping.
         */
        static juce::String group (juce::XmlElement* svg);

        /**
         * @brief Remove XML declaration and redundant attributes from an Svg element.
         *
         * This function takes a juce::XmlElement representing an Svg fragment,
         * converts it to a string, and strips away:
         * - The XML declaration (`<?xml ... ?>`) and any content before the \<svg\> tag.
         * - The "version" attribute inside the \<svg\> tag.
         *
         * The result is a cleaner Svg string suitable for embedding into larger
         * documents or for serialization without redundant headers.
         *
         * @param svg Pointer to a juce::XmlElement containing the Svg fragment.
         *            Must not be nullptr.
         *
         * @return A juce::String containing the Svg markup with the XML declaration
         *         and version attribute removed, trimmed of leading/trailing whitespace.
         *
         * @note This is useful when exporting Svg fragments that will be combined
         *       into a single document, where multiple XML declarations or version
         *       attributes would be invalid.
         */
        static juce::String declaration (juce::XmlElement* svg);
    };

    /*____________________________________________________________________________*/
    /** -------------------- FUNCTIONS FOR DRAWING Svg ---------------------------*/
    /**
     * @brief Draw an Svg file into a graphics context.
     *
     * Loads an Svg from a file, parses it into a juce::Drawable, and renders it
     * within the specified area of the given Graphics context.
     *
     * @param g     Reference to the juce::Graphics context to draw into.
     * @param area  Target rectangle in which the Svg will be drawn.
     * @param file  The juce::File containing the Svg data.
     *
     * @note The Svg is scaled and centred within the target area.
     */
    static void draw (juce::Graphics& g, juce::Rectangle<int> area, const juce::File& file);

    /**
     * @brief Draw an Svg element into a graphics context.
     *
     * Renders a juce::XmlElement representing an Svg fragment into the specified
     * area of the given Graphics context.
     *
     * @param g     Reference to the juce::Graphics context to draw into.
     * @param area  Target rectangle in which the Svg will be drawn.
     * @param e     Pointer to a juce::XmlElement containing the Svg data.
     *
     * @note The Svg is scaled and centred within the target area.
     *       The XmlElement must not be nullptr.
     */
    static void draw (juce::Graphics& g, juce::Rectangle<int> area, juce::XmlElement* e);

    /**
     * @brief Draw a juce::Drawable into a graphics context.
     *
     * Renders a juce::Drawable object directly into the specified area of the
     * given Graphics context.
     *
     * @param g         Reference to the juce::Graphics context to draw into.
     * @param area      Target rectangle in which the Drawable will be drawn.
     * @param drawable  Pointer to a juce::Drawable object to render.
     *
     * @note The Drawable is scaled and centred within the target area.
     *       The drawable pointer must not be nullptr.
     */
    static void draw (juce::Graphics& g, juce::Rectangle<int> area, juce::Drawable* drawable);

    /** @brief Document overload of getEllipsePath(). */
    static juce::Path getEllipsePath (const Document::Element& xml);

    /** @brief Document overload of getCirclePath(). */
    static juce::Path getCirclePath (const Document::Element& xml);

    /** @brief Document overload of getRectPath(). */
    static juce::Path getRectPath (const Document::Element& xml);

    /** @brief Extract geometry from a single Svg element by tag.
     *
     *  Single-element geometry reader — the geometry SSOT. Switches on the
     *  element tag and reads per-tag attributes directly:
     *  - \c path    → parseSVGPath of the \c d attribute.
     *  - \c rect    → addRectangle from x, y, width, height.
     *  - \c ellipse → addEllipse from cx, cy, rx, ry.
     *  - \c circle  → addEllipse from cx, cy, r.
     *  - Any other tag → returns an empty Path.
     *
     *  All per-tag attribute reading lives here and nowhere else. Helpers
     *  (getEllipsePath, getCirclePath, getRectPath, getAllFoundPath) delegate
     *  to this function per child, keeping geometry extraction DRY.
     *
     *  @param element  Svg element to extract geometry from.
     *  @return         Path containing the element's geometry, or empty when
     *                  the tag is not a supported shape element.
     */
    /** @brief Document overload of getElementPath() — same tag dispatch, same geometry SSOT. */
    static juce::Path getElementPath (const Document::Element& element);

    /**
     * @brief Enumeration of supported Svg element types.
     *
     * This enum class defines the categories of Svg elements that can be
     * parsed, filtered, or converted into juce::Path objects. It is typically
     * used to specify which type(s) of elements should be processed when
     * traversing an Svg document.
     */
    enum class ElementType
    {
        /**
         * @brief Select all supported element types.
         *
         * Use this when you want to include every element type
         * (path, ellipse, circle, rect).
         */
        all,

        /**
         * @brief Represents an Svg \<path\> element.
         *
         * Arbitrary vector paths defined by Svg path data.
         */
        path,

        /**
         * @brief Represents an Svg \<ellipse\> element.
         *
         * Defined by centre coordinates (cx, cy) and radii (rx, ry).
         */
        ellipse,

        /**
         * @brief Represents an Svg \<circle\> element.
         *
         * Defined by centre coordinates (cx, cy) and radius (r).
         */
        circle,

        /**
         * @brief Represents an Svg \<rect\> element.
         *
         * Defined by top‑left coordinates (x, y) and dimensions (width, height).
         */
        rect,
    };

    /**
     * @brief Collect all Svg shapes of a given type into a single juce::Path.
     *
     * This function inspects the children of the provided XmlElement and extracts
     * geometry based on the specified ElementType. The resulting shapes are
     * appended into a single juce::Path, which can then be rendered, transformed,
     * or used for hit‑testing.
     *
     * Supported element types:
     * - ElementType::path    → Parses each \<path\> child's "d" attribute directly.
     * - ElementType::ellipse → Delegates to getEllipsePath(), extracting cx/cy/rx/ry.
     * - ElementType::circle  → Delegates to getCirclePath(), extracting cx/cy/r.
     * - ElementType::rect    → Delegates to getRectPath(), extracting x/y/width/height.
     * - ElementType::all     → Collects all of the above element types.
     *
     * @param xml          Pointer to a juce::XmlElement containing Svg child
     *                     elements. Must not be nullptr.
     * @param elementToAdd The type of Svg element(s) to extract, as defined by
     *                     the ElementType enum.
     *
     * @return A juce::Path containing the combined geometry of all matching
     *         elements found in the XML.
     *
     * @note For ElementType::all, the function sequentially adds paths, ellipses,
     *       circles, and rectangles into the returned juce::Path.
     * @note This function does not perform any styling (stroke, fill, colour);
     *       it only extracts the raw geometry.
     */
    /** @brief Document overload of getAllFoundPath(). */
    static juce::Path
    getAllFoundPath (const Document::Element& xml, ElementType elementToAdd = ElementType::all);

    /**
     * @brief Extract geometry from an Svg element and its groups.
     *
     * Traverses the given XmlElement, collecting geometry of the specified
     * ElementType from both the element itself and any child \<g\> groups.
     * The resulting shapes are combined into a single juce::Path.
     *
     * @param svg          Pointer to a juce::XmlElement representing the root
     *                     or a fragment of an Svg document. Must not be nullptr.
     * @param elementToAdd The type of Svg element(s) to extract, as defined by
     *                     the ElementType enum.
     *
     * @return A juce::Path containing the combined geometry of all matching
     *         elements found in the XML.
     *
     * @note This overload does not apply any scaling or fitting; it extracts
     *       raw geometry only.
     */

    static juce::Path getPath (juce::XmlElement* svg, ElementType elementToAdd);

    /** @brief Document overload of getPath() — no scaling or fitting, raw geometry only. */
    static juce::Path getPath (const Document::Element& svg, ElementType elementToAdd);

    /**
     * @brief Extract geometry from an Svg string.
     *
     * Parses the given string into a Document, then delegates to the
     * Document overload of getPath() to extract geometry of the specified type.
     *
     * @param svgString    A juce::String containing valid Svg markup.
     * @param elementToAdd The type of Svg element(s) to extract, as defined by
     *                     the ElementType enum.
     *
     * @return A juce::Path containing the combined geometry of all matching
     *         elements found in the parsed Svg string.
     *
     * @note This overload is convenient when working with in‑memory Svg data
     *       rather than files or XmlElement objects.
     */
    static juce::Path
    getPath (const juce::String& svgString, ElementType elementToAdd = ElementType::all);

    /** @brief Document overload of getPath() — same viewBox/width/height fit-source rule. */
    static juce::Path getPath (const Document::Element& svg,
                               const juce::Rectangle<float>& areaToFit,
                               ElementType elementToAdd = ElementType::all);

    /**
     * @brief Extract and fit geometry from an Svg string into a target area.
     *
     * Parses the given Svg string into a Document, extracts geometry of the
     * specified ElementType, and applies a transform so that the geometry fits
     * within the specified target rectangle.
     *
     * The source bounds are determined by:
     * - The "viewBox" attribute, if present.
     * - Otherwise, the "width" and "height" attributes of the Svg element.
     *
     * @param svg          A juce::String containing valid Svg markup.
     * @param areaToFit    The rectangle into which the extracted geometry
     *                     should be scaled and fitted.
     * @param elementToAdd The type of Svg element(s) to extract, as defined by
     *                     the ElementType enum.
     *
     * @return A juce::Path containing the transformed geometry.
     *
     * @note This overload accepts a floating‑point rectangle for precise fitting.
     */
    static juce::Path getPath (const juce::String& svg,
                               const juce::Rectangle<float>& areaToFit,
                               ElementType elementToAdd = ElementType::all);

    /**
     * @brief Extract and fit geometry from an Svg string into a target area.
     *
     * Delegates to the juce::Rectangle\<float\> overload of getPath(), converting
     * @p areaToFit to floating point.
     *
     * The source bounds are determined by:
     * - The "viewBox" attribute, if present.
     * - Otherwise, the "width" and "height" attributes of the Svg element.
     *
     * @param svg          A juce::String containing valid Svg markup.
     * @param areaToFit    The rectangle into which the extracted geometry
     *                     should be scaled and fitted (integer coordinates).
     * @param elementToAdd The type of Svg element(s) to extract, as defined by
     *                     the ElementType enum.
     *
     * @return A juce::Path containing the transformed geometry.
     *
     * @note This overload accepts an integer rectangle, which is internally
     *       converted to a floating‑point rectangle for fitting.
     */

    static juce::Path getPath (const juce::String& svg,
                               const juce::Rectangle<int>& areaToFit,
                               ElementType elementToAdd = ElementType::all);

    /**
     * @brief Enumeration of Svg path rendering styles.
     *
     * Defines how a path should be represented when converted to Svg or drawn.
     * This is typically used to decide whether a path is rendered with a stroke,
     * a fill, or an alternate style.
     */
    enum class PathStyle
    {
        /**
         * @brief Render the path using stroke attributes.
         *
         * The path outline is drawn with a specified colour and stroke width.
         */
        stroke,

        /**
         * @brief Render the path using fill attributes.
         *
         * The interior of the path is filled with a specified colour.
         */
        fill,

        /**
         * @brief Render the path using an alternate style.
         *
         * This can be used for special cases or custom rendering modes
         * that differ from standard stroke or fill.
         */
        alternate,
    };

    /**
     * @brief Read a single key's value from an Svg element's style attribute.
     *
     * Parses the `style` attribute of \p element (e.g. `"fill:#ff00aa;stroke:#000;"`)
     * and returns the value associated with \p key. Returns an empty String when the
     * attribute is absent or the key is not present.
     *
     * @param element  Svg element to read the style attribute from.
     * @param key      The CSS property name to look up (e.g. Id::fill.toString()).
     * @return         The trimmed value, or empty String when absent.
     */
    static juce::String parseStyle (const Document::Element& element, juce::StringRef key);

    /** @brief Resolve the canonical name of an Svg element.
     *
     *  Returns the \c serif:id attribute when non-empty; otherwise the \c id
     *  attribute with all trailing ASCII digits stripped.
     *
     *  @param element  Svg element to query. Must not be nullptr.
     *  @return         Non-empty canonical name string, or empty when both
     *                  attributes are absent.
     */
    static juce::String getElementId (const Document::Element& element);

    /** @brief Extract paint-ready Shapes from a single Svg group element.
     *
     *  Walks every descendant of @p group via jam::Xml::applyFunctionRecursively.
     *  Per element:
     *  - Tag \c bounds → layout metadata consumed by Flex; no Shape emitted.
     *  - Tags \c path / \c rect / \c ellipse / \c circle → geometry extracted
     *    via getElementPath. \<g\> elements produce no geometry; the recursion
     *    visits their children automatically.
     *  - Per-element style only (no CSS inheritance). Fill and stroke ops read
     *    from the element's own style attribute via parseStyle.
     *  - Colour resolution: element name (from getElementId) looked up in
     *    @p colourScheme as an \c int entry. No match OR unnamed element (empty
     *    name) → self-styled: colourId=0, colour from own style attribute.
     *    Match → AttributedShape::colourId set to the LAF id, colour normalised to {}.
     *  - One element may emit both a fill Shape and a stroke Shape.
     *
     *  After the walk, AttributedGraphics::flatten() is called: fills emitted before
     *  strokes, same draw-call key (AttributedShape::operator==) merges geometry via
     *  path.addPath.
     *
     *  @param group        Top-level \<g\> element for one Svg cell. Must not be nullptr.
     *  @param colourScheme Node-scoped colour registry; int entries keyed by canonical
     *                      element name map to LAF colour ids.
     *  @return          Flattened, paint-ready AttributedGraphics for the cell.
     */
    static AttributedGraphics getAttributedGraphics (juce::XmlElement* group, const ColourScheme& colourScheme);

    /** @brief Document overload of getAttributedGraphics() — walks via Document::Element::applyFunctionRecursively. */
    static AttributedGraphics getAttributedGraphics (const Document::Element& group, const ColourScheme& colourScheme);

    /** @brief Self-styled overload — no LAF colour map.
     *
     *  Delegates to the two-argument overload with a function-local static empty
     *  ColourScheme. All shapes are self-styled (colourId==0, colour from Svg style).
     *
     *  @param group  Top-level \<g\> element for one Svg cell. Must not be nullptr.
     *  @return       Flattened, paint-ready AttributedGraphics for the cell.
     */
    static AttributedGraphics getAttributedGraphics (juce::XmlElement* group);

    /**
     * @brief CSS/Svg hex colour decoder.
     *
     * Replicates the `'#'`-branch logic of `juce::SVGParser::parseColour`
     * (`juce_SVGParser.cpp`) because `juce::Colour::fromString` only round-trips
     * `Colour::toString` 8-digit ARGB strings — it produces transparent black for
     * CSS shorthand (3-digit form) and treats 6-digit values as ARGB with alpha=0.
     *
     * The leading `#`, if present, is skipped; up to 8 hex nibbles are collected
     * into a nibble array (`hex[6]` and `hex[7]` default to 15, keeping alpha
     * opaque when fewer than 8 digits are present):
     *
     * - **`#RGB`** (3 digits or fewer) — CSS shorthand: each nibble expanded
     *   by ×0x11 (`'f'` → `0xff`). Fully opaque.
     * - **`#RRGGBB`** (6 digits) — opaque RGB; alpha defaults opaque via
     *   the nibble-array defaults.
     * - **`#RRGGBBAA`** (8 digits) — CSS alpha-last RGBA.
     *
     * @param value  CSS/Svg hex colour string with or without a leading `#`
     *               (e.g. `"#f00"`, `"#ff00aa"`, `"ff00aa"`, `"#80ff00aa"`).
     * @return       The decoded colour. Returns a default-constructed
     *               `juce::Colour()` (transparent black) when no valid hex
     *               digits are found.
     */
    static juce::Colour parseColour (juce::StringRef value);

    /**
     * @brief Forward declaration — defined in jam_svg_flex.h
     *        (juce_graphics consumers). Flex Svg layout system: parses a
     *        five-segment Svg document into LAF-aware paintable Shapes.
     */
    struct Flex;

#if JUCE_MODULE_AVAILABLE_juce_gui_basics
    /**
     * @brief Create a DrawablePath from an Svg element with a specified style.
     *
     * Extracts geometry from the given Svg XmlElement, wraps it in a juce::DrawablePath,
     * and applies either stroke or fill styling depending on the PathStyle.
     *
     * - PathStyle::stroke    → The path is stroked with the given colour and stroke width.
     * - PathStyle::fill      → The path is filled with the given colour.
     * - PathStyle::alternate → The path uses even‑odd winding (non‑zero winding disabled),
     *                          then falls through to stroke styling.
     *
     * @param svg          Pointer to a juce::XmlElement containing Svg data. Must not be nullptr.
     * @param colour       Colour to use for stroke or fill.
     * @param elementToAdd The type of Svg element(s) to extract, as defined by ElementType.
     * @param style        The rendering style (stroke, fill, or alternate).
     * @param strokeWidth  Stroke width in pixels (only used for stroke/alternate).
     *
     * @return A std::unique_ptr to a juce::DrawablePath configured with the extracted geometry.
     *
     * @note In the PathStyle::alternate case, the path winding rule is set to even‑odd
     *       (`setUsingNonZeroWinding(false)`), then stroke styling is applied.
     */
    static std::unique_ptr<juce::DrawablePath>
    getDrawablePath (juce::XmlElement* svg,
                     const juce::Colour& colour = juce::Colours::white,
                     ElementType elementToAdd = ElementType::all,
                     PathStyle style = PathStyle::fill,
                     float strokeWidth = 1.0f);

    /**
     * @brief Create a DrawablePath from an Svg string with a specified style.
     *
     * Parses the given Svg string into an XmlElement, then delegates to the
     * XmlElement overload of getDrawablePath() to extract geometry and apply styling.
     *
     * @param svgString    A C‑string containing valid Svg markup.
     * @param colour       Colour to use for stroke or fill.
     * @param elementToAdd The type of Svg element(s) to extract, as defined by ElementType.
     * @param style        The rendering style (stroke, fill, or alternate).
     * @param strokeWidth  Stroke width in pixels (only used for stroke/alternate).
     *
     * @return A std::unique_ptr to a juce::DrawablePath configured with the extracted geometry.
     */
    static std::unique_ptr<juce::DrawablePath>
    getDrawablePath (const char* svgString,
                     const juce::Colour& colour = juce::Colours::white,
                     ElementType elementToAdd = ElementType::all,
                     PathStyle style = PathStyle::fill,
                     float strokeWidth = 1.0f);

    /**
     * @brief Create a Drawable from an Svg string.
     *
     * Parses the given Svg string into an XmlElement and creates a juce::Drawable
     * from it, suitable for direct rendering.
     *
     * @param svgString A C‑string containing valid Svg markup.
     *
     * @return A std::unique_ptr to a juce::Drawable created from the Svg data.
     *
     * @note Unlike getDrawablePath(), this function returns a generic Drawable
     *       (not specifically a DrawablePath), which may internally be a composite
     *       of multiple shapes.
     */
    static std::unique_ptr<juce::Drawable> getDrawable (const char* svgString);
#endif// JUCE_MODULE_AVAILABLE_juce_gui_basics
    /*____________________________________________________________________________*/
    /** -------------------- FUNCTIONS FOR WRITING Svg ---------------------------*/

    /**
     * @brief Code templates used to generate Svg fragments as C++ source.
     *
     * Each member is a string literal with \c \@name\@ placeholders
     * substituted by the matching Format helper. Together they cover
     * the full pipeline from raw Svg markup to compiled-in string
     * literals and \c juce::Path parse expressions.
     */
    struct Template
    {
        /**
         * @brief Template for a full Svg document declaration.
         *
         * Contains XML declaration, DOCTYPE, and \<svg\> root element with
         * placeholders for width, height, and inner Svg content.
         *
         * Placeholders:
         * - \@svgWidth\@   → Width of the Svg in pixels.
         * - \@svgHeight\@  → Height of the Svg in pixels.
         * - \@svgString\@  → Inner Svg content (paths, groups, etc.).
         */
        static const juce::String declaration;

        /**
         * @brief Template for parsing an Svg path into a juce::Path.
         *
         * Produces a line of code invoking juce::DrawableImage::parseSVGPath().
         *
         * Placeholder:
         * - \@path\@ → The Svg path data string.
         */
        static const juce::String parseSVGPath;

        /**
         * @brief Template for declaring an array of juce::Path objects.
         *
         * Wraps multiple parsed paths into a juce::Array.
         *
         * Placeholders:
         * - \@varName\@ → Name of the array variable.
         * - \@paths\@   → List of path parsing expressions.
         */
        static const juce::String arrayPath;

        /**
         * @brief Templates for emitting \c const char* Svg fragments as source.
         *
         * Companion to the parent Template block: emits \c const char* pointers
         * and raw string literals rather than \c juce::Path parse expressions.
         */
        struct Ptr
        {
            /**
             * @brief Template for declaring an array of const char* pointers.
             *
             * Typically used to embed multiple Svg string literals.
             *
             * Placeholder:
             * - \@pointers\@ → List of string pointer entries.
             */
            static const juce::String chars;

            /**
             * @brief Template for embedding a raw Svg string literal.
             *
             * Wraps filtered Svg content inside a raw string literal
             * with a custom delimiter (SVGTEMPLATE).
             *
             * Placeholder:
             * - \@filtered\@ → The Svg markup to embed.
             */
            static const juce::String literal;

            /**
             * @brief Template for creating a Drawable from an Svg string.
             *
             * Produces a code snippet that parses an Svg string and
             * creates a juce::DrawableImage from it.
             *
             * Placeholder:
             * - \@chars\@ → The char* string containing Svg markup.
             */
            static const juce::String createFromSVG;
        };
    };

    /**
     * @brief Substitutes placeholders in the matching Template member to emit
     *        finished C++ source strings for Svg fragments.
     *
     * Each Format function takes the inputs required by its Template and
     * returns the fully-substituted code snippet. Together with Template,
     * Format is the writer half of the read/write pair: Template declares
     * the shape, Format produces the data.
     */
    struct Format
    {
        /*____________________________________________________________________________*/

        /**
         * @brief Substitutes placeholders in Template::Ptr to emit finished
         *        \c const char* Svg fragments as source.
         */
        struct Ptr
        {
            /**
             * @brief Substitute \@pointers\@ in Template::Ptr::chars.
             *
             * @param images  Concatenated list of pointer entries, e.g.
             *                `"\n\t\t\"foo\",\n\t\t\"bar\","`.
             * @return Finished code snippet with the placeholder replaced.
             */
            static juce::String chars (const juce::String& images);
            /**
             * @brief Substitute \@filtered\@ in Template::Ptr::literal with
             *        the XML declaration stripped from \p parsedXML.
             *
             * @param parsedXML  Svg element to serialise and strip the
             *                   XML declaration from via Filter::declaration().
             * @return Finished raw-string-literal code snippet.
             */
            static juce::String literal (juce::XmlElement* parsedXML);
            /**
             * @brief Substitute \@chars\@ in Template::Ptr::createFromSVG.
             *
             * @param chars  C-string expression holding Svg markup
             *               (e.g. a \c const char* identifier).
             * @return Finished code snippet that parses the Svg into a Drawable.
             */
            static juce::String createFromSVG (const juce::String& chars);
        };

        /**
         * @brief Convert a juce::Path into an Svg path data string.
         *
         * Tokenizes the path string, marks command tokens (A–Z) and numeric tokens,
         * then reassembles them into a properly formatted Svg path string.
         *
         * @param path The juce::Path to convert.
         * @return A juce::String containing the Svg path data, or an empty string
         *         if the path is empty.
         *
         * @note This function ensures that commands and coordinates are separated
         *       with commas and spaces for valid Svg syntax.
         */
        static juce::String pathToString (const juce::Path& path);

        /**
         * @brief Generate an Svg \<path\> element with stroke styling.
         *
         * Converts the given juce::Path into an Svg \<path\> element with no fill,
         * a stroke colour, and a stroke width.
         *
         * @param path        The juce::Path to convert.
         * @param colour      The stroke colour.
         * @param strokeWidth The stroke width in pixels.
         * @param pathId      Optional identifier for the path element.
         * @param dashLength  Dash length in path units, 0 (default) = solid,
         *                    no `stroke-dasharray` attribute emitted — mirrors
         *                    `AttributedShape::dashLength` (`jam_Svg.h`); non-zero
         *                    emits `stroke-dasharray="<dashLength>
         *                    <dashLength>"` (equal on/off segments, matching
         *                    `AttributedShape::dashLength`'s own single-value
         *                    convention).
         * @param gradientId  Empty (default) = flat `colour` as before. Non-empty
         *                    replaces the `stroke` attribute with
         *                    `url(#<gradientId>)` and omits `stroke-opacity`
         *                    (alpha already carried by the referenced
         *                    `<linearGradient>`'s own stops — see
         *                    `Format::linearGradient`); `colour` is then unused.
         *
         * @return A juce::String containing the Svg \<path\> element, or an empty
         *         string if the path is empty.
         */
        static juce::String stroke (const juce::Path& path,
                                    const juce::Colour& colour,
                                    float strokeWidth,
                                    const juce::String& pathId = juce::String(),
                                    float dashLength = 0.0f,
                                    const juce::String& gradientId = juce::String());
        /**
         * @brief Generate an Svg \<path\> element with fill styling.
         *
         * Converts the given juce::Path into an Svg \<path\> element with a fill
         * colour and no stroke.
         *
         * @param path       The juce::Path to convert.
         * @param colour     The fill colour.
         * @param pathId     Optional identifier for the path element.
         * @param gradientId Empty (default) = flat `colour` as before. Non-empty
         *                   replaces the `fill` attribute with
         *                   `url(#<gradientId>)` and omits `fill-opacity`
         *                   (alpha already carried by the referenced
         *                   `<linearGradient>`'s own stops — see
         *                   `Format::linearGradient`); `colour` is then unused.
         *
         * @return A juce::String containing the Svg \<path\> element, or an empty
         *         string if the path is empty.
         */
        static juce::String fill (const juce::Path& path,
                                  const juce::Colour& colour,
                                  const juce::String& pathId = juce::String(),
                                  const juce::String& gradientId = juce::String());

        /**
         * @brief Generate an Svg \<linearGradient\> definition with two colour stops.
         *
         * Built via `juce::XmlElement` (`Format::text`'s own precedent — the
         * only other Format writer whose markup nests attributes this way),
         * `gradientUnits="userSpaceOnUse"` so @p start/@p end read as the
         * same coordinate space every path element already draws in (no
         * `objectBoundingBox` renormalisation). Two `<stop>` children at
         * offsets `0%`/`100%`, each carrying its own `stop-color`
         * (`"#rrggbb"`) and `stop-opacity` (from the colour's own alpha) —
         * mirrors `Format::stroke`/`Format::fill`'s own rgb/alpha split.
         *
         * @param gradientId  Referenced by a sibling shape's `fill`/`stroke`
         *                    attribute as `url(#<gradientId>)` — deterministic
         *                    per-serialize-pass id, assigned by the caller
         *                    (`jam::MermaidDiagram::toSvgElement`).
         * @param start       Gradient axis start point (`AttributedShape::gradientStart`).
         * @param end         Gradient axis end point (`AttributedShape::gradientEnd`).
         * @param startColour Colour at the axis start (`AttributedShape::colourId`/`colour`).
         * @param endColour   Colour at the axis end (`AttributedShape::gradientEndColourId`).
         * @return            A juce::String containing the Svg \<linearGradient\> element.
         */
        static juce::String linearGradient (const juce::String& gradientId,
                                             juce::Point<float> start,
                                             juce::Point<float> end,
                                             const juce::Colour& startColour,
                                             const juce::Colour& endColour);

        /**
         * @brief Wrap Svg content in a \<defs\> element.
         *
         * Companion to `group()` — same wrap-in-a-tag shape, `<defs>` instead
         * of `<g>`, no id attribute (a document has at most one `<defs>` block;
         * `jam::MermaidDiagram::serialize` gathers every shape's own
         * `Format::linearGradient` output into one call).
         *
         * @param svgString The inner Svg content to wrap (one or more
         *                  `<linearGradient>` elements, concatenated).
         * @return          A juce::String containing the `<defs>` markup.
         */
        static juce::String defs (juce::StringRef svgString);

        /**
         * @brief Generate an Svg \<text\> element from an AttributedText payload.
         *
         * Position: x taken from \p text.bounds' left edge; y is the
         * *baseline*, derived as `text.bounds.getY() + fontHeight`, where
         * `fontHeight` is the sole attribute's `juce::Font::getHeight()`
         * (`jam::AttributedString::getAttributes()`). This is exact under the
         * shared measurement convention every producer of `AttributedText`
         * follows (`jam::MermaidDiagram::measureText`): `bounds`' height
         * already equals `fontHeight`, so `bounds.getY() + fontHeight` is
         * `bounds`' bottom edge — the baseline sits there by construction.
         * Content is the decoded text of \p text.text, XML-escaped via
         * `juce::XmlElement`'s own text-node serialization (OOTB escaping —
         * matches this file's own established writer style: hand-built markup
         * elsewhere in this struct never needs escaping since it carries no
         * free-form user text; \<text\> is the first Format writer whose
         * content is arbitrary source text, so it is the first to need it).
         *
         * @note Single-range only: \p text.text must carry exactly one
         *       `jam::AttributedString::Attribute` (one `append()` call) —
         *       asserted at entry. Multi-range `AttributedText` is not
         *       serialized; support is deferred to whichever diagram type
         *       first needs it.
         * @note Single-element only: multi-line text is NOT split into one
         *       \<text\> per line — every line of a multi-line label collapses
         *       onto the one baseline computed above. Multi-line \<tspan\>
         *       support is deferred to whichever diagram type first needs it.
         *
         * @param text    Text payload — `text.text`, `text.bounds`.
         * @param colour  Resolved fill colour (LAF- or self-resolved by the
         *                caller — mirrors `Format::stroke`/`Format::fill`,
         *                which likewise take a resolved `juce::Colour`, never
         *                a `colourId`).
         * @return        A juce::String containing the Svg \<text\> element,
         *                or an empty string when \p text.text is empty.
         */
        static juce::String text (const AttributedText& text, const juce::Colour& colour);

        /**
         * @brief Wrap Svg content in a \<g\> group element.
         *
         * Creates an Svg \<g\> element containing the provided Svg string, with
         * an optional group identifier.
         *
         * @param svgString The inner Svg content to group.
         * @param groupId   Optional identifier for the group element.
         *
         * @return A juce::String containing the grouped Svg markup.
         */
        static juce::String
        group (juce::StringRef svgString, const juce::String& groupId = juce::String());

        static juce::String
        anchor (juce::StringRef svgString, const juce::String& url);

        /**
         * @brief Generate an Svg \<rect\> element from a JUCE rectangle.
         *
         * Converts a juce::Rectangle\<int\> into an Svg \<rect\> element with stroke
         * styling and no fill.
         *
         * @param rectangle The rectangle to convert.
         * @param rectId    Identifier for the rect element (converted to a valid ID).
         * @param colour    Stroke colour for the rectangle.
         *
         * @return A juce::String containing the Svg \<rect\> element.
         *
         * @note The rectangle's x, y, width, and height are extracted and inserted
         *       into the Svg attributes.
         */
        static juce::String rect (const juce::Rectangle<int>& rectangle,
                                  const juce::String& rectId = juce::String(),
                                  const juce::Colour& colour = juce::Colours::magenta);

        /**
         * @brief Convert Svg elements into an array of path parsing strings.
         *
         * This function inspects the children of the given Svg XmlElement and
         * generates a juce::StringArray of code snippets that parse each shape
         * into a juce::Path using juce::DrawableImage::parseSVGPath().
         *
         * Supported element types:
         * - \<path\>    → Uses the "d" attribute directly.
         * - \<ellipse\> → Converted into a juce::Path ellipse, then serialized.
         * - \<circle\>  → Converted into a juce::Path circle, then serialized.
         * - \<rect\>    → Converted into a juce::Path rectangle, unless its style
         *               attribute is "fill:none;" (ignored).
         *
         * Each generated string is based on the Template::parseSVGPath format,
         * with \@path\@ replaced by the serialized path data.
         *
         * @param svg     Pointer to a juce::XmlElement containing Svg markup.
         *                Must not be nullptr.
         *
         * @return A juce::StringArray containing one entry per recognized Svg
         *         element, each entry being a code snippet that parses the
         *         corresponding path.
         *
         * @note The function first flattens group (\<g\>) elements using
         *       Filter::group() before iterating over child elements.
         * @note Ellipses, circles, and rectangles are converted into juce::Path
         *       objects before being serialized with Format::pathToString().
         */
        static juce::StringArray
        getStringArrayPath (juce::XmlElement* svg);
    };

    /**
     * @brief File-level Svg document assembly: wraps fragments into a
     *        standalone Svg file string with width/height headers.
     */
    struct File
    {
        /**
         * @brief Generate a complete Svg document string with dimensions and content.
         *
         * This function takes the provided width, height, and inner Svg markup,
         * and substitutes them into the Template::declaration string. The result
         * is a fully‑formed Svg document string that includes the XML declaration,
         * DOCTYPE, \<svg\> root element, and the supplied inner content.
         *
         * Placeholders replaced:
         * - \@svgWidth\@   → Replaced with the given width (in pixels).
         * - \@svgHeight\@  → Replaced with the given height (in pixels).
         * - \@svgString\@  → Replaced with the provided Svg fragment/content.
         *
         * @param width      The width of the Svg canvas in pixels.
         * @param height     The height of the Svg canvas in pixels.
         * @param svgString  The inner Svg markup to embed inside the \<svg\> element.
         *
         * @return A juce::String containing the complete Svg document.
         *
         * @note This function is typically used when writing an Svg file to disk,
         *       ensuring that the output is a valid standalone Svg document.
         */
        static juce::String
        getStringToWrite (int width, int height, juce::StringRef svgString);
    };

    /*____________________________________________________________________________*/
    /**
     * @brief Retrieve the width attribute from an Svg element.
     *
     * Reads the "width" attribute of the given Document and returns it
     * as an integer.
     *
     * @param svg The Document representing the root or fragment of an
     *            Svg document.
     *
     * @return The integer value of the "width" attribute. Returns 0 if
     *         the attribute is missing or cannot be parsed.
     */
    static int getSVGWidth (const Document::Element& svg);

    /**
     * @brief Retrieve the height attribute from an Svg element.
     *
     * Reads the "height" attribute of the given Document and returns it
     * as an integer.
     *
     * @param svg The Document representing the root or fragment of an
     *            Svg document.
     *
     * @return The integer value of the "height" attribute. Returns 0 if
     *         the attribute is missing or cannot be parsed.
     */
    static int getSVGHeight (const Document::Element& svg);

    /**
     * @brief Retrieve both width and height attributes from an Svg element.
     *
     * Convenience function that calls getSVGWidth() and getSVGHeight() and
     * returns the results as a std::pair.
     *
     * @param svg The Document representing the root or fragment of an
     *            Svg document.
     *
     * @return A std::pair<int, int> containing (width, height).
     *
     * @note This is useful when both dimensions are needed together, e.g.
     *       for scaling or fitting operations.
     */
    static auto getSVGSize (const Document::Element& svg);

    /*____________________________________________________________________________*/

    /**
     * @brief A lightweight container for Svg path fragments.
     *
     * The Snapshot class extends juce::StringArray to collect Svg fragments
     * (strokes and fills) generated from juce::Path objects. It is designed
     * to be used when exporting or serializing graphics into Svg format,
     * especially in background threads where thread-safety is critical.
     *
     * Each call to addStroke() or addFill() converts a juce::Path into an
     * Svg string fragment using the Format helpers, and stores it in the
     * underlying StringArray.
     *
     * @note Paths are explicitly copied before conversion to ensure thread-safety.
     *       This avoids race conditions if the original Path is owned or mutated
     *       by GUI components on the message thread.
     */
    struct Snapshot : public juce::StringArray
    {
        /**
         * @brief Default constructor.
         */
        Snapshot() = default;

        /**
         * @brief Add a stroked path to the snapshot.
         *
         * Converts the given juce::Path into an Svg \<path\> element with stroke
         * attributes, and appends it to the internal StringArray.
         *
         * @param path        The path to be converted. A copy is made internally
         *                    to ensure thread-safety.
         * @param colour      Stroke colour (default: magenta).
         * @param strokeWidth Stroke width in pixels (default: 1.0f).
         * @param strokeId    Identifier string for the Svg element (default: "stroke").
         *
         * @note The path is copied (`juce::Path(path)`) before conversion.
         *       This prevents concurrent access issues if the original path
         *       is modified on another thread (e.g. GUI paint routines).
         */
        void addStroke (const juce::Path& path,
                        juce::Colour colour = juce::Colours::magenta,
                        float strokeWidth = 1.0f,
                        juce::StringRef strokeId = "stroke")
        {
            juce::StringArray::add (
                Format::stroke (juce::Path (path), colour, strokeWidth, strokeId));
        }

        /**
         * @brief Add a filled path to the snapshot.
         *
         * Converts the given juce::Path into an Svg \<path\> element with fill
         * attributes, and appends it to the internal StringArray.
         *
         * @param path     The path to be converted. A copy is made internally
         *                 to ensure thread-safety.
         * @param colour   Fill colour (default: yellow).
         * @param fillId   Identifier string for the Svg element (default: "fill").
         *
         * @note The path is copied (`juce::Path(path)`) before conversion.
         *       This prevents concurrent access issues if the original path
         *       is modified on another thread (e.g. GUI paint routines).
         */
        void addFill (const juce::Path& path,
                      juce::Colour colour = juce::Colours::yellow,
                      juce::StringRef fillId = "fill")
        {
            juce::StringArray::add (Format::fill (juce::Path (path), colour, fillId));
        }

        /**
         * @brief Wrap all collected Svg fragments into a grouped \<g\> element.
         *
         * This method joins all strings currently stored in the Snapshot into
         * a single Svg fragment, separated by newlines, and then wraps them
         * inside an Svg \<g\> element with the specified group identifier.
         *
         * @param groupId  Identifier string for the Svg group element. This
         *                 will be used as the "id" attribute of the \<g\> tag.
         *
         * @return A juce::String containing the grouped Svg markup.
         *
         * @note Marked noexcept because it does not throw exceptions.
         * @note This is a convenience method for producing a self‑contained
         *       Svg group from the Snapshot contents, making it easier to
         *       embed the snapshot into larger Svg documents.
         */
        juce::String getGroup (juce::StringRef groupId) const noexcept
        {
            return Format::group (joinIntoString ("\n"), groupId);
        }
    };
};

/**_____________________________END OF NAMESPACE______________________________*/
} /** namespace jam */
