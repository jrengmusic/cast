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

#pragma once

namespace map
{
/*_____________________________________________________________________________*/

/**
 * @brief Curve taper values — the log/alog series plus linear and skew.
 *
 * Maps each taper preset to the exponent that shapes the parameter curve.
 * Positive keys are logarithmic (log10 = 10), negative keys are their
 * anti-log mirrors (alog10 = -10), and `skew` is the 1000-slot out-of-band
 * sentinel. Consumed by the taper evaluator to shape a normalized input.
 */
struct TaperMap : public jam::Bimap<int>
{
    TaperMap() : jam::Bimap<int> { {
            { linear, juce::String::fromUTF8 ("linear") },
            { skew,   juce::String::fromUTF8 ("skew") },
            { log45,  juce::String::fromUTF8 ("log45") },
            { log40,  juce::String::fromUTF8 ("log40") },
            { log35,  juce::String::fromUTF8 ("log35") },
            { log30,  juce::String::fromUTF8 ("log30") },
            { log25,  juce::String::fromUTF8 ("log25") },
            { log20,  juce::String::fromUTF8 ("log20") },
            { log15,  juce::String::fromUTF8 ("log15") },
            { log10,  juce::String::fromUTF8 ("log10") },
            { log5,   juce::String::fromUTF8 ("log5") },
            { log4,   juce::String::fromUTF8 ("log4") },
            { log3,   juce::String::fromUTF8 ("log3") },
            { log2,   juce::String::fromUTF8 ("log2") },
            { log1,   juce::String::fromUTF8 ("log1") },
            { alog1,  juce::String::fromUTF8 ("alog1") },
            { alog2,  juce::String::fromUTF8 ("alog2") },
            { alog3,  juce::String::fromUTF8 ("alog3") },
            { alog4,  juce::String::fromUTF8 ("alog4") },
            { alog5,  juce::String::fromUTF8 ("alog5") },
            { alog10, juce::String::fromUTF8 ("alog10") },
            { alog15, juce::String::fromUTF8 ("alog15") },
            { alog20, juce::String::fromUTF8 ("alog20") },
            { alog25, juce::String::fromUTF8 ("alog25") },
            { alog30, juce::String::fromUTF8 ("alog30") },
            { alog35, juce::String::fromUTF8 ("alog35") },
            { alog40, juce::String::fromUTF8 ("alog40") },
            { alog45, juce::String::fromUTF8 ("alog45") },
    } } {}

    enum value : int
    {
        linear = 0,
        skew   = 1000,
        log45  = 45,
        log40  = 40,
        log35  = 35,
        log30  = 30,
        log25  = 25,
        log20  = 20,
        log15  = 15,
        log10  = 10,
        log5   = 5,
        log4   = 4,
        log3   = 3,
        log2   = 2,
        log1   = 1,
        alog1  = -1,
        alog2  = -2,
        alog3  = -3,
        alog4  = -4,
        alog5  = -5,
        alog10 = -10,
        alog15 = -15,
        alog20 = -20,
        alog25 = -25,
        alog30 = -30,
        alog35 = -35,
        alog40 = -40,
        alog45 = -45,
    };

    static TaperMap* getInstance() noexcept
    {
        return jam::SharedInstance<TaperMap>::getInstance();
    }
};

//==============================================================================

/**
 * @brief XML token kinds — start tag, end tag, text, processing instruction, declaration, and comment.
 *
 * Classifies each token the XML tokeniser emits. `startTag` and `endTag`
 * bracket an element; `text` is the character data between them.
 * `processingInstruction` and `declaration` are markup instructions discarded
 * during tree construction; `comment` is a `<!-- -->` span likewise discarded.
 * The key is the discriminator the parser dispatches on when building the
 * element tree.
 */
struct XmlTokenType : public jam::Bimap<int>
{
    XmlTokenType() : jam::Bimap<int> { {
            { startTag,              juce::String::fromUTF8 ("startTag") },
            { endTag,                juce::String::fromUTF8 ("endTag") },
            { text,                  juce::String::fromUTF8 ("text") },
            { processingInstruction, juce::String::fromUTF8 ("processingInstruction") },
            { declaration,           juce::String::fromUTF8 ("declaration") },
            { comment,               juce::String::fromUTF8 ("comment") },
    } } {}

    enum value : int
    {
        startTag              = 0,
        endTag                = 1,
        text                  = 2,
        processingInstruction = 3,
        declaration           = 4,
        comment               = 5,
    };

    static XmlTokenType* getInstance() noexcept
    {
        return jam::SharedInstance<XmlTokenType>::getInstance();
    }
};

//==============================================================================

/**
 * @brief UI nine-segment anchor positions.
 *
 * Maps each of the nine slice anchors (centre plus the four corners and four
 * edges) to its key. Consumed by nine-slice resizing to pick which regions
 * stretch and which stay fixed when a component is scaled.
 */
struct Segment : public jam::Bimap<int>
{
    Segment() : jam::Bimap<int> { {
            { centre,      juce::String::fromUTF8 ("centre") },
            { topLeft,     juce::String::fromUTF8 ("topLeft") },
            { top,         juce::String::fromUTF8 ("top") },
            { topRight,    juce::String::fromUTF8 ("topRight") },
            { left,        juce::String::fromUTF8 ("left") },
            { right,       juce::String::fromUTF8 ("right") },
            { bottomLeft,  juce::String::fromUTF8 ("bottomLeft") },
            { bottom,      juce::String::fromUTF8 ("bottom") },
            { bottomRight, juce::String::fromUTF8 ("bottomRight") },
    } } {}

    enum value : int
    {
        centre      = 0,
        topLeft     = 1,
        top         = 2,
        topRight    = 3,
        left        = 4,
        right       = 5,
        bottomLeft  = 6,
        bottom      = 7,
        bottomRight = 8,
    };

    static Segment* getInstance() noexcept
    {
        return jam::SharedInstance<Segment>::getInstance();
    }
};

//==============================================================================

/**
 * @brief Button visual states — base states and their toggled-on combinations.
 *
 * Maps each of the eight button states (normal, over, down, disabled, and the
 * `…On` variants of each) to its key. The key indexes the per-state image or
 * style the button draws for a given mouse/toggle combination.
 */
struct ButtonState : public jam::Bimap<int>
{
    ButtonState() : jam::Bimap<int> { {
            { normal,     juce::String::fromUTF8 ("normal") },
            { over,       juce::String::fromUTF8 ("over") },
            { down,       juce::String::fromUTF8 ("down") },
            { disabled,   juce::String::fromUTF8 ("disabled") },
            { normalOn,   juce::String::fromUTF8 ("normalOn") },
            { overOn,     juce::String::fromUTF8 ("overOn") },
            { downOn,     juce::String::fromUTF8 ("downOn") },
            { disabledOn, juce::String::fromUTF8 ("disabledOn") },
    } } {}

    enum value : int
    {
        normal     = 0,
        over       = 1,
        down       = 2,
        disabled   = 3,
        normalOn   = 4,
        overOn     = 5,
        downOn     = 6,
        disabledOn = 7,
    };

    static ButtonState* getInstance() noexcept
    {
        return jam::SharedInstance<ButtonState>::getInstance();
    }
};

//==============================================================================

/**
 * @brief Layout position anchors — top/bottom/left/right/centre.
 *
 * Maps each anchor name to its key, consumed by layout code to align a child
 * within its parent's bounds.
 */
struct Position : public jam::Bimap<int>
{
    Position() : jam::Bimap<int> { {
            { bottom, juce::String::fromUTF8 ("bottom") },
            { top,    juce::String::fromUTF8 ("top") },
            { right,  juce::String::fromUTF8 ("right") },
            { left,   juce::String::fromUTF8 ("left") },
            { center, juce::String::fromUTF8 ("center") },
    } } {}

    enum value : int
    {
        bottom = 0,
        top    = 1,
        right  = 2,
        left   = 3,
        center = 4,
    };

    static Position* getInstance() noexcept
    {
        return jam::SharedInstance<Position>::getInstance();
    }
};

//==============================================================================

/**
 * @brief Axis orientation — vertical or horizontal.
 *
 * Maps each orientation name to its key, consumed by layout and draw code to
 * select the primary axis along which a list or control flows.
 */
struct Orientation : public jam::Bimap<int>
{
    Orientation() : jam::Bimap<int> { {
            { vertical,   juce::String::fromUTF8 ("vertical") },
            { horizontal, juce::String::fromUTF8 ("horizontal") },
    } } {}

    enum value : int
    {
        vertical   = 0,
        horizontal = 1,
    };

    static Orientation* getInstance() noexcept
    {
        return jam::SharedInstance<Orientation>::getInstance();
    }
};

//==============================================================================

/**
 * @brief Native window effects, per platform.
 *
 * Maps each window-effect name to its key. The entries are compiled
 * conditionally: macOS exposes background-blur and glass effects, Windows
 * exposes blur-behind and acrylic. Consumed by the native windowing layer to
 * request the matching platform effect.
 */
struct WindowFX : public jam::Bimap<int>
{
    WindowFX() : jam::Bimap<int> { {
#if JUCE_MAC
            { backgroundBlur,           juce::String::fromUTF8 ("backgroundBlur") },
            { visualFXWindowBackground, juce::String::fromUTF8 ("visualFXWindowBackground") },
            { glassFXRegular,           juce::String::fromUTF8 ("glassFXRegular") },
            { glassFXClear,             juce::String::fromUTF8 ("glassFXClear") },
#elif JUCE_WINDOWS
            { blurBehind, juce::String::fromUTF8 ("blurBehind") },
            { acrylic,    juce::String::fromUTF8 ("acrylic") },
#elif JUCE_LINUX
#endif
    } } {}

