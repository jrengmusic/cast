/*******************************************************************************
                        Codegen Annotated Source of Truth
————————————————————————————————————————————————————————————————————————————————

            ░░████████████░░████████████░░████████████░░████████████
            ░░████  ░░████░░████  ░░████░░████  ░░████    ░░████
            ░░████        ░░████  ░░████░░████            ░░████
            ░░████        ░░████████████░░████████████    ░░████
            ░░████        ░░████  ░░████        ░░████    ░░████
            ░░████  ░░████░░████  ░░████░░████  ░░████    ░░████
            ░░████████████░░████  ░░████░░████████████    ░░████

————————————————————————————————————————————————————————————————————————————————
                         FOR YOUR EYES ONLY, DO NOT EDIT
********************************************************************************/

/**
 * @file jam_Mermaid.h
 * @brief Mermaid diagram vocabulary — keywords, shapes, styles, operators, and
 * the packed geometry/marker tables that map them onto render primitives.
 *
 * The Mermaid struct is the single generated source of truth for every
 * diagram dialect the renderer accepts: front-matter diagram types, node
 * shapes and their construction/hit-test/size policies, edge strokes and
 * decorations, stylesheet colour-id and metric keys, and the full operator
 * alphabet (delimiters, messages, class relations, ER cardinalities). Each
 * `jam::Bimap` exposes a bidirectional key↔literal registry via getInstance();
 * each `jam::LookupTable` is a `static constexpr` direct-indexed conversion
 * consumed by the layout and draw passes.
 */

#pragma once

/**
 * @brief Mermaid diagram vocabulary, keyed by diagram dialect.
 *
 * Nested structs group the registries and lookup tables one dialect owns.
 * Keyword bimaps map an enum key to the exact source-literal the parser
 * matches; the accompanying `LookupTable`s collapse those keywords into
 * packed render decisions (stroke, decoration, shape, geometry) so the draw
 * pass stays branch-free.
 */
struct Mermaid
{
    /**
     * @brief Front-matter diagram-type keywords and their ordinal keys.
     *
     * Maps each accepted `diagramType` spelling (plus the C4 family and the
     * railroad-beta dialects) to a contiguous key used to index the
     * per-diagram arrowhead and marker tables. The key is the discriminator
     * the renderer switches on after the parser reports the diagram type.
     */
    struct DiagramType : public jam::Bimap<int, juce::Identifier>
    {
        DiagramType() : jam::Bimap<int, juce::Identifier> { {
                { info,               juce::Identifier { "info" } },
                { pie,                juce::Identifier { "pie" } },
                { packet,             juce::Identifier { "packet" } },
                { packetBeta,         juce::Identifier { "packet-beta" } },
                { xychart,            juce::Identifier { "xychart" } },
                { xychartBeta,        juce::Identifier { "xychart-beta" } },
                { radarBeta,          juce::Identifier { "radar-beta" } },
                { vennBeta,           juce::Identifier { "venn-beta" } },
                { quadrantChart,      juce::Identifier { "quadrantChart" } },
                { timeline,           juce::Identifier { "timeline" } },
                { journey,            juce::Identifier { "journey" } },
                { gantt,              juce::Identifier { "gantt" } },
                { kanban,             juce::Identifier { "kanban" } },
                { treeViewBeta,       juce::Identifier { "treeView-beta" } },
                { cynefinBeta,        juce::Identifier { "cynefin-beta" } },
                { treemapBeta,        juce::Identifier { "treemap-beta" } },
                { mindmap,            juce::Identifier { "mindmap" } },
                { ishikawa,           juce::Identifier { "ishikawa" } },
                { wardleyBeta,        juce::Identifier { "wardley-beta" } },
                { erDiagram,          juce::Identifier { "erDiagram" } },
                { stateDiagram,       juce::Identifier { "stateDiagram" } },
                { stateDiagramV2,     juce::Identifier { "stateDiagram-v2" } },
                { classDiagram,       juce::Identifier { "classDiagram" } },
                { classDiagramV2,     juce::Identifier { "classDiagram-v2" } },
                { requirementDiagram, juce::Identifier { "requirementDiagram" } },
                { graph,              juce::Identifier { "graph" } },
                { flowchart,          juce::Identifier { "flowchart" } },
                { flowchartElk,       juce::Identifier { "flowchart-elk" } },
                { block,              juce::Identifier { "block" } },
                { blockBeta,          juce::Identifier { "block-beta" } },
                { architectureBeta,   juce::Identifier { "architecture-beta" } },
                { gitGraph,           juce::Identifier { "gitGraph" } },
                { sankey,             juce::Identifier { "sankey" } },
                { sankeyBeta,         juce::Identifier { "sankey-beta" } },
                { sequenceDiagram,    juce::Identifier { "sequenceDiagram" } },
                { swimlaneBeta,       juce::Identifier { "swimlane-beta" } },
                { eventmodeling,      juce::Identifier { "eventmodeling" } },
                { railroadBeta,       juce::Identifier { "railroad-beta" } },
                { railroadEbnfBeta,   juce::Identifier { "railroad-ebnf-beta" } },
                { railroadAbnfBeta,   juce::Identifier { "railroad-abnf-beta" } },
                { railroadPegBeta,    juce::Identifier { "railroad-peg-beta" } },
                { c4Context,          juce::Identifier { "C4Context" } },
                { c4Container,        juce::Identifier { "C4Container" } },
                { c4Component,        juce::Identifier { "C4Component" } },
                { c4Dynamic,          juce::Identifier { "C4Dynamic" } },
                { c4Deployment,       juce::Identifier { "C4Deployment" } },
        } } {}

        enum value : int
        {
            info               = 0,
            pie                = 1,
            packet             = 2,
            packetBeta         = 3,
            xychart            = 4,
            xychartBeta        = 5,
            radarBeta          = 6,
            vennBeta           = 7,
            quadrantChart      = 8,
            timeline           = 9,
            journey            = 10,
            gantt              = 11,
            kanban             = 12,
            treeViewBeta       = 13,
            cynefinBeta        = 14,
            treemapBeta        = 15,
            mindmap            = 16,
            ishikawa           = 17,
            wardleyBeta        = 18,
            erDiagram          = 19,
            stateDiagram       = 20,
            stateDiagramV2     = 21,
            classDiagram       = 22,
            classDiagramV2     = 23,
            requirementDiagram = 24,
            graph              = 25,
            flowchart          = 26,
            flowchartElk       = 27,
            block              = 28,
            blockBeta          = 29,
            architectureBeta   = 30,
            gitGraph           = 31,
            sankey             = 32,
            sankeyBeta         = 33,
            sequenceDiagram    = 34,
            swimlaneBeta       = 35,
            eventmodeling      = 36,
            railroadBeta       = 37,
            railroadEbnfBeta   = 38,
            railroadAbnfBeta   = 39,
            railroadPegBeta    = 40,
            c4Context          = 41,
            c4Container        = 42,
            c4Component        = 43,
            c4Dynamic          = 44,
            c4Deployment       = 45,
        };

        static DiagramType* getInstance() noexcept
        {
            return jam::SharedInstance<DiagramType>::getInstance();
        }
    };

    //==============================================================================

    /**
     * @brief Node-shape keywords and their ordinal keys.
     *
     * Maps every shape name a diagram grammar accepts to the key that indexes
     * the `Shape` geometry, corner-radius, padding, and delimiter tables. The
     * key is the shape discriminator carried by a parsed node through layout
     * into the draw pass.
     *
     * `terminalDot` (25) and `terminalRing` (26) are the terminal marker
     * glyphs, distinct from the labeled circle nodes: they render the state
     * `[*]` start/end markers, git commits, and architecture junctions.
     * `circle` and `doubleCircle` are labeled node shapes only. `person` (27)
     * is the C4 actor figure silhouette.
     */
    struct NodeShape : public jam::Bimap<int>
    {
        NodeShape() : jam::Bimap<int> { {
                { rectangle,        juce::String::fromUTF8 ("rectangle") },
                { forkJoin,         juce::String::fromUTF8 ("forkJoin") },
                { roundRect,        juce::String::fromUTF8 ("roundRect") },
                { stadium,          juce::String::fromUTF8 ("stadium") },
                { subroutine,       juce::String::fromUTF8 ("subroutine") },
                { cylinder,         juce::String::fromUTF8 ("cylinder") },
                { actorBox,         juce::String::fromUTF8 ("actorBox") },
                { circle,           juce::String::fromUTF8 ("circle") },
                { doubleCircle,     juce::String::fromUTF8 ("doubleCircle") },
                { diamond,          juce::String::fromUTF8 ("diamond") },
                { hexagon,          juce::String::fromUTF8 ("hexagon") },
                { parallelogram,    juce::String::fromUTF8 ("parallelogram") },
                { parallelogramAlt, juce::String::fromUTF8 ("parallelogramAlt") },
                { trapezoid,        juce::String::fromUTF8 ("trapezoid") },
                { trapezoidAlt,     juce::String::fromUTF8 ("trapezoidAlt") },
                { asymmetric,       juce::String::fromUTF8 ("asymmetric") },
                { mindmapDefault,   juce::String::fromUTF8 ("mindmapDefault") },
                { bang,             juce::String::fromUTF8 ("bang") },
                { cloud,            juce::String::fromUTF8 ("cloud") },
                { text,             juce::String::fromUTF8 ("text") },
                { space,            juce::String::fromUTF8 ("space") },
                { blockArrow,       juce::String::fromUTF8 ("blockArrow") },
                { triangle,         juce::String::fromUTF8 ("triangle") },
                { cross,            juce::String::fromUTF8 ("cross") },
                { barb,             juce::String::fromUTF8 ("barb") },
                { terminalDot,      juce::String::fromUTF8 ("terminalDot") },
                { terminalRing,     juce::String::fromUTF8 ("terminalRing") },
                { person,           juce::String::fromUTF8 ("person") },
        } } {}

        enum value : int
        {
            rectangle        = 0,
            forkJoin         = 1,
            roundRect        = 2,
            stadium          = 3,
            subroutine       = 4,
            cylinder         = 5,
            actorBox         = 6,
            circle           = 7,
            doubleCircle     = 8,
            diamond          = 9,
            hexagon          = 10,
            parallelogram    = 11,
            parallelogramAlt = 12,
            trapezoid        = 13,
            trapezoidAlt     = 14,
            asymmetric       = 15,
            mindmapDefault   = 16,
            bang             = 17,
            cloud            = 18,
            text             = 19,
            space            = 20,
            blockArrow       = 21,
            triangle         = 22,
            cross            = 23,
            barb             = 24,
            terminalDot      = 25,
            terminalRing     = 26,
            person           = 27,
        };

        static NodeShape* getInstance() noexcept
        {
            return jam::SharedInstance<NodeShape>::getInstance();
        }
    };

    //==============================================================================

    /**
     * @brief Flowchart direction keywords.
     *
     * `Keyword` maps the four/`TB`/`TD`/`BT`/`LR`/`RL` orientation literals
     * to keys the layout pass uses to set the graph flow direction.
     */
    struct Flowchart
    {
        struct Keyword : public jam::Bimap<int>
        {
            Keyword() : jam::Bimap<int> { {
                    { tb, juce::String::fromUTF8 ("TB") },
                    { td, juce::String::fromUTF8 ("TD") },
                    { bt, juce::String::fromUTF8 ("BT") },
                    { lr, juce::String::fromUTF8 ("LR") },
                    { rl, juce::String::fromUTF8 ("RL") },
            } } {}

            enum value : int
            {
                tb = 0,
                td = 1,
                bt = 2,
                lr = 3,
                rl = 4,
            };

            static Keyword* getInstance() noexcept
            {
                return jam::SharedInstance<Keyword>::getInstance();
            }
        };
    };

    //==============================================================================

    /**
     * @brief Edge stroke styles — solid, dotted, thick, invisible.
     *
     * `solid`/`dotted`/`thick` map to line styles the draw pass renders;
     * `invisible` marks an edge that carries structure but no stroked path.
     */
    struct EdgeStroke : public jam::Bimap<int>
    {
        EdgeStroke() : jam::Bimap<int> { {
                { solid,     juce::String::fromUTF8 ("solid") },
                { dotted,    juce::String::fromUTF8 ("dotted") },
                { thick,     juce::String::fromUTF8 ("thick") },
                { invisible, juce::String::fromUTF8 ("invisible") },
        } } {}

        enum value : int
        {
            solid     = 0,
            dotted    = 1,
            thick     = 2,
            invisible = 3,
        };

        static EdgeStroke* getInstance() noexcept
        {
            return jam::SharedInstance<EdgeStroke>::getInstance();
        }
    };

    //==============================================================================

    /**
     * @brief Edge endpoint decorations and the crows-foot / class-relation marks.
     *
     * Maps each decoration name (arrow, circle, diamond, inheritance, and the
     * ER crows-foot variants) to the key that indexes the `Marker` path,
     * anchor, stroke, and geometry tables at either end of an edge.
     */
    struct EdgeDecoration : public jam::Bimap<int>
    {
        EdgeDecoration() : jam::Bimap<int> { {
                { none,              juce::String::fromUTF8 ("none") },
                { circle,            juce::String::fromUTF8 ("circle") },
                { cross,             juce::String::fromUTF8 ("cross") },
                { diamond,           juce::String::fromUTF8 ("diamond") },
                { diamondFilled,     juce::String::fromUTF8 ("diamondFilled") },
                { circleCross,       juce::String::fromUTF8 ("circleCross") },
                { crowsFootOne,      juce::String::fromUTF8 ("crowsFootOne") },
                { crowsFootZeroOne,  juce::String::fromUTF8 ("crowsFootZeroOne") },
                { crowsFootMany,     juce::String::fromUTF8 ("crowsFootMany") },
                { crowsFootZeroMany, juce::String::fromUTF8 ("crowsFootZeroMany") },
                { arrow,             juce::String::fromUTF8 ("arrow") },
                { inheritance,       juce::String::fromUTF8 ("inheritance") },
                { dependency,        juce::String::fromUTF8 ("dependency") },
                { lollipop,          juce::String::fromUTF8 ("lollipop") },
        } } {}

        enum value : int
        {
            none              = 0,
            circle            = 1,
            cross             = 2,
            diamond           = 3,
            diamondFilled     = 4,
            circleCross       = 5,
            crowsFootOne      = 6,
            crowsFootZeroOne  = 7,
            crowsFootMany     = 8,
            crowsFootZeroMany = 9,
            arrow             = 10,
            inheritance       = 11,
            dependency        = 12,
            lollipop          = 13,
        };

        static EdgeDecoration* getInstance() noexcept
        {
            return jam::SharedInstance<EdgeDecoration>::getInstance();
        }
    };

    //==============================================================================

    /**
     * @brief Marker paint modes — solid fill, stroked outline, or background.
     *
     * Selects how a decoration's path is filled when the draw pass renders it.
     */
    struct MarkerPaint : public jam::Bimap<int>
    {
        MarkerPaint() : jam::Bimap<int> { {
                { solid,      juce::String::fromUTF8 ("solid") },
                { stroked,    juce::String::fromUTF8 ("stroked") },
                { background, juce::String::fromUTF8 ("background") },
        } } {}

        enum value : int
        {
            solid      = 0,
            stroked    = 1,
            background = 2,
        };

        static MarkerPaint* getInstance() noexcept
        {
            return jam::SharedInstance<MarkerPaint>::getInstance();
        }
    };

    //==============================================================================

