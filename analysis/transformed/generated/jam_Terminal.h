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
 * @file jam_Terminal.h
 * @brief Terminal control-sequence vocabulary — DEC/ANSI modes, CSI/ESC/OSC/SGR
 * codes, and the bidirectional registries mapping them to their names.
 */

#pragma once

namespace map
{
/*_____________________________________________________________________________*/

/**
 * @brief Terminal screen buffer selection — normal vs alternate.
 *
 * The key selects which screen buffer the terminal writes to. `normal` is the
 * primary buffer; `alternate` is the full-screen application buffer swapped in
 * by the DECSET 1049 sequence and swapped out on restore. The value is the
 * source-literal name serialized by the terminal emulator.
 */
struct Screen : public jam::Bimap<int>
{
    Screen() : jam::Bimap<int> { {
            { normal,    juce::String::fromUTF8 ("normal") },
            { alternate, juce::String::fromUTF8 ("alternate") },
    } } {}

    enum value : int
    {
        normal    = 0,
        alternate = 1,
    };

    static Screen* getInstance() noexcept
    {
        return jam::SharedInstance<Screen>::getInstance();
    }
};

//==============================================================================

/**
 * @brief Terminal mouse-tracking modes.
 *
 * Maps each mouse-tracking level — off, click-only, drag, and all-event — to
 * its key. The key is the mode reported by the terminal for the active
 * mouse-tracking DECSET value.
 */
struct MouseTracking : public jam::Bimap<int>
{
    MouseTracking() : jam::Bimap<int> { {
            { off,   juce::String::fromUTF8 ("off") },
            { click, juce::String::fromUTF8 ("click") },
            { drag,  juce::String::fromUTF8 ("drag") },
            { all,   juce::String::fromUTF8 ("all") },
    } } {}

    enum value : int
    {
        off   = 0,
        click = 1,
        drag  = 2,
        all   = 3,
    };

    static MouseTracking* getInstance() noexcept
    {
        return jam::SharedInstance<MouseTracking>::getInstance();
    }
};

//==============================================================================

/** @brief DEC private mode numbers used in CSI ? Ps h / CSI ? Ps l (DECSET/DECRST), DECRQM (CSI ? Ps $ p), and DECRQSS (CSI Ps $ q). */
struct DEC : public jam::Bimap<int>
{
    DEC() : jam::Bimap<int> { {
            { applicationCursor,     juce::String::fromUTF8 ("applicationCursor") },
            { columnMode,            juce::String::fromUTF8 ("columnMode") },
            { reverseVideo,          juce::String::fromUTF8 ("reverseVideo") },
            { originMode,            juce::String::fromUTF8 ("originMode") },
            { autoWrap,              juce::String::fromUTF8 ("autoWrap") },
            { autoRepeat,            juce::String::fromUTF8 ("autoRepeat") },
            { cursorVisible,         juce::String::fromUTF8 ("cursorVisible") },
            { numericKeypad,         juce::String::fromUTF8 ("numericKeypad") },
            { alternateScreenBuffer, juce::String::fromUTF8 ("alternateScreenBuffer") },
            { mouseClick,            juce::String::fromUTF8 ("mouseClick") },
            { mouseHighlight,        juce::String::fromUTF8 ("mouseHighlight") },
            { mouseDrag,             juce::String::fromUTF8 ("mouseDrag") },
            { mouseAll,              juce::String::fromUTF8 ("mouseAll") },
            { focusEvents,           juce::String::fromUTF8 ("focusEvents") },
            { mouseSgr,              juce::String::fromUTF8 ("mouseSgr") },
            { alternateScreenClear,  juce::String::fromUTF8 ("alternateScreenClear") },
            { decSaveCursor,         juce::String::fromUTF8 ("decSaveCursor") },
            { alternateScreen,       juce::String::fromUTF8 ("alternateScreen") },
            { bracketedPaste,        juce::String::fromUTF8 ("bracketedPaste") },
            { syncOutput,            juce::String::fromUTF8 ("syncOutput") },
            { graphemeClustering,    juce::String::fromUTF8 ("graphemeClustering") },
            { win32InputMode,        juce::String::fromUTF8 ("win32InputMode") },
    } } {}

