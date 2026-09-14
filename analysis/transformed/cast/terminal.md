## Screen

```
@brief Terminal screen buffer selection — normal vs alternate.

The key selects which screen buffer the terminal writes to. `normal` is the
primary buffer; `alternate` is the full-screen application buffer swapped in
by the DECSET 1049 sequence and swapped out on restore. The value is the
source-literal name serialized by the terminal emulator.
```

+-----------+-----+
| name      | key |
+===========+=====+
| normal    | 0   |
| alternate | 1   |
+-----------+-----+

## MouseTracking

```
@brief Terminal mouse-tracking modes.

Maps each mouse-tracking level — off, click-only, drag, and all-event — to
its key. The key is the mode reported by the terminal for the active
mouse-tracking DECSET value.
```

+-------+-----+
| name  | key |
+=======+=====+
| off   | 0   |
| click | 1   |
| drag  | 2   |
| all   | 3   |
+-------+-----+

## DEC

@brief DEC private mode numbers used in CSI ? Ps h / CSI ? Ps l (DECSET/DECRST), DECRQM (CSI ? Ps $ p), and DECRQSS (CSI Ps $ q).

+-----------------------+------+----------------------------------------------------------------------+
| name                  | key  | comment                                                              |
+=======================+======+======================================================================+
| applicationCursor     | 1    | DECCKM — application cursor keys                                     |
| columnMode            | 3    | DECCOLM — column mode (132-column)                                   |
| reverseVideo          | 5    | DECSCNM — reverse video (screen background/foreground swap)          |
| originMode            | 6    | DECOM — origin mode (relative vs absolute cursor addressing)         |
| autoWrap              | 7    | DECAWM — auto-wrap mode                                              |
| autoRepeat            | 8    | DECARM — auto-repeat mode                                            |
| cursorVisible         | 25   | DECTCEM — text cursor enable (visibility)                            |
| numericKeypad         | 66   | DECNKM — numeric keypad (application vs numeric)                     |
| alternateScreenBuffer | 47   | Alternate screen buffer (legacy, xterm)                              |
| mouseClick            | 1000 | Mouse tracking (X11 button-event)                                    |
| mouseHighlight        | 1001 | Mouse highlight tracking                                             |
| mouseDrag             | 1002 | Mouse button-event + motion-with-button                              |
| mouseAll              | 1003 | Mouse any-event tracking                                             |
| focusEvents           | 1004 | Focus in/out events                                                  |
| mouseSgr              | 1006 | Mouse SGR encoding (vs UTF-8)                                        |
| alternateScreenClear  | 1047 | Alternate screen + clear                                             |
| decSaveCursor         | 1048 | Save cursor (DECSC variant — see file-doc's "Collision resolutions") |
| alternateScreen       | 1049 | Alternate screen + save/clear cursor                                 |
| bracketedPaste        | 2004 | Bracketed paste mode                                                 |
| syncOutput            | 2026 | Synchronized output                                                  |
| graphemeClustering    | 2027 | UAX #29 grapheme cluster width advance                               |
| win32InputMode        | 9001 | Win32 keyboard input mode                                            |
+-----------------------+------+----------------------------------------------------------------------+

## OSC

@brief OSC (Operating System Command) command codes.

+----------------------+------+--------------------------------------------------+
| name                 | key  | comment                                          |
+======================+======+==================================================+
| setWindowTitle       | 0    | Set icon + window title (OSC 0)                  |
| setTitleOnly         | 2    | Set window title only (OSC 2)                    |
| setPaletteEntry      | 4    | Set/query palette colour entry (OSC 4)           |
| setCwd               | 7    | Set current working directory (OSC 7)            |
| hyperlink            | 8    | Hyperlink open/close (OSC 8, informal spec)      |
| desktopNotify        | 9    | Desktop notification, body only (OSC 9)          |
| setDefaultForeground | 10   | Set default foreground colour (OSC 10)           |
| setDefaultBackground | 11   | Set default background colour (OSC 11)           |
| setCursorColor       | 12   | Set cursor colour (OSC 12)                       |
| setClipboard         | 52   | Read/set clipboard (OSC 52)                      |
| resetCursorColor     | 112  | Reset cursor colour to default (OSC 112)         |
| shellIntegration     | 133  | Shell integration semantic markers (OSC 133)     |
| notifyTitleBody      | 777  | Desktop notification with title + body (OSC 777) |
| iterm2Image          | 1337 | iTerm2 inline image (OSC 1337)                   |
+----------------------+------+--------------------------------------------------+

## SGR

@brief SGR (Select Graphic Rendition) parameter numbers.

+------------------------+-----+---------------------------------------------------------------------+
| name                   | key | comment                                                             |
+========================+=====+=====================================================================+
| reset                  | 0   | Reset all attributes                                                |
| bold                   | 1   | Bold / increased intensity                                          |
| dim                    | 2   | Dim / decreased intensity / faint                                   |
| italic                 | 3   | Italic                                                              |
| underline              | 4   | Underline (sub-param 4:n selects style)                             |
| blink                  | 5   | Blink (slow)                                                        |
| rapidBlink             | 6   | Rapid blink (xterm maps to slow)                                    |
| inverse                | 7   | Inverse / reverse video                                             |
| hidden                 | 8   | Hidden / concealed                                                  |
| strike                 | 9   | Strikethrough                                                       |
| fontDefault            | 10  | Default font                                                        |
| doubleUnderline        | 21  | Doubly underlined (ECMA-48 §8.3.117; xterm/kitty/ghostty consensus) |
| noBoldDim              | 22  | Normal weight (bold off, dim off)                                   |
| noItalic               | 23  | Italic off                                                          |
| noUnderline            | 24  | Underline off                                                       |
| noBlink                | 25  | Blink off                                                           |
| proportional           | 26  | Proportional spacing                                                |
| noInverse              | 27  | Inverse off                                                         |
| noHidden               | 28  | Concealed off                                                       |
| noStrike               | 29  | Strikethrough off                                                   |
| extendedForeground     | 38  | Extended foreground colour (sub-param: 5:idx or 2:r:g:b)            |
| defaultForeground      | 39  | Default foreground                                                  |
| extendedBackground     | 48  | Extended background colour (sub-param: 5:idx or 2:r:g:b)            |
| defaultBackground      | 49  | Default background                                                  |
| overline               | 53  | Overline                                                            |
| noOverline             | 55  | Overline off                                                        |
| extendedUnderlineColor | 58  | Extended underline colour (sub-param: 5:idx or 2:r:g:b)             |
| defaultUnderline       | 59  | Default underline colour (follow fg)                                |
| superscript            | 73  | Superscript (baseline up)                                           |
| subscript              | 74  | Subscript (baseline down)                                           |
| noSuperscript          | 75  | Superscript off                                                     |
| noSubscript            | 76  | Subscript off                                                       |
| fontFirst              | 11  | first font-select code (fonts 1-9 are 11-19)                        |
| fontLast               | 19  | last font-select code                                               |
| foregroundFirst        | 30  | first standard foreground colour code                               |
| foregroundLast         | 37  | last standard foreground colour code                                |
| backgroundFirst        | 40  | first standard background colour code                               |
| backgroundLast         | 47  | last standard background colour code                                |
| brightForegroundFirst  | 90  | first bright foreground colour code                                 |
| brightForegroundLast   | 97  | last bright foreground colour code                                  |
| brightBackgroundFirst  | 100 | first bright background colour code                                 |
| brightBackgroundLast   | 107 | last bright background colour code                                  |
+------------------------+-----+---------------------------------------------------------------------+

## ColorMode

@brief SGR extended-colour sub-type discriminators (the value following 38/48/58).

+------------+-----+----------------------------------+
| name       | key | comment                          |
+============+=====+==================================+
| rgb24      | 2   | 24-bit RGB triple follows        |
| palette256 | 5   | 256-colour palette index follows |
+------------+-----+----------------------------------+

## UnderlineStyle

@brief SGR 4:n underline-style sub-parameter values.

+------------+-----+------------------+
| name       | key | comment          |
+============+=====+==================+
| none       | 0   | No underline     |
| single     | 1   | Single underline |
| doubleLine | 2   | Double underline |
| curly      | 3   | Curly underline  |
| dotted     | 4   | Dotted underline |
| dashed     | 5   | Dashed underline |
+------------+-----+------------------+

## CSI

@brief CSI final bytes — the byte in 0x40-0x7E that identifies the command.

+----------------------------+-----------------+---------------------------------------------------------------------------------------------+
| name                       | key             | comment                                                                                     |
+============================+=================+=============================================================================================+
| insertCharacter            | Chars::at       | Insert character(s) (ICH)                                                                   |
| cursorUp                   | Chars::upperA   | Cursor up (CUU)                                                                             |
| cursorDown                 | Chars::upperB   | Cursor down (CUD)                                                                           |
| cursorForward              | Chars::upperC   | Cursor forward / right (CUF)                                                                |
| cursorBack                 | Chars::upperD   | Cursor backward / left (CUB)                                                                |
| cursorNextLine             | Chars::upperE   | Cursor next line (CNL)                                                                      |
| cursorPreviousLine         | Chars::upperF   | Cursor previous line (CPL)                                                                  |
| cursorHorizontalAbsolute   | Chars::upperG   | Cursor horizontal absolute (CHA)                                                            |
| cursorPosition             | Chars::upperH   | Cursor position (CUP)                                                                       |
| cursorForwardTabulation    | Chars::upperI   | Cursor forward tabulation (CHT)                                                             |
| eraseInDisplay             | Chars::upperJ   | Erase in display (ED)                                                                       |
| eraseInLine                | Chars::upperK   | Erase in line (EL)                                                                          |
| insertLine                 | Chars::upperL   | Insert line(s) (IL)                                                                         |
| deleteLine                 | Chars::upperM   | Delete line(s) (DL)                                                                         |
| deleteCharacter            | Chars::upperP   | Delete character(s) (DCH)                                                                   |
| scrollUp                   | Chars::upperS   | Scroll up (SU)                                                                              |
| scrollDown                 | Chars::upperT   | Scroll down (SD)                                                                            |
| eraseCharacter             | Chars::upperX   | Erase character(s) (ECH)                                                                    |
| cursorBackwardTabulation   | Chars::upperZ   | Cursor backward tabulation (CBT)                                                            |
| horizontalPositionAbsolute | Chars::backtick | Horizontal position absolute (HPA)                                                          |
| horizontalPositionRelative | Chars::lowerA   | Horizontal position relative (HPR)                                                          |
| repeatCharacter            | Chars::lowerB   | Repeat preceding graphic character (REP)                                                    |
| deviceAttributes           | Chars::lowerC   | Device attributes (DA)                                                                      |
| verticalPositionAbsolute   | Chars::lowerD   | Vertical position absolute (VPA)                                                            |
| verticalPositionRelative   | Chars::lowerE   | Vertical position relative (VPR)                                                            |
| horizontalVerticalPosition | Chars::lowerF   | Horizontal vertical position (HVP)                                                          |
| tabulationClear            | Chars::lowerG   | Tabulation clear (TBC)                                                                      |
| setMode                    | Chars::lowerH   | Set mode, private/ANSI (SM)                                                                 |
| resetMode                  | Chars::lowerL   | Reset mode, private/ANSI (RM)                                                               |
| selectGraphicRendition     | Chars::lowerM   | Select graphic rendition (SGR)                                                              |
| deviceStatusReport         | Chars::lowerN   | Device status report (DSR)                                                                  |
| requestMode                | Chars::lowerP   | DECRQM — Request Mode (CSI ? Pd $ p; the intermediate '$' is inter[], never the final byte) |
| setCursorStyle             | Chars::lowerQ   | DECSCUSR — set cursor style (intermediate ' ')                                              |
| setScrollingRegion         | Chars::lowerR   | DECSTBM — set top/bottom margins                                                            |
| windowOps                  | Chars::lowerT   | Window operations (resize/report)                                                           |
| keyboardProtocol           | Chars::lowerU   | Progressive keyboard protocol                                                               |
+----------------------------+-----------------+---------------------------------------------------------------------------------------------+

## WindowOps

@brief CSI t (window-ops) sub-command values.

+------------------+-----+--------------------------------+
| name             | key | comment                        |
+==================+=====+================================+
| reportTextPixels | 14  | Report text area in pixels     |
| reportCellPixels | 16  | Report cell size in pixels     |
| reportTextChars  | 18  | Report text area in characters |
+------------------+-----+--------------------------------+

## DSR

@brief CSI n (DSR) sub-command values.

+----------------------+-----+--------------------------------------+
| name                 | key | comment                              |
+======================+=====+======================================+
| status               | 5   | Report terminal status (OK / not OK) |
| reportCursorPosition | 6   | Report cursor position (CPR)         |
+----------------------+-----+--------------------------------------+

## TabClear

@brief CSI g (TBC) sub-mode values.

+---------------+-----+---------------------------------+
| name          | key | comment                         |
+===============+=====+=================================+
| currentColumn | 0   | Clear tab stop at cursor column |
| allStops      | 3   | Clear all tab stops             |
+---------------+-----+---------------------------------+

## CursorShape

@brief DECSCUSR (CSI Ps SP q) cursor-shape codes.

+-------------------+-----+-----------------------------------+
| name              | key | comment                           |
+===================+=====+===================================+
| defaultShape      | 0   | Blinking block (terminal default) |
| blinkingBlock     | 1   | blinkingBlock                     |
| steadyBlock       | 2   | steadyBlock                       |
| blinkingUnderline | 3   | blinkingUnderline                 |
| steadyUnderline   | 4   | steadyUnderline                   |
| blinkingBar       | 5   | blinkingBar                       |
| steadyBar         | 6   | steadyBar                         |
+-------------------+-----+-----------------------------------+

## DECRQSS

@brief DECRQSS (CSI Ps $ q) recognised setting codes.

+------------------------------+------+--------------------------------+
| name                         | key  | comment                        |
+==============================+======+================================+
| decrqssCursorPosition        | 1    | Cursor position                |
| decrqssOriginMode            | 6    | Origin mode                    |
| decrqssAutoWrap              | 7    | Auto-wrap mode                 |
| decrqssCursorVisible         | 25   | Cursor visibility              |
| decrqssAlternateScreenBuffer | 47   | Alt screen (legacy)            |
| decrqssAlternateScreenClear  | 1047 | Alt screen + clear             |
| decrqssAlternateScreen       | 1049 | Alt screen + save/clear cursor |
| decrqssSyncOutput            | 2026 | Sync output                    |
+------------------------------+------+--------------------------------+

## CsiIntermediate

@brief CSI intermediate bytes — the bytes preceding the final that select a private or otherwise-disambiguated command variant.

+---------------+--------------------+-----------------------------------------------------------+
| name          | key                | comment                                                   |
+===============+====================+===========================================================+
| privateMarker | Chars::question    | Private-mode marker (DECSET/DECRST, DECRQM, secondary DA) |
| greaterThan   | Chars::greaterThan | Secondary DA / push keyboard flags                        |
| lessThan      | Chars::lessThan    | Pop keyboard flags                                        |
| equals        | Chars::equals      | Set/OR keyboard flags directly                            |
| space         | Chars::space       | DECSCUSR intermediate (CSI Ps SP q)                       |
| dollar        | Chars::dollarSign  | DECRQM / DECRQSS intermediate (CSI ... $ p/q)             |
+---------------+--------------------+-----------------------------------------------------------+

## ESC

@brief ESC final bytes for the most common two-byte ESC sequences.

+---------------------+--------------------+-----------------------------------------------+
| name                | key                | comment                                       |
+=====================+====================+===============================================+
| index               | Chars::upperD      | Index — line feed without CR (IND)            |
| nextLine            | Chars::upperE      | Next line — CR + IND (NEL)                    |
| horizontalTabSet    | Chars::upperH      | Horizontal tab set / set tab stop (HTS)       |
| reverseIndex        | Chars::upperM      | Reverse index — scroll down or cursor up (RI) |
| resetToInitialState | Chars::lowerC      | Reset to initial state, hard reset (RIS)      |
| saveCursor          | Chars::seven       | DECSC — save cursor                           |
| restoreCursor       | Chars::eight       | DECRC — restore cursor                        |
| applicationKeypad   | Chars::equals      | DECKPAM — application keypad                  |
| normalKeypad        | Chars::greaterThan | DECKPNM — normal keypad                       |
+---------------------+--------------------+-----------------------------------------------+

## CharsetIntermediate

@brief ESC charset designator intermediate bytes.

+------+-------------------+-------------------+
| name | key               | comment           |
+======+===================+===================+
| g0   | Chars::openParen  | Select G0 charset |
| g1   | Chars::closeParen | Select G1 charset |
| g2   | Chars::asterisk   | Select G2 charset |
| g3   | Chars::plus       | Select G3 charset |
+------+-------------------+-------------------+

## CharsetDesignator

@brief ESC charset designator final bytes.

+-------------+---------------+-------------------------------------+
| name        | key           | comment                             |
+=============+===============+=====================================+
| ascii       | Chars::upperB | US ASCII                            |
| decLineDraw | Chars::zero   | DEC Special Graphics (line drawing) |
+-------------+---------------+-------------------------------------+

## DecEscIntermediate

@brief DEC-private ESC sequence intermediate byte (ESC # ...).

+------+-------------+--------------------------------------------------+
| name | key         | comment                                          |
+======+=============+==================================================+
| hash | Chars::hash | DEC private intermediate (e.g. ESC # 8 = DECALN) |
+------+-------------+--------------------------------------------------+

## DecEscFinal

@brief DEC private ESC final byte for DECALN (screen alignment test).

+---------------------+--------------+-------------------------------+
| name                | key          | comment                       |
+=====================+==============+===============================+
| screenAlignmentTest | Chars::eight | DECALN — fill screen with 'E' |
+---------------------+--------------+-------------------------------+

## ModeReport

@brief DECRQM response state values — the n in CSI ? Ps ; n $ y.

+------------------+-----+-------------------------------------------------------------------------------------+
| name             | key | comment                                                                             |
+==================+=====+=====================================================================================+
| notRecognised    | 0   | Mode not recognised                                                                 |
| set              | 1   | Mode is currently set                                                               |
| modeReportReset  | 2   | Mode is currently reset (family-qualified — see file-doc's "Collision resolutions") |
| permanentlySet   | 3   | Mode is permanently set (not user-toggleable)                                       |
| permanentlyReset | 4   | Mode is permanently reset                                                           |
+------------------+-----+-------------------------------------------------------------------------------------+

## ANSI

@brief ANSI mode numbers used in CSI Ps h / CSI Ps l (SM/RM).

+-------------+-----+---------------------------------+
| name        | key | comment                         |
+=============+=====+=================================+
| insertMode  | 4   | IRM — insert/replace mode       |
| newLineMode | 20  | LNM — line feed / new-line mode |
+-------------+-----+---------------------------------+

## ShellIntegration

@brief OSC 133 sub-command letters (the first byte after 133;).

+--------------+---------------+---------------------------------------------------+
| name         | key           | comment                                           |
+==============+===============+===================================================+
| promptStart  | Chars::upperA | A — prompt start (fires promptRow)                |
| commandStart | Chars::upperB | B — command start (no-op for output tracking)     |
| outputStart  | Chars::upperC | C — command output start (fires outputBlockStart) |
| outputEnd    | Chars::upperD | D — command output end (fires outputBlockEnd)     |
+--------------+---------------+---------------------------------------------------+

## KeyboardAssignMode

@brief Progressive keyboard protocol assignment sub-parameter values — the mode in CSI = flags ; mode u.

+-------------------------+-----+----------------------------------------------+
| name                    | key | comment                                      |
+=========================+=====+==============================================+
| keyboardSetAllFlags     | 1   | Replace current flags with the given flags   |
| keyboardSetGivenFlags   | 2   | OR the given flags into current flags        |
| keyboardResetGivenFlags | 3   | AND-NOT the given flags out of current flags |
+-------------------------+-----+----------------------------------------------+