    /**
     * @brief Stylesheet keys — metric names.
     *
     * The bimap maps a small integer key to a metric name.
     *
     * The metric name is the bare key for a measurement. Font size,
     * padding, radius, and stroke are examples. The theme stores the
     * measurement value under this name. Each row records which
     * measurement slot the key names.
     */
    struct StyleSheet : public jam::Bimap<int, juce::Identifier>
    {
        StyleSheet() : jam::Bimap<int, juce::Identifier> { {
                { stateFontSize,                   juce::Identifier { "stateFontSize" } },
                { lineHeight,                      juce::Identifier { "lineHeight" } },
                { stateNodeStrokeWidth,            juce::Identifier { "stateNodeStrokeWidth" } },
                { stateEdgeStrokeWidth,            juce::Identifier { "stateEdgeStrokeWidth" } },
                { stateClusterStrokeWidth,         juce::Identifier { "stateClusterStrokeWidth" } },
                { stateCornerRadius,               juce::Identifier { "stateCornerRadius" } },
                { stateTerminalSize,               juce::Identifier { "stateTerminalSize" } },
                { stateTerminalInnerSize,          juce::Identifier { "stateTerminalInnerSize" } },
                { stateTerminalStrokeWidth,        juce::Identifier { "stateTerminalStrokeWidth" } },
                { arrowheadWidth,                  juce::Identifier { "arrowheadWidth" } },
                { arrowheadLength,                 juce::Identifier { "arrowheadLength" } },
                { nodeSeparation,                  juce::Identifier { "nodeSeparation" } },
                { rankSeparation,                  juce::Identifier { "rankSeparation" } },
                { canvasMargin,                    juce::Identifier { "canvasMargin" } },
                { groupPadding,                    juce::Identifier { "groupPadding" } },
                { nodePadding,                     juce::Identifier { "nodePadding" } },
                { groupTitleHeight,                juce::Identifier { "groupTitleHeight" } },
                { edgeLabelPadding,                juce::Identifier { "edgeLabelPadding" } },
                { flowchartForkJoinCornerRadius,   juce::Identifier { "flowchartForkJoinCornerRadius" } },
                { rectangleCornerRadius,           juce::Identifier { "rectangleCornerRadius" } },
                { actorBoxCornerRadius,            juce::Identifier { "actorBoxCornerRadius" } },
                { roundRectCornerRadius,           juce::Identifier { "roundRectCornerRadius" } },
                { mindmapDefaultCornerRadius,      juce::Identifier { "mindmapDefaultCornerRadius" } },
                { flowchartCircleEmptyMinSize,     juce::Identifier { "flowchartCircleEmptyMinSize" } },
                { flowchartDoubleCircleInnerInset, juce::Identifier { "flowchartDoubleCircleInnerInset" } },
                { erEntityPadding,                 juce::Identifier { "erEntityPadding" } },
                { erMinEntityWidth,                juce::Identifier { "erMinEntityWidth" } },
                { erMinEntityHeight,               juce::Identifier { "erMinEntityHeight" } },
                { erNodeSeparation,                juce::Identifier { "erNodeSeparation" } },
                { erRankSeparation,                juce::Identifier { "erRankSeparation" } },
                { erEdgeStrokeWidth,               juce::Identifier { "erEdgeStrokeWidth" } },
                { erEdgeDashLength,                juce::Identifier { "erEdgeDashLength" } },
                { erMarkerBarHeight,               juce::Identifier { "erMarkerBarHeight" } },
                { erMarkerBarSpacing,              juce::Identifier { "erMarkerBarSpacing" } },
                { erMarkerFootSpread,              juce::Identifier { "erMarkerFootSpread" } },
                { erMarkerFootLength,              juce::Identifier { "erMarkerFootLength" } },
                { erMarkerCircleRadius,            juce::Identifier { "erMarkerCircleRadius" } },
                { sequenceActorMargin,             juce::Identifier { "sequenceActorMargin" } },
                { sequenceActorWidth,              juce::Identifier { "sequenceActorWidth" } },
                { sequenceActorHeight,             juce::Identifier { "sequenceActorHeight" } },
                { sequenceMessageMargin,           juce::Identifier { "sequenceMessageMargin" } },
                { sequenceBoxMargin,               juce::Identifier { "sequenceBoxMargin" } },
                { sequenceNoteMargin,              juce::Identifier { "sequenceNoteMargin" } },
                { sequenceActivationWidth,         juce::Identifier { "sequenceActivationWidth" } },
                { sequenceLifelineStrokeWidth,     juce::Identifier { "sequenceLifelineStrokeWidth" } },
                { sequenceMessageStrokeWidth,      juce::Identifier { "sequenceMessageStrokeWidth" } },
                { sequenceMessageDashLength,       juce::Identifier { "sequenceMessageDashLength" } },
                { classPadding,                    juce::Identifier { "classPadding" } },
                { classTitleFontSize,              juce::Identifier { "classTitleFontSize" } },
                { classBodyFontSize,               juce::Identifier { "classBodyFontSize" } },
                { classDividerStrokeWidth,         juce::Identifier { "classDividerStrokeWidth" } },
                { classEdgeStrokeWidth,            juce::Identifier { "classEdgeStrokeWidth" } },
                { classEdgeDashLength,             juce::Identifier { "classEdgeDashLength" } },
                { classCardinalityFontSize,        juce::Identifier { "classCardinalityFontSize" } },
                { classCardinalityDistance,        juce::Identifier { "classCardinalityDistance" } },
                { classCardinalityOffset,          juce::Identifier { "classCardinalityOffset" } },
                { classExtensionLength,            juce::Identifier { "classExtensionLength" } },
                { classExtensionWidth,             juce::Identifier { "classExtensionWidth" } },
                { classDiamondLength,              juce::Identifier { "classDiamondLength" } },
                { classDiamondWidth,               juce::Identifier { "classDiamondWidth" } },
                { classDependencyLength,           juce::Identifier { "classDependencyLength" } },
                { classDependencyWidth,            juce::Identifier { "classDependencyWidth" } },
                { classLollipopRadius,             juce::Identifier { "classLollipopRadius" } },
                { c4ShapeWidth,                    juce::Identifier { "c4ShapeWidth" } },
                { c4ShapeHeight,                   juce::Identifier { "c4ShapeHeight" } },
                { c4ShapePadding,                  juce::Identifier { "c4ShapePadding" } },
                { c4ShapeMargin,                   juce::Identifier { "c4ShapeMargin" } },
                { c4DiagramMarginX,                juce::Identifier { "c4DiagramMarginX" } },
                { c4DiagramMarginY,                juce::Identifier { "c4DiagramMarginY" } },
                { c4ElementFontSize,               juce::Identifier { "c4ElementFontSize" } },
                { c4StereotypeFontSize,            juce::Identifier { "c4StereotypeFontSize" } },
                { c4DescriptionFontSize,           juce::Identifier { "c4DescriptionFontSize" } },
                { c4SectionGap,                    juce::Identifier { "c4SectionGap" } },
                { c4BoundaryLabelFontSize,         juce::Identifier { "c4BoundaryLabelFontSize" } },
                { c4BoundaryTypeFontSize,          juce::Identifier { "c4BoundaryTypeFontSize" } },
                { c4BoundaryDescrFontSize,         juce::Identifier { "c4BoundaryDescrFontSize" } },
                { c4BoundaryLabelOffset,           juce::Identifier { "c4BoundaryLabelOffset" } },
                { c4BoundaryTypeOffset,            juce::Identifier { "c4BoundaryTypeOffset" } },
                { c4BoundaryDescrOffset,           juce::Identifier { "c4BoundaryDescrOffset" } },
                { c4MessageFontSize,               juce::Identifier { "c4MessageFontSize" } },
                { c4BoundaryDashLength,            juce::Identifier { "c4BoundaryDashLength" } },
                { c4BoundaryCornerRadius,          juce::Identifier { "c4BoundaryCornerRadius" } },
                { c4ShapeCornerRadius,             juce::Identifier { "c4ShapeCornerRadius" } },
                { c4ShapeStrokeWidth,              juce::Identifier { "c4ShapeStrokeWidth" } },
                { c4BoundaryStrokeWidth,           juce::Identifier { "c4BoundaryStrokeWidth" } },
                { c4EdgeStrokeWidth,               juce::Identifier { "c4EdgeStrokeWidth" } },
                { architectureIconSize,            juce::Identifier { "architectureIconSize" } },
                { architecturePadding,             juce::Identifier { "architecturePadding" } },
                { architectureFontSize,            juce::Identifier { "architectureFontSize" } },
                { architectureEdgeStrokeWidth,     juce::Identifier { "architectureEdgeStrokeWidth" } },
                { architectureEdgeLength,          juce::Identifier { "architectureEdgeLength" } },
                { architectureGroupStrokeWidth,    juce::Identifier { "architectureGroupStrokeWidth" } },
                { architectureGroupDashLength,     juce::Identifier { "architectureGroupDashLength" } },
                { architectureGroupIconSize,       juce::Identifier { "architectureGroupIconSize" } },
                { architectureLabelWidth,          juce::Identifier { "architectureLabelWidth" } },
                { architectureGroupLabelPadding,   juce::Identifier { "architectureGroupLabelPadding" } },
                { requirementPadding,              juce::Identifier { "requirementPadding" } },
                { requirementEdgeDashLength,       juce::Identifier { "requirementEdgeDashLength" } },
                { requirementRankSeparation,       juce::Identifier { "requirementRankSeparation" } },
                { flowchartSubroutineBarInset,     juce::Identifier { "flowchartSubroutineBarInset" } },
                { ganttDayWidth,                   juce::Identifier { "ganttDayWidth" } },
                { ganttBarHeight,                  juce::Identifier { "ganttBarHeight" } },
                { ganttBarGap,                     juce::Identifier { "ganttBarGap" } },
                { ganttTopPadding,                 juce::Identifier { "ganttTopPadding" } },
                { ganttLeftPadding,                juce::Identifier { "ganttLeftPadding" } },
                { ganttRightPadding,               juce::Identifier { "ganttRightPadding" } },
                { ganttGridLineStartPadding,       juce::Identifier { "ganttGridLineStartPadding" } },
                { ganttTitleTopMargin,             juce::Identifier { "ganttTitleTopMargin" } },
                { ganttFontSize,                   juce::Identifier { "ganttFontSize" } },
                { ganttSectionFontSize,            juce::Identifier { "ganttSectionFontSize" } },
                { ganttAxisFontSize,               juce::Identifier { "ganttAxisFontSize" } },
                { ganttTaskStrokeWidth,            juce::Identifier { "ganttTaskStrokeWidth" } },
                { ganttGridStrokeWidth,            juce::Identifier { "ganttGridStrokeWidth" } },
                { ganttSectionLabelPadding,        juce::Identifier { "ganttSectionLabelPadding" } },
                { ganttTaskLabelPadding,           juce::Identifier { "ganttTaskLabelPadding" } },
                { railroadPadding,                 juce::Identifier { "railroadPadding" } },
                { railroadVerticalSeparation,      juce::Identifier { "railroadVerticalSeparation" } },
                { railroadHorizontalSeparation,    juce::Identifier { "railroadHorizontalSeparation" } },
                { railroadArcRadius,               juce::Identifier { "railroadArcRadius" } },
                { railroadFontSize,                juce::Identifier { "railroadFontSize" } },
                { railroadStrokeWidth,             juce::Identifier { "railroadStrokeWidth" } },
                { railroadMarkerRadius,            juce::Identifier { "railroadMarkerRadius" } },
                { railroadBoxCornerRadius,         juce::Identifier { "railroadBoxCornerRadius" } },
                { railroadRuleNameGap,             juce::Identifier { "railroadRuleNameGap" } },
                { railroadMarkerGap,               juce::Identifier { "railroadMarkerGap" } },
                { railroadBaselineMinimum,         juce::Identifier { "railroadBaselineMinimum" } },
                { railroadDashLength,              juce::Identifier { "railroadDashLength" } },
                { railroadLineWidth,               juce::Identifier { "railroadLineWidth" } },
        } } {}

        enum value : int
        {
            stateFontSize                   = 0,
            lineHeight                      = 1,
            stateNodeStrokeWidth            = 2,
            stateEdgeStrokeWidth            = 3,
            stateClusterStrokeWidth         = 4,
            stateCornerRadius               = 5,
            stateTerminalSize               = 6,
            stateTerminalInnerSize          = 7,
            stateTerminalStrokeWidth        = 8,
            arrowheadWidth                  = 9,
            arrowheadLength                 = 10,
            nodeSeparation                  = 11,
            rankSeparation                  = 12,
            canvasMargin                    = 13,
            groupPadding                    = 14,
            nodePadding                     = 15,
            groupTitleHeight                = 16,
            edgeLabelPadding                = 17,
            flowchartForkJoinCornerRadius   = 18,
            rectangleCornerRadius           = 19,
            actorBoxCornerRadius            = 20,
            roundRectCornerRadius           = 21,
            mindmapDefaultCornerRadius      = 22,
            flowchartCircleEmptyMinSize     = 23,
            flowchartDoubleCircleInnerInset = 24,
            erEntityPadding                 = 25,
            erMinEntityWidth                = 26,
            erMinEntityHeight               = 27,
            erNodeSeparation                = 28,
            erRankSeparation                = 29,
            erEdgeStrokeWidth               = 30,
            erEdgeDashLength                = 31,
            erMarkerBarHeight               = 32,
            erMarkerBarSpacing              = 33,
            erMarkerFootSpread              = 34,
            erMarkerFootLength              = 35,
            erMarkerCircleRadius            = 36,
            sequenceActorMargin             = 37,
            sequenceActorWidth              = 38,
            sequenceActorHeight             = 39,
            sequenceMessageMargin           = 40,
            sequenceBoxMargin               = 41,
            sequenceNoteMargin              = 42,
            sequenceActivationWidth         = 43,
            sequenceLifelineStrokeWidth     = 44,
            sequenceMessageStrokeWidth      = 45,
            sequenceMessageDashLength       = 46,
            classPadding                    = 47,
            classTitleFontSize              = 48,
            classBodyFontSize               = 49,
            classDividerStrokeWidth         = 50,
            classEdgeStrokeWidth            = 51,
            classEdgeDashLength             = 52,
            classCardinalityFontSize        = 53,
            classCardinalityDistance        = 54,
            classCardinalityOffset          = 55,
            classExtensionLength            = 56,
            classExtensionWidth             = 57,
            classDiamondLength              = 58,
            classDiamondWidth               = 59,
            classDependencyLength           = 60,
            classDependencyWidth            = 61,
            classLollipopRadius             = 62,
            c4ShapeWidth                    = 63,
            c4ShapeHeight                   = 64,
            c4ShapePadding                  = 65,
            c4ShapeMargin                   = 66,
            c4DiagramMarginX                = 67,
            c4DiagramMarginY                = 68,
            c4ElementFontSize               = 69,
            c4StereotypeFontSize            = 70,
            c4DescriptionFontSize           = 71,
            c4SectionGap                    = 72,
            c4BoundaryLabelFontSize         = 73,
            c4BoundaryTypeFontSize          = 74,
            c4BoundaryDescrFontSize         = 75,
            c4BoundaryLabelOffset           = 76,
            c4BoundaryTypeOffset            = 77,
            c4BoundaryDescrOffset           = 78,
            c4MessageFontSize               = 79,
            c4BoundaryDashLength            = 80,
            c4BoundaryCornerRadius          = 81,
            c4ShapeCornerRadius             = 82,
            c4ShapeStrokeWidth              = 83,
            c4BoundaryStrokeWidth           = 84,
            c4EdgeStrokeWidth               = 85,
            architectureIconSize            = 86,
            architecturePadding             = 87,
            architectureFontSize            = 88,
            architectureEdgeStrokeWidth     = 89,
            architectureEdgeLength          = 90,
            architectureGroupStrokeWidth    = 92,
            architectureGroupDashLength     = 93,
            architectureGroupIconSize       = 94,
            architectureLabelWidth          = 95,
            architectureGroupLabelPadding   = 96,
            requirementPadding              = 97,
            requirementEdgeDashLength       = 98,
            requirementRankSeparation       = 99,
            flowchartSubroutineBarInset     = 100,
            ganttDayWidth                   = 101,
            ganttBarHeight                  = 102,
            ganttBarGap                     = 103,
            ganttTopPadding                 = 104,
            ganttLeftPadding                = 105,
            ganttRightPadding               = 106,
            ganttGridLineStartPadding       = 107,
            ganttTitleTopMargin             = 108,
            ganttFontSize                   = 109,
            ganttSectionFontSize            = 110,
            ganttAxisFontSize               = 111,
            ganttTaskStrokeWidth            = 112,
            ganttGridStrokeWidth            = 113,
            ganttSectionLabelPadding        = 114,
            ganttTaskLabelPadding           = 115,
            railroadPadding                 = 116,
            railroadVerticalSeparation      = 117,
            railroadHorizontalSeparation    = 118,
            railroadArcRadius               = 119,
            railroadFontSize                = 120,
            railroadStrokeWidth             = 121,
            railroadMarkerRadius            = 122,
            railroadBoxCornerRadius         = 123,
            railroadRuleNameGap             = 124,
            railroadMarkerGap               = 125,
            railroadBaselineMinimum         = 126,
            railroadDashLength              = 127,
            railroadLineWidth               = 128,
        };

        static StyleSheet* getInstance() noexcept
        {
            return jam::SharedInstance<StyleSheet>::getInstance();
        }
    };

    //==============================================================================

    /**
     * @brief Mermaid operator alphabet — delimiters, messages, and relation tokens.
     *
     * Maps every multi-character operator the parser recognises to its key,
     * from comment and class delimiters through flowchart/class/ER relation
     * arrows to sequence-message and state-stereotype markers. The key is
     * what the `Shape`, `Class`, `ER`, and `Sequence` tables use as their own
     * lookup keys when converting an operator into a render decision.
     */
    struct Operator : public jam::Bimap<int>
    {
        Operator() : jam::Bimap<int> { {
                { commentOpen,            juce::String::fromUTF8 ("%%") },
                { commentClose,           juce::String::fromUTF8 ("\n") },
                { classDef,               juce::String::fromUTF8 (":::") },
                { attributeBlock,         juce::String::fromUTF8 ("@{") },
                { arrow,                  juce::String::fromUTF8 ("<-->") },
                { thickForward,           juce::String::fromUTF8 ("==>") },
                { dottedForward,          juce::String::fromUTF8 ("-.->") },
                { forward,                juce::String::fromUTF8 ("-->") },
                { thickLink,              juce::String::fromUTF8 ("===") },
                { dottedLink,             juce::String::fromUTF8 ("-.-") },
                { link,                   juce::String::fromUTF8 ("---") },
                { thick,                  juce::String::fromUTF8 ("==") },
                { dotted,                 juce::String::fromUTF8 ("-.") },
                { invisible,              juce::String::fromUTF8 ("~~~") },
                { doubleOpenBrace,        juce::String::fromUTF8 ("{{") },
                { doubleCloseBrace,       juce::String::fromUTF8 ("}}") },
                { doubleOpenBracket,      juce::String::fromUTF8 ("[[") },
                { doubleCloseBracket,     juce::String::fromUTF8 ("]]") },
                { doubleOpenParen,        juce::String::fromUTF8 ("((") },
                { doubleCloseParen,       juce::String::fromUTF8 ("))") },
                { openBracketSlash,       juce::String::fromUTF8 ("[/") },
                { slashCloseBracket,      juce::String::fromUTF8 ("/]") },
                { openBracketBackslash,   juce::String::fromUTF8 ("[\\") },
                { backslashCloseBracket,  juce::String::fromUTF8 ("\\]") },
                { openBracketOpenParen,   juce::String::fromUTF8 ("[(") },
                { closeParenCloseBracket, juce::String::fromUTF8 (")]") },
                { tripleOpenParen,        juce::String::fromUTF8 ("(((") },
                { tripleCloseParen,       juce::String::fromUTF8 (")))") },
                { openParenOpenBracket,   juce::String::fromUTF8 ("([") },
                { closeBracketCloseParen, juce::String::fromUTF8 ("])") },
                { openBrace,              juce::String::fromUTF8 ("{") },
                { closeBrace,             juce::String::fromUTF8 ("}") },
                { openBracket,            juce::String::fromUTF8 ("[") },
                { closeBracket,           juce::String::fromUTF8 ("]") },
                { openParen,              juce::String::fromUTF8 ("(") },
                { closeParen,             juce::String::fromUTF8 (")") },
                { pipe,                   juce::String::fromUTF8 ("|") },
                { colon,                  juce::String::fromUTF8 (":") },
                { comma,                  juce::String::fromUTF8 (",") },
                { semicolon,              juce::String::fromUTF8 (";") },
                { at,                     juce::String::fromUTF8 ("@") },
                { hash,                   juce::String::fromUTF8 ("#") },
                { percent,                juce::String::fromUTF8 ("%") },
                { ampersand,              juce::String::fromUTF8 ("&") },
                { plus,                   juce::String::fromUTF8 ("+") },
                { dash,                   juce::String::fromUTF8 ("-") },
                { dot,                    juce::String::fromUTF8 (".") },
                { equals,                 juce::String::fromUTF8 ("=") },
                { slash,                  juce::String::fromUTF8 ("/") },
                { backslash,              juce::String::fromUTF8 ("\\") },
                { lessThan,               juce::String::fromUTF8 ("<") },
                { greaterThan,            juce::String::fromUTF8 (">") },
                { tilde,                  juce::String::fromUTF8 ("~") },
                { asterisk,               juce::String::fromUTF8 ("*") },
                { singleQuote,            juce::String::fromUTF8 ("'") },
                { doubleQuote,            juce::String::fromUTF8 ("\"") },
                { elseToken,              juce::String::fromUTF8 ("else") },
                { messageAsync,           juce::String::fromUTF8 ("->>") },
                { messageAsyncDotted,     juce::String::fromUTF8 ("-->>") },
                { messageSolid,           juce::String::fromUTF8 ("->") },
                { messageReverse,         juce::String::fromUTF8 ("<--") },
                { messageReverseSolid,    juce::String::fromUTF8 ("<-") },
                { messageCross,           juce::String::fromUTF8 ("-x") },
                { messageCrossDotted,     juce::String::fromUTF8 ("--x") },
                { classToken,             juce::String::fromUTF8 ("class") },
                { inherit,                juce::String::fromUTF8 ("<|--") },
                { inheritClose,           juce::String::fromUTF8 ("--|>") },
                { realizationOpen,        juce::String::fromUTF8 ("<|..") },
                { realizationClose,       juce::String::fromUTF8 ("..|>") },
                { composition,            juce::String::fromUTF8 ("*--") },
                { compositionClose,       juce::String::fromUTF8 ("--*") },
                { aggregation,            juce::String::fromUTF8 ("o--") },
                { aggregationClose,       juce::String::fromUTF8 ("--o") },
                { dependency,             juce::String::fromUTF8 ("..>") },
                { dependencyOpen,         juce::String::fromUTF8 ("<..") },
                { classLink,              juce::String::fromUTF8 ("--") },
                { dashedLink,             juce::String::fromUTF8 ("..") },
                { annotationOpen,         juce::String::fromUTF8 ("<<") },
                { annotationClose,        juce::String::fromUTF8 (">>") },
                { staticMarker,           juce::String::fromUTF8 ("$") },
                { state,                  juce::String::fromUTF8 ("state") },
                { choice,                 juce::String::fromUTF8 ("<<choice>>") },
                { fork,                   juce::String::fromUTF8 ("<<fork>>") },
                { join,                   juce::String::fromUTF8 ("<<join>>") },
                { erExactlyOne,           juce::String::fromUTF8 ("||") },
                { erZeroOrOne,            juce::String::fromUTF8 ("o|") },
                { erZeroOrOneAlt,         juce::String::fromUTF8 ("|o") },
                { erOneOrMore,            juce::String::fromUTF8 ("|{") },
                { erOneOrMoreAlt,         juce::String::fromUTF8 ("}|") },
                { erZeroOrMore,           juce::String::fromUTF8 ("o{") },
                { erZeroOrMoreAlt,        juce::String::fromUTF8 ("}o") },
                { requirement,            juce::String::fromUTF8 ("requirement") },
                { element,                juce::String::fromUTF8 ("element") },
                { block,                  juce::String::fromUTF8 ("block") },
                { groupModifier,          juce::String::fromUTF8 ("{group}") },
                { lollipop,               juce::String::fromUTF8 ("()--") },
                { lollipopClose,          juce::String::fromUTF8 ("--()") },
        } } {}