    enum value : int
    {
        applicationCursor     = 1,   ///< DECCKM — application cursor keys
        columnMode            = 3,   ///< DECCOLM — column mode (132-column)
        reverseVideo          = 5,   ///< DECSCNM — reverse video (screen background/foreground swap)
        originMode            = 6,   ///< DECOM — origin mode (relative vs absolute cursor addressing)
        autoWrap              = 7,   ///< DECAWM — auto-wrap mode
        autoRepeat            = 8,   ///< DECARM — auto-repeat mode
        cursorVisible         = 25,  ///< DECTCEM — text cursor enable (visibility)
        numericKeypad         = 66,  ///< DECNKM — numeric keypad (application vs numeric)
        alternateScreenBuffer = 47,  ///< Alternate screen buffer (legacy, xterm)
        mouseClick            = 1000,///< Mouse tracking (X11 button-event)
        mouseHighlight        = 1001,///< Mouse highlight tracking
        mouseDrag             = 1002,///< Mouse button-event + motion-with-button
        mouseAll              = 1003,///< Mouse any-event tracking
        focusEvents           = 1004,///< Focus in/out events
        mouseSgr              = 1006,///< Mouse SGR encoding (vs UTF-8)
        alternateScreenClear  = 1047,///< Alternate screen + clear
        decSaveCursor         = 1048,///< Save cursor (DECSC variant — see file-doc's "Collision resolutions")
        alternateScreen       = 1049,///< Alternate screen + save/clear cursor
        bracketedPaste        = 2004,///< Bracketed paste mode
        syncOutput            = 2026,///< Synchronized output
        graphemeClustering    = 2027,///< UAX #29 grapheme cluster width advance
        win32InputMode        = 9001,///< Win32 keyboard input mode
    };

    static DEC* getInstance() noexcept
    {
        return jam::SharedInstance<DEC>::getInstance();
    }
};

//==============================================================================

/** @brief OSC (Operating System Command) command codes. */
struct OSC : public jam::Bimap<int>
{
    OSC() : jam::Bimap<int> { {
            { setWindowTitle,       juce::String::fromUTF8 ("setWindowTitle") },
            { setTitleOnly,         juce::String::fromUTF8 ("setTitleOnly") },
            { setPaletteEntry,      juce::String::fromUTF8 ("setPaletteEntry") },
            { setCwd,               juce::String::fromUTF8 ("setCwd") },
            { hyperlink,            juce::String::fromUTF8 ("hyperlink") },
            { desktopNotify,        juce::String::fromUTF8 ("desktopNotify") },
            { setDefaultForeground, juce::String::fromUTF8 ("setDefaultForeground") },
            { setDefaultBackground, juce::String::fromUTF8 ("setDefaultBackground") },
            { setCursorColor,       juce::String::fromUTF8 ("setCursorColor") },
            { setClipboard,         juce::String::fromUTF8 ("setClipboard") },
            { resetCursorColor,     juce::String::fromUTF8 ("resetCursorColor") },
            { shellIntegration,     juce::String::fromUTF8 ("shellIntegration") },
            { notifyTitleBody,      juce::String::fromUTF8 ("notifyTitleBody") },
            { iterm2Image,          juce::String::fromUTF8 ("iterm2Image") },
    } } {}

    enum value : int
    {
        setWindowTitle       = 0,   ///< Set icon + window title (OSC 0)
        setTitleOnly         = 2,   ///< Set window title only (OSC 2)
        setPaletteEntry      = 4,   ///< Set/query palette colour entry (OSC 4)
        setCwd               = 7,   ///< Set current working directory (OSC 7)
        hyperlink            = 8,   ///< Hyperlink open/close (OSC 8, informal spec)
        desktopNotify        = 9,   ///< Desktop notification, body only (OSC 9)
        setDefaultForeground = 10,  ///< Set default foreground colour (OSC 10)
        setDefaultBackground = 11,  ///< Set default background colour (OSC 11)
        setCursorColor       = 12,  ///< Set cursor colour (OSC 12)
        setClipboard         = 52,  ///< Read/set clipboard (OSC 52)
        resetCursorColor     = 112, ///< Reset cursor colour to default (OSC 112)
        shellIntegration     = 133, ///< Shell integration semantic markers (OSC 133)
        notifyTitleBody      = 777, ///< Desktop notification with title + body (OSC 777)
        iterm2Image          = 1337,///< iTerm2 inline image (OSC 1337)
    };