    enum value : int
    {
#if JUCE_MAC
        backgroundBlur           = 0,
        visualFXWindowBackground = 1,
        glassFXRegular           = 2,
        glassFXClear             = 3,
#elif JUCE_WINDOWS
        blurBehind = 0,
        acrylic    = 1,
#elif JUCE_LINUX
#endif
    };

    static WindowFX* getInstance() noexcept
    {
        return jam::SharedInstance<WindowFX>::getInstance();
    }
};

//==============================================================================

/**
 * @brief Markdown block element kinds.
 *
 * Maps each block-level construct the Markdown parser produces — paragraph,
 * heading, blockquote, code block, list and its items, thematic break, HTML
 * block, table and its rows/cells, mermaid diagram, and image — to its key.
 * The key is the block discriminator the document model stores per node.
 */
struct BlockType : public jam::Bimap<int>
{
    BlockType() : jam::Bimap<int> { {
            { paragraph,     juce::String::fromUTF8 ("paragraph") },
            { document,      juce::String::fromUTF8 ("document") },
            { heading,       juce::String::fromUTF8 ("heading") },
            { blockquote,    juce::String::fromUTF8 ("blockquote") },
            { codeBlock,     juce::String::fromUTF8 ("codeBlock") },
            { list,          juce::String::fromUTF8 ("list") },
            { listItem,      juce::String::fromUTF8 ("listItem") },
            { thematicBreak, juce::String::fromUTF8 ("thematicBreak") },
            { htmlBlock,     juce::String::fromUTF8 ("htmlBlock") },
            { table,         juce::String::fromUTF8 ("table") },
            { tableRow,      juce::String::fromUTF8 ("tableRow") },
            { tableCell,     juce::String::fromUTF8 ("tableCell") },
            { mermaid,       juce::String::fromUTF8 ("mermaid") },
            { image,         juce::String::fromUTF8 ("image") },
            { tableBorder,   juce::String::fromUTF8 ("tableBorder") },
    } } {}

    enum value : int
    {
        paragraph     = 0,
        document      = 1,
        heading       = 2,
        blockquote    = 3,
        codeBlock     = 4,
        list          = 5,
        listItem      = 6,
        thematicBreak = 7,
        htmlBlock     = 8,
        table         = 9,
        tableRow      = 10,
        tableCell     = 11,
        mermaid       = 12,
        image         = 13,
        tableBorder   = 14,
    };

    static BlockType* getInstance() noexcept
    {
        return jam::SharedInstance<BlockType>::getInstance();
    }
};

//==============================================================================

/**
 * @brief Markdown inline-token kinds.
 *
 * Maps each inline construct the Markdown parser emits — text, character
 * reference, code span, emphasis delimiter, link open/close, image open,
 * autolink, raw HTML, and line break — to its key. The key is the token
 * discriminator consumed when building inline spans.
 */
struct MarkdownTokenType : public jam::Bimap<int>
{
    MarkdownTokenType() : jam::Bimap<int> { {
            { text,               juce::String::fromUTF8 ("text") },
            { characterReference, juce::String::fromUTF8 ("characterReference") },
            { codeSpan,           juce::String::fromUTF8 ("codeSpan") },
            { emphasisDelimiter,  juce::String::fromUTF8 ("emphasisDelimiter") },
            { linkOpen,           juce::String::fromUTF8 ("linkOpen") },
            { imageOpen,          juce::String::fromUTF8 ("imageOpen") },
            { linkClose,          juce::String::fromUTF8 ("linkClose") },
            { autolink,           juce::String::fromUTF8 ("autolink") },
            { rawHtml,            juce::String::fromUTF8 ("rawHtml") },
            { lineBreak,          juce::String::fromUTF8 ("lineBreak") },
    } } {}

    enum value : int
    {
        text               = 0,
        characterReference = 1,
        codeSpan           = 2,
        emphasisDelimiter  = 3,
        linkOpen           = 4,
        imageOpen          = 5,
        linkClose          = 6,
        autolink           = 7,
        rawHtml            = 8,
        lineBreak          = 9,
    };

    static MarkdownTokenType* getInstance() noexcept
    {
        return jam::SharedInstance<MarkdownTokenType>::getInstance();
    }
};

//==============================================================================

/**
 * @brief Document token kinds — text, operators, region, comment.
 *
 * Classifies each token the generic document scanner emits. `operators` covers
 * punctuation and structural delimiters; `region` marks region open/close
 * bracketing; `comment` marks comment content. The key drives syntax
 * highlighting and token dispatch.
 */
struct DocumentTokenType : public jam::Bimap<int>
{
    DocumentTokenType() : jam::Bimap<int> { {
            { text,      juce::String::fromUTF8 ("text") },
            { operators, juce::String::fromUTF8 ("operators") },
            { region,    juce::String::fromUTF8 ("region") },
            { comment,   juce::String::fromUTF8 ("comment") },
    } } {}

    enum value : int
    {
        text      = 0,
        operators = 1,
        region    = 2,
        comment   = 3,
    };

    static DocumentTokenType* getInstance() noexcept
    {
        return jam::SharedInstance<DocumentTokenType>::getInstance();
    }
};

//==============================================================================

/**
 * @brief Byte classification for syntax scanning.
 *
 * Buckets each byte the scanners look at into text, operator, region-open,
 * region-close, or quote. The key is the classification the byte-to-class
 * lookup tables store per codepoint, letting the tokeniser skip re-testing a
 * byte on every pass.
 */
struct Byte : public jam::Bimap<int>
{
    Byte() : jam::Bimap<int> { {
            { text,        juce::String::fromUTF8 ("text") },
            { operators,   juce::String::fromUTF8 ("operators") },
            { regionOpen,  juce::String::fromUTF8 ("regionOpen") },
            { regionClose, juce::String::fromUTF8 ("regionClose") },
            { quote,       juce::String::fromUTF8 ("quote") },
    } } {}

    enum value : int
    {
        text        = 0,
        operators   = 1,
        regionOpen  = 2,
        regionClose = 3,
        quote       = 4,
    };

    static Byte* getInstance() noexcept
    {
        return jam::SharedInstance<Byte>::getInstance();
    }
};

//==============================================================================

/**
 * @brief Heading levels one through six.
 *
 * Maps each heading level to its numeric key, which doubles as the heading
 * depth the document model records and the renderer uses for font sizing.
 */
struct HeadingLevel : public jam::Bimap<int>
{
    HeadingLevel() : jam::Bimap<int> { {
            { level1, juce::String::fromUTF8 ("level1") },
            { level2, juce::String::fromUTF8 ("level2") },
            { level3, juce::String::fromUTF8 ("level3") },
            { level4, juce::String::fromUTF8 ("level4") },
            { level5, juce::String::fromUTF8 ("level5") },
            { level6, juce::String::fromUTF8 ("level6") },
    } } {}

    enum value : int
    {
        level1 = 1,
        level2 = 2,
        level3 = 3,
        level4 = 4,
        level5 = 5,
        level6 = 6,
    };

    static HeadingLevel* getInstance() noexcept
    {
        return jam::SharedInstance<HeadingLevel>::getInstance();
    }
};

//==============================================================================

/**
 * @brief Raw-text HTML tag kinds (script/pre/textarea/style).
 *
 * Maps the four HTML elements whose content is treated as raw text rather
 * than parsed markup to their keys. The key indexes the closing-tag table
 * (`map::rawTextEnd`) that terminates the raw-text run.
 */
struct HtmlType1Tag : public jam::Bimap<int>
{
    HtmlType1Tag() : jam::Bimap<int> { {
            { script,   juce::String::fromUTF8 ("script") },
            { pre,      juce::String::fromUTF8 ("pre") },
            { textarea, juce::String::fromUTF8 ("textarea") },
            { style,    juce::String::fromUTF8 ("style") },
    } } {}

    enum value : int
    {
        script   = 0,
        pre      = 1,
        textarea = 2,
        style    = 3,
    };