        enum value : int
        {
            commentOpen            = 0,
            commentClose           = 1,
            classDef               = 2,
            attributeBlock         = 3,
            arrow                  = 4,
            thickForward           = 5,
            dottedForward          = 6,
            forward                = 7,
            thickLink              = 8,
            dottedLink             = 9,
            link                   = 10,
            thick                  = 11,
            dotted                 = 12,
            invisible              = 13,
            doubleOpenBrace        = 14,
            doubleCloseBrace       = 15,
            doubleOpenBracket      = 16,
            doubleCloseBracket     = 17,
            doubleOpenParen        = 18,
            doubleCloseParen       = 19,
            openBracketSlash       = 20,
            slashCloseBracket      = 21,
            openBracketBackslash   = 22,
            backslashCloseBracket  = 23,
            openBracketOpenParen   = 24,
            closeParenCloseBracket = 25,
            tripleOpenParen        = 26,
            tripleCloseParen       = 27,
            openParenOpenBracket   = 28,
            closeBracketCloseParen = 29,
            openBrace              = 30,
            closeBrace             = 31,
            openBracket            = 32,
            closeBracket           = 33,
            openParen              = 34,
            closeParen             = 35,
            pipe                   = 36,
            colon                  = 37,
            comma                  = 38,
            semicolon              = 39,
            at                     = 40,
            hash                   = 41,
            percent                = 42,
            ampersand              = 43,
            plus                   = 44,
            dash                   = 45,
            dot                    = 46,
            equals                 = 47,
            slash                  = 48,
            backslash              = 49,
            lessThan               = 50,
            greaterThan            = 51,
            tilde                  = 52,
            asterisk               = 53,
            singleQuote            = 54,
            doubleQuote            = 55,
            elseToken              = 56,
            messageAsync           = 57,
            messageAsyncDotted     = 58,
            messageSolid           = 59,
            messageReverse         = 60,
            messageReverseSolid    = 61,
            messageCross           = 62,
            messageCrossDotted     = 63,
            classToken             = 64,
            inherit                = 65,
            inheritClose           = 66,
            realizationOpen        = 67,
            realizationClose       = 68,
            composition            = 69,
            compositionClose       = 70,
            aggregation            = 71,
            aggregationClose       = 72,
            dependency             = 73,
            dependencyOpen         = 74,
            classLink              = 75,
            dashedLink             = 76,
            annotationOpen         = 77,
            annotationClose        = 78,
            staticMarker           = 79,
            state                  = 80,
            choice                 = 81,
            fork                   = 82,
            join                   = 83,
            erExactlyOne           = 84,
            erZeroOrOne            = 85,
            erZeroOrOneAlt         = 86,
            erOneOrMore            = 87,
            erOneOrMoreAlt         = 88,
            erZeroOrMore           = 89,
            erZeroOrMoreAlt        = 90,
            requirement            = 91,
            element                = 92,
            block                  = 93,
            groupModifier          = 94,
            lollipop               = 95,
            lollipopClose          = 96,
        };

        static Operator* getInstance() noexcept
        {
            return jam::SharedInstance<Operator>::getInstance();
        }
    };

    //==============================================================================

    /**
     * @brief Mermaid lexical token kinds.
     *
     * Classifies each token the tokeniser emits — whitespace, comment, text,
     * string, number, operators, keyword, and end-of-file — so the parser can
     * dispatch on token kind without re-scanning the source text.
     */
    struct TokenType : public jam::Bimap<int>
    {
        TokenType() : jam::Bimap<int> { {
                { newline,    juce::String::fromUTF8 ("newline") },
                { whitespace, juce::String::fromUTF8 ("whitespace") },
                { comment,    juce::String::fromUTF8 ("comment") },
                { text,       juce::String::fromUTF8 ("text") },
                { string,     juce::String::fromUTF8 ("string") },
                { number,     juce::String::fromUTF8 ("number") },
                { operators,  juce::String::fromUTF8 ("operators") },
                { keyword,    juce::String::fromUTF8 ("keyword") },
                { endOfFile,  juce::String::fromUTF8 ("endOfFile") },
        } } {}

        enum value : int
        {
            newline    = 0,
            whitespace = 1,
            comment    = 2,
            text       = 3,
            string     = 4,
            number     = 5,
            operators  = 6,
            keyword    = 7,
            endOfFile  = 8,
        };

        static TokenType* getInstance() noexcept
        {
            return jam::SharedInstance<TokenType>::getInstance();
        }
    };

    //==============================================================================

    /**
     * @brief Byte-to-classification table for Mermaid tokenisation.
     *
     * Direct-indexes every byte the tokeniser encounters into a `map::Byte`
     * class: operator delimiter, quote, or text. Unlisted bytes fall back to
     * the row-zero default (`map::Byte::text`).
     */
    static constexpr jam::LookupTable<int, int, 256> characters {
        {
            { 0x0, map::Byte::text },
            { 0x7b, map::Byte::operators },
            { 0x7d, map::Byte::operators },
            { 0x5b, map::Byte::operators },
            { 0x5d, map::Byte::operators },
            { 0x28, map::Byte::operators },
            { 0x29, map::Byte::operators },
            { 0x7c, map::Byte::operators },
            { 0x3a, map::Byte::operators },
            { 0x3b, map::Byte::operators },
            { 0x2c, map::Byte::operators },
            { 0x40, map::Byte::operators },
            { 0x23, map::Byte::operators },
            { 0x25, map::Byte::operators },
            { 0x26, map::Byte::operators },
            { 0x2b, map::Byte::operators },
            { 0x2d, map::Byte::operators },
            { 0x2e, map::Byte::operators },
            { 0x3d, map::Byte::operators },
            { 0x2f, map::Byte::operators },
            { 0x5c, map::Byte::operators },
            { 0x3c, map::Byte::operators },
            { 0x3e, map::Byte::operators },
            { 0x7e, map::Byte::operators },
            { 0x2a, map::Byte::operators },
            { 0x24, map::Byte::operators },
            { 0x22, map::Byte::quote },
            { 0x27, map::Byte::quote },
        }
    };

    //==============================================================================

    /**
     * @brief Block-diagram link operator → (stroke, directed) packing.
     *
     * Maps each block link operator to an `EdgeStroke` plus a direction flag
     * (`true` = forward/arrow). Consumed by the block layout pass to pick the
     * line style and endpoint of a block edge.
     */
    static constexpr jam::LookupTable<int, jam::Union<uint8_t, uint8_t>, Operator::invisible + 1> blockLinks {
        {
            { Operator::dottedForward, jam::Union<uint8_t, uint8_t>::pack (EdgeStroke::dotted, true) },
            { Operator::dottedLink, jam::Union<uint8_t, uint8_t>::pack (EdgeStroke::dotted, false) },
            { Operator::thickForward, jam::Union<uint8_t, uint8_t>::pack (EdgeStroke::thick, true) },
            { Operator::thickLink, jam::Union<uint8_t, uint8_t>::pack (EdgeStroke::thick, false) },
            { Operator::forward, jam::Union<uint8_t, uint8_t>::pack (EdgeStroke::solid, true) },
            { Operator::link, jam::Union<uint8_t, uint8_t>::pack (EdgeStroke::solid, false) },
            { Operator::invisible, jam::Union<uint8_t, uint8_t>::pack (EdgeStroke::invisible, false) },
        }
    };

    //==============================================================================

    /**
     * @brief Flowchart link operator → (stroke, source-decoration, target-decoration).
     *
     * Maps each flowchart edge operator to a packed triple the draw pass uses:
     * the line's `EdgeStroke` and the `EdgeDecoration` at each endpoint. This
     * is the single place an operator becomes its visual arrow shape.
     */
    static constexpr jam::LookupTable<int, jam::Union<uint8_t, uint8_t, uint8_t>, Operator::dashedLink + 1> flowchartLinks {
        {
            { Operator::forward, jam::Union<uint8_t, uint8_t, uint8_t>::pack (EdgeStroke::solid, EdgeDecoration::none, EdgeDecoration::arrow) },
            { Operator::thickForward, jam::Union<uint8_t, uint8_t, uint8_t>::pack (EdgeStroke::thick, EdgeDecoration::none, EdgeDecoration::arrow) },
            { Operator::dottedForward, jam::Union<uint8_t, uint8_t, uint8_t>::pack (EdgeStroke::dotted, EdgeDecoration::none, EdgeDecoration::arrow) },
            { Operator::link, jam::Union<uint8_t, uint8_t, uint8_t>::pack (EdgeStroke::solid, EdgeDecoration::none, EdgeDecoration::none) },
            { Operator::thickLink, jam::Union<uint8_t, uint8_t, uint8_t>::pack (EdgeStroke::thick, EdgeDecoration::none, EdgeDecoration::none) },
            { Operator::dottedLink, jam::Union<uint8_t, uint8_t, uint8_t>::pack (EdgeStroke::dotted, EdgeDecoration::none, EdgeDecoration::none) },
            { Operator::thick, jam::Union<uint8_t, uint8_t, uint8_t>::pack (EdgeStroke::thick, EdgeDecoration::none, EdgeDecoration::none) },
            { Operator::dotted, jam::Union<uint8_t, uint8_t, uint8_t>::pack (EdgeStroke::dotted, EdgeDecoration::none, EdgeDecoration::none) },
            { Operator::classLink, jam::Union<uint8_t, uint8_t, uint8_t>::pack (EdgeStroke::solid, EdgeDecoration::none, EdgeDecoration::none) },
            { Operator::dashedLink, jam::Union<uint8_t, uint8_t, uint8_t>::pack (EdgeStroke::dotted, EdgeDecoration::none, EdgeDecoration::none) },
            { Operator::invisible, jam::Union<uint8_t, uint8_t, uint8_t>::pack (EdgeStroke::invisible, EdgeDecoration::none, EdgeDecoration::none) },
            { Operator::arrow, jam::Union<uint8_t, uint8_t, uint8_t>::pack (EdgeStroke::solid, EdgeDecoration::arrow, EdgeDecoration::arrow) },
            { Operator::aggregation, jam::Union<uint8_t, uint8_t, uint8_t>::pack (EdgeStroke::solid, EdgeDecoration::circle, EdgeDecoration::none) },
            { Operator::aggregationClose, jam::Union<uint8_t, uint8_t, uint8_t>::pack (EdgeStroke::solid, EdgeDecoration::none, EdgeDecoration::circle) },
            { Operator::messageCross, jam::Union<uint8_t, uint8_t, uint8_t>::pack (EdgeStroke::solid, EdgeDecoration::none, EdgeDecoration::cross) },
            { Operator::messageCrossDotted, jam::Union<uint8_t, uint8_t, uint8_t>::pack (EdgeStroke::solid, EdgeDecoration::none, EdgeDecoration::cross) },
            { Operator::messageReverse, jam::Union<uint8_t, uint8_t, uint8_t>::pack (EdgeStroke::solid, EdgeDecoration::arrow, EdgeDecoration::none) },
            { Operator::messageReverseSolid, jam::Union<uint8_t, uint8_t, uint8_t>::pack (EdgeStroke::solid, EdgeDecoration::arrow, EdgeDecoration::none) },
        }
    };

    //==============================================================================

    /**
     * @brief Cluster concept ordinals — one member per grouping kind.
     *
     * The unified cluster tag. Every grouping element in the sealed AST is
     * `Id::g` and carries an `Id::group` property stamped here at creation, so
     * consumers read the concept unconditionally instead of probing a tag.
     * Each member is owned by one diagram family: `subgraph` (flowchart),
     * `composite` (state `state X {}`, also the retag of a referenced node),
     * `region` (state `--` concurrency separator, anonymous), `boundary` (C4),
     * `group` (architecture), `journeySection`, `ganttSection`, `column`
     * (kanban), `branch` (git), `frame` (sequence alt/opt/loop/par/critical/
     * break/rect), `box` (sequence), `rule` (railroad), `blockComposite`,
     * `classNamespace`.
     */
    struct GroupType : public jam::Bimap<int>
    {
        GroupType() : jam::Bimap<int> { {
                { subgraph,       juce::String::fromUTF8 ("subgraph") },
                { composite,      juce::String::fromUTF8 ("composite") },
                { region,         juce::String::fromUTF8 ("region") },
                { boundary,       juce::String::fromUTF8 ("boundary") },
                { group,          juce::String::fromUTF8 ("group") },
                { journeySection, juce::String::fromUTF8 ("journeySection") },
                { ganttSection,   juce::String::fromUTF8 ("ganttSection") },
                { column,         juce::String::fromUTF8 ("column") },
                { branch,         juce::String::fromUTF8 ("branch") },
                { frame,          juce::String::fromUTF8 ("frame") },
                { box,            juce::String::fromUTF8 ("box") },
                { rule,           juce::String::fromUTF8 ("rule") },
                { blockComposite, juce::String::fromUTF8 ("blockComposite") },
                { classNamespace, juce::String::fromUTF8 ("classNamespace") },
        } } {}

        enum value : int
        {
            subgraph       = 0,
            composite      = 1,
            region         = 2,
            boundary       = 3,
            group          = 4,
            journeySection = 5,
            ganttSection   = 6,
            column         = 7,
            branch         = 8,
            frame          = 9,
            box            = 10,
            rule           = 11,
            blockComposite = 12,
            classNamespace = 13,
        };

        static GroupType* getInstance() noexcept
        {
            return jam::SharedInstance<GroupType>::getInstance();
        }
    };

    //==============================================================================

    /**
     * @brief Cluster layout-ownership policy — who positions and draws a group.
     *
     * `layered` groups are positioned and drawn by the generic layered
     * machinery (`MermaidLayout`'s setGroupLayout/setBounds/setIndex/translate
     * and `MermaidGraphics::addCluster`). `dedicated` groups are owned by their
     * own diagram family: the family pass positions them, and the layered
     * machinery only recurses through them. Consulted as routing data via
     * `Mermaid::Group::layout`; the one predicate is
     * `MermaidLayout::isLayered`.
     */
    struct GroupLayoutPolicy : public jam::Bimap<int>
    {
        GroupLayoutPolicy() : jam::Bimap<int> { {
                { layered,   juce::String::fromUTF8 ("layered") },
                { dedicated, juce::String::fromUTF8 ("dedicated") },
        } } {}

        enum value : int
        {
            layered   = 0,
            dedicated = 1,
        };

        static GroupLayoutPolicy* getInstance() noexcept
        {
            return jam::SharedInstance<GroupLayoutPolicy>::getInstance();
        }
    };

    //==============================================================================

    /**
     * @brief Node-shape geometry — construction, hit-test, and sizing policies.
     *
     * Groups the policy bimaps and the tables that map a `NodeShape` key into
     * the full recipe the draw pass needs: how to build the outline
     * (`ConstructionPolicy`), how to hit-test it (`HitTestClassification`),
     * how to adjust its size (`SizeAdjustPolicy` / `MindmapSizePolicy` /
     * `BlockSizePolicy`), plus the delimiter, corner-radius, padding, and
     * geometry tables keyed by shape.
     */
    struct Shape
    {
            using GeometryValue =
                jam::Union<jam::Union<uint8_t, uint8_t, uint8_t, uint8_t>, jam::Union<uint8_t, uint8_t>>;

        /**
         * @brief Outline construction policy for a node shape.
         *
         * Selects which geometry builder the draw pass runs — polygon,
         * rounded rectangle, stadium, ellipse, double ellipse, cylinder,
         * subroutine, cloud, bang, fork/join, silhouette, or none.
         */
        struct ConstructionPolicy : public jam::Bimap<int>
        {
            ConstructionPolicy() : jam::Bimap<int> { {
                    { polygon,          juce::String::fromUTF8 ("polygon") },
                    { roundedRectangle, juce::String::fromUTF8 ("roundedRectangle") },
                    { stadium,          juce::String::fromUTF8 ("stadium") },
                    { ellipse,          juce::String::fromUTF8 ("ellipse") },
                    { doubleEllipse,    juce::String::fromUTF8 ("doubleEllipse") },
                    { cylinder,         juce::String::fromUTF8 ("cylinder") },
                    { subroutine,       juce::String::fromUTF8 ("subroutine") },
                    { cloud,            juce::String::fromUTF8 ("cloud") },
                    { bang,             juce::String::fromUTF8 ("bang") },
                    { forkJoin,         juce::String::fromUTF8 ("forkJoin") },
                    { silhouette,       juce::String::fromUTF8 ("silhouette") },
                    { none,             juce::String::fromUTF8 ("none") },
            } } {}

            enum value : int
            {
                polygon          = 0,
                roundedRectangle = 1,
                stadium          = 2,
                ellipse          = 3,
                doubleEllipse    = 4,
                cylinder         = 5,
                subroutine       = 6,
                cloud            = 7,
                bang             = 8,
                forkJoin         = 9,
                silhouette       = 10,
                none             = 11,
            };

            static ConstructionPolicy* getInstance() noexcept
            {
                return jam::SharedInstance<ConstructionPolicy>::getInstance();
            }
        };

        //==============================================================================

        /**
         * @brief Hit-test classification for a node shape.
         *
         * Tells the hit-test pass which geometric predicate to use — bounding
         * box, ellipse containment, or polygon point-in-polygon.
         */
        struct HitTestClassification : public jam::Bimap<int>
        {
            HitTestClassification() : jam::Bimap<int> { {
                    { boundingBox, juce::String::fromUTF8 ("boundingBox") },
                    { ellipse,     juce::String::fromUTF8 ("ellipse") },
                    { polygon,     juce::String::fromUTF8 ("polygon") },
            } } {}

            enum value : int
            {
                boundingBox = 0,
                ellipse     = 1,
                polygon     = 2,
            };

            static HitTestClassification* getInstance() noexcept
            {
                return jam::SharedInstance<HitTestClassification>::getInstance();
            }
        };

        //==============================================================================