    static OSC* getInstance() noexcept
    {
        return jam::SharedInstance<OSC>::getInstance();
    }
};

//==============================================================================

/** @brief SGR (Select Graphic Rendition) parameter numbers. */
struct SGR : public jam::Bimap<int>
{
    SGR() : jam::Bimap<int> { {
            { reset,                  juce::String::fromUTF8 ("reset") },
            { bold,                   juce::String::fromUTF8 ("bold") },
            { dim,                    juce::String::fromUTF8 ("dim") },
            { italic,                 juce::String::fromUTF8 ("italic") },
            { underline,              juce::String::fromUTF8 ("underline") },
            { blink,                  juce::String::fromUTF8 ("blink") },
            { rapidBlink,             juce::String::fromUTF8 ("rapidBlink") },
            { inverse,                juce::String::fromUTF8 ("inverse") },
            { hidden,                 juce::String::fromUTF8 ("hidden") },
            { strike,                 juce::String::fromUTF8 ("strike") },
            { fontDefault,            juce::String::fromUTF8 ("fontDefault") },
            { doubleUnderline,        juce::String::fromUTF8 ("doubleUnderline") },
            { noBoldDim,              juce::String::fromUTF8 ("noBoldDim") },
            { noItalic,               juce::String::fromUTF8 ("noItalic") },
            { noUnderline,            juce::String::fromUTF8 ("noUnderline") },
            { noBlink,                juce::String::fromUTF8 ("noBlink") },
            { proportional,           juce::String::fromUTF8 ("proportional") },
            { noInverse,              juce::String::fromUTF8 ("noInverse") },
            { noHidden,               juce::String::fromUTF8 ("noHidden") },
            { noStrike,               juce::String::fromUTF8 ("noStrike") },
            { extendedForeground,     juce::String::fromUTF8 ("extendedForeground") },
            { defaultForeground,      juce::String::fromUTF8 ("defaultForeground") },
            { extendedBackground,     juce::String::fromUTF8 ("extendedBackground") },
            { defaultBackground,      juce::String::fromUTF8 ("defaultBackground") },
            { overline,               juce::String::fromUTF8 ("overline") },
            { noOverline,             juce::String::fromUTF8 ("noOverline") },
            { extendedUnderlineColor, juce::String::fromUTF8 ("extendedUnderlineColor") },
            { defaultUnderline,       juce::String::fromUTF8 ("defaultUnderline") },
            { superscript,            juce::String::fromUTF8 ("superscript") },
            { subscript,              juce::String::fromUTF8 ("subscript") },
            { noSuperscript,          juce::String::fromUTF8 ("noSuperscript") },
            { noSubscript,            juce::String::fromUTF8 ("noSubscript") },
            { fontFirst,              juce::String::fromUTF8 ("fontFirst") },
            { fontLast,               juce::String::fromUTF8 ("fontLast") },
            { foregroundFirst,        juce::String::fromUTF8 ("foregroundFirst") },
            { foregroundLast,         juce::String::fromUTF8 ("foregroundLast") },
            { backgroundFirst,        juce::String::fromUTF8 ("backgroundFirst") },
            { backgroundLast,         juce::String::fromUTF8 ("backgroundLast") },
            { brightForegroundFirst,  juce::String::fromUTF8 ("brightForegroundFirst") },
            { brightForegroundLast,   juce::String::fromUTF8 ("brightForegroundLast") },
            { brightBackgroundFirst,  juce::String::fromUTF8 ("brightBackgroundFirst") },
            { brightBackgroundLast,   juce::String::fromUTF8 ("brightBackgroundLast") },
    } } {}

    enum value : int
    {
        reset                  = 0,  ///< Reset all attributes
        bold                   = 1,  ///< Bold / increased intensity
        dim                    = 2,  ///< Dim / decreased intensity / faint
        italic                 = 3,  ///< Italic
        underline              = 4,  ///< Underline (sub-param 4:n selects style)
        blink                  = 5,  ///< Blink (slow)
        rapidBlink             = 6,  ///< Rapid blink (xterm maps to slow)
        inverse                = 7,  ///< Inverse / reverse video
        hidden                 = 8,  ///< Hidden / concealed
        strike                 = 9,  ///< Strikethrough
        fontDefault            = 10, ///< Default font
        doubleUnderline        = 21, ///< Doubly underlined (ECMA-48 §8.3.117; xterm/kitty/ghostty consensus)
        noBoldDim              = 22, ///< Normal weight (bold off, dim off)
        noItalic               = 23, ///< Italic off
        noUnderline            = 24, ///< Underline off
        noBlink                = 25, ///< Blink off
        proportional           = 26, ///< Proportional spacing
        noInverse              = 27, ///< Inverse off
        noHidden               = 28, ///< Concealed off
        noStrike               = 29, ///< Strikethrough off
        extendedForeground     = 38, ///< Extended foreground colour (sub-param: 5:idx or 2:r:g:b)
        defaultForeground      = 39, ///< Default foreground
        extendedBackground     = 48, ///< Extended background colour (sub-param: 5:idx or 2:r:g:b)
        defaultBackground      = 49, ///< Default background
        overline               = 53, ///< Overline
        noOverline             = 55, ///< Overline off
        extendedUnderlineColor = 58, ///< Extended underline colour (sub-param: 5:idx or 2:r:g:b)
        defaultUnderline       = 59, ///< Default underline colour (follow fg)
        superscript            = 73, ///< Superscript (baseline up)
        subscript              = 74, ///< Subscript (baseline down)
        noSuperscript          = 75, ///< Superscript off
        noSubscript            = 76, ///< Subscript off
        fontFirst              = 11, ///< first font-select code (fonts 1-9 are 11-19)
        fontLast               = 19, ///< last font-select code
        foregroundFirst        = 30, ///< first standard foreground colour code
        foregroundLast         = 37, ///< last standard foreground colour code
        backgroundFirst        = 40, ///< first standard background colour code
        backgroundLast         = 47, ///< last standard background colour code
        brightForegroundFirst  = 90, ///< first bright foreground colour code
        brightForegroundLast   = 97, ///< last bright foreground colour code
        brightBackgroundFirst  = 100,///< first bright background colour code
        brightBackgroundLast   = 107,///< last bright background colour code
    };

