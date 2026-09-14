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
 * @file jam_Generated.h
 * @brief Generated-header umbrella — re-exports and instantiates every registry.
 *
 * The Generated struct aggregates the framework's generated shared-instance
 * registries: constructing it constructs every generated bimap exactly once,
 * giving a single point of ownership for the whole generated vocabulary.
 */

#pragma once

#include "jam_Identifiers.h"
#include "jam_Text.h"
#include "jam_Chars.h"
#include "jam_Files.h"
#include "jam_Bimaps.h"
#include "jam_Terminal.h"
#include "jam_Colours.h"
#include "jam_HashMaps.h"
#include "jam_LookupTables.h"
#include "jam_Mermaid.h"

namespace map
{
/*_____________________________________________________________________________*/

struct Generated
{
    jam::SharedInstance<TaperMap>                              taperMap                   { std::in_place };///< Curve taper values.
    jam::SharedInstance<XmlTokenType>                          xmlTokenType               { std::in_place };///< XML token kinds.
    jam::SharedInstance<Segment>                               segment                    { std::in_place };///< UI nine-segment anchors.
    jam::SharedInstance<ButtonState>                           buttonState                { std::in_place };///< Button visual states.
    jam::SharedInstance<Position>                              position                   { std::in_place };///< Layout position anchors.
    jam::SharedInstance<Orientation>                           orientation                { std::in_place };///< Axis orientation.
    jam::SharedInstance<WindowFX>                              windowFX                   { std::in_place };///< Native window effects.
    jam::SharedInstance<BlockType>                             blockType                  { std::in_place };///< Markdown block kinds.
    jam::SharedInstance<MarkdownTokenType>                     markdownTokenType          { std::in_place };///< Markdown inline-token kinds.
    jam::SharedInstance<DocumentTokenType>                     documentTokenType          { std::in_place };///< Document token kinds.
    jam::SharedInstance<Byte>                                  byte                       { std::in_place };///< Byte classification.
    jam::SharedInstance<HeadingLevel>                          headingLevel               { std::in_place };///< Heading levels one to six.
    jam::SharedInstance<HtmlType1Tag>                          htmlType1Tag               { std::in_place };///< Raw-text HTML tag kinds.
    jam::SharedInstance<HtmlBlockTag>                          htmlBlockTag               { std::in_place };///< Block-level HTML tags.
    jam::SharedInstance<SyntaxTokenType>                       syntaxTokenType            { std::in_place };///< Syntax-highlight token kinds.
    jam::SharedInstance<Family>                                family                     { std::in_place };///< Language family classification.
    jam::SharedInstance<OpenBlock>                             openBlock                  { std::in_place };///< Markdown block-open constructs.
    jam::SharedInstance<CppKeyword>                            cppKeyword                 { std::in_place };///< C++ keyword set.
    jam::SharedInstance<JsKeyword>                             jsKeyword                  { std::in_place };///< JavaScript keyword set.
    jam::SharedInstance<PythonKeyword>                         pythonKeyword              { std::in_place };///< Python keyword set.
    jam::SharedInstance<CssTokenType>                          cssTokenType               { std::in_place };///< CSS token kinds.
    jam::SharedInstance<CssRuleType>                           cssRuleType                { std::in_place };///< CSS at-rule kinds.
    jam::SharedInstance<HtmlTokenType>                         htmlTokenType              { std::in_place };///< HTML token kinds.
    jam::SharedInstance<HtmlVoidTag>                           htmlVoidTag                { std::in_place };///< HTML void-element tags.
    jam::SharedInstance<VariDisplayMode>                       variDisplayMode            { std::in_place };///< Vari display modes.
    jam::SharedInstance<PluginWrapper>                         pluginWrapper              { std::in_place };///< Plugin wrapper formats.
    jam::SharedInstance<AnalyzerMode>                          analyzerMode               { std::in_place };///< Spectrum analyzer modes.
    jam::SharedInstance<Appearance>                            appearance                 { std::in_place };///< Application appearance themes.
    jam::SharedInstance<AudioParameter>                        audioParameter             { std::in_place };///< Audio parameter kinds.
    jam::SharedInstance<Display>                               display                    { std::in_place };///< Parameter display modes.
    jam::SharedInstance<FontRasterizerBackend>                 fontRasterizerBackend      { std::in_place };///< Font rasterizer backends.
    jam::SharedInstance<ImageResample>                         imageResample              { std::in_place };///< Image resample filters.
    jam::SharedInstance<MouseButton>                           mouseButton                { std::in_place };///< Mouse buttons.
    jam::SharedInstance<Oversampling>                          oversampling               { std::in_place };///< Oversampling factors.
    jam::SharedInstance<UIScaleMap>                            UIScaleMap                 { std::in_place };///< UI scale presets.
    jam::SharedInstance<HtmlStandardTag>                       htmlStandardTag            { std::in_place };///< Standard HTML element tags.
    jam::SharedInstance<ParameterPage>                         parameterPage              { std::in_place };///< Parameter page labels.
    jam::SharedInstance<ViewOrientation>                       viewOrientation            { std::in_place };///< Device view orientations.
    jam::SharedInstance<AtRuleType>                            atRuleType                 { std::in_place };///< CSS at-rule discriminators.
    jam::SharedInstance<BlockTag>                              blockTag                   { std::in_place };///< Markdown block to HTML tag map.
    jam::SharedInstance<HeadingTag>                            headingTag                 { std::in_place };///< Heading level to HTML tag map.
    jam::SharedInstance<Sharps>                                sharps                     { std::in_place };///< Sharp key names.
    jam::SharedInstance<Flats>                                 flats                      { std::in_place };///< Flat key names.
    jam::SharedInstance<HtmlBlockType>                         htmlBlockType              { std::in_place };///< HTML block start-condition kinds.
    jam::SharedInstance<Screen>                                screen                     { std::in_place };///< Terminal screen buffer mode.
    jam::SharedInstance<MouseTracking>                         mouseTracking              { std::in_place };///< Terminal mouse-tracking modes.
    jam::SharedInstance<DEC>                                   DEC                        { std::in_place };///< DEC private mode numbers.
    jam::SharedInstance<OSC>                                   OSC                        { std::in_place };///< OSC command codes.
    jam::SharedInstance<SGR>                                   SGR                        { std::in_place };///< SGR rendition parameters.
    jam::SharedInstance<ColorMode>                             colorMode                  { std::in_place };///< SGR extended-colour modes.
    jam::SharedInstance<UnderlineStyle>                        underlineStyle             { std::in_place };///< SGR underline styles.
    jam::SharedInstance<CSI>                                   CSI                        { std::in_place };///< CSI final-byte commands.
    jam::SharedInstance<WindowOps>                             windowOps                  { std::in_place };///< CSI window-operation codes.
    jam::SharedInstance<DSR>                                   DSR                        { std::in_place };///< DSR sub-command codes.
    jam::SharedInstance<TabClear>                              tabClear                   { std::in_place };///< Tab-clear sub-mode codes.
    jam::SharedInstance<CursorShape>                           cursorShape                { std::in_place };///< Cursor shape codes.
    jam::SharedInstance<DECRQSS>                               DECRQSS                    { std::in_place };///< DECRQSS setting codes.
    jam::SharedInstance<CsiIntermediate>                       csiIntermediate            { std::in_place };///< CSI intermediate bytes.
    jam::SharedInstance<ESC>                                   ESC                        { std::in_place };///< ESC final-byte sequences.
    jam::SharedInstance<CharsetIntermediate>                   charsetIntermediate        { std::in_place };///< Charset-select intermediate bytes.
    jam::SharedInstance<CharsetDesignator>                     charsetDesignator          { std::in_place };///< Charset-designator final bytes.
    jam::SharedInstance<DecEscIntermediate>                    decEscIntermediate         { std::in_place };///< DEC ESC intermediate byte.
    jam::SharedInstance<DecEscFinal>                           decEscFinal                { std::in_place };///< DEC ESC final byte.
    jam::SharedInstance<ModeReport>                            modeReport                 { std::in_place };///< DECRQM response states.
    jam::SharedInstance<ANSI>                                  ANSI                       { std::in_place };///< ANSI mode numbers.
    jam::SharedInstance<ShellIntegration>                      shellIntegration           { std::in_place };///< OSC 133 sub-commands.
    jam::SharedInstance<KeyboardAssignMode>                    keyboardAssignMode         { std::in_place };///< Keyboard flag-assignment modes.
    jam::SharedInstance<ColourNames>                           colourNames                { std::in_place };///< Named colour palette.
    jam::SharedInstance<ColourId>                              colourId                   { std::in_place };///< Component colour-id to CSS variable-name map.
    jam::SharedInstance<Mermaid::DiagramType>                  diagramType                { std::in_place };///< Mermaid diagram kinds.
    jam::SharedInstance<Mermaid::NodeShape>                    nodeShape                  { std::in_place };///< Mermaid node shapes.
    jam::SharedInstance<Mermaid::Flowchart::Keyword>           flowchartKeyword           { std::in_place };///< Mermaid flowchart direction keywords.
    jam::SharedInstance<Mermaid::EdgeStroke>                   edgeStroke                 { std::in_place };///< Mermaid edge stroke styles.
    jam::SharedInstance<Mermaid::EdgeDecoration>               edgeDecoration             { std::in_place };///< Mermaid edge decorations.
    jam::SharedInstance<Mermaid::MarkerPaint>                  markerPaint                { std::in_place };///< Mermaid marker paints.
    jam::SharedInstance<Mermaid::StyleSheet>                   mermaidStyleSheet          { std::in_place };///< Mermaid stylesheet keys.
    jam::SharedInstance<Mermaid::Operator>                     mermaidOperator            { std::in_place };///< Mermaid operators.
    jam::SharedInstance<Mermaid::TokenType>                    mermaidTokenType           { std::in_place };///< Mermaid token kinds.
    jam::SharedInstance<Mermaid::GroupType>                    groupType                  { std::in_place };///< Mermaid group kinds across diagram families.
    jam::SharedInstance<Mermaid::GroupLayoutPolicy>            groupLayoutPolicy          { std::in_place };///< Mermaid group layout-ownership policies.
    jam::SharedInstance<Mermaid::Shape::ConstructionPolicy>    shapeConstructionPolicy    { std::in_place };///< Mermaid node-shape outline construction policies.
    jam::SharedInstance<Mermaid::Shape::HitTestClassification> shapeHitTestClassification { std::in_place };///< Mermaid node-shape hit-test predicates.
    jam::SharedInstance<Mermaid::Shape::SizeAdjustPolicy>      shapeSizeAdjustPolicy      { std::in_place };///< Mermaid node-shape size-adjust policies.
    jam::SharedInstance<Mermaid::Shape::MindmapSizePolicy>     shapeMindmapSizePolicy     { std::in_place };///< Mermaid mindmap node-size policies.
    jam::SharedInstance<Mermaid::Shape::BlockSizePolicy>       shapeBlockSizePolicy       { std::in_place };///< Mermaid block-diagram node-size policies.
    jam::SharedInstance<Mermaid::C4::Keyword>                  C4Keyword                  { std::in_place };///< Mermaid C4 element/relation/boundary keywords.
    jam::SharedInstance<Mermaid::Sequence::Keyword>            sequenceKeyword            { std::in_place };///< Mermaid sequence frame keywords.
    jam::SharedInstance<Mermaid::Git::Keyword>                 gitKeyword                 { std::in_place };///< Mermaid git commit kinds.
    jam::SharedInstance<Mermaid::Gantt::Keyword>               ganttKeyword               { std::in_place };///< Mermaid gantt task status and directive keywords.
    jam::SharedInstance<Mermaid::Kanban::Keyword>              kanbanKeyword              { std::in_place };///< Mermaid kanban priorities.
    jam::SharedInstance<Mermaid::Architecture::Keyword>        architectureKeyword        { std::in_place };///< Mermaid architecture keywords.
    jam::SharedInstance<Mermaid::Xy::Keyword>                  xyKeyword                  { std::in_place };///< Mermaid XY-chart series kinds.
    jam::SharedInstance<Mermaid::Quadrant::Keyword>            quadrantKeyword            { std::in_place };///< Mermaid quadrant label keywords.
    jam::SharedInstance<Mermaid::Requirement::Keyword>         requirementKeyword         { std::in_place };///< Mermaid requirement keywords.
    jam::SharedInstance<Mermaid::Railroad::TermType>           railroadTermType           { std::in_place };///< Mermaid railroad diagram term kinds.
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace map