        /**
         * @brief Size-adjust policy for a node shape.
         *
         * Selects how the layout pass adapts a shape's bounds to its label —
         * diamond scaling, fork/join clamping, circle conditioning, or the
         * width/scale-only adjustments for rounded rectangles, cylinders,
         * hexagons, and trapezoids. `circleConditional` is the policy the
         * terminal marker glyphs (`terminalDot`/`terminalRing`) carry: a text
         * label drives the size, and an empty label collapses to the metric
         * floor.
         */
        struct SizeAdjustPolicy : public jam::Bimap<int>
        {
            SizeAdjustPolicy() : jam::Bimap<int> { {
                    { none,               juce::String::fromUTF8 ("none") },
                    { diamondScale,       juce::String::fromUTF8 ("diamondScale") },
                    { forkJoinClamp,      juce::String::fromUTF8 ("forkJoinClamp") },
                    { circleConditional,  juce::String::fromUTF8 ("circleConditional") },
                    { roundRectScale,     juce::String::fromUTF8 ("roundRectScale") },
                    { cylinderScale,      juce::String::fromUTF8 ("cylinderScale") },
                    { hexagonScale,       juce::String::fromUTF8 ("hexagonScale") },
                    { trapezoidWidthOnly, juce::String::fromUTF8 ("trapezoidWidthOnly") },
            } } {}

            enum value : int
            {
                none               = 0,
                diamondScale       = 1,
                forkJoinClamp      = 2,
                circleConditional  = 3,
                roundRectScale     = 4,
                cylinderScale      = 5,
                hexagonScale       = 6,
                trapezoidWidthOnly = 7,
            };

            static SizeAdjustPolicy* getInstance() noexcept
            {
                return jam::SharedInstance<SizeAdjustPolicy>::getInstance();
            }
        };

        //==============================================================================

        /**
         * @brief Mindmap-specific size policy for a node shape.
         *
         * Selects the mindmap sizing behaviour — none, rectangle, rounded,
         * circular, or hexagon — used when a node is laid out inside a mindmap.
         */
        struct MindmapSizePolicy : public jam::Bimap<int>
        {
            MindmapSizePolicy() : jam::Bimap<int> { {
                    { none,      juce::String::fromUTF8 ("none") },
                    { rectangle, juce::String::fromUTF8 ("rectangle") },
                    { rounded,   juce::String::fromUTF8 ("rounded") },
                    { circular,  juce::String::fromUTF8 ("circular") },
                    { hexagon,   juce::String::fromUTF8 ("hexagon") },
            } } {}

            enum value : int
            {
                none      = 0,
                rectangle = 1,
                rounded   = 2,
                circular  = 3,
                hexagon   = 4,
            };

            static MindmapSizePolicy* getInstance() noexcept
            {
                return jam::SharedInstance<MindmapSizePolicy>::getInstance();
            }
        };

        //==============================================================================

        /**
         * @brief Block-diagram size policy for a node shape.
         *
         * Selects the block sizing behaviour — none, circular, or hexagon —
         * used when a node is laid out inside a block diagram.
         */
        struct BlockSizePolicy : public jam::Bimap<int>
        {
            BlockSizePolicy() : jam::Bimap<int> { {
                    { none,     juce::String::fromUTF8 ("none") },
                    { circular, juce::String::fromUTF8 ("circular") },
                    { hexagon,  juce::String::fromUTF8 ("hexagon") },
            } } {}

            enum value : int
            {
                none     = 0,
                circular = 1,
                hexagon  = 2,
            };

            static BlockSizePolicy* getInstance() noexcept
            {
                return jam::SharedInstance<BlockSizePolicy>::getInstance();
            }
        };

        //==============================================================================

        /**
         * @brief Delimiter pair → node shape mapping.
         *
         * Maps each node's bracketing delimiters (the opening and closing
         * `Operator` tokens) to the `NodeShape` key they denote. This is the
         * table the parser consults to turn a bracketed node literal into its
         * shape discriminator.
         */
        static constexpr jam::LookupTable<int, jam::Union<uint8_t, uint8_t, uint8_t>, 13> shapes {
            {
                { 0, jam::Union<uint8_t, uint8_t, uint8_t>::pack (Operator::openBracketSlash, Operator::slashCloseBracket, NodeShape::parallelogram) },
                { 1, jam::Union<uint8_t, uint8_t, uint8_t>::pack (Operator::openBracketBackslash, Operator::backslashCloseBracket, NodeShape::parallelogramAlt) },
                { 2, jam::Union<uint8_t, uint8_t, uint8_t>::pack (Operator::openBracketSlash, Operator::backslashCloseBracket, NodeShape::trapezoid) },
                { 3, jam::Union<uint8_t, uint8_t, uint8_t>::pack (Operator::openBracketBackslash, Operator::slashCloseBracket, NodeShape::trapezoidAlt) },
                { 4, jam::Union<uint8_t, uint8_t, uint8_t>::pack (Operator::doubleOpenBracket, Operator::doubleCloseBracket, NodeShape::subroutine) },
                { 5, jam::Union<uint8_t, uint8_t, uint8_t>::pack (Operator::openBracketOpenParen, Operator::closeParenCloseBracket, NodeShape::cylinder) },
                { 6, jam::Union<uint8_t, uint8_t, uint8_t>::pack (Operator::tripleOpenParen, Operator::tripleCloseParen, NodeShape::doubleCircle) },
                { 7, jam::Union<uint8_t, uint8_t, uint8_t>::pack (Operator::doubleOpenParen, Operator::doubleCloseParen, NodeShape::circle) },
                { 8, jam::Union<uint8_t, uint8_t, uint8_t>::pack (Operator::openParenOpenBracket, Operator::closeBracketCloseParen, NodeShape::stadium) },
                { 9, jam::Union<uint8_t, uint8_t, uint8_t>::pack (Operator::doubleOpenBrace, Operator::doubleCloseBrace, NodeShape::hexagon) },
                { 10, jam::Union<uint8_t, uint8_t, uint8_t>::pack (Operator::openBracket, Operator::closeBracket, NodeShape::rectangle) },
                { 11, jam::Union<uint8_t, uint8_t, uint8_t>::pack (Operator::openParen, Operator::closeParen, NodeShape::roundRect) },
                { 12, jam::Union<uint8_t, uint8_t, uint8_t>::pack (Operator::openBrace, Operator::closeBrace, NodeShape::diamond) },
            }
        };

        //==============================================================================

        /**
         * @brief Node shape → corner-radius stylesheet key.
         *
         * Maps the shapes that carry a rounded corner to the `StyleSheet`
         * metric key holding their corner radius; shapes without an entry use
         * no corner radius.
         */
        static constexpr jam::LookupTable<int, int, 28> cornerRadii {
            {
                { NodeShape::rectangle, StyleSheet::rectangleCornerRadius },
                { NodeShape::roundRect, StyleSheet::roundRectCornerRadius },
                { NodeShape::actorBox, StyleSheet::actorBoxCornerRadius },
                { NodeShape::mindmapDefault, StyleSheet::mindmapDefaultCornerRadius },
            }
        };

        //==============================================================================

        /**
         * @brief Node shape → svgPaths ordinal.
         *
         * Maps the shapes whose outline is static svgPaths data; currently the
         * C4 person silhouette. The first row pins the table default to the
         * empty path.
         */
        static constexpr jam::LookupTable<int, int, 28> pathIds {
            {
                { NodeShape::rectangle, 0 },
                { NodeShape::person, 14 },
            }
        };

        //==============================================================================

        /**
         * @brief Node shape → (horizontal, vertical) padding.
         *
         * Dispatch data the layout pass reads to size a shape around its text:
         * each `NodeShape` maps to its packed horizontal/vertical label
         * padding. Every entry is present, including the terminal marker
         * glyphs, so a shape lookup never needs a fallback branch.
         */
        static constexpr jam::LookupTable<int, jam::Union<float, float>, 28> paddings {
            {
                { NodeShape::rectangle, jam::Union<float, float>::pack (1.0f, 1.0f) },
                { NodeShape::forkJoin, jam::Union<float, float>::pack (1.0f, 1.0f) },
                { NodeShape::roundRect, jam::Union<float, float>::pack (1.0f, 1.0f) },
                { NodeShape::stadium, jam::Union<float, float>::pack (0.43f, 0.5f) },
                { NodeShape::subroutine, jam::Union<float, float>::pack (0.54f, 0.5f) },
                { NodeShape::cylinder, jam::Union<float, float>::pack (1.0f, 1.0f) },
                { NodeShape::actorBox, jam::Union<float, float>::pack (1.0f, 1.0f) },
                { NodeShape::circle, jam::Union<float, float>::pack (1.0f, 1.0f) },
                { NodeShape::doubleCircle, jam::Union<float, float>::pack (1.0f, 1.0f) },
                { NodeShape::diamond, jam::Union<float, float>::pack (1.0f, 1.0f) },
                { NodeShape::hexagon, jam::Union<float, float>::pack (1.0f, 1.0f) },
                { NodeShape::parallelogram, jam::Union<float, float>::pack (0.894f, 0.5f) },
                { NodeShape::parallelogramAlt, jam::Union<float, float>::pack (0.904f, 0.5f) },
                { NodeShape::trapezoid, jam::Union<float, float>::pack (1.0f, 1.0f) },
                { NodeShape::trapezoidAlt, jam::Union<float, float>::pack (1.0f, 1.0f) },
                { NodeShape::asymmetric, jam::Union<float, float>::pack (1.0f, 1.0f) },
                { NodeShape::mindmapDefault, jam::Union<float, float>::pack (1.0f, 1.0f) },
                { NodeShape::bang, jam::Union<float, float>::pack (1.0f, 1.0f) },
                { NodeShape::cloud, jam::Union<float, float>::pack (1.0f, 1.0f) },
                { NodeShape::text, jam::Union<float, float>::pack (1.0f, 1.0f) },
                { NodeShape::space, jam::Union<float, float>::pack (1.0f, 1.0f) },
                { NodeShape::blockArrow, jam::Union<float, float>::pack (1.0f, 1.0f) },
                { NodeShape::triangle, jam::Union<float, float>::pack (1.0f, 1.0f) },
                { NodeShape::cross, jam::Union<float, float>::pack (1.0f, 1.0f) },
                { NodeShape::barb, jam::Union<float, float>::pack (1.0f, 1.0f) },
                { NodeShape::terminalDot, jam::Union<float, float>::pack (1.0f, 1.0f) },
                { NodeShape::terminalRing, jam::Union<float, float>::pack (1.0f, 1.0f) },
                { NodeShape::person, jam::Union<float, float>::pack (1.0f, 1.0f) },
            }
        };

        //==============================================================================

        /**
         * @brief Node shape → full geometry recipe.
         *
         * The single dispatch table the layout and draw passes read to resolve
         * a `NodeShape` into the packed recipe: construction policy, hit-test
         * classification, size-adjust policy, mindmap size policy, block size
         * policy, and polygon side count. The terminal marker glyphs
         * (`terminalDot`/`terminalRing`) carry an ellipse/double-ellipse
         * construction with `circleConditional` sizing and no mindmap/block
         * policy.
         */
        static constexpr jam::LookupTable<int, GeometryValue, 28> geometry {
            {
                { NodeShape::rectangle,
                     GeometryValue::pack (
                         jam::Union<uint8_t, uint8_t, uint8_t, uint8_t>::pack (
                             ConstructionPolicy::roundedRectangle,
                             HitTestClassification::boundingBox,
                             SizeAdjustPolicy::none,
                             MindmapSizePolicy::rectangle),
                         jam::Union<uint8_t, uint8_t>::pack (BlockSizePolicy::none, 0)) },
                { NodeShape::forkJoin,
                     GeometryValue::pack (
                         jam::Union<uint8_t, uint8_t, uint8_t, uint8_t>::pack (
                             ConstructionPolicy::forkJoin,
                             HitTestClassification::boundingBox,
                             SizeAdjustPolicy::forkJoinClamp,
                             MindmapSizePolicy::none),
                         jam::Union<uint8_t, uint8_t>::pack (BlockSizePolicy::none, 0)) },
                { NodeShape::roundRect,
                     GeometryValue::pack (
                         jam::Union<uint8_t, uint8_t, uint8_t, uint8_t>::pack (
                             ConstructionPolicy::roundedRectangle,
                             HitTestClassification::boundingBox,
                             SizeAdjustPolicy::roundRectScale,
                             MindmapSizePolicy::rounded),
                         jam::Union<uint8_t, uint8_t>::pack (BlockSizePolicy::none, 0)) },
                { NodeShape::stadium,
                     GeometryValue::pack (
                         jam::Union<uint8_t, uint8_t, uint8_t, uint8_t>::pack (
                             ConstructionPolicy::stadium,
                             HitTestClassification::boundingBox,
                             SizeAdjustPolicy::none,
                             MindmapSizePolicy::none),
                         jam::Union<uint8_t, uint8_t>::pack (BlockSizePolicy::none, 0)) },
                { NodeShape::subroutine,
                     GeometryValue::pack (
                         jam::Union<uint8_t, uint8_t, uint8_t, uint8_t>::pack (
                             ConstructionPolicy::subroutine,
                             HitTestClassification::boundingBox,
                             SizeAdjustPolicy::none,
                             MindmapSizePolicy::none),
                         jam::Union<uint8_t, uint8_t>::pack (BlockSizePolicy::none, 0)) },
                { NodeShape::cylinder,
                     GeometryValue::pack (
                         jam::Union<uint8_t, uint8_t, uint8_t, uint8_t>::pack (
                             ConstructionPolicy::cylinder,
                             HitTestClassification::boundingBox,
                             SizeAdjustPolicy::cylinderScale,
                             MindmapSizePolicy::none),
                         jam::Union<uint8_t, uint8_t>::pack (BlockSizePolicy::none, 0)) },
                { NodeShape::actorBox,
                     GeometryValue::pack (
                         jam::Union<uint8_t, uint8_t, uint8_t, uint8_t>::pack (
                             ConstructionPolicy::roundedRectangle,
                             HitTestClassification::boundingBox,
                             SizeAdjustPolicy::none,
                             MindmapSizePolicy::none),
                         jam::Union<uint8_t, uint8_t>::pack (BlockSizePolicy::none, 0)) },
                { NodeShape::circle,
                     GeometryValue::pack (
                         jam::Union<uint8_t, uint8_t, uint8_t, uint8_t>::pack (
                             ConstructionPolicy::ellipse,
                             HitTestClassification::ellipse,
                             SizeAdjustPolicy::circleConditional,
                             MindmapSizePolicy::circular),
                         jam::Union<uint8_t, uint8_t>::pack (BlockSizePolicy::circular, 0)) },
                { NodeShape::doubleCircle,
                     GeometryValue::pack (
                         jam::Union<uint8_t, uint8_t, uint8_t, uint8_t>::pack (
                             ConstructionPolicy::doubleEllipse,
                             HitTestClassification::ellipse,
                             SizeAdjustPolicy::circleConditional,
                             MindmapSizePolicy::circular),
                         jam::Union<uint8_t, uint8_t>::pack (BlockSizePolicy::circular, 0)) },
                { NodeShape::diamond,
                     GeometryValue::pack (
                         jam::Union<uint8_t, uint8_t, uint8_t, uint8_t>::pack (
                             ConstructionPolicy::polygon,
                             HitTestClassification::polygon,
                             SizeAdjustPolicy::diamondScale,
                             MindmapSizePolicy::none),
                         jam::Union<uint8_t, uint8_t>::pack (BlockSizePolicy::none, 4)) },
                { NodeShape::hexagon,
                     GeometryValue::pack (
                         jam::Union<uint8_t, uint8_t, uint8_t, uint8_t>::pack (
                             ConstructionPolicy::polygon,
                             HitTestClassification::polygon,
                             SizeAdjustPolicy::hexagonScale,
                             MindmapSizePolicy::hexagon),
                         jam::Union<uint8_t, uint8_t>::pack (BlockSizePolicy::hexagon, 6)) },
                { NodeShape::parallelogram,
                     GeometryValue::pack (
                         jam::Union<uint8_t, uint8_t, uint8_t, uint8_t>::pack (
                             ConstructionPolicy::polygon,
                             HitTestClassification::polygon,
                             SizeAdjustPolicy::none,
                             MindmapSizePolicy::none),
                         jam::Union<uint8_t, uint8_t>::pack (BlockSizePolicy::none, 4)) },
                { NodeShape::parallelogramAlt,
                     GeometryValue::pack (
                         jam::Union<uint8_t, uint8_t, uint8_t, uint8_t>::pack (
                             ConstructionPolicy::polygon,
                             HitTestClassification::polygon,
                             SizeAdjustPolicy::none,
                             MindmapSizePolicy::none),
                         jam::Union<uint8_t, uint8_t>::pack (BlockSizePolicy::none, 4)) },
                { NodeShape::trapezoid,
                     GeometryValue::pack (
                         jam::Union<uint8_t, uint8_t, uint8_t, uint8_t>::pack (
                             ConstructionPolicy::polygon,
                             HitTestClassification::polygon,
                             SizeAdjustPolicy::trapezoidWidthOnly,
                             MindmapSizePolicy::none),
                         jam::Union<uint8_t, uint8_t>::pack (BlockSizePolicy::none, 4)) },
                { NodeShape::trapezoidAlt,
                     GeometryValue::pack (
                         jam::Union<uint8_t, uint8_t, uint8_t, uint8_t>::pack (
                             ConstructionPolicy::polygon,
                             HitTestClassification::polygon,
                             SizeAdjustPolicy::trapezoidWidthOnly,
                             MindmapSizePolicy::none),
                         jam::Union<uint8_t, uint8_t>::pack (BlockSizePolicy::none, 4)) },
                { NodeShape::asymmetric,
                     GeometryValue::pack (
                         jam::Union<uint8_t, uint8_t, uint8_t, uint8_t>::pack (
                             ConstructionPolicy::polygon,
                             HitTestClassification::polygon,
                             SizeAdjustPolicy::none,
                             MindmapSizePolicy::none),
                         jam::Union<uint8_t, uint8_t>::pack (BlockSizePolicy::none, 5)) },
                { NodeShape::mindmapDefault,
                     GeometryValue::pack (
                         jam::Union<uint8_t, uint8_t, uint8_t, uint8_t>::pack (
                             ConstructionPolicy::roundedRectangle,
                             HitTestClassification::boundingBox,
                             SizeAdjustPolicy::none,
                             MindmapSizePolicy::rounded),
                         jam::Union<uint8_t, uint8_t>::pack (BlockSizePolicy::none, 0)) },
                { NodeShape::bang,
                     GeometryValue::pack (
                         jam::Union<uint8_t, uint8_t, uint8_t, uint8_t>::pack (
                             ConstructionPolicy::bang,
                             HitTestClassification::boundingBox,
                             SizeAdjustPolicy::none,
                             MindmapSizePolicy::rounded),
                         jam::Union<uint8_t, uint8_t>::pack (BlockSizePolicy::none, 0)) },
                { NodeShape::cloud,
                     GeometryValue::pack (
                         jam::Union<uint8_t, uint8_t, uint8_t, uint8_t>::pack (
                             ConstructionPolicy::cloud,
                             HitTestClassification::boundingBox,
                             SizeAdjustPolicy::none,
                             MindmapSizePolicy::rounded),
                         jam::Union<uint8_t, uint8_t>::pack (BlockSizePolicy::none, 0)) },
                { NodeShape::text,
                     GeometryValue::pack (
                         jam::Union<uint8_t, uint8_t, uint8_t, uint8_t>::pack (
                             ConstructionPolicy::none,
                             HitTestClassification::boundingBox,
                             SizeAdjustPolicy::none,
                             MindmapSizePolicy::none),
                         jam::Union<uint8_t, uint8_t>::pack (BlockSizePolicy::none, 0)) },
                { NodeShape::space,
                     GeometryValue::pack (
                         jam::Union<uint8_t, uint8_t, uint8_t, uint8_t>::pack (
                             ConstructionPolicy::none,
                             HitTestClassification::boundingBox,
                             SizeAdjustPolicy::none,
                             MindmapSizePolicy::none),
                         jam::Union<uint8_t, uint8_t>::pack (BlockSizePolicy::none, 0)) },
                { NodeShape::blockArrow,
                     GeometryValue::pack (
                         jam::Union<uint8_t, uint8_t, uint8_t, uint8_t>::pack (
                             ConstructionPolicy::none,
                             HitTestClassification::boundingBox,
                             SizeAdjustPolicy::none,
                             MindmapSizePolicy::none),
                         jam::Union<uint8_t, uint8_t>::pack (BlockSizePolicy::none, 0)) },
                { NodeShape::triangle,
                     GeometryValue::pack (
                         jam::Union<uint8_t, uint8_t, uint8_t, uint8_t>::pack (
                             ConstructionPolicy::polygon,
                             HitTestClassification::polygon,
                             SizeAdjustPolicy::none,
                             MindmapSizePolicy::none),
                         jam::Union<uint8_t, uint8_t>::pack (BlockSizePolicy::none, 3)) },
                { NodeShape::cross,
                     GeometryValue::pack (
                         jam::Union<uint8_t, uint8_t, uint8_t, uint8_t>::pack (
                             ConstructionPolicy::polygon,
                             HitTestClassification::polygon,
                             SizeAdjustPolicy::none,
                             MindmapSizePolicy::none),
                         jam::Union<uint8_t, uint8_t>::pack (BlockSizePolicy::none, 4)) },
                { NodeShape::barb,
                     GeometryValue::pack (
                         jam::Union<uint8_t, uint8_t, uint8_t, uint8_t>::pack (
                             ConstructionPolicy::polygon,
                             HitTestClassification::polygon,
                             SizeAdjustPolicy::none,
                             MindmapSizePolicy::none),
                         jam::Union<uint8_t, uint8_t>::pack (BlockSizePolicy::none, 4)) },
                { NodeShape::terminalDot,
                     GeometryValue::pack (
                         jam::Union<uint8_t, uint8_t, uint8_t, uint8_t>::pack (
                             ConstructionPolicy::ellipse,
                             HitTestClassification::ellipse,
                             SizeAdjustPolicy::circleConditional,
                             MindmapSizePolicy::none),
                         jam::Union<uint8_t, uint8_t>::pack (BlockSizePolicy::none, 0)) },
                { NodeShape::terminalRing,
                     GeometryValue::pack (
                         jam::Union<uint8_t, uint8_t, uint8_t, uint8_t>::pack (
                             ConstructionPolicy::doubleEllipse,
                             HitTestClassification::ellipse,
                             SizeAdjustPolicy::circleConditional,
                             MindmapSizePolicy::none),
                         jam::Union<uint8_t, uint8_t>::pack (BlockSizePolicy::none, 0)) },
                { NodeShape::person,
                     GeometryValue::pack (
                         jam::Union<uint8_t, uint8_t, uint8_t, uint8_t>::pack (
                             ConstructionPolicy::silhouette,
                             HitTestClassification::boundingBox,
                             SizeAdjustPolicy::none,
                             MindmapSizePolicy::none),
                         jam::Union<uint8_t, uint8_t>::pack (BlockSizePolicy::none, 0)) },
            }
        };
    };