    static SGR* getInstance() noexcept
    {
        return jam::SharedInstance<SGR>::getInstance();
    }
};

//==============================================================================

/** @brief SGR extended-colour sub-type discriminators (the value following 38/48/58). */
struct ColorMode : public jam::Bimap<int>
{
    ColorMode() : jam::Bimap<int> { {
            { rgb24,      juce::String::fromUTF8 ("rgb24") },
            { palette256, juce::String::fromUTF8 ("palette256") },
    } } {}

    enum value : int
    {
        rgb24      = 2,///< 24-bit RGB triple follows
        palette256 = 5,///< 256-colour palette index follows
    };

    static ColorMode* getInstance() noexcept
    {
        return jam::SharedInstance<ColorMode>::getInstance();
    }
};

//==============================================================================

/** @brief SGR 4:n underline-style sub-parameter values. */
struct UnderlineStyle : public jam::Bimap<int>
{
    UnderlineStyle() : jam::Bimap<int> { {
            { none,       juce::String::fromUTF8 ("none") },
            { single,     juce::String::fromUTF8 ("single") },
            { doubleLine, juce::String::fromUTF8 ("doubleLine") },
            { curly,      juce::String::fromUTF8 ("curly") },
            { dotted,     juce::String::fromUTF8 ("dotted") },
            { dashed,     juce::String::fromUTF8 ("dashed") },
    } } {}

    enum value : int
    {
        none       = 0,///< No underline
        single     = 1,///< Single underline
        doubleLine = 2,///< Double underline
        curly      = 3,///< Curly underline
        dotted     = 4,///< Dotted underline
        dashed     = 5,///< Dashed underline
    };

    static UnderlineStyle* getInstance() noexcept
    {
        return jam::SharedInstance<UnderlineStyle>::getInstance();
    }
};

//==============================================================================

/** @brief CSI final bytes — the byte in 0x40-0x7E that identifies the command. */
struct CSI : public jam::Bimap<int>
{
    CSI() : jam::Bimap<int> { {
            { insertCharacter,            juce::String::fromUTF8 ("insertCharacter") },
            { cursorUp,                   juce::String::fromUTF8 ("cursorUp") },
            { cursorDown,                 juce::String::fromUTF8 ("cursorDown") },
            { cursorForward,              juce::String::fromUTF8 ("cursorForward") },
            { cursorBack,                 juce::String::fromUTF8 ("cursorBack") },
            { cursorNextLine,             juce::String::fromUTF8 ("cursorNextLine") },
            { cursorPreviousLine,         juce::String::fromUTF8 ("cursorPreviousLine") },
            { cursorHorizontalAbsolute,   juce::String::fromUTF8 ("cursorHorizontalAbsolute") },
            { cursorPosition,             juce::String::fromUTF8 ("cursorPosition") },
            { cursorForwardTabulation,    juce::String::fromUTF8 ("cursorForwardTabulation") },
            { eraseInDisplay,             juce::String::fromUTF8 ("eraseInDisplay") },
            { eraseInLine,                juce::String::fromUTF8 ("eraseInLine") },
            { insertLine,                 juce::String::fromUTF8 ("insertLine") },
            { deleteLine,                 juce::String::fromUTF8 ("deleteLine") },
            { deleteCharacter,            juce::String::fromUTF8 ("deleteCharacter") },
            { scrollUp,                   juce::String::fromUTF8 ("scrollUp") },
            { scrollDown,                 juce::String::fromUTF8 ("scrollDown") },
            { eraseCharacter,             juce::String::fromUTF8 ("eraseCharacter") },
            { cursorBackwardTabulation,   juce::String::fromUTF8 ("cursorBackwardTabulation") },
            { horizontalPositionAbsolute, juce::String::fromUTF8 ("horizontalPositionAbsolute") },
            { horizontalPositionRelative, juce::String::fromUTF8 ("horizontalPositionRelative") },
            { repeatCharacter,            juce::String::fromUTF8 ("repeatCharacter") },
            { deviceAttributes,           juce::String::fromUTF8 ("deviceAttributes") },
            { verticalPositionAbsolute,   juce::String::fromUTF8 ("verticalPositionAbsolute") },
            { verticalPositionRelative,   juce::String::fromUTF8 ("verticalPositionRelative") },
            { horizontalVerticalPosition, juce::String::fromUTF8 ("horizontalVerticalPosition") },
            { tabulationClear,            juce::String::fromUTF8 ("tabulationClear") },
            { setMode,                    juce::String::fromUTF8 ("setMode") },
            { resetMode,                  juce::String::fromUTF8 ("resetMode") },
            { selectGraphicRendition,     juce::String::fromUTF8 ("selectGraphicRendition") },
            { deviceStatusReport,         juce::String::fromUTF8 ("deviceStatusReport") },
            { requestMode,                juce::String::fromUTF8 ("requestMode") },
            { setCursorStyle,             juce::String::fromUTF8 ("setCursorStyle") },
            { setScrollingRegion,         juce::String::fromUTF8 ("setScrollingRegion") },
            { windowOps,                  juce::String::fromUTF8 ("windowOps") },
            { keyboardProtocol,           juce::String::fromUTF8 ("keyboardProtocol") },
    } } {}