    static HtmlType1Tag* getInstance() noexcept
    {
        return jam::SharedInstance<HtmlType1Tag>::getInstance();
    }
};

//==============================================================================

/**
 * @brief Block-level HTML element tags.
 *
 * Maps each block-level HTML tag name to its key. The key is the tag
 * discriminator used when the HTML parser classifies a start tag as a block
 * element; the ordinal values match the CommonMark block-tag table.
 */
struct HtmlBlockTag : public jam::Bimap<int>
{
    HtmlBlockTag() : jam::Bimap<int> { {
            { address,    juce::String::fromUTF8 ("address") },
            { article,    juce::String::fromUTF8 ("article") },
            { aside,      juce::String::fromUTF8 ("aside") },
            { basefont,   juce::String::fromUTF8 ("basefont") },
            { blockquote, juce::String::fromUTF8 ("blockquote") },
            { body,       juce::String::fromUTF8 ("body") },
            { caption,    juce::String::fromUTF8 ("caption") },
            { center,     juce::String::fromUTF8 ("center") },
            { colgroup,   juce::String::fromUTF8 ("colgroup") },
            { dd,         juce::String::fromUTF8 ("dd") },
            { details,    juce::String::fromUTF8 ("details") },
            { dialog,     juce::String::fromUTF8 ("dialog") },
            { dir,        juce::String::fromUTF8 ("dir") },
            { dl,         juce::String::fromUTF8 ("dl") },
            { dt,         juce::String::fromUTF8 ("dt") },
            { fieldset,   juce::String::fromUTF8 ("fieldset") },
            { figcaption, juce::String::fromUTF8 ("figcaption") },
            { figure,     juce::String::fromUTF8 ("figure") },
            { footer,     juce::String::fromUTF8 ("footer") },
            { form,       juce::String::fromUTF8 ("form") },
            { frame,      juce::String::fromUTF8 ("frame") },
            { frameset,   juce::String::fromUTF8 ("frameset") },
            { h1,         juce::String::fromUTF8 ("h1") },
            { h2,         juce::String::fromUTF8 ("h2") },
            { h3,         juce::String::fromUTF8 ("h3") },
            { h4,         juce::String::fromUTF8 ("h4") },
            { h5,         juce::String::fromUTF8 ("h5") },
            { h6,         juce::String::fromUTF8 ("h6") },
            { head,       juce::String::fromUTF8 ("head") },
            { header,     juce::String::fromUTF8 ("header") },
            { html,       juce::String::fromUTF8 ("html") },
            { iframe,     juce::String::fromUTF8 ("iframe") },
            { legend,     juce::String::fromUTF8 ("legend") },
            { li,         juce::String::fromUTF8 ("li") },
            { menu,       juce::String::fromUTF8 ("menu") },
            { menuitem,   juce::String::fromUTF8 ("menuitem") },
            { nav,        juce::String::fromUTF8 ("nav") },
            { noframes,   juce::String::fromUTF8 ("noframes") },
            { ol,         juce::String::fromUTF8 ("ol") },
            { optgroup,   juce::String::fromUTF8 ("optgroup") },
            { option,     juce::String::fromUTF8 ("option") },
            { lowerP,     juce::String::fromUTF8 ("p") },
            { param,      juce::String::fromUTF8 ("param") },
            { section,    juce::String::fromUTF8 ("section") },
            { search,     juce::String::fromUTF8 ("search") },
            { title,      juce::String::fromUTF8 ("title") },
            { summary,    juce::String::fromUTF8 ("summary") },
            { table,      juce::String::fromUTF8 ("table") },
            { tbody,      juce::String::fromUTF8 ("tbody") },
            { td,         juce::String::fromUTF8 ("td") },
            { tfoot,      juce::String::fromUTF8 ("tfoot") },
            { th,         juce::String::fromUTF8 ("th") },
            { thead,      juce::String::fromUTF8 ("thead") },
            { tr,         juce::String::fromUTF8 ("tr") },
            { ul,         juce::String::fromUTF8 ("ul") },
    } } {}

    enum value : int
    {
        address    = 0,
        article    = 1,
        aside      = 2,
        basefont   = 4,
        blockquote = 5,
        body       = 6,
        caption    = 7,
        center     = 8,
        colgroup   = 10,
        dd         = 11,
        details    = 12,
        dialog     = 13,
        dir        = 14,
        dl         = 16,
        dt         = 17,
        fieldset   = 18,
        figcaption = 19,
        figure     = 20,
        footer     = 21,
        form       = 22,
        frame      = 23,
        frameset   = 24,
        h1         = 25,
        h2         = 26,
        h3         = 27,
        h4         = 28,
        h5         = 29,
        h6         = 30,
        head       = 31,
        header     = 32,
        html       = 34,
        iframe     = 35,
        legend     = 36,
        li         = 37,
        menu       = 40,
        menuitem   = 41,
        nav        = 42,
        noframes   = 43,
        ol         = 44,
        optgroup   = 45,
        option     = 46,
        lowerP     = 47,
        param      = 48,
        section    = 49,
        search     = 50,
        title      = 51,
        summary    = 52,
        table      = 53,
        tbody      = 54,
        td         = 55,
        tfoot      = 56,
        th         = 57,
        thead      = 58,
        tr         = 59,
        ul         = 61,
    };

    static HtmlBlockTag* getInstance() noexcept
    {
        return jam::SharedInstance<HtmlBlockTag>::getInstance();
    }
};

//==============================================================================

/**
 * @brief Syntax-highlight token kinds.
 *
 * Maps each highlight category — keyword, string, comment, number, and
 * punctuation — to its key. The key is the token class the syntax highlighter
 * maps to a colour in the active theme.
 */
struct SyntaxTokenType : public jam::Bimap<int>
{
    SyntaxTokenType() : jam::Bimap<int> { {
            { keyword,     juce::String::fromUTF8 ("keyword") },
            { string,      juce::String::fromUTF8 ("string") },
            { comment,     juce::String::fromUTF8 ("comment") },
            { number,      juce::String::fromUTF8 ("number") },
            { punctuation, juce::String::fromUTF8 ("punctuation") },
    } } {}

    enum value : int
    {
        keyword     = 0,
        string      = 1,
        comment     = 2,
        number      = 3,
        punctuation = 4,
    };

    static SyntaxTokenType* getInstance() noexcept
    {
        return jam::SharedInstance<SyntaxTokenType>::getInstance();
    }
};

//==============================================================================

/**
 * @brief Language family classification.
 *
 * Maps each language family — C-like, Python-like, shell, markup, data, and
 * JavaScript — to its key. The key is the family discriminator a file
 * extension resolves to via `map::languageFamily`, selecting which scanner
 * the code view runs.
 */
struct Family : public jam::Bimap<int>
{
    Family() : jam::Bimap<int> { {
            { cLang,  juce::String::fromUTF8 ("cLang") },
            { python, juce::String::fromUTF8 ("python") },
            { shell,  juce::String::fromUTF8 ("shell") },
            { markup, juce::String::fromUTF8 ("markup") },
            { data,   juce::String::fromUTF8 ("data") },
            { js,     juce::String::fromUTF8 ("js") },
    } } {}

    enum value : int
    {
        cLang  = 0,
        python = 1,
        shell  = 2,
        markup = 3,
        data   = 4,
        js     = 5,
    };

    static Family* getInstance() noexcept
    {
        return jam::SharedInstance<Family>::getInstance();
    }
};

//==============================================================================

/**
 * @brief Markdown block-open constructs.
 *
 * Maps the constructs that can open a block at the start of a line — thematic
 * break, ATX heading, fenced code, HTML block, reference definition, and grid
 * table — to their keys. The key is the opener discriminator the block parser
 * dispatches on before reading the block body.
 */
struct OpenBlock : public jam::Bimap<int>
{
    OpenBlock() : jam::Bimap<int> { {
            { thematicBreak,       juce::String::fromUTF8 ("thematicBreak") },
            { atxHeading,          juce::String::fromUTF8 ("atxHeading") },
            { fencedCode,          juce::String::fromUTF8 ("fencedCode") },
            { htmlBlock,           juce::String::fromUTF8 ("htmlBlock") },
            { referenceDefinition, juce::String::fromUTF8 ("referenceDefinition") },
            { gridTable,           juce::String::fromUTF8 ("gridTable") },
    } } {}

    enum value : int
    {
        thematicBreak       = 0,
        atxHeading          = 1,
        fencedCode          = 2,
        htmlBlock           = 3,
        referenceDefinition = 4,
        gridTable           = 5,
    };