    //==============================================================================

    /**
     * @brief Cluster routing data — the `GroupType` → `GroupLayoutPolicy` map.
     *
     * `layout` is the lookup the layered machinery consults to decide whether a
     * group is laid out generically or owned by its diagram family. Layered:
     * `subgraph`, `composite`, `boundary`, `group`, `journeySection`.
     * Dedicated: `region`, `ganttSection`, `column`, `branch`, `frame`, `box`,
     * `rule`, `blockComposite`, `classNamespace`.
     */
    struct Group
    {
        static constexpr jam::LookupTable<int, int, 14> layout {
            {
                { GroupType::subgraph, GroupLayoutPolicy::layered },
                { GroupType::composite, GroupLayoutPolicy::layered },
                { GroupType::region, GroupLayoutPolicy::dedicated },
                { GroupType::boundary, GroupLayoutPolicy::layered },
                { GroupType::group, GroupLayoutPolicy::layered },
                { GroupType::journeySection, GroupLayoutPolicy::layered },
                { GroupType::ganttSection, GroupLayoutPolicy::dedicated },
                { GroupType::column, GroupLayoutPolicy::dedicated },
                { GroupType::branch, GroupLayoutPolicy::dedicated },
                { GroupType::frame, GroupLayoutPolicy::dedicated },
                { GroupType::box, GroupLayoutPolicy::dedicated },
                { GroupType::rule, GroupLayoutPolicy::dedicated },
                { GroupType::blockComposite, GroupLayoutPolicy::dedicated },
                { GroupType::classNamespace, GroupLayoutPolicy::dedicated },
            }
        };
    };

    //==============================================================================

    /**
     * @brief SVG path ordinal → d= path data.
     *
     * The one source of every static outline the draw passes render: edge
     * markers, arrowheads, the C4 person silhouette, and the architecture
     * icons. Consumers reach a row through the pathIds tables; ordinal 0 is
     * the empty path. Icon rows hold their d= data directly.
     */
    static constexpr jam::LookupTable<int, const char*, 20> svgPaths {
        {
            { 0,  "" },
            { 1,  "M 0,5 A 5,5 0 1,0 10,5 A 5,5 0 1,0 0,5" },
            { 2,  "M 1,1 l 9,9 M 10,1 l -9,9" },
            { 3,  "M 18,7 L9,13 L1,7 L9,1 Z" },
            { 4,  "M3,0 L3,18 M9,0 L9,18" },
            { 5,  "M 3,9 A 6,6 0 1,0 15,9 A 6,6 0 1,0 3,9 M21,0 L21,18" },
            { 6,  "M3,9 L3,27 M9,18 Q27,0 45,18 Q27,36 9,18" },
            { 7,  "M 3,18 A 6,6 0 1,0 15,18 A 6,6 0 1,0 3,18 M21,18 Q39,0 57,18 Q39,36 21,18" },
            { 8,  "M 1,1 V 13 L18,7 Z" },
            { 9,  "M 18,7 L9,13 L14,7 L9,1 Z" },
            { 10, "M 1,7 A 6,6 0 1,0 13,7 A 6,6 0 1,0 1,7" },
            { 11, "M 19,7 L9,13 L14,7 L9,1 Z" },
            { 12, "M 0 0 L 10 5 L 0 10 z" },
            { 13, "M 0 0 L 13.33 6.67 L 0 13.33 z" },
            { 14, "M256,511.764C203.827,511.764 102.37,504.921\n61.3,454.957C59.525,298.746 91.231,281.582 198.596,260C226.632,289.807\n284.469,288.935 313.404,260C420.769,281.582 452.475,298.746\n450.7,454.957C409.63,504.921 308.173,511.764\n256,511.764ZM256,250.793C178.951,249.32 151.833,167.393\n153.093,121.407C153.768,77.465 157.472,-2.126 256,0C354.528,-2.126\n358.232,77.465 358.907,121.407C360.167,167.393 333.049,249.32\n256,250.793Z" },
            { 15, "M53.636,27.727C44.283,27.727 36.304,33.613\n33.202,41.883L38.31,43.798C40.637,37.596 46.621,33.182\n53.636,33.182C55.541,33.182 57.369,33.507 59.069,34.105C65.436,36.345\n70,42.413 70,49.545C70,58.583 62.674,65.909\n53.636,65.909L26.364,65.909C17.326,65.909 10,58.583\n10,49.545C10,42.413 14.564,36.345 20.931,34.105C20.916,33.799\n20.909,33.491 20.909,33.182C20.909,22.638 29.456,14.091\n40,14.091C48.842,14.091 56.28,20.102 58.451,28.26C56.902,27.911\n55.29,27.727 53.636,27.727Z" },
            { 16, "M23.616,13.108C27.912,11.156 33.708,10 40,10C46.292,10 52.088,11.156\n56.384,13.108C60.5,14.98 64,17.972 64,22C64,26.028 60.5,29.02\n56.384,30.892C52.088,32.844 46.292,34 40,34C33.708,34 27.912,32.844\n23.616,30.892C19.5,29.02 16,26.028 16,22C16,17.972 19.5,14.98\n23.616,13.108ZM16,30.644C17.828,32.372 20.016,33.652\n21.96,34.532C26.888,36.772 33.272,38 40,38C46.728,38 53.112,36.772\n58.04,34.532C59.984,33.648 62.172,32.372 64,30.644L64,34C64,38.028\n60.5,41.02 56.384,42.892C52.088,44.844 46.292,46 40,46C33.708,46\n27.912,44.84 23.616,42.892C19.5,41.02 16,38.028\n16,34L16,30.644ZM16,42.644C17.828,44.372 20.016,45.652\n21.96,46.532C26.888,48.772 33.272,50 40,50C46.728,50 53.112,48.772\n58.04,46.532C59.984,45.648 62.172,44.372 64,42.644L64,46C64,50.028\n60.5,53.02 56.384,54.892C52.088,56.844 46.292,58 40,58C33.708,58\n27.912,56.844 23.616,54.892C19.5,53.02 16,50.028\n16,46L16,42.644ZM16,54.644C17.828,56.372 20.016,57.652\n21.96,58.532C26.888,60.772 33.272,62 40,62C46.728,62 53.112,60.772\n58.04,58.532C59.984,57.648 62.172,56.372 64,54.644L64,58C64,62.028\n60.5,65.02 56.384,66.892C52.088,68.844 46.292,70 40,70C33.708,70\n27.912,68.844 23.616,66.892C19.5,65.02 16,62.028 16,58L16,54.644Z" },
            { 17, "M25,10L55,10C59.114,10 62.5,13.386 62.5,17.5L62.5,62.5C62.5,66.614\n59.114,70 55,70L25,70C20.886,70 17.5,66.614\n17.5,62.5L17.5,17.5C17.5,13.386 20.886,10\n25,10ZM58.75,15.625C58.75,14.596 57.904,13.75\n56.875,13.75C55.846,13.75 55,14.596 55,15.625C55,16.654 55.846,17.5\n56.875,17.5C57.904,17.5 58.75,16.654\n58.75,15.625ZM35.807,57.539C36.411,56.931 38.222,54.299\n40.236,51.25C48.374,51.12 54.997,44.39 54.997,36.252C54.997,28.023\n48.226,21.252 39.997,21.252C31.768,21.252 24.997,28.023\n24.997,36.252C24.997,41.784 28.057,46.884 32.939,49.488C31.442,50.515\n30.332,51.325 29.958,51.693C29.183,52.468 28.747,53.52\n28.747,54.616C28.747,56.884 30.614,58.751 32.883,58.751C33.979,58.751\n35.032,58.315 35.807,57.539ZM42.944,47.11L45.265,43.514C45.801,42.67\n44.83,41.699 43.99,42.239C41.894,43.581 39.16,45.344\n36.659,46.994C31.966,45.536 28.746,41.164 28.746,36.25C28.746,30.078\n33.826,25 39.998,25C46.164,25.011 51.233,30.088\n51.233,36.254C51.233,41.312 47.823,45.778\n42.944,47.11ZM43.75,36.25C43.75,34.193 42.057,32.5 40,32.5C37.943,32.5\n36.25,34.193 36.25,36.25C36.25,38.307 37.943,40 40,40C42.057,40\n43.75,38.307 43.75,36.25ZM23.125,66.25C24.154,66.25 25,65.404\n25,64.375C25,63.346 24.154,62.5 23.125,62.5C22.096,62.5 21.25,63.346\n21.25,64.375C21.25,65.404 22.096,66.25\n23.125,66.25ZM25,15.625C25,14.596 24.154,13.75\n23.125,13.75C22.096,13.75 21.25,14.596 21.25,15.625C21.25,16.654\n22.096,17.5 23.125,17.5C24.154,17.5 25,16.654\n25,15.625ZM58.75,64.375C58.75,63.346 57.904,62.5\n56.875,62.5C55.846,62.5 55,63.346 55,64.375C55,65.404 55.846,66.25\n56.875,66.25C57.904,66.25 58.75,65.404 58.75,64.375Z" },
            { 18, "M10,40C10,23.542 23.542,10 40,10C56.458,10 70,23.542 70,40C70,56.458\n56.458,70 40,70C23.542,70 10,56.458 10,40M38.125,14.039C35.612,14.804\n33.119,17.114 31.049,20.995C30.511,22.003 30.018,23.093\n29.567,24.265C32.211,24.854 35.087,25.229\n38.125,25.341L38.125,14.039ZM25.934,23.271C26.469,21.829 27.07,20.481\n27.737,19.229C28.39,18.005 29.139,16.835 29.976,15.73C26.783,17.052\n23.883,18.992 21.441,21.438C22.801,22.13 24.299,22.743\n25.934,23.275L25.934,23.271ZM23.159,38.125C23.294,34.113 23.864,30.299\n24.794,26.845C22.763,26.198 20.798,25.361 18.925,24.344C15.938,28.353\n14.165,33.137 13.818,38.125L23.159,38.125ZM28.409,27.846C27.523,31.205\n27.02,34.653 26.909,38.125L38.125,38.125L38.125,29.091C34.712,28.979\n31.439,28.548\n28.409,27.846M41.875,29.088L41.875,38.125L53.087,38.125C52.977,34.653\n52.475,31.205 51.591,27.846C48.561,28.548 45.288,28.975\n41.875,29.091L41.875,29.088ZM26.912,41.875C27.044,45.576 27.572,49.053\n28.409,52.154C31.6,51.429 34.854,51.014\n38.125,50.913L38.125,41.875L26.912,41.875ZM41.875,41.875L41.875,50.909C45.288,51.021\n48.561,51.452 51.591,52.154C52.428,49.053 52.956,45.576\n53.091,41.875L41.875,41.875ZM29.567,55.735C30.018,56.907 30.511,57.998\n31.049,59.005C33.119,62.886 35.616,65.192\n38.125,65.961L38.125,54.663C35.087,54.775 32.211,55.15\n29.567,55.739L29.567,55.735ZM29.98,64.27C29.141,63.165 28.391,61.995\n27.738,60.771C27.048,59.463 26.446,58.112 25.934,56.725C24.391,57.221\n22.889,57.835 21.441,58.562C23.883,61.008 26.783,62.948\n29.976,64.27L29.98,64.27ZM24.794,53.155C23.821,49.469 23.272,45.685\n23.155,41.875L13.817,41.875C14.165,46.863 15.938,51.647\n18.925,55.656C20.688,54.689 22.656,53.849\n24.794,53.155M50.024,64.27C53.215,62.949 56.114,61.01\n58.555,58.566C57.108,57.839 55.608,57.225 54.066,56.729C53.554,58.114\n52.952,59.465 52.263,60.771C51.61,61.995 50.861,63.165\n50.024,64.27M41.875,54.659L41.875,65.961C44.388,65.196 46.881,62.886\n48.951,59.005C49.491,57.998 49.985,56.907 50.432,55.735C47.619,55.117\n44.754,54.758 41.875,54.663L41.875,54.659ZM55.206,53.155C57.344,53.849\n59.312,54.689 61.075,55.656C64.062,51.647 65.835,46.863\n66.183,41.875L56.845,41.875C56.728,45.685 56.179,49.469\n55.206,53.155M66.183,38.125C65.835,33.137 64.062,28.353\n61.075,24.344C59.312,25.311 57.344,26.151 55.206,26.845C56.136,30.295\n56.706,34.113 56.845,38.125L66.183,38.125ZM52.262,19.229C52.927,20.484\n53.53,21.831 54.07,23.271C55.61,22.775 57.109,22.161\n58.555,21.434C56.114,18.991 53.215,17.054 50.024,15.734C50.841,16.795\n51.591,17.973 52.262,19.229M50.432,24.265C50.004,23.146 49.51,22.054\n48.951,20.995C46.881,17.114 44.388,14.808\n41.875,14.039L41.875,25.338C44.913,25.225 47.789,24.85\n50.432,24.261L50.432,24.265Z" },
            { 19, "M16.667,16.667C13.009,16.667 10,19.677 10,23.333L10,33.334C10,35.162\n11.505,36.667 13.334,36.667L66.666,36.667C68.495,36.667 70,35.162\n70,33.334L70,23.333C70,19.677 66.991,16.667\n63.333,16.667L16.667,16.667ZM46.666,23.333C44.838,23.333 43.333,24.839\n43.333,26.667C43.333,28.495 44.838,30 46.666,30L46.7,30C48.528,30\n50.034,28.495 50.034,26.667C50.034,24.839 48.528,23.333\n46.7,23.333L46.666,23.333ZM56.667,23.333C54.838,23.333 53.333,24.839\n53.333,26.667C53.333,28.495 54.838,30 56.667,30L56.7,30C58.529,30\n60.033,28.495 60.033,26.667C60.033,24.839 58.529,23.333\n56.7,23.333L56.667,23.333ZM10,56.667L10,46.666C10,44.838 11.505,43.333\n13.334,43.333L66.666,43.333C68.495,43.333 70,44.838\n70,46.666L70,56.667C70,60.323 66.991,63.333\n63.333,63.333L16.667,63.333C13.009,63.333 10,60.323\n10,56.667ZM46.666,50C44.838,50 43.333,51.505\n43.333,53.333C43.333,55.161 44.838,56.667\n46.666,56.667L46.7,56.667C48.528,56.667 50.034,55.161\n50.034,53.333C50.034,51.505 48.528,50\n46.7,50L46.666,50ZM56.667,50C54.838,50 53.333,51.505\n53.333,53.333C53.333,55.161 54.838,56.667\n56.667,56.667L56.7,56.667C58.529,56.667 60.033,55.161\n60.033,53.333C60.033,51.505 58.529,50 56.7,50L56.667,50Z" },
        }
    };

    //==============================================================================

    /**
     * @brief Edge-decoration rendering data.
     *
     * Groups the tables that turn an `EdgeDecoration` key into the data the
     * draw pass needs to render the marker at an edge endpoint: its svgPaths
     * ordinal, its anchor point, its paint/stroke pair, and its fallback
     * geometry.
     */
    struct Marker
    {
        /**
         * @brief Decoration → svgPaths ordinal.
         *
         * Maps each edge decoration to the svgPaths row the draw pass renders;
         * empty decorations carry the empty-path ordinal and are skipped.
         */
        static constexpr jam::LookupTable<int, int, 14> pathIds {
            {
                { EdgeDecoration::none,              0 },
                { EdgeDecoration::circle,            1 },
                { EdgeDecoration::cross,             2 },
                { EdgeDecoration::diamond,           3 },
                { EdgeDecoration::diamondFilled,     3 },
                { EdgeDecoration::circleCross,       0 },
                { EdgeDecoration::crowsFootOne,      4 },
                { EdgeDecoration::crowsFootZeroOne,  5 },
                { EdgeDecoration::crowsFootMany,     6 },
                { EdgeDecoration::crowsFootZeroMany, 7 },
                { EdgeDecoration::arrow,             0 },
                { EdgeDecoration::inheritance,       8 },
                { EdgeDecoration::dependency,        9 },
                { EdgeDecoration::lollipop,          10 },
            }
        };