    enum value : int
    {
        insertCharacter            = Chars::at,      ///< Insert character(s) (ICH)
        cursorUp                   = Chars::upperA,  ///< Cursor up (CUU)
        cursorDown                 = Chars::upperB,  ///< Cursor down (CUD)
        cursorForward              = Chars::upperC,  ///< Cursor forward / right (CUF)
        cursorBack                 = Chars::upperD,  ///< Cursor backward / left (CUB)
        cursorNextLine             = Chars::upperE,  ///< Cursor next line (CNL)
        cursorPreviousLine         = Chars::upperF,  ///< Cursor previous line (CPL)
        cursorHorizontalAbsolute   = Chars::upperG,  ///< Cursor horizontal absolute (CHA)
        cursorPosition             = Chars::upperH,  ///< Cursor position (CUP)
        cursorForwardTabulation    = Chars::upperI,  ///< Cursor forward tabulation (CHT)
        eraseInDisplay             = Chars::upperJ,  ///< Erase in display (ED)
        eraseInLine                = Chars::upperK,  ///< Erase in line (EL)
        insertLine                 = Chars::upperL,  ///< Insert line(s) (IL)
        deleteLine                 = Chars::upperM,  ///< Delete line(s) (DL)
        deleteCharacter            = Chars::upperP,  ///< Delete character(s) (DCH)
        scrollUp                   = Chars::upperS,  ///< Scroll up (SU)
        scrollDown                 = Chars::upperT,  ///< Scroll down (SD)
        eraseCharacter             = Chars::upperX,  ///< Erase character(s) (ECH)
        cursorBackwardTabulation   = Chars::upperZ,  ///< Cursor backward tabulation (CBT)
        horizontalPositionAbsolute = Chars::backtick,///< Horizontal position absolute (HPA)
        horizontalPositionRelative = Chars::lowerA,  ///< Horizontal position relative (HPR)
        repeatCharacter            = Chars::lowerB,  ///< Repeat preceding graphic character (REP)
        deviceAttributes           = Chars::lowerC,  ///< Device attributes (DA)
        verticalPositionAbsolute   = Chars::lowerD,  ///< Vertical position absolute (VPA)
        verticalPositionRelative   = Chars::lowerE,  ///< Vertical position relative (VPR)
        horizontalVerticalPosition = Chars::lowerF,  ///< Horizontal vertical position (HVP)
        tabulationClear            = Chars::lowerG,  ///< Tabulation clear (TBC)
        setMode                    = Chars::lowerH,  ///< Set mode, private/ANSI (SM)
        resetMode                  = Chars::lowerL,  ///< Reset mode, private/ANSI (RM)
        selectGraphicRendition     = Chars::lowerM,  ///< Select graphic rendition (SGR)
        deviceStatusReport         = Chars::lowerN,  ///< Device status report (DSR)
        requestMode                = Chars::lowerP,  ///< DECRQM — Request Mode (CSI ? Pd $ p; the intermediate '$' is inter[], never the final byte)
        setCursorStyle             = Chars::lowerQ,  ///< DECSCUSR — set cursor style (intermediate ' ')
        setScrollingRegion         = Chars::lowerR,  ///< DECSTBM — set top/bottom margins
        windowOps                  = Chars::lowerT,  ///< Window operations (resize/report)
        keyboardProtocol           = Chars::lowerU,  ///< Progressive keyboard protocol
    };

    static CSI* getInstance() noexcept
    {
        return jam::SharedInstance<CSI>::getInstance();
    }
};

//==============================================================================

/** @brief CSI t (window-ops) sub-command values. */
struct WindowOps : public jam::Bimap<int>
{
    WindowOps() : jam::Bimap<int> { {
            { reportTextPixels, juce::String::fromUTF8 ("reportTextPixels") },
            { reportCellPixels, juce::String::fromUTF8 ("reportCellPixels") },
            { reportTextChars,  juce::String::fromUTF8 ("reportTextChars") },
    } } {}

    enum value : int
    {
        reportTextPixels = 14,///< Report text area in pixels
        reportCellPixels = 16,///< Report cell size in pixels
        reportTextChars  = 18,///< Report text area in characters
    };