    static OpenBlock* getInstance() noexcept
    {
        return jam::SharedInstance<OpenBlock>::getInstance();
    }
};

//==============================================================================

/**
 * @brief C++ keyword set.
 *
 * Maps every C++ keyword (including alternative-token spellings and the C++20
 * `co_*`/`concept`/`char8_t` additions) to its key. The key is the token class
 * the C++ scanner marks as a keyword for highlighting.
 */
struct CppKeyword : public jam::Bimap<int>
{
    CppKeyword() : jam::Bimap<int> { {
            { tokenAlignas,          juce::String::fromUTF8 ("alignas") },
            { tokenAlignof,          juce::String::fromUTF8 ("alignof") },
            { tokenAnd,              juce::String::fromUTF8 ("and") },
            { tokenAnd_eq,           juce::String::fromUTF8 ("and_eq") },
            { tokenAsm,              juce::String::fromUTF8 ("asm") },
            { tokenAuto,             juce::String::fromUTF8 ("auto") },
            { tokenBitand,           juce::String::fromUTF8 ("bitand") },
            { tokenBitor,            juce::String::fromUTF8 ("bitor") },
            { tokenBool,             juce::String::fromUTF8 ("bool") },
            { tokenBreak,            juce::String::fromUTF8 ("break") },
            { tokenCase,             juce::String::fromUTF8 ("case") },
            { tokenCatch,            juce::String::fromUTF8 ("catch") },
            { tokenChar,             juce::String::fromUTF8 ("char") },
            { tokenClass,            juce::String::fromUTF8 ("class") },
            { tokenCompl,            juce::String::fromUTF8 ("compl") },
            { tokenConst,            juce::String::fromUTF8 ("const") },
            { tokenConst_cast,       juce::String::fromUTF8 ("const_cast") },
            { tokenConstexpr,        juce::String::fromUTF8 ("constexpr") },
            { tokenConsteval,        juce::String::fromUTF8 ("consteval") },
            { tokenConstinit,        juce::String::fromUTF8 ("constinit") },
            { tokenContinue,         juce::String::fromUTF8 ("continue") },
            { tokenCo_await,         juce::String::fromUTF8 ("co_await") },
            { tokenCo_return,        juce::String::fromUTF8 ("co_return") },
            { tokenCo_yield,         juce::String::fromUTF8 ("co_yield") },
            { tokenDecltype,         juce::String::fromUTF8 ("decltype") },
            { tokenDefault,          juce::String::fromUTF8 ("default") },
            { tokenDelete,           juce::String::fromUTF8 ("delete") },
            { tokenDo,               juce::String::fromUTF8 ("do") },
            { tokenDouble,           juce::String::fromUTF8 ("double") },
            { tokenDynamic_cast,     juce::String::fromUTF8 ("dynamic_cast") },
            { tokenElse,             juce::String::fromUTF8 ("else") },
            { tokenEnum,             juce::String::fromUTF8 ("enum") },
            { tokenExplicit,         juce::String::fromUTF8 ("explicit") },
            { tokenExport,           juce::String::fromUTF8 ("export") },
            { tokenExtern,           juce::String::fromUTF8 ("extern") },
            { tokenFalse,            juce::String::fromUTF8 ("false") },
            { tokenFloat,            juce::String::fromUTF8 ("float") },
            { tokenFor,              juce::String::fromUTF8 ("for") },
            { tokenFriend,           juce::String::fromUTF8 ("friend") },
            { tokenGoto,             juce::String::fromUTF8 ("goto") },
            { tokenIf,               juce::String::fromUTF8 ("if") },
            { tokenInline,           juce::String::fromUTF8 ("inline") },
            { tokenInt,              juce::String::fromUTF8 ("int") },
            { tokenLong,             juce::String::fromUTF8 ("long") },
            { tokenMutable,          juce::String::fromUTF8 ("mutable") },
            { tokenNamespace,        juce::String::fromUTF8 ("namespace") },
            { tokenNew,              juce::String::fromUTF8 ("new") },
            { tokenNoexcept,         juce::String::fromUTF8 ("noexcept") },
            { tokenNot,              juce::String::fromUTF8 ("not") },
            { tokenNot_eq,           juce::String::fromUTF8 ("not_eq") },
            { tokenNullptr,          juce::String::fromUTF8 ("nullptr") },
            { tokenOperator,         juce::String::fromUTF8 ("operator") },
            { tokenOr,               juce::String::fromUTF8 ("or") },
            { tokenOr_eq,            juce::String::fromUTF8 ("or_eq") },
            { tokenPrivate,          juce::String::fromUTF8 ("private") },
            { tokenProtected,        juce::String::fromUTF8 ("protected") },
            { tokenPublic,           juce::String::fromUTF8 ("public") },
            { tokenRegister,         juce::String::fromUTF8 ("register") },
            { tokenReinterpret_cast, juce::String::fromUTF8 ("reinterpret_cast") },
            { tokenRequires,         juce::String::fromUTF8 ("requires") },
            { tokenReturn,           juce::String::fromUTF8 ("return") },
            { tokenShort,            juce::String::fromUTF8 ("short") },
            { tokenSigned,           juce::String::fromUTF8 ("signed") },
            { tokenSizeof,           juce::String::fromUTF8 ("sizeof") },
            { tokenStatic,           juce::String::fromUTF8 ("static") },
            { tokenStatic_assert,    juce::String::fromUTF8 ("static_assert") },
            { tokenStatic_cast,      juce::String::fromUTF8 ("static_cast") },
            { tokenStruct,           juce::String::fromUTF8 ("struct") },
            { tokenSwitch,           juce::String::fromUTF8 ("switch") },
            { tokenTemplate,         juce::String::fromUTF8 ("template") },
            { tokenThis,             juce::String::fromUTF8 ("this") },
            { tokenThread_local,     juce::String::fromUTF8 ("thread_local") },
            { tokenThrow,            juce::String::fromUTF8 ("throw") },
            { tokenTrue,             juce::String::fromUTF8 ("true") },
            { tokenTry,              juce::String::fromUTF8 ("try") },
            { tokenTypedef,          juce::String::fromUTF8 ("typedef") },
            { tokenTypeid,           juce::String::fromUTF8 ("typeid") },
            { tokenTypename,         juce::String::fromUTF8 ("typename") },
            { tokenUnion,            juce::String::fromUTF8 ("union") },
            { tokenUnsigned,         juce::String::fromUTF8 ("unsigned") },
            { tokenUsing,            juce::String::fromUTF8 ("using") },
            { tokenVirtual,          juce::String::fromUTF8 ("virtual") },
            { tokenVoid,             juce::String::fromUTF8 ("void") },
            { tokenVolatile,         juce::String::fromUTF8 ("volatile") },
            { tokenWchar_t,          juce::String::fromUTF8 ("wchar_t") },
            { tokenWhile,            juce::String::fromUTF8 ("while") },
            { tokenXor,              juce::String::fromUTF8 ("xor") },
            { tokenXor_eq,           juce::String::fromUTF8 ("xor_eq") },
            { tokenOverride,         juce::String::fromUTF8 ("override") },
            { tokenFinal,            juce::String::fromUTF8 ("final") },
            { import,                juce::String::fromUTF8 ("import") },
            { tokenModule,           juce::String::fromUTF8 ("module") },
            { tokenConcept,          juce::String::fromUTF8 ("concept") },
            { tokenChar8_t,          juce::String::fromUTF8 ("char8_t") },
            { tokenChar16_t,         juce::String::fromUTF8 ("char16_t") },
            { tokenChar32_t,         juce::String::fromUTF8 ("char32_t") },
    } } {}

    enum value : int
    {
        tokenAlignas          = 0,
        tokenAlignof          = 1,
        tokenAnd              = 2,
        tokenAnd_eq           = 3,
        tokenAsm              = 4,
        tokenAuto             = 5,
        tokenBitand           = 6,
        tokenBitor            = 7,
        tokenBool             = 8,
        tokenBreak            = 9,
        tokenCase             = 10,
        tokenCatch            = 11,
        tokenChar             = 12,
        tokenClass            = 13,
        tokenCompl            = 14,
        tokenConst            = 15,
        tokenConst_cast       = 16,
        tokenConstexpr        = 17,
        tokenConsteval        = 18,
        tokenConstinit        = 19,
        tokenContinue         = 20,
        tokenCo_await         = 21,
        tokenCo_return        = 22,
        tokenCo_yield         = 23,
        tokenDecltype         = 24,
        tokenDefault          = 25,
        tokenDelete           = 26,
        tokenDo               = 27,
        tokenDouble           = 28,
        tokenDynamic_cast     = 29,
        tokenElse             = 30,
        tokenEnum             = 31,
        tokenExplicit         = 32,
        tokenExport           = 33,
        tokenExtern           = 34,
        tokenFalse            = 35,
        tokenFloat            = 36,
        tokenFor              = 37,
        tokenFriend           = 38,
        tokenGoto             = 39,
        tokenIf               = 40,
        tokenInline           = 41,
        tokenInt              = 42,
        tokenLong             = 43,
        tokenMutable          = 44,
        tokenNamespace        = 45,
        tokenNew              = 46,
        tokenNoexcept         = 47,
        tokenNot              = 48,
        tokenNot_eq           = 49,
        tokenNullptr          = 50,
        tokenOperator         = 51,
        tokenOr               = 52,
        tokenOr_eq            = 53,
        tokenPrivate          = 54,
        tokenProtected        = 55,
        tokenPublic           = 56,
        tokenRegister         = 57,
        tokenReinterpret_cast = 58,
        tokenRequires         = 59,
        tokenReturn           = 60,
        tokenShort            = 61,
        tokenSigned           = 62,
        tokenSizeof           = 63,
        tokenStatic           = 64,
        tokenStatic_assert    = 65,
        tokenStatic_cast      = 66,
        tokenStruct           = 67,
        tokenSwitch           = 68,
        tokenTemplate         = 69,
        tokenThis             = 70,
        tokenThread_local     = 71,
        tokenThrow            = 72,
        tokenTrue             = 73,
        tokenTry              = 74,
        tokenTypedef          = 75,
        tokenTypeid           = 76,
        tokenTypename         = 77,
        tokenUnion            = 78,
        tokenUnsigned         = 79,
        tokenUsing            = 80,
        tokenVirtual          = 81,
        tokenVoid             = 82,
        tokenVolatile         = 83,
        tokenWchar_t          = 84,
        tokenWhile            = 85,
        tokenXor              = 86,
        tokenXor_eq           = 87,
        tokenOverride         = 88,
        tokenFinal            = 89,
        import                = 90,
        tokenModule           = 91,
        tokenConcept          = 92,
        tokenChar8_t          = 93,
        tokenChar16_t         = 94,
        tokenChar32_t         = 95,
    };