        //==============================================================================

        /**
         * @brief Decoration → (width, height) anchor.
         *
         * Maps each edge decoration to the anchor extent the layout pass uses
         * to place and rotate the marker at the edge endpoint.
         */
        static constexpr jam::LookupTable<int, jam::Union<float, float>, 14> anchors {
            {
                { EdgeDecoration::none, jam::Union<float, float>::pack (0.0f, 0.0f) },
                { EdgeDecoration::circle, jam::Union<float, float>::pack (11.0f, 5.0f) },
                { EdgeDecoration::cross, jam::Union<float, float>::pack (12.0f, 5.2f) },
                { EdgeDecoration::diamond, jam::Union<float, float>::pack (1.0f, 7.0f) },
                { EdgeDecoration::diamondFilled, jam::Union<float, float>::pack (1.0f, 7.0f) },
                { EdgeDecoration::circleCross, jam::Union<float, float>::pack (0.0f, 0.0f) },
                { EdgeDecoration::crowsFootOne, jam::Union<float, float>::pack (18.0f, 9.0f) },
                { EdgeDecoration::crowsFootZeroOne, jam::Union<float, float>::pack (30.0f, 9.0f) },
                { EdgeDecoration::crowsFootMany, jam::Union<float, float>::pack (27.0f, 18.0f) },
                { EdgeDecoration::crowsFootZeroMany, jam::Union<float, float>::pack (39.0f, 18.0f) },
                { EdgeDecoration::arrow, jam::Union<float, float>::pack (0.0f, 0.0f) },
                { EdgeDecoration::inheritance, jam::Union<float, float>::pack (1.0f, 7.0f) },
                { EdgeDecoration::dependency, jam::Union<float, float>::pack (13.0f, 7.0f) },
                { EdgeDecoration::lollipop, jam::Union<float, float>::pack (1.0f, 7.0f) },
            }
        };

        //==============================================================================

        /**
         * @brief Decoration → (paint, stroke width).
         *
         * Maps each edge decoration to its `MarkerPaint` mode and stroke width,
         * consumed by the draw pass when stroking or filling the marker path.
         */
        static constexpr jam::LookupTable<int, jam::Union<uint8_t, float>, 14> strokes {
            {
                { EdgeDecoration::none, jam::Union<uint8_t, float>::pack (MarkerPaint::stroked, 0.0f) },
                { EdgeDecoration::circle, jam::Union<uint8_t, float>::pack (MarkerPaint::stroked, 1.0f) },
                { EdgeDecoration::cross, jam::Union<uint8_t, float>::pack (MarkerPaint::stroked, 2.0f) },
                { EdgeDecoration::diamond, jam::Union<uint8_t, float>::pack (MarkerPaint::stroked, 1.0f) },
                { EdgeDecoration::diamondFilled, jam::Union<uint8_t, float>::pack (MarkerPaint::solid, 1.0f) },
                { EdgeDecoration::circleCross, jam::Union<uint8_t, float>::pack (MarkerPaint::stroked, 0.0f) },
                { EdgeDecoration::crowsFootOne, jam::Union<uint8_t, float>::pack (MarkerPaint::stroked, 1.0f) },
                { EdgeDecoration::crowsFootZeroOne, jam::Union<uint8_t, float>::pack (MarkerPaint::stroked, 1.0f) },
                { EdgeDecoration::crowsFootMany, jam::Union<uint8_t, float>::pack (MarkerPaint::stroked, 1.0f) },
                { EdgeDecoration::crowsFootZeroMany, jam::Union<uint8_t, float>::pack (MarkerPaint::stroked, 1.0f) },
                { EdgeDecoration::arrow, jam::Union<uint8_t, float>::pack (MarkerPaint::solid, 0.0f) },
                { EdgeDecoration::inheritance, jam::Union<uint8_t, float>::pack (MarkerPaint::stroked, 1.0f) },
                { EdgeDecoration::dependency, jam::Union<uint8_t, float>::pack (MarkerPaint::solid, 1.0f) },
                { EdgeDecoration::lollipop, jam::Union<uint8_t, float>::pack (MarkerPaint::background, 1.0f) },
            }
        };

        //==============================================================================

        /**
         * @brief Decoration → (shape, scale, filled) geometry.
         *
         * Maps each edge decoration to the fallback node geometry used when the
         * marker has no dedicated path: a `NodeShape`, a scale factor, and a
         * fill flag.
         */
        static constexpr jam::LookupTable<int, jam::Union<uint8_t, float, uint8_t>, 14> decorationGeometry {
            {
                { EdgeDecoration::none, jam::Union<uint8_t, float, uint8_t>::pack (NodeShape::rectangle, 1.0f, false) },
                { EdgeDecoration::circle, jam::Union<uint8_t, float, uint8_t>::pack (NodeShape::circle, 1.0f, false) },
                { EdgeDecoration::cross, jam::Union<uint8_t, float, uint8_t>::pack (NodeShape::cross, 1.0f, true) },
                { EdgeDecoration::diamond, jam::Union<uint8_t, float, uint8_t>::pack (NodeShape::diamond, 1.0f, false) },
                { EdgeDecoration::diamondFilled, jam::Union<uint8_t, float, uint8_t>::pack (NodeShape::diamond, 1.0f, true) },
                { EdgeDecoration::circleCross, jam::Union<uint8_t, float, uint8_t>::pack (NodeShape::cross, 1.2f, true) },
                { EdgeDecoration::crowsFootOne, jam::Union<uint8_t, float, uint8_t>::pack (NodeShape::rectangle, 1.0f, true) },
                { EdgeDecoration::crowsFootZeroOne, jam::Union<uint8_t, float, uint8_t>::pack (NodeShape::rectangle, 1.0f, true) },
                { EdgeDecoration::crowsFootMany, jam::Union<uint8_t, float, uint8_t>::pack (NodeShape::triangle, 1.4f, true) },
                { EdgeDecoration::crowsFootZeroMany, jam::Union<uint8_t, float, uint8_t>::pack (NodeShape::triangle, 1.4f, true) },
                { EdgeDecoration::arrow, jam::Union<uint8_t, float, uint8_t>::pack (NodeShape::barb, 1.0f, true) },
                { EdgeDecoration::inheritance, jam::Union<uint8_t, float, uint8_t>::pack (NodeShape::triangle, 1.3f, false) },
                { EdgeDecoration::dependency, jam::Union<uint8_t, float, uint8_t>::pack (NodeShape::rectangle, 1.0f, false) },
                { EdgeDecoration::lollipop, jam::Union<uint8_t, float, uint8_t>::pack (NodeShape::rectangle, 1.0f, false) },
            }
        };
    };

    //==============================================================================

    /**
     * @brief Per-diagram arrowhead rendering data.
     *
     * Groups the tables that give each diagram type its arrowhead path and
     * anchor; diagrams share a default arrowhead except where overridden here
     * (state, C4, sequence, architecture).
     */
    struct Arrowhead
    {
        /**
         * @brief Diagram type → arrowhead svgPaths ordinal.
         *
         * Maps a diagram type to the svgPaths row its edge arrowheads render;
         * types without a row take the first row's ordinal — the default arrowhead.
         */
        static constexpr jam::LookupTable<int, int, 46> pathIds {
            {
                { DiagramType::stateDiagramV2,   11 },
                { DiagramType::c4Context,        12 },
                { DiagramType::c4Container,      12 },
                { DiagramType::c4Component,      12 },
                { DiagramType::c4Dynamic,        12 },
                { DiagramType::c4Deployment,     12 },
                { DiagramType::sequenceDiagram,  12 },
                { DiagramType::architectureBeta, 13 },
            }
        };

        //==============================================================================

        /**
         * @brief Diagram type → arrowhead anchor extent.
         *
         * Maps a diagram type to the arrowhead anchor the layout pass uses to
         * place the arrow at the edge endpoint.
         */
        static constexpr jam::LookupTable<int, jam::Union<float, float>, 46> anchors {
            {
                { DiagramType::stateDiagramV2, jam::Union<float, float>::pack (19.0f, 7.0f) },
                { DiagramType::c4Context, jam::Union<float, float>::pack (9.0f, 5.0f) },
                { DiagramType::c4Container, jam::Union<float, float>::pack (9.0f, 5.0f) },
                { DiagramType::c4Component, jam::Union<float, float>::pack (9.0f, 5.0f) },
                { DiagramType::c4Dynamic, jam::Union<float, float>::pack (9.0f, 5.0f) },
                { DiagramType::c4Deployment, jam::Union<float, float>::pack (9.0f, 5.0f) },
                { DiagramType::sequenceDiagram, jam::Union<float, float>::pack (9.0f, 5.0f) },
                { DiagramType::architectureBeta, jam::Union<float, float>::pack (12.33f, 6.67f) },
            }
        };
    };

    //==============================================================================

    /**
     * @brief C4-model diagram vocabulary and role/geometry tables.
     *
     * Groups the C4 element/relation keyword bimap with the tables that map
     * those keywords into render roles, node traits, stereotype labels, and
     * relation geometry.
     */
    struct C4
    {
        /**
         * @brief C4 element, relation, and boundary keywords.
         *
         * Maps every C4 keyword — person/software/container/component and their
         * Db/Queue/Ext variants, relation spellings (`Rel`, `Rel_U`, `BiRel`),
         * boundaries, deployment nodes, and the style/layout directives — to
         * its key. The key is the discriminator the C4 layout and draw passes
         * consult when rendering a model element or relation.
         */
        struct Keyword : public jam::Bimap<int>
        {
            Keyword() : jam::Bimap<int> { {
                    { system,             juce::String::fromUTF8 ("System") },
                    { person,             juce::String::fromUTF8 ("Person") },
                    { personExt,          juce::String::fromUTF8 ("Person_Ext") },
                    { systemDb,           juce::String::fromUTF8 ("SystemDb") },
                    { systemQueue,        juce::String::fromUTF8 ("SystemQueue") },
                    { systemExt,          juce::String::fromUTF8 ("System_Ext") },
                    { systemDbExt,        juce::String::fromUTF8 ("SystemDb_Ext") },
                    { systemQueueExt,     juce::String::fromUTF8 ("SystemQueue_Ext") },
                    { container,          juce::String::fromUTF8 ("Container") },
                    { containerDb,        juce::String::fromUTF8 ("ContainerDb") },
                    { containerQueue,     juce::String::fromUTF8 ("ContainerQueue") },
                    { containerExt,       juce::String::fromUTF8 ("Container_Ext") },
                    { containerDbExt,     juce::String::fromUTF8 ("ContainerDb_Ext") },
                    { containerQueueExt,  juce::String::fromUTF8 ("ContainerQueue_Ext") },
                    { component,          juce::String::fromUTF8 ("Component") },
                    { componentDb,        juce::String::fromUTF8 ("ComponentDb") },
                    { componentQueue,     juce::String::fromUTF8 ("ComponentQueue") },
                    { componentExt,       juce::String::fromUTF8 ("Component_Ext") },
                    { componentDbExt,     juce::String::fromUTF8 ("ComponentDb_Ext") },
                    { componentQueueExt,  juce::String::fromUTF8 ("ComponentQueue_Ext") },
                    { biRel,              juce::String::fromUTF8 ("BiRel") },
                    { relIndex,           juce::String::fromUTF8 ("RelIndex") },
                    { relBack,            juce::String::fromUTF8 ("Rel_Back") },
                    { relU,               juce::String::fromUTF8 ("Rel_U") },
                    { relD,               juce::String::fromUTF8 ("Rel_D") },
                    { relL,               juce::String::fromUTF8 ("Rel_L") },
                    { relR,               juce::String::fromUTF8 ("Rel_R") },
                    { rel,                juce::String::fromUTF8 ("Rel") },
                    { enterpriseBoundary, juce::String::fromUTF8 ("Enterprise_Boundary") },
                    { systemBoundary,     juce::String::fromUTF8 ("System_Boundary") },
                    { containerBoundary,  juce::String::fromUTF8 ("Container_Boundary") },
                    { boundary,           juce::String::fromUTF8 ("Boundary") },
                    { deploymentNode,     juce::String::fromUTF8 ("Deployment_Node") },
                    { node,               juce::String::fromUTF8 ("Node") },
                    { nodeL,              juce::String::fromUTF8 ("Node_L") },
                    { nodeR,              juce::String::fromUTF8 ("Node_R") },
                    { relUp,              juce::String::fromUTF8 ("Rel_Up") },
                    { relDown,            juce::String::fromUTF8 ("Rel_Down") },
                    { relLeft,            juce::String::fromUTF8 ("Rel_Left") },
                    { relRight,           juce::String::fromUTF8 ("Rel_Right") },
                    { updateElementStyle, juce::String::fromUTF8 ("UpdateElementStyle") },
                    { updateRelStyle,     juce::String::fromUTF8 ("UpdateRelStyle") },
                    { updateLayoutConfig, juce::String::fromUTF8 ("UpdateLayoutConfig") },
                    { title,              juce::String::fromUTF8 ("title") },
            } } {}

            enum value : int
            {
                system             = 0,
                person             = 1,
                personExt          = 2,
                systemDb           = 3,
                systemQueue        = 4,
                systemExt          = 5,
                systemDbExt        = 6,
                systemQueueExt     = 7,
                container          = 8,
                containerDb        = 9,
                containerQueue     = 10,
                containerExt       = 11,
                containerDbExt     = 12,
                containerQueueExt  = 13,
                component          = 14,
                componentDb        = 15,
                componentQueue     = 16,
                componentExt       = 17,
                componentDbExt     = 18,
                componentQueueExt  = 19,
                biRel              = 20,
                relIndex           = 21,
                relBack            = 22,
                relU               = 23,
                relD               = 24,
                relL               = 25,
                relR               = 26,
                rel                = 27,
                enterpriseBoundary = 28,
                systemBoundary     = 29,
                containerBoundary  = 30,
                boundary           = 31,
                deploymentNode     = 32,
                node               = 33,
                nodeL              = 34,
                nodeR              = 35,
                relUp              = 36,
                relDown            = 37,
                relLeft            = 38,
                relRight           = 39,
                updateElementStyle = 40,
                updateRelStyle     = 41,
                updateLayoutConfig = 42,
                title              = 43,
            };

            static Keyword* getInstance() noexcept
            {
                return jam::SharedInstance<Keyword>::getInstance();
            }
        };

        //==============================================================================

        /**
         * @brief C4 keyword → render role.
         *
         * Buckets each C4 keyword into one of four roles (0–3) the layout pass
         * switches on: element, relation, boundary/node, or style directive.
         */
        static constexpr jam::LookupTable<int, int, Keyword::title + 1> roles {
            {
                { Keyword::system,             0 },
                { Keyword::person,             0 },
                { Keyword::personExt,          0 },
                { Keyword::systemDb,           0 },
                { Keyword::systemQueue,        0 },
                { Keyword::systemExt,          0 },
                { Keyword::systemDbExt,        0 },
                { Keyword::systemQueueExt,     0 },
                { Keyword::container,          0 },
                { Keyword::containerDb,        0 },
                { Keyword::containerQueue,     0 },
                { Keyword::containerExt,       0 },
                { Keyword::containerDbExt,     0 },
                { Keyword::containerQueueExt,  0 },
                { Keyword::component,          0 },
                { Keyword::componentDb,        0 },
                { Keyword::componentQueue,     0 },
                { Keyword::componentExt,       0 },
                { Keyword::componentDbExt,     0 },
                { Keyword::componentQueueExt,  0 },
                { Keyword::biRel,              1 },
                { Keyword::relIndex,           1 },
                { Keyword::relBack,            1 },
                { Keyword::relU,               1 },
                { Keyword::relD,               1 },
                { Keyword::relL,               1 },
                { Keyword::relR,               1 },
                { Keyword::rel,                1 },
                { Keyword::enterpriseBoundary, 2 },
                { Keyword::systemBoundary,     2 },
                { Keyword::containerBoundary,  2 },
                { Keyword::boundary,           2 },
                { Keyword::deploymentNode,     2 },
                { Keyword::node,               2 },
                { Keyword::nodeL,              2 },
                { Keyword::nodeR,              2 },
                { Keyword::relUp,              1 },
                { Keyword::relDown,            1 },
                { Keyword::relLeft,            1 },
                { Keyword::relRight,           1 },
                { Keyword::updateElementStyle, 3 },
                { Keyword::updateRelStyle,     3 },
                { Keyword::updateLayoutConfig, 3 },
                { Keyword::title,              3 },
            }
        };

        //==============================================================================

        /**
         * @brief C4 element index → (shape, stereotype index, usesTechn) trait.
         *
         * Maps each C4 element ordinal to its `NodeShape`, a stereotype index
         * into `stereotypes`, and whether its call carries a technology
         * positional argument (Container/Component families, ordinals 8–19);
         * consumed when drawing a C4 element node.
         */
        static constexpr jam::LookupTable<int, jam::Union<uint8_t, uint8_t, uint8_t>, 20> traits {
            {
                { 0, jam::Union<uint8_t, uint8_t, uint8_t>::pack (NodeShape::rectangle, 2, false) },
                { 1, jam::Union<uint8_t, uint8_t, uint8_t>::pack (NodeShape::person, 0, false) },
                { 2, jam::Union<uint8_t, uint8_t, uint8_t>::pack (NodeShape::person, 1, false) },
                { 3, jam::Union<uint8_t, uint8_t, uint8_t>::pack (NodeShape::cylinder, 2, false) },
                { 4, jam::Union<uint8_t, uint8_t, uint8_t>::pack (NodeShape::stadium, 2, false) },
                { 5, jam::Union<uint8_t, uint8_t, uint8_t>::pack (NodeShape::rectangle, 3, false) },
                { 6, jam::Union<uint8_t, uint8_t, uint8_t>::pack (NodeShape::cylinder, 3, false) },
                { 7, jam::Union<uint8_t, uint8_t, uint8_t>::pack (NodeShape::stadium, 3, false) },
                { 8, jam::Union<uint8_t, uint8_t, uint8_t>::pack (NodeShape::rectangle, 4, true) },
                { 9, jam::Union<uint8_t, uint8_t, uint8_t>::pack (NodeShape::cylinder, 4, true) },
                { 10, jam::Union<uint8_t, uint8_t, uint8_t>::pack (NodeShape::stadium, 4, true) },
                { 11, jam::Union<uint8_t, uint8_t, uint8_t>::pack (NodeShape::rectangle, 5, true) },
                { 12, jam::Union<uint8_t, uint8_t, uint8_t>::pack (NodeShape::cylinder, 5, true) },
                { 13, jam::Union<uint8_t, uint8_t, uint8_t>::pack (NodeShape::stadium, 5, true) },
                { 14, jam::Union<uint8_t, uint8_t, uint8_t>::pack (NodeShape::rectangle, 6, true) },
                { 15, jam::Union<uint8_t, uint8_t, uint8_t>::pack (NodeShape::cylinder, 6, true) },
                { 16, jam::Union<uint8_t, uint8_t, uint8_t>::pack (NodeShape::stadium, 6, true) },
                { 17, jam::Union<uint8_t, uint8_t, uint8_t>::pack (NodeShape::rectangle, 7, true) },
                { 18, jam::Union<uint8_t, uint8_t, uint8_t>::pack (NodeShape::cylinder, 7, true) },
                { 19, jam::Union<uint8_t, uint8_t, uint8_t>::pack (NodeShape::stadium, 7, true) },
            }
        };