    static WindowOps* getInstance() noexcept
    {
        return jam::SharedInstance<WindowOps>::getInstance();
    }
};

//==============================================================================

/** @brief CSI n (DSR) sub-command values. */
struct DSR : public jam::Bimap<int>
{
    DSR() : jam::Bimap<int> { {
            { status,               juce::String::fromUTF8 ("status") },
            { reportCursorPosition, juce::String::fromUTF8 ("reportCursorPosition") },
    } } {}

    enum value : int
    {
        status               = 5,///< Report terminal status (OK / not OK)
        reportCursorPosition = 6,///< Report cursor position (CPR)
    };

    static DSR* getInstance() noexcept
    {
        return jam::SharedInstance<DSR>::getInstance();
    }
};

//==============================================================================

/** @brief CSI g (TBC) sub-mode values. */
struct TabClear : public jam::Bimap<int>
{
    TabClear() : jam::Bimap<int> { {
            { currentColumn, juce::String::fromUTF8 ("currentColumn") },
            { allStops,      juce::String::fromUTF8 ("allStops") },
    } } {}

    enum value : int
    {
        currentColumn = 0,///< Clear tab stop at cursor column
        allStops      = 3,///< Clear all tab stops
    };

    static TabClear* getInstance() noexcept
    {
        return jam::SharedInstance<TabClear>::getInstance();
    }
};

//==============================================================================

/** @brief DECSCUSR (CSI Ps SP q) cursor-shape codes. */
struct CursorShape : public jam::Bimap<int>
{
    CursorShape() : jam::Bimap<int> { {
            { defaultShape,      juce::String::fromUTF8 ("defaultShape") },
            { blinkingBlock,     juce::String::fromUTF8 ("blinkingBlock") },
            { steadyBlock,       juce::String::fromUTF8 ("steadyBlock") },
            { blinkingUnderline, juce::String::fromUTF8 ("blinkingUnderline") },
            { steadyUnderline,   juce::String::fromUTF8 ("steadyUnderline") },
            { blinkingBar,       juce::String::fromUTF8 ("blinkingBar") },
            { steadyBar,         juce::String::fromUTF8 ("steadyBar") },
    } } {}

    enum value : int
    {
        defaultShape      = 0,///< Blinking block (terminal default)
        blinkingBlock     = 1,///< blinkingBlock
        steadyBlock       = 2,///< steadyBlock
        blinkingUnderline = 3,///< blinkingUnderline
        steadyUnderline   = 4,///< steadyUnderline
        blinkingBar       = 5,///< blinkingBar
        steadyBar         = 6,///< steadyBar
    };

    static CursorShape* getInstance() noexcept
    {
        return jam::SharedInstance<CursorShape>::getInstance();
    }
};

//==============================================================================

/** @brief DECRQSS (CSI Ps $ q) recognised setting codes. */
struct DECRQSS : public jam::Bimap<int>
{
    DECRQSS() : jam::Bimap<int> { {
            { decrqssCursorPosition,        juce::String::fromUTF8 ("decrqssCursorPosition") },
            { decrqssOriginMode,            juce::String::fromUTF8 ("decrqssOriginMode") },
            { decrqssAutoWrap,              juce::String::fromUTF8 ("decrqssAutoWrap") },
            { decrqssCursorVisible,         juce::String::fromUTF8 ("decrqssCursorVisible") },
            { decrqssAlternateScreenBuffer, juce::String::fromUTF8 ("decrqssAlternateScreenBuffer") },
            { decrqssAlternateScreenClear,  juce::String::fromUTF8 ("decrqssAlternateScreenClear") },
            { decrqssAlternateScreen,       juce::String::fromUTF8 ("decrqssAlternateScreen") },
            { decrqssSyncOutput,            juce::String::fromUTF8 ("decrqssSyncOutput") },
    } } {}

    enum value : int
    {
        decrqssCursorPosition        = 1,   ///< Cursor position
        decrqssOriginMode            = 6,   ///< Origin mode
        decrqssAutoWrap              = 7,   ///< Auto-wrap mode
        decrqssCursorVisible         = 25,  ///< Cursor visibility
        decrqssAlternateScreenBuffer = 47,  ///< Alt screen (legacy)
        decrqssAlternateScreenClear  = 1047,///< Alt screen + clear
        decrqssAlternateScreen       = 1049,///< Alt screen + save/clear cursor
        decrqssSyncOutput            = 2026,///< Sync output
    };