    static CppKeyword* getInstance() noexcept
    {
        return jam::SharedInstance<CppKeyword>::getInstance();
    }
};

//==============================================================================

/**
 * @brief JavaScript keyword set.
 *
 * Maps every JavaScript reserved word and contextual keyword (including the
 * module-syntax and TypeScript additions `import`, `from`, `type`,
 * `interface`, `enum`, `implements`) to its key. The key is the token class
 * the JS scanner marks as a keyword for highlighting.
 */
struct JsKeyword : public jam::Bimap<int>
{
    JsKeyword() : jam::Bimap<int> { {
            { await,          juce::String::fromUTF8 ("await") },
            { tokenBreak,     juce::String::fromUTF8 ("break") },
            { tokenCase,      juce::String::fromUTF8 ("case") },
            { tokenCatch,     juce::String::fromUTF8 ("catch") },
            { tokenClass,     juce::String::fromUTF8 ("class") },
            { tokenConst,     juce::String::fromUTF8 ("const") },
            { tokenContinue,  juce::String::fromUTF8 ("continue") },
            { debugger,       juce::String::fromUTF8 ("debugger") },
            { tokenDefault,   juce::String::fromUTF8 ("default") },
            { tokenDelete,    juce::String::fromUTF8 ("delete") },
            { tokenDo,        juce::String::fromUTF8 ("do") },
            { tokenElse,      juce::String::fromUTF8 ("else") },
            { tokenExport,    juce::String::fromUTF8 ("export") },
            { extends,        juce::String::fromUTF8 ("extends") },
            { tokenFalse,     juce::String::fromUTF8 ("false") },
            { finally,        juce::String::fromUTF8 ("finally") },
            { tokenFor,       juce::String::fromUTF8 ("for") },
            { function,       juce::String::fromUTF8 ("function") },
            { tokenIf,        juce::String::fromUTF8 ("if") },
            { import,         juce::String::fromUTF8 ("import") },
            { in,             juce::String::fromUTF8 ("in") },
            { instanceof,     juce::String::fromUTF8 ("instanceof") },
            { let,            juce::String::fromUTF8 ("let") },
            { tokenNew,       juce::String::fromUTF8 ("new") },
            { null,           juce::String::fromUTF8 ("null") },
            { of,             juce::String::fromUTF8 ("of") },
            { tokenReturn,    juce::String::fromUTF8 ("return") },
            { super,          juce::String::fromUTF8 ("super") },
            { tokenSwitch,    juce::String::fromUTF8 ("switch") },
            { tokenThis,      juce::String::fromUTF8 ("this") },
            { tokenThrow,     juce::String::fromUTF8 ("throw") },
            { tokenTrue,      juce::String::fromUTF8 ("true") },
            { tokenTry,       juce::String::fromUTF8 ("try") },
            { typeof,         juce::String::fromUTF8 ("typeof") },
            { undefined,      juce::String::fromUTF8 ("undefined") },
            { var,            juce::String::fromUTF8 ("var") },
            { tokenVoid,      juce::String::fromUTF8 ("void") },
            { tokenWhile,     juce::String::fromUTF8 ("while") },
            { yield,          juce::String::fromUTF8 ("yield") },
            { async,          juce::String::fromUTF8 ("async") },
            { from,           juce::String::fromUTF8 ("from") },
            { as,             juce::String::fromUTF8 ("as") },
            { type,           juce::String::fromUTF8 ("type") },
            { tokenInterface, juce::String::fromUTF8 ("interface") },
            { tokenEnum,      juce::String::fromUTF8 ("enum") },
            { implements,     juce::String::fromUTF8 ("implements") },
    } } {}

    enum value : int
    {
        await          = 0,
        tokenBreak     = 1,
        tokenCase      = 2,
        tokenCatch     = 3,
        tokenClass     = 4,
        tokenConst     = 5,
        tokenContinue  = 6,
        debugger       = 7,
        tokenDefault   = 8,
        tokenDelete    = 9,
        tokenDo        = 10,
        tokenElse      = 11,
        tokenExport    = 12,
        extends        = 13,
        tokenFalse     = 14,
        finally        = 15,
        tokenFor       = 16,
        function       = 17,
        tokenIf        = 18,
        import         = 19,
        in             = 20,
        instanceof     = 21,
        let            = 22,
        tokenNew       = 23,
        null           = 24,
        of             = 25,
        tokenReturn    = 26,
        super          = 27,
        tokenSwitch    = 28,
        tokenThis      = 29,
        tokenThrow     = 30,
        tokenTrue      = 31,
        tokenTry       = 32,
        typeof         = 33,
        undefined      = 34,
        var            = 35,
        tokenVoid      = 36,
        tokenWhile     = 37,
        yield          = 38,
        async          = 39,
        from           = 40,
        as             = 41,
        type           = 42,
        tokenInterface = 43,
        tokenEnum      = 44,
        implements     = 45,
    };

    static JsKeyword* getInstance() noexcept
    {
        return jam::SharedInstance<JsKeyword>::getInstance();
    }
};

//==============================================================================

/**
 * @brief Python keyword set.
 *
 * Maps every Python keyword to its key. The key is the token class the Python
 * scanner marks as a keyword for highlighting.
 */
struct PythonKeyword : public jam::Bimap<int>
{
    PythonKeyword() : jam::Bimap<int> { {
            { tokenFalse,    juce::String::fromUTF8 ("false") },
            { none,          juce::String::fromUTF8 ("none") },
            { tokenTrue,     juce::String::fromUTF8 ("true") },
            { tokenAnd,      juce::String::fromUTF8 ("and") },
            { as,            juce::String::fromUTF8 ("as") },
            { tokenAssert,   juce::String::fromUTF8 ("assert") },
            { async,         juce::String::fromUTF8 ("async") },
            { await,         juce::String::fromUTF8 ("await") },
            { tokenBreak,    juce::String::fromUTF8 ("break") },
            { tokenClass,    juce::String::fromUTF8 ("class") },
            { tokenContinue, juce::String::fromUTF8 ("continue") },
            { def,           juce::String::fromUTF8 ("def") },
            { del,           juce::String::fromUTF8 ("del") },
            { elif,          juce::String::fromUTF8 ("elif") },
            { tokenElse,     juce::String::fromUTF8 ("else") },
            { except,        juce::String::fromUTF8 ("except") },
            { finally,       juce::String::fromUTF8 ("finally") },
            { tokenFor,      juce::String::fromUTF8 ("for") },
            { from,          juce::String::fromUTF8 ("from") },
            { global,        juce::String::fromUTF8 ("global") },
            { tokenIf,       juce::String::fromUTF8 ("if") },
            { import,        juce::String::fromUTF8 ("import") },
            { in,            juce::String::fromUTF8 ("in") },
            { is,            juce::String::fromUTF8 ("is") },
            { lambda,        juce::String::fromUTF8 ("lambda") },
            { nonlocal,      juce::String::fromUTF8 ("nonlocal") },
            { tokenNot,      juce::String::fromUTF8 ("not") },
            { tokenOr,       juce::String::fromUTF8 ("or") },
            { pass,          juce::String::fromUTF8 ("pass") },
            { raise,         juce::String::fromUTF8 ("raise") },
            { tokenReturn,   juce::String::fromUTF8 ("return") },
            { tokenTry,      juce::String::fromUTF8 ("try") },
            { tokenWhile,    juce::String::fromUTF8 ("while") },
            { with,          juce::String::fromUTF8 ("with") },
            { yield,         juce::String::fromUTF8 ("yield") },
    } } {}

    enum value : int
    {
        tokenFalse    = 0,
        none          = 1,
        tokenTrue     = 2,
        tokenAnd      = 3,
        as            = 4,
        tokenAssert   = 5,
        async         = 6,
        await         = 7,
        tokenBreak    = 8,
        tokenClass    = 9,
        tokenContinue = 10,
        def           = 11,
        del           = 12,
        elif          = 13,
        tokenElse     = 14,
        except        = 15,
        finally       = 16,
        tokenFor      = 17,
        from          = 18,
        global        = 19,
        tokenIf       = 20,
        import        = 21,
        in            = 22,
        is            = 23,
        lambda        = 24,
        nonlocal      = 25,
        tokenNot      = 26,
        tokenOr       = 27,
        pass          = 28,
        raise         = 29,
        tokenReturn   = 30,
        tokenTry      = 31,
        tokenWhile    = 32,
        with          = 33,
        yield         = 34,
    };