        //==============================================================================

        /**
         * @brief C4 boundary keyword → (fixed type, deployment stroke) flags.
         *
         * Maps the eight boundary-declaring keywords to whether they carry a
         * fixed `Id::type` ordinal and whether their perimeter draws solid
         * (deployment) rather than dotted (logical) stroke.
         */
        static constexpr jam::LookupTable<int, jam::Union<uint8_t, uint8_t>, Keyword::nodeR + 1> boundaries {
            {
                { Keyword::enterpriseBoundary, jam::Union<uint8_t, uint8_t>::pack (true, false) },
                { Keyword::systemBoundary, jam::Union<uint8_t, uint8_t>::pack (true, false) },
                { Keyword::containerBoundary, jam::Union<uint8_t, uint8_t>::pack (true, false) },
                { Keyword::boundary, jam::Union<uint8_t, uint8_t>::pack (false, false) },
                { Keyword::deploymentNode, jam::Union<uint8_t, uint8_t>::pack (false, true) },
                { Keyword::node, jam::Union<uint8_t, uint8_t>::pack (false, true) },
                { Keyword::nodeL, jam::Union<uint8_t, uint8_t>::pack (false, true) },
                { Keyword::nodeR, jam::Union<uint8_t, uint8_t>::pack (false, true) },
            }
        };

        //==============================================================================

        /**
         * @brief C4 element index → stereotype label.
         *
         * Maps each C4 element ordinal to the stereotype text drawn above the
         * element node ("Person", "Software System", "Container", "Component").
         */
        static constexpr jam::LookupTable<int, const char*, 8> stereotypes {
            {
                { 0, "Person" },
                { 1, "Person" },
                { 2, "Software System" },
                { 3, "Software System" },
                { 4, "Container" },
                { 5, "Container" },
                { 6, "Component" },
                { 7, "Component" },
            }
        };

        //==============================================================================

        /**
         * @brief C4 relation keyword → (bidirectional, index, …) flags.
         *
         * Maps the relation spellings (`Rel_U`, `BiRel`, `RelIndex`, `RelBack`)
         * to the flags the layout pass uses to orient and label a relation.
         */
        static constexpr jam::LookupTable<int, jam::Union<uint8_t, uint8_t, uint8_t>, Keyword::title + 1> relations {
            {
                { Keyword::relU, jam::Union<uint8_t, uint8_t, uint8_t>::pack (false, true, false) },
                { Keyword::biRel, jam::Union<uint8_t, uint8_t, uint8_t>::pack (true, true, false) },
                { Keyword::relIndex, jam::Union<uint8_t, uint8_t, uint8_t>::pack (false, true, true) },
                { Keyword::relBack, jam::Union<uint8_t, uint8_t, uint8_t>::pack (true, false, false) },
            }
        };
    };

    //==============================================================================

    /**
     * @brief Class-diagram relation operator → decoration geometry.
     *
     * Maps each class relation operator (inheritance, realization, composition,
     * aggregation, dependency, lollipop) to a packed quadruple — the operator
     * itself, the source decoration, the target decoration, and an "open"
     * flag — consumed when drawing a class-diagram edge.
     */
    struct Class
    {
            using RelationValue = jam::Union<uint8_t, uint8_t, uint8_t, uint8_t>;

        static constexpr jam::LookupTable<int, RelationValue, 16> relations {
            {
                { 0, RelationValue::pack (Operator::realizationOpen, EdgeDecoration::inheritance, EdgeDecoration::none, true) },
                { 1, RelationValue::pack (Operator::realizationClose, EdgeDecoration::none, EdgeDecoration::inheritance, true) },
                { 2, RelationValue::pack (Operator::inherit, EdgeDecoration::inheritance, EdgeDecoration::none, false) },
                { 3, RelationValue::pack (Operator::inheritClose, EdgeDecoration::none, EdgeDecoration::inheritance, false) },
                { 4, RelationValue::pack (Operator::composition, EdgeDecoration::diamondFilled, EdgeDecoration::none, false) },
                { 5, RelationValue::pack (Operator::compositionClose, EdgeDecoration::none, EdgeDecoration::diamondFilled, false) },
                { 6, RelationValue::pack (Operator::aggregation, EdgeDecoration::diamond, EdgeDecoration::none, false) },
                { 7, RelationValue::pack (Operator::aggregationClose, EdgeDecoration::none, EdgeDecoration::diamond, false) },
                { 8, RelationValue::pack (Operator::dependencyOpen, EdgeDecoration::dependency, EdgeDecoration::none, true) },
                { 9, RelationValue::pack (Operator::dependency, EdgeDecoration::none, EdgeDecoration::dependency, true) },
                { 10, RelationValue::pack (Operator::messageReverse, EdgeDecoration::dependency, EdgeDecoration::none, false) },
                { 11, RelationValue::pack (Operator::forward, EdgeDecoration::none, EdgeDecoration::dependency, false) },
                { 12, RelationValue::pack (Operator::lollipop, EdgeDecoration::lollipop, EdgeDecoration::none, false) },
                { 13, RelationValue::pack (Operator::lollipopClose, EdgeDecoration::none, EdgeDecoration::lollipop, false) },
                { 14, RelationValue::pack (Operator::dashedLink, EdgeDecoration::none, EdgeDecoration::none, true) },
                { 15, RelationValue::pack (Operator::classLink, EdgeDecoration::none, EdgeDecoration::none, false) },
            }
        };
    };

    //==============================================================================

    /**
     * @brief ER-diagram cardinality operator → decoration mapping.
     *
     * Maps each ER cardinality operator (exactly-one, zero/one, one/many,
     * zero/many) to the crows-foot `EdgeDecoration` drawn at the relation end.
     */
    struct ER
    {
        static constexpr jam::LookupTable<int, int, Operator::erZeroOrMoreAlt + 1> cardinalities {
            {
                { Operator::commentOpen, EdgeDecoration::none },
                { Operator::erExactlyOne, EdgeDecoration::crowsFootOne },
                { Operator::erZeroOrOne, EdgeDecoration::crowsFootZeroOne },
                { Operator::erZeroOrOneAlt, EdgeDecoration::crowsFootZeroOne },
                { Operator::erOneOrMore, EdgeDecoration::crowsFootMany },
                { Operator::erOneOrMoreAlt, EdgeDecoration::crowsFootMany },
                { Operator::erZeroOrMore, EdgeDecoration::crowsFootZeroMany },
                { Operator::erZeroOrMoreAlt, EdgeDecoration::crowsFootZeroMany },
            }
        };
    };

    //==============================================================================

    /**
     * @brief Sequence-diagram frame keywords and message-arrow geometry.
     *
     * Groups the sequence frame keyword bimap with the table that maps each
     * message operator to its packed (operator, stroke, reverse, decoration)
     * arrow recipe.
     */
    struct Sequence
    {
        /**
         * @brief Sequence frame keywords.
         *
         * Maps the frame openers (`alt`, `opt`, `loop`, `par`, `critical`,
         * `break`, `rect`) to their keys.
         */
        struct Keyword : public jam::Bimap<int>
        {
            Keyword() : jam::Bimap<int> { {
                    { alt,        juce::String::fromUTF8 ("alt") },
                    { opt,        juce::String::fromUTF8 ("opt") },
                    { loop,       juce::String::fromUTF8 ("loop") },
                    { par,        juce::String::fromUTF8 ("par") },
                    { critical,   juce::String::fromUTF8 ("critical") },
                    { tokenBreak, juce::String::fromUTF8 ("break") },
                    { rect,       juce::String::fromUTF8 ("rect") },
            } } {}

            enum value : int
            {
                alt        = 0,
                opt        = 1,
                loop       = 2,
                par        = 3,
                critical   = 4,
                tokenBreak = 5,
                rect       = 6,
            };

            static Keyword* getInstance() noexcept
            {
                return jam::SharedInstance<Keyword>::getInstance();
            }
        };

        //==============================================================================

        /**
         * @brief Sequence message operator → (stroke, reverse, decoration).
         *
         * Maps each sequence message arrow operator to its line stroke, its
         * direction flag, and its endpoint decoration, consumed by the draw
         * pass when rendering a message edge.
         */
        static constexpr jam::LookupTable<int, jam::Union<uint8_t, uint8_t, uint8_t, uint8_t>, 8> arrows {
            {
                { 0, jam::Union<uint8_t, uint8_t, uint8_t, uint8_t>::pack (Operator::messageAsyncDotted, EdgeStroke::dotted, false, EdgeDecoration::none) },
                { 1, jam::Union<uint8_t, uint8_t, uint8_t, uint8_t>::pack (Operator::messageCrossDotted, EdgeStroke::dotted, false, EdgeDecoration::cross) },
                { 2, jam::Union<uint8_t, uint8_t, uint8_t, uint8_t>::pack (Operator::messageAsync, EdgeStroke::solid, false, EdgeDecoration::none) },
                { 3, jam::Union<uint8_t, uint8_t, uint8_t, uint8_t>::pack (Operator::messageReverse, EdgeStroke::dotted, true, EdgeDecoration::none) },
                { 4, jam::Union<uint8_t, uint8_t, uint8_t, uint8_t>::pack (Operator::forward, EdgeStroke::dotted, false, EdgeDecoration::none) },
                { 5, jam::Union<uint8_t, uint8_t, uint8_t, uint8_t>::pack (Operator::messageReverseSolid, EdgeStroke::solid, true, EdgeDecoration::none) },
                { 6, jam::Union<uint8_t, uint8_t, uint8_t, uint8_t>::pack (Operator::messageCross, EdgeStroke::solid, false, EdgeDecoration::cross) },
                { 7, jam::Union<uint8_t, uint8_t, uint8_t, uint8_t>::pack (Operator::messageSolid, EdgeStroke::solid, false, EdgeDecoration::none) },
            }
        };
    };

    //==============================================================================

    /**
     * @brief State-diagram stereotype operator → node shape.
     *
     * Maps the state stereotype markers (`<<choice>>`, `<<fork>>`, `<<join>>`)
     * to the `NodeShape` the layout pass renders for them.
     */
    struct State
    {
        static constexpr jam::LookupTable<int, jam::Union<uint8_t, uint8_t>, 3> stereotypes {
            {
                { 0, jam::Union<uint8_t, uint8_t>::pack (Operator::choice, NodeShape::diamond) },
                { 1, jam::Union<uint8_t, uint8_t>::pack (Operator::fork, NodeShape::forkJoin) },
                { 2, jam::Union<uint8_t, uint8_t>::pack (Operator::join, NodeShape::forkJoin) },
            }
        };
    };

    //==============================================================================

    /**
     * @brief Git-graph keywords and commit-marker geometry.
     *
     * Groups the git keyword bimap with the tables that map commit kinds and
     * flags to their render role and marker decoration.
     */
    struct Git
    {
        /**
         * @brief Git commit, branch, flag, and attribute keywords.
         *
         * Maps commit kinds (`commit`, `branch`, `checkout`, `merge`,
         * `cherry-pick`), commit flags (`NORMAL`, `REVERSE`, `HIGHLIGHT`), and
         * commit attribute keys (`id`, `tag`, `type`, `order`, `parent`) to
         * their keys.
         */
        struct Keyword : public jam::Bimap<int>
        {
            Keyword() : jam::Bimap<int> { {
                    { commit,      juce::String::fromUTF8 ("commit") },
                    { branch,      juce::String::fromUTF8 ("branch") },
                    { checkout,    juce::String::fromUTF8 ("checkout") },
                    { tokenSwitch, juce::String::fromUTF8 ("switch") },
                    { merge,       juce::String::fromUTF8 ("merge") },
                    { cherryPick,  juce::String::fromUTF8 ("cherry-pick") },
                    { normal,      juce::String::fromUTF8 ("NORMAL") },
                    { reverse,     juce::String::fromUTF8 ("REVERSE") },
                    { highlight,   juce::String::fromUTF8 ("HIGHLIGHT") },
                    { id,          juce::String::fromUTF8 ("id") },
                    { tag,         juce::String::fromUTF8 ("tag") },
                    { type,        juce::String::fromUTF8 ("type") },
                    { order,       juce::String::fromUTF8 ("order") },
                    { parent,      juce::String::fromUTF8 ("parent") },
            } } {}

            enum value : int
            {
                commit      = 0,
                branch      = 1,
                checkout    = 2,
                tokenSwitch = 3,
                merge       = 4,
                cherryPick  = 5,
                normal      = 6,
                reverse     = 7,
                highlight   = 8,
                id          = 9,
                tag         = 10,
                type        = 11,
                order       = 12,
                parent      = 13,
            };

            static Keyword* getInstance() noexcept
            {
                return jam::SharedInstance<Keyword>::getInstance();
            }
        };

        //==============================================================================

        /**
         * @brief Git keyword → render role.
         *
         * Buckets each git keyword into a role (0 = commit kind, 1 = flag,
         * 2 = commit attribute key) the layout pass switches on.
         */
        static constexpr jam::LookupTable<int, int, Keyword::parent + 1> roles {
            {
                { Keyword::commit,      0 },
                { Keyword::branch,      0 },
                { Keyword::checkout,    0 },
                { Keyword::tokenSwitch, 0 },
                { Keyword::merge,       0 },
                { Keyword::cherryPick,  0 },
                { Keyword::normal,      1 },
                { Keyword::reverse,     1 },
                { Keyword::highlight,   1 },
                { Keyword::id,          2 },
                { Keyword::tag,         2 },
                { Keyword::type,        2 },
                { Keyword::order,       2 },
                { Keyword::parent,      2 },
            }
        };

        //==============================================================================

        /**
         * @brief Git flag → (reverse, decoration) marker geometry.
         *
         * Maps the git flags that carry a marker (`reverse`, `highlight`,
         * `merge`, `cherry-pick`) to the decoration drawn on the commit node.
         */
        static constexpr jam::LookupTable<int, jam::Union<uint8_t, uint8_t>, Keyword::parent + 1> markerGeometry {
            {
                { Keyword::reverse, jam::Union<uint8_t, uint8_t>::pack (true, EdgeDecoration::cross) },
                { Keyword::highlight, jam::Union<uint8_t, uint8_t>::pack (false, EdgeDecoration::none) },
                { Keyword::merge, jam::Union<uint8_t, uint8_t>::pack (true, EdgeDecoration::circle) },
                { Keyword::cherryPick, jam::Union<uint8_t, uint8_t>::pack (true, EdgeDecoration::diamond) },
            }
        };
    };

    //==============================================================================

    /**
     * @brief Gantt-diagram keywords and time/status tables.
     *
     * Groups the gantt keyword bimap with the tables that drive gantt layout:
     * roles, duration units, task status colour ids, section band colours, axis
     * tick days, and the millisecond/day/date-format constants.
     */
    struct Gantt
    {
        /**
         * @brief Gantt directive and task-status keywords.
         *
         * Maps the gantt directives (`dateFormat`, `axisFormat`, `todayMarker`,
         * `excludes`, `includes`) and task statuses (`done`, `active`, `crit`,
         * `milestone`) to their keys.
         */
        struct Keyword : public jam::Bimap<int>
        {
            Keyword() : jam::Bimap<int> { {
                    { dateFormat,  juce::String::fromUTF8 ("dateFormat") },
                    { axisFormat,  juce::String::fromUTF8 ("axisFormat") },
                    { todayMarker, juce::String::fromUTF8 ("todayMarker") },
                    { excludes,    juce::String::fromUTF8 ("excludes") },
                    { includes,    juce::String::fromUTF8 ("includes") },
                    { done,        juce::String::fromUTF8 ("done") },
                    { active,      juce::String::fromUTF8 ("active") },
                    { crit,        juce::String::fromUTF8 ("crit") },
                    { milestone,   juce::String::fromUTF8 ("milestone") },
            } } {}

            enum value : int
            {
                dateFormat  = 0,
                axisFormat  = 1,
                todayMarker = 2,
                excludes    = 3,
                includes    = 4,
                done        = 5,
                active      = 6,
                crit        = 7,
                milestone   = 8,
            };

            static Keyword* getInstance() noexcept
            {
                return jam::SharedInstance<Keyword>::getInstance();
            }
        };

        //==============================================================================

        /**
         * @brief Gantt keyword → render role.
         *
         * Buckets each gantt keyword into a role (0 = directive, 1 = task
         * status) the layout pass switches on.
         */
        static constexpr jam::LookupTable<int, int, Keyword::milestone + 1> roles {
            {
                { Keyword::dateFormat,  0 },
                { Keyword::axisFormat,  0 },
                { Keyword::todayMarker, 0 },
                { Keyword::excludes,    0 },
                { Keyword::includes,    0 },
                { Keyword::done,        1 },
                { Keyword::active,      1 },
                { Keyword::crit,        1 },
                { Keyword::milestone,   1 },
            }
        };

        //==============================================================================

        /**
         * @brief Duration-unit suffix → day multiplier.
         *
         * Maps the single-letter duration suffix on a gantt duration literal
         * (`w`, `h`, `m`, `y`) to its multiplier in days; a bare number is one
         * day.
         */
        static constexpr jam::LookupTable<int, float, 128> durationUnits {
            {
                { 0, 1.0f },
                { static_cast<int> ('w'), 7.0f },
                { static_cast<int> ('h'), 1.0f / 24.0f },
                { static_cast<int> ('m'), 30.0f },
                { static_cast<int> ('y'), 365.0f },
            }
        };

        //==============================================================================

        /**
         * @brief Gantt task status → fill colour id.
         *
         * Maps each task status (`done`, `active`, `crit`, `milestone`) to the
         * `StyleSheet` colour id used to fill its bar.
         */
        static constexpr jam::LookupTable<int, int, Keyword::milestone + 1> statusColourIds {
            {
                { Keyword::done, map::ColourId::mermaidDiagramGanttTaskDoneColourId },
                { Keyword::active, map::ColourId::mermaidDiagramGanttTaskActiveColourId },
                { Keyword::crit, map::ColourId::mermaidDiagramGanttTaskCritColourId },
                { Keyword::milestone, map::ColourId::mermaidDiagramGanttMilestoneColourId },
            }
        };

        //==============================================================================

        /**
         * @brief Gantt section band index → colour id.
         *
         * Maps the two alternating section bands to their `StyleSheet` colour
         * ids, used to shade section rows.
         */
        static constexpr jam::LookupTable<int, int, 2> sectionBandColourIds {
            {
                { 0, map::ColourId::mermaidDiagramGanttSectionBandColourId },
                { 1, map::ColourId::mermaidDiagramGanttSectionAccentColourId },
            }
        };

        //==============================================================================

        /**
         * @brief Axis tick index → day interval.
         *
         * Maps the five axis tick densities to their day spacing, used to
         * choose a tick interval that keeps the time axis legible.
         */
        static constexpr jam::LookupTable<int, double, 5> axisTickDays {
            {
                { 0, 1.0 },
                { 1, 2.0 },
                { 2, 7.0 },
                { 3, 14.0 },
                { 4, 28.0 },
            }
        };