    static DECRQSS* getInstance() noexcept
    {
        return jam::SharedInstance<DECRQSS>::getInstance();
    }
};

//==============================================================================

/** @brief CSI intermediate bytes — the bytes preceding the final that select a private or otherwise-disambiguated command variant. */
struct CsiIntermediate : public jam::Bimap<int>
{
    CsiIntermediate() : jam::Bimap<int> { {
            { privateMarker, juce::String::fromUTF8 ("privateMarker") },
            { greaterThan,   juce::String::fromUTF8 ("greaterThan") },
            { lessThan,      juce::String::fromUTF8 ("lessThan") },
            { equals,        juce::String::fromUTF8 ("equals") },
            { space,         juce::String::fromUTF8 ("space") },
            { dollar,        juce::String::fromUTF8 ("dollar") },
    } } {}

    enum value : int
    {
        privateMarker = Chars::question,   ///< Private-mode marker (DECSET/DECRST, DECRQM, secondary DA)
        greaterThan   = Chars::greaterThan,///< Secondary DA / push keyboard flags
        lessThan      = Chars::lessThan,   ///< Pop keyboard flags
        equals        = Chars::equals,     ///< Set/OR keyboard flags directly
        space         = Chars::space,      ///< DECSCUSR intermediate (CSI Ps SP q)
        dollar        = Chars::dollarSign, ///< DECRQM / DECRQSS intermediate (CSI ... $ p/q)
    };

    static CsiIntermediate* getInstance() noexcept
    {
        return jam::SharedInstance<CsiIntermediate>::getInstance();
    }
};

//==============================================================================

/** @brief ESC final bytes for the most common two-byte ESC sequences. */
struct ESC : public jam::Bimap<int>
{
    ESC() : jam::Bimap<int> { {
            { index,               juce::String::fromUTF8 ("index") },
            { nextLine,            juce::String::fromUTF8 ("nextLine") },
            { horizontalTabSet,    juce::String::fromUTF8 ("horizontalTabSet") },
            { reverseIndex,        juce::String::fromUTF8 ("reverseIndex") },
            { resetToInitialState, juce::String::fromUTF8 ("resetToInitialState") },
            { saveCursor,          juce::String::fromUTF8 ("saveCursor") },
            { restoreCursor,       juce::String::fromUTF8 ("restoreCursor") },
            { applicationKeypad,   juce::String::fromUTF8 ("applicationKeypad") },
            { normalKeypad,        juce::String::fromUTF8 ("normalKeypad") },
    } } {}

    enum value : int
    {
        index               = Chars::upperD,     ///< Index — line feed without CR (IND)
        nextLine            = Chars::upperE,     ///< Next line — CR + IND (NEL)
        horizontalTabSet    = Chars::upperH,     ///< Horizontal tab set / set tab stop (HTS)
        reverseIndex        = Chars::upperM,     ///< Reverse index — scroll down or cursor up (RI)
        resetToInitialState = Chars::lowerC,     ///< Reset to initial state, hard reset (RIS)
        saveCursor          = Chars::seven,      ///< DECSC — save cursor
        restoreCursor       = Chars::eight,      ///< DECRC — restore cursor
        applicationKeypad   = Chars::equals,     ///< DECKPAM — application keypad
        normalKeypad        = Chars::greaterThan,///< DECKPNM — normal keypad
    };

    static ESC* getInstance() noexcept
    {
        return jam::SharedInstance<ESC>::getInstance();
    }
};

//==============================================================================

/** @brief ESC charset designator intermediate bytes. */
struct CharsetIntermediate : public jam::Bimap<int>
{
    CharsetIntermediate() : jam::Bimap<int> { {
            { g0, juce::String::fromUTF8 ("g0") },
            { g1, juce::String::fromUTF8 ("g1") },
            { g2, juce::String::fromUTF8 ("g2") },
            { g3, juce::String::fromUTF8 ("g3") },
    } } {}

    enum value : int
    {
        g0 = Chars::openParen, ///< Select G0 charset
        g1 = Chars::closeParen,///< Select G1 charset
        g2 = Chars::asterisk,  ///< Select G2 charset
        g3 = Chars::plus,      ///< Select G3 charset
    };

    static CharsetIntermediate* getInstance() noexcept
    {
        return jam::SharedInstance<CharsetIntermediate>::getInstance();
    }
};

//==============================================================================

/** @brief ESC charset designator final bytes. */
struct CharsetDesignator : public jam::Bimap<int>
{
    CharsetDesignator() : jam::Bimap<int> { {
            { ascii,       juce::String::fromUTF8 ("ascii") },
            { decLineDraw, juce::String::fromUTF8 ("decLineDraw") },
    } } {}

    enum value : int
    {
        ascii       = Chars::upperB,///< US ASCII
        decLineDraw = Chars::zero,  ///< DEC Special Graphics (line drawing)
    };

    static CharsetDesignator* getInstance() noexcept
    {
        return jam::SharedInstance<CharsetDesignator>::getInstance();
    }
};

//==============================================================================

/** @brief DEC-private ESC sequence intermediate byte (ESC # ...). */
struct DecEscIntermediate : public jam::Bimap<int>
{
    DecEscIntermediate() : jam::Bimap<int> { {
            { hash, juce::String::fromUTF8 ("hash") },
    } } {}