    static PythonKeyword* getInstance() noexcept
    {
        return jam::SharedInstance<PythonKeyword>::getInstance();
    }
};

//==============================================================================

/**
 * @brief CSS token kinds.
 *
 * Maps each token kind the CSS tokeniser emits — ident, function, at-keyword,
 * hash, string (and bad string), url (and bad url), delim, number, percentage,
 * dimension, whitespace, the bracket/paren/brace pairs, and end-of-file — to
 * its key. The key is the discriminator the CSS parser dispatches on.
 */
struct CssTokenType : public jam::Bimap<int>
{
    CssTokenType() : jam::Bimap<int> { {
            { ident,        juce::String::fromUTF8 ("ident") },
            { function,     juce::String::fromUTF8 ("function") },
            { atKeyword,    juce::String::fromUTF8 ("atKeyword") },
            { hash,         juce::String::fromUTF8 ("hash") },
            { string,       juce::String::fromUTF8 ("string") },
            { badString,    juce::String::fromUTF8 ("badString") },
            { url,          juce::String::fromUTF8 ("url") },
            { badUrl,       juce::String::fromUTF8 ("badUrl") },
            { delim,        juce::String::fromUTF8 ("delim") },
            { number,       juce::String::fromUTF8 ("number") },
            { percentage,   juce::String::fromUTF8 ("percentage") },
            { dimension,    juce::String::fromUTF8 ("dimension") },
            { whitespace,   juce::String::fromUTF8 ("whitespace") },
            { colon,        juce::String::fromUTF8 ("colon") },
            { semicolon,    juce::String::fromUTF8 ("semicolon") },
            { comma,        juce::String::fromUTF8 ("comma") },
            { openBracket,  juce::String::fromUTF8 ("openBracket") },
            { closeBracket, juce::String::fromUTF8 ("closeBracket") },
            { openParen,    juce::String::fromUTF8 ("openParen") },
            { closeParen,   juce::String::fromUTF8 ("closeParen") },
            { openBrace,    juce::String::fromUTF8 ("openBrace") },
            { closeBrace,   juce::String::fromUTF8 ("closeBrace") },
            { endOfFile,    juce::String::fromUTF8 ("endOfFile") },
    } } {}

    enum value : int
    {
        ident        = 0,
        function     = 1,
        atKeyword    = 2,
        hash         = 3,
        string       = 4,
        badString    = 5,
        url          = 6,
        badUrl       = 7,
        delim        = 8,
        number       = 9,
        percentage   = 10,
        dimension    = 11,
        whitespace   = 12,
        colon        = 13,
        semicolon    = 14,
        comma        = 15,
        openBracket  = 16,
        closeBracket = 17,
        openParen    = 18,
        closeParen   = 19,
        openBrace    = 20,
        closeBrace   = 21,
        endOfFile    = 22,
    };

    static CssTokenType* getInstance() noexcept
    {
        return jam::SharedInstance<CssTokenType>::getInstance();
    }
};

//==============================================================================

/**
 * @brief CSS at-rule kinds.
 *
 * Maps each at-rule the CSS parser recognises — style, charset, import, media,
 * font-face, page, and namespace (plus the unknown-rule fallback) — to its
 * key. The key is the rule discriminator used to build the stylesheet model.
 */
struct CssRuleType : public jam::Bimap<int>
{
    CssRuleType() : jam::Bimap<int> { {
            { styleRule,     juce::String::fromUTF8 ("styleRule") },
            { unknownRule,   juce::String::fromUTF8 ("unknownRule") },
            { charsetRule,   juce::String::fromUTF8 ("charsetRule") },
            { importRule,    juce::String::fromUTF8 ("importRule") },
            { mediaRule,     juce::String::fromUTF8 ("mediaRule") },
            { fontFaceRule,  juce::String::fromUTF8 ("fontFaceRule") },
            { pageRule,      juce::String::fromUTF8 ("pageRule") },
            { namespaceRule, juce::String::fromUTF8 ("namespaceRule") },
    } } {}

    enum value : int
    {
        styleRule     = 1,
        unknownRule   = 0,
        charsetRule   = 2,
        importRule    = 3,
        mediaRule     = 4,
        fontFaceRule  = 5,
        pageRule      = 6,
        namespaceRule = 10,
    };

    static CssRuleType* getInstance() noexcept
    {
        return jam::SharedInstance<CssRuleType>::getInstance();
    }
};

//==============================================================================

/**
 * @brief HTML token kinds.
 *
 * Maps each token kind the HTML tokeniser emits — start tag, end tag, comment,
 * character, doctype, and end-of-file — to its key. The key is the
 * discriminator the HTML parser dispatches on when building the DOM.
 */
struct HtmlTokenType : public jam::Bimap<int>
{
    HtmlTokenType() : jam::Bimap<int> { {
            { startTag,  juce::String::fromUTF8 ("startTag") },
            { endTag,    juce::String::fromUTF8 ("endTag") },
            { comment,   juce::String::fromUTF8 ("comment") },
            { character, juce::String::fromUTF8 ("character") },
            { doctype,   juce::String::fromUTF8 ("doctype") },
            { endOfFile, juce::String::fromUTF8 ("endOfFile") },
    } } {}

    enum value : int
    {
        startTag  = 0,
        endTag    = 1,
        comment   = 2,
        character = 3,
        doctype   = 4,
        endOfFile = 5,
    };

    static HtmlTokenType* getInstance() noexcept
    {
        return jam::SharedInstance<HtmlTokenType>::getInstance();
    }
};

//==============================================================================

/**
 * @brief HTML void-element tags.
 *
 * Maps each void HTML element (the tags that cannot have children and must not
 * be closed) to its key. The key is the discriminator the HTML parser uses to
 * emit a self-closing node without waiting for an end tag.
 */
struct HtmlVoidTag : public jam::Bimap<int>
{
    HtmlVoidTag() : jam::Bimap<int> { {
            { area,   juce::String::fromUTF8 ("area") },
            { base,   juce::String::fromUTF8 ("base") },
            { br,     juce::String::fromUTF8 ("br") },
            { col,    juce::String::fromUTF8 ("col") },
            { embed,  juce::String::fromUTF8 ("embed") },
            { hr,     juce::String::fromUTF8 ("hr") },
            { img,    juce::String::fromUTF8 ("img") },
            { input,  juce::String::fromUTF8 ("input") },
            { link,   juce::String::fromUTF8 ("link") },
            { meta,   juce::String::fromUTF8 ("meta") },
            { source, juce::String::fromUTF8 ("source") },
            { track,  juce::String::fromUTF8 ("track") },
            { wbr,    juce::String::fromUTF8 ("wbr") },
    } } {}

    enum value : int
    {
        area   = 0,
        base   = 1,
        br     = 2,
        col    = 3,
        embed  = 4,
        hr     = 5,
        img    = 6,
        input  = 7,
        link   = 8,
        meta   = 9,
        source = 10,
        track  = 11,
        wbr    = 12,
    };

    static HtmlVoidTag* getInstance() noexcept
    {
        return jam::SharedInstance<HtmlVoidTag>::getInstance();
    }
};

//==============================================================================

/**
 * @brief Vari display modes.
 *
 * Maps each Vari knob readout mode — Hertz, note, or kilohertz — to its key.
 * The key selects the format the parameter display uses for frequency values.
 */
struct VariDisplayMode : public jam::Bimap<int>
{
    VariDisplayMode() : jam::Bimap<int> { {
            { hz,   juce::String::fromUTF8 ("hz") },
            { note, juce::String::fromUTF8 ("note") },
            { khz,  juce::String::fromUTF8 ("khz") },
    } } {}

    enum value : int
    {
        hz   = 2,
        note = 1,
        khz  = 3,
    };

    static VariDisplayMode* getInstance() noexcept
    {
        return jam::SharedInstance<VariDisplayMode>::getInstance();
    }
};

//==============================================================================

/**
 * @brief Plugin wrapper formats.
 *
 * Maps each plugin wrapper format — VST, VST3, AU, AUv3, AAX, Standalone,
 * Unity, and LV2 (plus the undefined fallback) — to its key. The value is the
 * full attribution string shown in the host's plugin metadata.
 */
struct PluginWrapper : public jam::Bimap<int>
{
    PluginWrapper() : jam::Bimap<int> { {
            { undefined,  juce::String::fromUTF8 ("Undefined") },
            { vst,        juce::String::fromUTF8 ("VST by Steinberg Media Technologies, GmbH.") },
            { vst3,       juce::String::fromUTF8 ("VST3 by Steinberg Media Technologies, GmbH.") },
            { au,         juce::String::fromUTF8 ("AU by Apple Computer, Inc.") },
            { auv3,       juce::String::fromUTF8 ("AUv3 by Apple Computer, Inc.") },
            { aax,        juce::String::fromUTF8 ("AAX by Avid Technology, Inc.") },
            { standalone, juce::String::fromUTF8 ("Standalone") },
            { unity,      juce::String::fromUTF8 ("Unity Native Audio Plugin by Unity Technologies") },
            { lv2,        juce::String::fromUTF8 ("LV2 by Steve Harris, David Robillard, and other members of linux-audio-dev") },
    } } {}

    enum value : int
    {
        undefined  = 0,
        vst        = 1,
        vst3       = 2,
        au         = 3,
        auv3       = 4,
        aax        = 5,
        standalone = 6,
        unity      = 7,
        lv2        = 8,
    };