    //==============================================================================

        static constexpr double      millisecondsPerDay { 86400000.0 };///< Milliseconds in one day, for date arithmetic.
        static constexpr const char* axisFormat         { "%Y-%m-%d" };///< Default date format for the time axis.
    };

    //==============================================================================

    /**
     * @brief Kanban-diagram priority keywords.
     *
     * Maps the four kanban priority labels (`Very Low`, `Low`, `High`,
     * `Very High`) to their keys.
     */
    struct Kanban
    {
        struct Keyword : public jam::Bimap<int>
        {
            Keyword() : jam::Bimap<int> { {
                    { veryLow,  juce::String::fromUTF8 ("Very Low") },
                    { low,      juce::String::fromUTF8 ("Low") },
                    { high,     juce::String::fromUTF8 ("High") },
                    { veryHigh, juce::String::fromUTF8 ("Very High") },
            } } {}

            enum value : int
            {
                veryLow  = 0,
                low      = 1,
                high     = 2,
                veryHigh = 3,
            };

            static Keyword* getInstance() noexcept
            {
                return jam::SharedInstance<Keyword>::getInstance();
            }
        };
    };

    //==============================================================================

    /**
     * @brief Mindmap node-shape delimiter mapping.
     *
     * Maps each mindmap node's bracketing delimiters to the `NodeShape` they
     * denote, consumed by the mindmap layout pass.
     */
    struct Mindmap
    {
        static constexpr jam::LookupTable<int, jam::Union<uint8_t, uint8_t, uint8_t>, 6> shapes {
            {
                { 0, jam::Union<uint8_t, uint8_t, uint8_t>::pack (Operator::doubleOpenParen, Operator::doubleCloseParen, NodeShape::circle) },
                { 1, jam::Union<uint8_t, uint8_t, uint8_t>::pack (Operator::doubleCloseParen, Operator::doubleOpenParen, NodeShape::bang) },
                { 2, jam::Union<uint8_t, uint8_t, uint8_t>::pack (Operator::doubleOpenBrace, Operator::doubleCloseBrace, NodeShape::hexagon) },
                { 3, jam::Union<uint8_t, uint8_t, uint8_t>::pack (Operator::openBracket, Operator::closeBracket, NodeShape::rectangle) },
                { 4, jam::Union<uint8_t, uint8_t, uint8_t>::pack (Operator::closeParen, Operator::openParen, NodeShape::cloud) },
                { 5, jam::Union<uint8_t, uint8_t, uint8_t>::pack (Operator::openParen, Operator::closeParen, NodeShape::roundRect) },
            }
        };
    };

    //==============================================================================

    /**
     * @brief Architecture-diagram keywords, links, anchors, and icons.
     *
     * Groups the architecture keyword bimap with the tables that map those
     * keywords into render roles, link geometry, side anchors, and node icons.
     */
    struct Architecture
    {
        /**
         * @brief Architecture node and side keywords.
         *
         * Maps the node kinds (`group`, `service`, `junction`, `cloud`,
         * `database`, `disk`, `internet`, `server`, `unknown`, `blank`) and the
         * side labels (`L`, `R`, `T`, `B`) to their keys.
         */
        struct Keyword : public jam::Bimap<int>
        {
            Keyword() : jam::Bimap<int> { {
                    { group,    juce::String::fromUTF8 ("group") },
                    { service,  juce::String::fromUTF8 ("service") },
                    { junction, juce::String::fromUTF8 ("junction") },
                    { sideL,    juce::String::fromUTF8 ("L") },
                    { sideR,    juce::String::fromUTF8 ("R") },
                    { sideT,    juce::String::fromUTF8 ("T") },
                    { sideB,    juce::String::fromUTF8 ("B") },
                    { cloud,    juce::String::fromUTF8 ("cloud") },
                    { database, juce::String::fromUTF8 ("database") },
                    { disk,     juce::String::fromUTF8 ("disk") },
                    { internet, juce::String::fromUTF8 ("internet") },
                    { server,   juce::String::fromUTF8 ("server") },
                    { unknown,  juce::String::fromUTF8 ("unknown") },
                    { blank,    juce::String::fromUTF8 ("blank") },
            } } {}

            enum value : int
            {
                group    = 0,
                service  = 1,
                junction = 2,
                sideL    = 3,
                sideR    = 4,
                sideT    = 5,
                sideB    = 6,
                cloud    = 7,
                database = 8,
                disk     = 9,
                internet = 10,
                server   = 11,
                unknown  = 12,
                blank    = 13,
            };

            static Keyword* getInstance() noexcept
            {
                return jam::SharedInstance<Keyword>::getInstance();
            }
        };

        //==============================================================================

        /**
         * @brief Architecture keyword → render role.
         *
         * Buckets each architecture keyword into a role (0 = group, 1 = side,
         * 2 = node) the layout pass switches on.
         */
        static constexpr jam::LookupTable<int, int, Keyword::blank + 1> roles {
            {
                { Keyword::group,    0 },
                { Keyword::service,  0 },
                { Keyword::junction, 0 },
                { Keyword::sideL,    1 },
                { Keyword::sideR,    1 },
                { Keyword::sideT,    1 },
                { Keyword::sideB,    1 },
                { Keyword::cloud,    2 },
                { Keyword::database, 2 },
                { Keyword::disk,     2 },
                { Keyword::internet, 2 },
                { Keyword::server,   2 },
                { Keyword::unknown,  2 },
                { Keyword::blank,    2 },
            }
        };

        //==============================================================================

        /**
         * @brief Architecture link operator → (source, target) arrow flags.
         *
         * Maps the architecture link operators to the source/target arrow flags
         * the draw pass uses when rendering an architecture edge.
         */
        static constexpr jam::LookupTable<int, jam::Union<uint8_t, uint8_t, uint8_t>, 5> links {
            {
                { 0, jam::Union<uint8_t, uint8_t, uint8_t>::pack (Operator::arrow, true, true) },
                { 1, jam::Union<uint8_t, uint8_t, uint8_t>::pack (Operator::messageReverse, true, false) },
                { 2, jam::Union<uint8_t, uint8_t, uint8_t>::pack (Operator::forward, false, true) },
                { 3, jam::Union<uint8_t, uint8_t, uint8_t>::pack (Operator::messageSolid, false, true) },
                { 4, jam::Union<uint8_t, uint8_t, uint8_t>::pack (Operator::classLink, false, false) },
            }
        };

        //==============================================================================

        /**
         * @brief Architecture side → (x, y) anchor.
         *
         * Maps each side label (`L`, `R`, `T`, `B`) to its normalized edge
         * anchor point, consumed when attaching an edge to a group side.
         */
        static constexpr jam::LookupTable<int, jam::Union<float, float>, Keyword::sideB + 1> anchors {
            {
                { Keyword::sideR, jam::Union<float, float>::pack (1.0f, 0.5f) },
                { Keyword::sideL, jam::Union<float, float>::pack (0.0f, 0.5f) },
                { Keyword::sideT, jam::Union<float, float>::pack (0.5f, 0.0f) },
                { Keyword::sideB, jam::Union<float, float>::pack (0.5f, 1.0f) },
            }
        };

        //==============================================================================

        /**
         * @brief Architecture node keyword → svgPaths ordinal.
         *
         * Maps the node kinds that carry an icon (cloud, database, disk,
         * internet, server) to their icon row; unknown and blank map to the
         * empty path. The first row pins the table default to the empty path.
         */
        static constexpr jam::LookupTable<int, int, Keyword::blank + 1> pathIds {
            {
                { Keyword::blank,    0 },
                { Keyword::cloud,    15 },
                { Keyword::database, 16 },
                { Keyword::disk,     17 },
                { Keyword::internet, 18 },
                { Keyword::server,   19 },
                { Keyword::unknown,  0 },
            }
        };
    };

    //==============================================================================

    /**
     * @brief XY-chart series keywords.
     *
     * Maps the two xy-chart series kinds (`bar`, `line`) to their keys.
     */
    struct Xy
    {
        struct Keyword : public jam::Bimap<int>
        {
            Keyword() : jam::Bimap<int> { {
                    { bar,  juce::String::fromUTF8 ("bar") },
                    { line, juce::String::fromUTF8 ("line") },
            } } {}

            enum value : int
            {
                bar  = 0,
                line = 1,
            };

            static Keyword* getInstance() noexcept
            {
                return jam::SharedInstance<Keyword>::getInstance();
            }
        };
    };

    //==============================================================================

    /**
     * @brief Quadrant-chart quadrant keywords.
     *
     * Maps the four quadrant labels (`quadrant-1` through `quadrant-4`) to
     * their keys.
     */
    struct Quadrant
    {
        struct Keyword : public jam::Bimap<int>
        {
            Keyword() : jam::Bimap<int> { {
                    { quadrant1, juce::String::fromUTF8 ("quadrant-1") },
                    { quadrant2, juce::String::fromUTF8 ("quadrant-2") },
                    { quadrant3, juce::String::fromUTF8 ("quadrant-3") },
                    { quadrant4, juce::String::fromUTF8 ("quadrant-4") },
            } } {}

            enum value : int
            {
                quadrant1 = 0,
                quadrant2 = 1,
                quadrant3 = 2,
                quadrant4 = 3,
            };

            static Keyword* getInstance() noexcept
            {
                return jam::SharedInstance<Keyword>::getInstance();
            }
        };
    };

    //==============================================================================

    /**
     * @brief Requirement-diagram keywords, roles, and label tables.
     *
     * Groups the requirement keyword bimap with the tables that map those
     * keywords into render roles, stereotype labels, and attribute labels.
     */
    struct Requirement
    {
        /**
         * @brief Requirement element, relation, attribute, and risk keywords.
         *
         * Maps the requirement vocabulary — element/relation kinds
         * (`requirement`, `functionalRequirement`, `element`, `contains`,
         * `derives`, …), attribute names (`id`, `text`, `risk`, `type`,
         * `docref`), risk levels (`low`, `medium`, `high`), and verification
         * methods (`analysis`, `demonstration`, `inspection`, `test`) — to
         * their keys.
         */
        struct Keyword : public jam::Bimap<int>
        {
            Keyword() : jam::Bimap<int> { {
                    { requirement,            juce::String::fromUTF8 ("requirement") },
                    { functionalRequirement,  juce::String::fromUTF8 ("functionalRequirement") },
                    { interfaceRequirement,   juce::String::fromUTF8 ("interfaceRequirement") },
                    { performanceRequirement, juce::String::fromUTF8 ("performanceRequirement") },
                    { physicalRequirement,    juce::String::fromUTF8 ("physicalRequirement") },
                    { designConstraint,       juce::String::fromUTF8 ("designConstraint") },
                    { element,                juce::String::fromUTF8 ("element") },
                    { contains,               juce::String::fromUTF8 ("contains") },
                    { copies,                 juce::String::fromUTF8 ("copies") },
                    { derives,                juce::String::fromUTF8 ("derives") },
                    { satisfies,              juce::String::fromUTF8 ("satisfies") },
                    { verifies,               juce::String::fromUTF8 ("verifies") },
                    { refines,                juce::String::fromUTF8 ("refines") },
                    { traces,                 juce::String::fromUTF8 ("traces") },
                    { id,                     juce::String::fromUTF8 ("id") },
                    { text,                   juce::String::fromUTF8 ("text") },
                    { risk,                   juce::String::fromUTF8 ("risk") },
                    { verifymethod,           juce::String::fromUTF8 ("verifymethod") },
                    { type,                   juce::String::fromUTF8 ("type") },
                    { docref,                 juce::String::fromUTF8 ("docref") },
                    { low,                    juce::String::fromUTF8 ("low") },
                    { medium,                 juce::String::fromUTF8 ("medium") },
                    { high,                   juce::String::fromUTF8 ("high") },
                    { analysis,               juce::String::fromUTF8 ("analysis") },
                    { demonstration,          juce::String::fromUTF8 ("demonstration") },
                    { inspection,             juce::String::fromUTF8 ("inspection") },
                    { test,                   juce::String::fromUTF8 ("test") },
            } } {}

            enum value : int
            {
                requirement            = 0,
                functionalRequirement  = 1,
                interfaceRequirement   = 2,
                performanceRequirement = 3,
                physicalRequirement    = 4,
                designConstraint       = 5,
                element                = 6,
                contains               = 7,
                copies                 = 8,
                derives                = 9,
                satisfies              = 10,
                verifies               = 11,
                refines                = 12,
                traces                 = 13,
                id                     = 14,
                text                   = 15,
                risk                   = 16,
                verifymethod           = 17,
                type                   = 18,
                docref                 = 19,
                low                    = 20,
                medium                 = 21,
                high                   = 22,
                analysis               = 23,
                demonstration          = 24,
                inspection             = 25,
                test                   = 26,
            };

            static Keyword* getInstance() noexcept
            {
                return jam::SharedInstance<Keyword>::getInstance();
            }
        };

        //==============================================================================

        /**
         * @brief Requirement keyword → render role.
         *
         * Buckets each requirement keyword into a role (0 = element, 1 =
         * relation, 2 = attribute, 3 = risk level, 4 = verification method)
         * the layout pass switches on.
         */
        static constexpr jam::LookupTable<int, int, Keyword::test + 1> roles {
            {
                { Keyword::requirement,            0 },
                { Keyword::functionalRequirement,  0 },
                { Keyword::interfaceRequirement,   0 },
                { Keyword::performanceRequirement, 0 },
                { Keyword::physicalRequirement,    0 },
                { Keyword::designConstraint,       0 },
                { Keyword::element,                0 },
                { Keyword::contains,               1 },
                { Keyword::copies,                 1 },
                { Keyword::derives,                1 },
                { Keyword::satisfies,              1 },
                { Keyword::verifies,               1 },
                { Keyword::refines,                1 },
                { Keyword::traces,                 1 },
                { Keyword::id,                     2 },
                { Keyword::text,                   2 },
                { Keyword::risk,                   2 },
                { Keyword::verifymethod,           2 },
                { Keyword::type,                   2 },
                { Keyword::docref,                 2 },
                { Keyword::low,                    3 },
                { Keyword::medium,                 3 },
                { Keyword::high,                   3 },
                { Keyword::analysis,               4 },
                { Keyword::demonstration,          4 },
                { Keyword::inspection,             4 },
                { Keyword::test,                   4 },
            }
        };

        //==============================================================================

        /**
         * @brief Requirement element kind → stereotype label.
         *
         * Maps the requirement element kinds to the stereotype text drawn above
         * the element node ("Requirement", "Functional Requirement", …).
         */
        static constexpr jam::LookupTable<int, const char*, 7> stereotypes {
            {
                { Keyword::requirement,            "Requirement" },
                { Keyword::functionalRequirement,  "Functional Requirement" },
                { Keyword::interfaceRequirement,   "Interface Requirement" },
                { Keyword::performanceRequirement, "Performance Requirement" },
                { Keyword::physicalRequirement,    "Physical Requirement" },
                { Keyword::designConstraint,       "Design Constraint" },
                { Keyword::element,                "Element" },
            }
        };

        //==============================================================================

        /**
         * @brief Requirement attribute/risk/method keyword → display label.
         *
         * Maps the attribute names, risk levels, and verification methods to
         * the human-readable label drawn on the requirement node.
         */
        static constexpr jam::LookupTable<int, const char*, Keyword::test + 1> attributeLabels {
            {
                { Keyword::id,            "ID" },
                { Keyword::text,          "Text" },
                { Keyword::risk,          "Risk" },
                { Keyword::verifymethod,  "Verification" },
                { Keyword::type,          "Type" },
                { Keyword::docref,        "Doc Ref" },
                { Keyword::low,           "Low" },
                { Keyword::medium,        "Medium" },
                { Keyword::high,          "High" },
                { Keyword::analysis,      "Analysis" },
                { Keyword::demonstration, "Demonstration" },
                { Keyword::inspection,    "Inspection" },
                { Keyword::test,          "Test" },
            }
        };
    };

    //==============================================================================

    /**
     * @brief Railroad-diagram term types and leaf-colour tables.
     *
     * Groups the railroad term-type bimap with the tables that map each term
     * type to its leaf fill/border/text colour ids, and the sentinel used for
     * unbounded repetition.
     */
    struct Railroad
    {
        /**
         * @brief Railroad term-type keywords.
         *
         * Maps the railroad term kinds (`terminal`, `nonterminal`, `sequence`,
         * `choice`, `optional`, `repetition`, `special`) to their keys.
         */
        struct TermType : public jam::Bimap<int>
        {
            TermType() : jam::Bimap<int> { {
                    { terminal,    juce::String::fromUTF8 ("terminal") },
                    { nonterminal, juce::String::fromUTF8 ("nonterminal") },
                    { sequence,    juce::String::fromUTF8 ("sequence") },
                    { choice,      juce::String::fromUTF8 ("choice") },
                    { optional,    juce::String::fromUTF8 ("optional") },
                    { repetition,  juce::String::fromUTF8 ("repetition") },
                    { special,     juce::String::fromUTF8 ("special") },
            } } {}

            enum value : int
            {
                terminal    = 0,
                nonterminal = 1,
                sequence    = 2,
                choice      = 3,
                optional    = 4,
                repetition  = 5,
                special     = 6,
            };

            static TermType* getInstance() noexcept
            {
                return jam::SharedInstance<TermType>::getInstance();
            }
        };

        //==============================================================================

        /**
         * @brief Term type → leaf fill colour id.
         *
         * Maps the railroad leaf term types (terminal, nonterminal, special) to
         * their `StyleSheet` fill colour id.
         */
        static constexpr jam::LookupTable<int, int, TermType::special + 1> leafFillColourIds {
            {
                { TermType::nonterminal, map::ColourId::mermaidDiagramRailroadNonTerminalFillColourId },
                { TermType::terminal, map::ColourId::mermaidDiagramRailroadTerminalFillColourId },
                { TermType::special, map::ColourId::mermaidDiagramRailroadSpecialFillColourId },
            }
        };

        //==============================================================================

        /**
         * @brief Term type → leaf border colour id.
         *
         * Maps the railroad leaf term types to their `StyleSheet` border colour
         * id.
         */
        static constexpr jam::LookupTable<int, int, TermType::special + 1> leafBorderColourIds {
            {
                { TermType::nonterminal, map::ColourId::mermaidDiagramRailroadNonTerminalBorderColourId },
                { TermType::terminal, map::ColourId::mermaidDiagramRailroadTerminalBorderColourId },
                { TermType::special, map::ColourId::mermaidDiagramRailroadSpecialBorderColourId },
            }
        };

        //==============================================================================

        /**
         * @brief Term type → leaf text colour id.
         *
         * Maps the railroad leaf term types to their `StyleSheet` text colour
         * id.
         */
        static constexpr jam::LookupTable<int, int, TermType::special + 1> leafTextColourIds {
            {
                { TermType::nonterminal, map::ColourId::mermaidDiagramRailroadNonTerminalTextColourId },
                { TermType::terminal, map::ColourId::mermaidDiagramRailroadTerminalTextColourId },
                { TermType::special, map::ColourId::mermaidDiagramRailroadNonTerminalTextColourId },
            }
        };

    //==============================================================================

        static constexpr int unboundedRepetition { -1 };///< Sentinel marking a repetition with no upper bound.
    };
};