    enum value : int
    {
        hash = Chars::hash,///< DEC private intermediate (e.g. ESC # 8 = DECALN)
    };

    static DecEscIntermediate* getInstance() noexcept
    {
        return jam::SharedInstance<DecEscIntermediate>::getInstance();
    }
};

//==============================================================================

/** @brief DEC private ESC final byte for DECALN (screen alignment test). */
struct DecEscFinal : public jam::Bimap<int>
{
    DecEscFinal() : jam::Bimap<int> { {
            { screenAlignmentTest, juce::String::fromUTF8 ("screenAlignmentTest") },
    } } {}

    enum value : int
    {
        screenAlignmentTest = Chars::eight,///< DECALN — fill screen with 'E'
    };

    static DecEscFinal* getInstance() noexcept
    {
        return jam::SharedInstance<DecEscFinal>::getInstance();
    }
};

//==============================================================================

/** @brief DECRQM response state values — the n in CSI ? Ps ; n $ y. */
struct ModeReport : public jam::Bimap<int>
{
    ModeReport() : jam::Bimap<int> { {
            { notRecognised,    juce::String::fromUTF8 ("notRecognised") },
            { set,              juce::String::fromUTF8 ("set") },
            { modeReportReset,  juce::String::fromUTF8 ("modeReportReset") },
            { permanentlySet,   juce::String::fromUTF8 ("permanentlySet") },
            { permanentlyReset, juce::String::fromUTF8 ("permanentlyReset") },
    } } {}

    enum value : int
    {
        notRecognised    = 0,///< Mode not recognised
        set              = 1,///< Mode is currently set
        modeReportReset  = 2,///< Mode is currently reset (family-qualified — see file-doc's "Collision resolutions")
        permanentlySet   = 3,///< Mode is permanently set (not user-toggleable)
        permanentlyReset = 4,///< Mode is permanently reset
    };

    static ModeReport* getInstance() noexcept
    {
        return jam::SharedInstance<ModeReport>::getInstance();
    }
};

//==============================================================================

/** @brief ANSI mode numbers used in CSI Ps h / CSI Ps l (SM/RM). */
struct ANSI : public jam::Bimap<int>
{
    ANSI() : jam::Bimap<int> { {
            { insertMode,  juce::String::fromUTF8 ("insertMode") },
            { newLineMode, juce::String::fromUTF8 ("newLineMode") },
    } } {}

    enum value : int
    {
        insertMode  = 4, ///< IRM — insert/replace mode
        newLineMode = 20,///< LNM — line feed / new-line mode
    };

    static ANSI* getInstance() noexcept
    {
        return jam::SharedInstance<ANSI>::getInstance();
    }
};

//==============================================================================

/** @brief OSC 133 sub-command letters (the first byte after 133;). */
struct ShellIntegration : public jam::Bimap<int>
{
    ShellIntegration() : jam::Bimap<int> { {
            { promptStart,  juce::String::fromUTF8 ("promptStart") },
            { commandStart, juce::String::fromUTF8 ("commandStart") },
            { outputStart,  juce::String::fromUTF8 ("outputStart") },
            { outputEnd,    juce::String::fromUTF8 ("outputEnd") },
    } } {}

    enum value : int
    {
        promptStart  = Chars::upperA,///< A — prompt start (fires promptRow)
        commandStart = Chars::upperB,///< B — command start (no-op for output tracking)
        outputStart  = Chars::upperC,///< C — command output start (fires outputBlockStart)
        outputEnd    = Chars::upperD,///< D — command output end (fires outputBlockEnd)
    };

    static ShellIntegration* getInstance() noexcept
    {
        return jam::SharedInstance<ShellIntegration>::getInstance();
    }
};

//==============================================================================

/** @brief Progressive keyboard protocol assignment sub-parameter values — the mode in CSI = flags ; mode u. */
struct KeyboardAssignMode : public jam::Bimap<int>
{
    KeyboardAssignMode() : jam::Bimap<int> { {
            { keyboardSetAllFlags,     juce::String::fromUTF8 ("keyboardSetAllFlags") },
            { keyboardSetGivenFlags,   juce::String::fromUTF8 ("keyboardSetGivenFlags") },
            { keyboardResetGivenFlags, juce::String::fromUTF8 ("keyboardResetGivenFlags") },
    } } {}

    enum value : int
    {
        keyboardSetAllFlags     = 1,///< Replace current flags with the given flags
        keyboardSetGivenFlags   = 2,///< OR the given flags into current flags
        keyboardResetGivenFlags = 3,///< AND-NOT the given flags out of current flags
    };

    static KeyboardAssignMode* getInstance() noexcept
    {
        return jam::SharedInstance<KeyboardAssignMode>::getInstance();
    }
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace map