    static PluginWrapper* getInstance() noexcept
    {
        return jam::SharedInstance<PluginWrapper>::getInstance();
    }
};

//==============================================================================

/**
 * @brief Spectrum analyzer display modes.
 *
 * Maps each analyzer readout mode — polygon trace or bar graph — to its key.
 * The key selects the geometry the analyzer draws for its spectrum data.
 */
struct AnalyzerMode : public jam::Bimap<int>
{
    AnalyzerMode() : jam::Bimap<int> { {
            { polygon, juce::String::fromUTF8 ("POLYGON") },
            { bars,    juce::String::fromUTF8 ("BARS") },
    } } {}

    enum value : int
    {
        polygon = 1,
        bars    = 2,
    };

    static AnalyzerMode* getInstance() noexcept
    {
        return jam::SharedInstance<AnalyzerMode>::getInstance();
    }
};

//==============================================================================

/**
 * @brief Application appearance themes.
 *
 * Maps each appearance theme — light, dark, or automatic OS-following — to its
 * key. The key selects the colour scheme the UI applies across all components.
 */
struct Appearance : public jam::Bimap<int>
{
    Appearance() : jam::Bimap<int> { {
            { automatic, juce::String::fromUTF8 ("AUTO") },
            { light,     juce::String::fromUTF8 ("LIGHT") },
            { dark,      juce::String::fromUTF8 ("DARK") },
    } } {}

    enum value : int
    {
        automatic = 3,
        light     = 1,
        dark      = 2,
    };

    template <typename Slot, typename Apply>
    static void setAppearance (const Slot& lightSlot, const Slot& darkSlot, bool isDarkActive, Apply&& apply)
    {
        apply (lightSlot);

        if (isDarkActive)
            if (darkSlot != Slot {})
                apply (darkSlot);
    }

    static Appearance* getInstance() noexcept
    {
        return jam::SharedInstance<Appearance>::getInstance();
    }
};

//==============================================================================

/**
 * @brief Audio parameter kinds.
 *
 * Maps each parameter type — floating-point, boolean, choice, and integer — to
 * its key. The key is the discriminator used when constructing the matching
 * `juce::AudioProcessorParameter` from a parameter descriptor.
 */
struct AudioParameter : public jam::Bimap<int>
{
    AudioParameter() : jam::Bimap<int> { {
            { floatingPoint, juce::String::fromUTF8 ("floatingPoint") },
            { boolean,       juce::String::fromUTF8 ("boolean") },
            { choice,        juce::String::fromUTF8 ("choice") },
            { integer,       juce::String::fromUTF8 ("integer") },
    } } {}

    enum value : int
    {
        floatingPoint = 0,
        boolean       = 1,
        choice        = 2,
        integer       = 3,
    };

    static AudioParameter* getInstance() noexcept
    {
        return jam::SharedInstance<AudioParameter>::getInstance();
    }
};

//==============================================================================

/**
 * @brief Parameter display modes.
 *
 * Maps each parameter display mode — numeric readout or spectrum analyzer — to
 * its key. The key selects which visualisation a parameter panel shows.
 */
struct Display : public jam::Bimap<int>
{
    Display() : jam::Bimap<int> { {
            { numbers,  juce::String::fromUTF8 ("NUMBERS") },
            { analyzer, juce::String::fromUTF8 ("ANALYZER") },
    } } {}

    enum value : int
    {
        numbers  = 1,
        analyzer = 2,
    };

    static Display* getInstance() noexcept
    {
        return jam::SharedInstance<Display>::getInstance();
    }
};

//==============================================================================

/**
 * @brief Font rasterizer backends.
 *
 * Maps each glyph rasterizer — FreeType, edge-table, or the native platform
 * rasterizer — to its key. The key selects the backend the text engine uses to
 * rasterize glyph outlines.
 */
struct FontRasterizerBackend : public jam::Bimap<int>
{
    FontRasterizerBackend() : jam::Bimap<int> { {
            { freetype,  juce::String::fromUTF8 ("freetype") },
            { edgeTable, juce::String::fromUTF8 ("edgeTable") },
            { native,    juce::String::fromUTF8 ("native") },
    } } {}

    enum value : int
    {
        freetype  = 0,
        edgeTable = 1,
        native    = 2,
    };

    static FontRasterizerBackend* getInstance() noexcept
    {
        return jam::SharedInstance<FontRasterizerBackend>::getInstance();
    }
};

//==============================================================================

/**
 * @brief Image resample filters.
 *
 * Maps each resampling filter — linear or nearest-neighbour — to its key. The
 * key selects the interpolation the image pipeline uses when resizing.
 */
struct ImageResample : public jam::Bimap<int>
{
    ImageResample() : jam::Bimap<int> { {
            { linear,  juce::String::fromUTF8 ("linear") },
            { nearest, juce::String::fromUTF8 ("nearest") },
    } } {}

    enum value : int
    {
        linear  = 0,
        nearest = 1,
    };

    static ImageResample* getInstance() noexcept
    {
        return jam::SharedInstance<ImageResample>::getInstance();
    }
};

//==============================================================================

/**
 * @brief Mouse buttons.
 *
 * Maps each mouse button — none, left, middle, and right — to its key. The key
 * is the discriminator a component's mouse handler switches on.
 */
struct MouseButton : public jam::Bimap<int>
{
    MouseButton() : jam::Bimap<int> { {
            { none,   juce::String::fromUTF8 ("none") },
            { left,   juce::String::fromUTF8 ("left") },
            { middle, juce::String::fromUTF8 ("middle") },
            { right,  juce::String::fromUTF8 ("right") },
    } } {}

    enum value : int
    {
        none   = 0,
        left   = 1,
        middle = 2,
        right  = 3,
    };

    static MouseButton* getInstance() noexcept
    {
        return jam::SharedInstance<MouseButton>::getInstance();
    }
};

//==============================================================================

/**
 * @brief Oversampling factors.
 *
 * Maps each oversampling factor — off, 2x, 4x, and 8x — to its key. The key is
 * the factor the DSP chain uses to configure its resampling stage.
 */
struct Oversampling : public jam::Bimap<int>
{
    Oversampling() : jam::Bimap<int> { {
            { off, juce::String::fromUTF8 ("OFF") },
            { x2,  juce::String::fromUTF8 ("x2") },
            { x4,  juce::String::fromUTF8 ("x4") },
            { x8,  juce::String::fromUTF8 ("x8") },
    } } {}

    enum value : int
    {
        off = 1,
        x2  = 2,
        x4  = 3,
        x8  = 4,
    };

    static Oversampling* getInstance() noexcept
    {
        return jam::SharedInstance<Oversampling>::getInstance();
    }
};

//==============================================================================

/**
 * @brief UI scale presets.
 *
 * Maps each UI scale preset — mini, small, medium, large, and huge — to its
 * key. The key is the multiplier index the UI applies to size all components.
 */
struct UIScaleMap : public jam::Bimap<int>
{
    UIScaleMap() : jam::Bimap<int> { {
            { medium,     juce::String::fromUTF8 ("MEDIUM") },
            { mini,       juce::String::fromUTF8 ("MINI") },
            { tokenSmall, juce::String::fromUTF8 ("SMALL") },
            { large,      juce::String::fromUTF8 ("LARGE") },
            { huge,       juce::String::fromUTF8 ("HUGE") },
    } } {}

    enum value : int
    {
        medium     = 3,
        mini       = 1,
        tokenSmall = 2,
        large      = 4,
        huge       = 5,
    };

    static UIScaleMap* getInstance() noexcept
    {
        return jam::SharedInstance<UIScaleMap>::getInstance();
    }
};

//==============================================================================

/**
 * @brief Standard HTML element tags.
 *
 * Maps each standard (non-void, non-block) HTML element the HTML parser builds
 * a normal node for — div, label, button, select, and input — to its key.
 */
struct HtmlStandardTag : public jam::Bimap<int>
{
    HtmlStandardTag() : jam::Bimap<int> { {
            { div,    juce::String::fromUTF8 ("div") },
            { label,  juce::String::fromUTF8 ("label") },
            { button, juce::String::fromUTF8 ("button") },
            { select, juce::String::fromUTF8 ("select") },
            { input,  juce::String::fromUTF8 ("input") },
    } } {}

    enum value : int
    {
        div    = 0,
        label  = 1,
        button = 2,
        select = 3,
        input  = 4,
    };

    static HtmlStandardTag* getInstance() noexcept
    {
        return jam::SharedInstance<HtmlStandardTag>::getInstance();
    }
};

//==============================================================================

/**
 * @brief Parameter page labels.
 *
 * Maps each parameter page (A, B, C) to its key. The key selects which page of
 * parameters a panel displays.
 */
struct ParameterPage : public jam::Bimap<int>
{
    ParameterPage() : jam::Bimap<int> { {
            { pageA, juce::String::fromUTF8 ("A") },
            { pageB, juce::String::fromUTF8 ("B") },
            { pageC, juce::String::fromUTF8 ("C") },
    } } {}

    enum value : int
    {
        pageA = 0,
        pageB = 1,
        pageC = 2,
    };

    static ParameterPage* getInstance() noexcept
    {
        return jam::SharedInstance<ParameterPage>::getInstance();
    }
};

//==============================================================================

/**
 * @brief Device view orientations.
 *
 * Maps each device orientation — landscape or portrait — to its key. The key
 * selects the layout the UI uses for the active device rotation.
 */
struct ViewOrientation : public jam::Bimap<int>
{
    ViewOrientation() : jam::Bimap<int> { {
            { landscape, juce::String::fromUTF8 ("LANDSCAPE") },
            { portrait,  juce::String::fromUTF8 ("PORTRAIT") },
    } } {}

    enum value : int
    {
        landscape = 1,
        portrait  = 2,
    };

    static ViewOrientation* getInstance() noexcept
    {
        return jam::SharedInstance<ViewOrientation>::getInstance();
    }
};

//==============================================================================

/**
 * @brief CSS at-rule discriminators.
 *
 * Maps each at-rule discriminator to its key. The key is the rule category the
 * CSS parser assigns after reading the at-keyword, driving which rule builder
 * consumes the prelude.
 */
struct AtRuleType : public jam::Bimap<int>
{
    AtRuleType() : jam::Bimap<int> { {
            { fontFace,       juce::String::fromUTF8 ("font-face") },
            { media,          juce::String::fromUTF8 ("media") },
            { import,         juce::String::fromUTF8 ("import") },
            { charset,        juce::String::fromUTF8 ("charset") },
            { page,           juce::String::fromUTF8 ("page") },
            { tokenNamespace, juce::String::fromUTF8 ("namespace") },
    } } {}

    enum value : int
    {
        fontFace       = CssRuleType::fontFaceRule,
        media          = CssRuleType::mediaRule,
        import         = CssRuleType::importRule,
        charset        = CssRuleType::charsetRule,
        page           = CssRuleType::pageRule,
        tokenNamespace = CssRuleType::namespaceRule,
    };

    static AtRuleType* getInstance() noexcept
    {
        return jam::SharedInstance<AtRuleType>::getInstance();
    }
};

//==============================================================================

/**
 * @brief Markdown block kind to HTML tag map.
 *
 * Maps each Markdown block kind to the `juce::Identifier` of the HTML element
 * it serializes to when the document is written out as HTML.
 */
struct BlockTag : public jam::Bimap<int, juce::Identifier>
{
    BlockTag() : jam::Bimap<int, juce::Identifier> { {
            { body,       juce::Identifier { "body" } },
            { p,          juce::Identifier { "p" } },
            { blockquote, juce::Identifier { "blockquote" } },
            { pre,        juce::Identifier { "pre" } },
            { hr,         juce::Identifier { "hr" } },
            { html,       juce::Identifier { "html" } },
            { table,      juce::Identifier { "table" } },
            { tr,         juce::Identifier { "tr" } },
            { li,         juce::Identifier { "li" } },
            { mermaid,    juce::Identifier { "mermaid" } },
            { img,        juce::Identifier { "img" } },
    } } {}

    enum value : int
    {
        body       = BlockType::document,
        p          = BlockType::paragraph,
        blockquote = BlockType::blockquote,
        pre        = BlockType::codeBlock,
        hr         = BlockType::thematicBreak,
        html       = BlockType::htmlBlock,
        table      = BlockType::table,
        tr         = BlockType::tableRow,
        li         = BlockType::listItem,
        mermaid    = BlockType::mermaid,
        img        = BlockType::image,
    };

    static BlockTag* getInstance() noexcept
    {
        return jam::SharedInstance<BlockTag>::getInstance();
    }
};

//==============================================================================

/**
 * @brief Heading level to HTML tag map.
 *
 * Maps each heading level to the `juce::Identifier` of the HTML heading
 * element (`h1` through `h6`) it serializes to.
 */
struct HeadingTag : public jam::Bimap<int, juce::Identifier>
{
    HeadingTag() : jam::Bimap<int, juce::Identifier> { {
            { h1, juce::Identifier { "h1" } },
            { h2, juce::Identifier { "h2" } },
            { h3, juce::Identifier { "h3" } },
            { h4, juce::Identifier { "h4" } },
            { h5, juce::Identifier { "h5" } },
            { h6, juce::Identifier { "h6" } },
    } } {}

    enum value : int
    {
        h1 = HeadingLevel::level1,
        h2 = HeadingLevel::level2,
        h3 = HeadingLevel::level3,
        h4 = HeadingLevel::level4,
        h5 = HeadingLevel::level5,
        h6 = HeadingLevel::level6,
    };

    static HeadingTag* getInstance() noexcept
    {
        return jam::SharedInstance<HeadingTag>::getInstance();
    }
};

//==============================================================================

/**
 * @brief Sharp key names.
 *
 * Maps each sharp musical key name (C, C#, D, … B) to its key. The key is the
 * pitch-class ordinal used by the note-name conversion tables.
 */
struct Sharps : public jam::Bimap<int>
{
    Sharps() : jam::Bimap<int> { {
            { c,      juce::String::fromUTF8 ("C") },
            { cSharp, juce::String::fromUTF8 ("C#") },
            { d,      juce::String::fromUTF8 ("D") },
            { dSharp, juce::String::fromUTF8 ("D#") },
            { e,      juce::String::fromUTF8 ("E") },
            { f,      juce::String::fromUTF8 ("F") },
            { fSharp, juce::String::fromUTF8 ("F#") },
            { g,      juce::String::fromUTF8 ("G") },
            { gSharp, juce::String::fromUTF8 ("G#") },
            { a,      juce::String::fromUTF8 ("A") },
            { aSharp, juce::String::fromUTF8 ("A#") },
            { b,      juce::String::fromUTF8 ("B") },
    } } {}

    enum value : int
    {
        c      = 0,
        cSharp = 1,
        d      = 2,
        dSharp = 3,
        e      = 4,
        f      = 5,
        fSharp = 6,
        g      = 7,
        gSharp = 8,
        a      = 9,
        aSharp = 10,
        b      = 11,
    };

    static Sharps* getInstance() noexcept
    {
        return jam::SharedInstance<Sharps>::getInstance();
    }
};

//==============================================================================

/**
 * @brief Flat key names.
 *
 * Maps each flat musical key name (C, Db, D, … B) to its key. The key is the
 * pitch-class ordinal used by the note-name conversion tables.
 */
struct Flats : public jam::Bimap<int>
{
    Flats() : jam::Bimap<int> { {
            { c,     juce::String::fromUTF8 ("C") },
            { dFlat, juce::String::fromUTF8 ("Db") },
            { d,     juce::String::fromUTF8 ("D") },
            { eFlat, juce::String::fromUTF8 ("Eb") },
            { e,     juce::String::fromUTF8 ("E") },
            { f,     juce::String::fromUTF8 ("F") },
            { gFlat, juce::String::fromUTF8 ("Gb") },
            { g,     juce::String::fromUTF8 ("G") },
            { aFlat, juce::String::fromUTF8 ("Ab") },
            { a,     juce::String::fromUTF8 ("A") },
            { bFlat, juce::String::fromUTF8 ("Bb") },
            { b,     juce::String::fromUTF8 ("B") },
    } } {}

    enum value : int
    {
        c     = 0,
        dFlat = 1,
        d     = 2,
        eFlat = 3,
        e     = 4,
        f     = 5,
        gFlat = 6,
        g     = 7,
        aFlat = 8,
        a     = 9,
        bFlat = 10,
        b     = 11,
    };

    static Flats* getInstance() noexcept
    {
        return jam::SharedInstance<Flats>::getInstance();
    }
};

//==============================================================================

/**
 * @brief HTML block start-condition kinds (§4.6).
 *
 * Maps each HTML block start condition — comment, processing instruction,
 * declaration, and CDATA (plus the none fallback) — to its key. The key
 * indexes the opening/closing delimiter tables (`map::markupOpen` /
 * `map::markupClose`) that bracket the raw block.
 */
struct HtmlBlockType : public jam::Bimap<int>
{
    HtmlBlockType() : jam::Bimap<int> { {
            { none,                  juce::String::fromUTF8 ("none") },
            { rawText,               juce::String::fromUTF8 ("rawText") },
            { comment,               juce::String::fromUTF8 ("comment") },
            { processingInstruction, juce::String::fromUTF8 ("processingInstruction") },
            { declaration,           juce::String::fromUTF8 ("declaration") },
            { cdata,                 juce::String::fromUTF8 ("cdata") },
            { blockTag,              juce::String::fromUTF8 ("blockTag") },
            { anyTag,                juce::String::fromUTF8 ("anyTag") },
    } } {}

    enum value : int
    {
        none                  = 0,///< no §4.6 start condition matched
        rawText               = 1,///< §4.6 type 1 — script/pre/textarea/style raw text, ends on its close tag
        comment               = 2,///< §4.6 type 2 — <!-- … -->
        processingInstruction = 3,///< §4.6 type 3 — <? … ?>
        declaration           = 4,///< §4.6 type 4 — <! + ASCII letter … >
        cdata                 = 5,///< §4.6 type 5 — <![CDATA[ … ]]>
        blockTag              = 6,///< §4.6 type 6 — known block-level tag, ends on blank line
        anyTag                = 7,///< §4.6 type 7 — any complete open/closing tag, ends on blank line
    };

    static HtmlBlockType* getInstance() noexcept
    {
        return jam::SharedInstance<HtmlBlockType>::getInstance();
    }
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace map
