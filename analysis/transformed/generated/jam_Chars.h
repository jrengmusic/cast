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
 * @file jam_Chars.h
 * @brief Character codepoints, token literals, and the character relation maps.
 */

#pragma once

/**
 * @brief Character codepoints, token literals, and the character relation maps.
 *
 * Two groups of constants. The juce::juce_wchar constants name Unicode
 * codepoints the parsers branch on; the const char* constants name
 * multi-character token literals. Two maps and one predicate read those
 * constants: `escape` maps each control character to its named escape
 * letter, `enclosure` maps each opening character to the closing character
 * that pairs with it, and `isNumeric` answers whether a codepoint belongs in a numeric literal.
 */
struct Chars
{
    static constexpr juce::juce_wchar ampersand                 { 0x26     };///< U+0026
    static constexpr juce::juce_wchar applicationProgramCommand { 0x9f     };///< U+009F
    static constexpr juce::juce_wchar asterisk                  { 0x2a     };///< U+002A
    static constexpr juce::juce_wchar at                        { 0x40     };///< U+0040
    static constexpr juce::juce_wchar backslash                 { 0x5c     };///< U+005C
    static constexpr juce::juce_wchar backspace                 { 0x8      };///< U+0008
    static constexpr juce::juce_wchar backtick                  { 0x60     };///< U+0060
    static constexpr juce::juce_wchar caret                     { 0x5e     };///< U+005E
    static constexpr juce::juce_wchar carriageReturn            { 0xd      };///< U+000D
    static constexpr juce::juce_wchar closeBrace                { 0x7d     };///< U+007D
    static constexpr juce::juce_wchar closeBracket              { 0x5d     };///< U+005D
    static constexpr juce::juce_wchar closeParen                { 0x29     };///< U+0029
    static constexpr juce::juce_wchar colon                     { 0x3a     };///< U+003A
    static constexpr juce::juce_wchar comma                     { 0x2c     };///< U+002C
    static constexpr juce::juce_wchar dash                      { 0x2d     };///< U+002D
    static constexpr juce::juce_wchar deleteCharacter           { 0x7f     };///< U+007F
    static constexpr juce::juce_wchar dollarSign                { 0x24     };///< U+0024
    static constexpr juce::juce_wchar dot                       { 0x2e     };///< U+002E
    static constexpr juce::juce_wchar doubleQuote               { 0x22     };///< U+0022
    static constexpr juce::juce_wchar eight                     { 0x38     };///< U+0038
    static constexpr juce::juce_wchar equals                    { 0x3d     };///< U+003D
    static constexpr juce::juce_wchar exclamation               { 0x21     };///< U+0021
    static constexpr juce::juce_wchar five                      { 0x35     };///< U+0035
    static constexpr juce::juce_wchar formFeed                  { 0xc      };///< U+000C
    static constexpr juce::juce_wchar four                      { 0x34     };///< U+0034
    static constexpr juce::juce_wchar greaterThan               { 0x3e     };///< U+003E
    static constexpr juce::juce_wchar hash                      { 0x23     };///< U+0023
    static constexpr juce::juce_wchar lessThan                  { 0x3c     };///< U+003C
    static constexpr juce::juce_wchar lowerA                    { 0x61     };///< U+0061
    static constexpr juce::juce_wchar lowerB                    { 0x62     };///< U+0062
    static constexpr juce::juce_wchar lowerC                    { 0x63     };///< U+0063
    static constexpr juce::juce_wchar lowerD                    { 0x64     };///< U+0064
    static constexpr juce::juce_wchar lowerE                    { 0x65     };///< U+0065
    static constexpr juce::juce_wchar lowerF                    { 0x66     };///< U+0066
    static constexpr juce::juce_wchar lowerG                    { 0x67     };///< U+0067
    static constexpr juce::juce_wchar lowerH                    { 0x68     };///< U+0068
    static constexpr juce::juce_wchar lowerI                    { 0x69     };///< U+0069
    static constexpr juce::juce_wchar lowerJ                    { 0x6a     };///< U+006A
    static constexpr juce::juce_wchar lowerK                    { 0x6b     };///< U+006B
    static constexpr juce::juce_wchar lowerL                    { 0x6c     };///< U+006C
    static constexpr juce::juce_wchar lowerM                    { 0x6d     };///< U+006D
    static constexpr juce::juce_wchar lowerN                    { 0x6e     };///< U+006E
    static constexpr juce::juce_wchar lowerO                    { 0x6f     };///< U+006F
    static constexpr juce::juce_wchar lowerP                    { 0x70     };///< U+0070
    static constexpr juce::juce_wchar lowerQ                    { 0x71     };///< U+0071
    static constexpr juce::juce_wchar lowerR                    { 0x72     };///< U+0072
    static constexpr juce::juce_wchar lowerS                    { 0x73     };///< U+0073
    static constexpr juce::juce_wchar lowerT                    { 0x74     };///< U+0074
    static constexpr juce::juce_wchar lowerU                    { 0x75     };///< U+0075
    static constexpr juce::juce_wchar lowerV                    { 0x76     };///< U+0076
    static constexpr juce::juce_wchar lowerW                    { 0x77     };///< U+0077
    static constexpr juce::juce_wchar lowerX                    { 0x78     };///< U+0078
    static constexpr juce::juce_wchar lowerY                    { 0x79     };///< U+0079
    static constexpr juce::juce_wchar lowerZ                    { 0x7a     };///< U+007A
    static constexpr juce::juce_wchar maximumCodePoint          { 0x10ffff };///< U+10FFFF
    static constexpr juce::juce_wchar newline                   { 0xa      };///< U+000A
    static constexpr juce::juce_wchar nine                      { 0x39     };///< U+0039
    static constexpr juce::juce_wchar nonAsciiStart             { 0x80     };///< U+0080
    static constexpr juce::juce_wchar nullCharacter             { 0x0      };///< U+0000
    static constexpr juce::juce_wchar one                       { 0x31     };///< U+0031
    static constexpr juce::juce_wchar openBrace                 { 0x7b     };///< U+007B
    static constexpr juce::juce_wchar openBracket               { 0x5b     };///< U+005B
    static constexpr juce::juce_wchar openParen                 { 0x28     };///< U+0028
    static constexpr juce::juce_wchar percent                   { 0x25     };///< U+0025
    static constexpr juce::juce_wchar pipe                      { 0x7c     };///< U+007C
    static constexpr juce::juce_wchar plus                      { 0x2b     };///< U+002B
    static constexpr juce::juce_wchar question                  { 0x3f     };///< U+003F
    static constexpr juce::juce_wchar replacementCharacter      { 0xfffd   };///< U+FFFD
    static constexpr juce::juce_wchar semicolon                 { 0x3b     };///< U+003B
    static constexpr juce::juce_wchar seven                     { 0x37     };///< U+0037
    static constexpr juce::juce_wchar shiftOut                  { 0xe      };///< U+000E
    static constexpr juce::juce_wchar singleQuote               { 0x27     };///< U+0027
    static constexpr juce::juce_wchar six                       { 0x36     };///< U+0036
    static constexpr juce::juce_wchar slash                     { 0x2f     };///< U+002F
    static constexpr juce::juce_wchar space                     { 0x20     };///< U+0020
    static constexpr juce::juce_wchar surrogateRangeEnd         { 0xdfff   };///< U+DFFF
    static constexpr juce::juce_wchar surrogateRangeStart       { 0xd800   };///< U+D800
    static constexpr juce::juce_wchar tab                       { 0x9      };///< U+0009
    static constexpr juce::juce_wchar three                     { 0x33     };///< U+0033
    static constexpr juce::juce_wchar tilde                     { 0x7e     };///< U+007E
    static constexpr juce::juce_wchar two                       { 0x32     };///< U+0032
    static constexpr juce::juce_wchar underscore                { 0x5f     };///< U+005F
    static constexpr juce::juce_wchar unitSeparator             { 0x1f     };///< U+001F
    static constexpr juce::juce_wchar upperA                    { 0x41     };///< U+0041
    static constexpr juce::juce_wchar upperB                    { 0x42     };///< U+0042
    static constexpr juce::juce_wchar upperC                    { 0x43     };///< U+0043
    static constexpr juce::juce_wchar upperD                    { 0x44     };///< U+0044
    static constexpr juce::juce_wchar upperE                    { 0x45     };///< U+0045
    static constexpr juce::juce_wchar upperF                    { 0x46     };///< U+0046
    static constexpr juce::juce_wchar upperG                    { 0x47     };///< U+0047
    static constexpr juce::juce_wchar upperH                    { 0x48     };///< U+0048
    static constexpr juce::juce_wchar upperI                    { 0x49     };///< U+0049
    static constexpr juce::juce_wchar upperJ                    { 0x4a     };///< U+004A
    static constexpr juce::juce_wchar upperK                    { 0x4b     };///< U+004B
    static constexpr juce::juce_wchar upperL                    { 0x4c     };///< U+004C
    static constexpr juce::juce_wchar upperM                    { 0x4d     };///< U+004D
    static constexpr juce::juce_wchar upperN                    { 0x4e     };///< U+004E
    static constexpr juce::juce_wchar upperO                    { 0x4f     };///< U+004F
    static constexpr juce::juce_wchar upperP                    { 0x50     };///< U+0050
    static constexpr juce::juce_wchar upperQ                    { 0x51     };///< U+0051
    static constexpr juce::juce_wchar upperR                    { 0x52     };///< U+0052
    static constexpr juce::juce_wchar upperS                    { 0x53     };///< U+0053
    static constexpr juce::juce_wchar upperT                    { 0x54     };///< U+0054
    static constexpr juce::juce_wchar upperU                    { 0x55     };///< U+0055
    static constexpr juce::juce_wchar upperV                    { 0x56     };///< U+0056
    static constexpr juce::juce_wchar upperW                    { 0x57     };///< U+0057
    static constexpr juce::juce_wchar upperX                    { 0x58     };///< U+0058
    static constexpr juce::juce_wchar upperY                    { 0x59     };///< U+0059
    static constexpr juce::juce_wchar upperZ                    { 0x5a     };///< U+005A
    static constexpr juce::juce_wchar verticalTab               { 0xb      };///< U+000B
    static constexpr juce::juce_wchar zero                      { 0x30     };///< U+0030

    static constexpr const char* const annotationClose        { ">>"                                  };///< Annotation close marker.
    static constexpr const char* const annotationOpen         { "<<"                                  };///< Annotation open marker.
    static constexpr const char* const attributeBlock         { "@{"                                  };///< Attribute block open.
    static constexpr const char* const block                  { "block"                               };///< Block keyword.
    static constexpr const char* const cdataClose             { "]]>"                                 };///< CDATA close.
    static constexpr const char* const cdataOpen              { "<![CDATA["                           };///< CDATA open.
    static constexpr const char* const classDef               { ":::"                                 };///< CAST class/token delimiter.
    static constexpr const char* const classToken             { "class"                               };///< Class keyword.
    static constexpr const char* const csiIntroducer          { "\x1b["                               };///< CSI escape introducer.
    static constexpr const char* const cssCommentClose        { "*/"                                  };///< CSS comment close.
    static constexpr const char* const cssCommentOpen         { "/*"                                  };///< CSS comment open.
    static constexpr const char* const declarationClose       { ">"                                   };///< HTML declaration close.
    static constexpr const char* const declarationOpen        { "<!"                                  };///< HTML declaration open.
    static constexpr const char* const doubleDash             { "--"                                  };///< Double dash.
    static constexpr const char* const doubleDashChevronRight { "-->"                                 };///< HTML comment close.
    static constexpr const char* const doubleDot              { ".."                                  };///< Double dot.
    static constexpr const char* const doublePercent          { "%%"                                  };///< Mermaid comment marker.
    static constexpr const char* const elseToken              { "else"                                };///< Else keyword.
    static constexpr const char* const encoding               { "encoding"                            };///< Encoding keyword.
    static constexpr const char* const endTagOpen             { "</"                                  };///< HTML end-tag open.
    static constexpr const char* const escapedPipe            { "\\|"                                 };///< Escaped pipe in a table cell.
    static constexpr const char* const ftpAutolinkPrefix      { "ftp://"                              };///< FTP autolink prefix.
    static constexpr const char* const groupModifier          { "{group}"                             };///< Group modifier token.
    static constexpr const char* const httpAutolinkPrefix     { "http://"                             };///< HTTP autolink prefix.
    static constexpr const char* const httpsAutolinkPrefix    { "https://"                            };///< HTTPS autolink prefix.
    static constexpr const char* const important              { "important"                           };///< CSS important keyword.
    static constexpr const char* const markupCommentOpen      { "<!--"                                };///< HTML comment open.
    static constexpr const char* const messageReverseSolid    { "<-"                                  };///< Mermaid reverse message arrow.
    static constexpr const char* const messageSolid           { "->"                                  };///< Mermaid message arrow.
    static constexpr const char* const preCloseTag            { "</pre>"                              };///< Pre close tag.
    static constexpr const char* const processingClose        { "?>"                                  };///< Processing-instruction close.
    static constexpr const char* const processingOpen         { "<?"                                  };///< Processing-instruction open.
    static constexpr const char* const quotes                 { "\"'"                                 };///< Both quote characters.
    static constexpr const char* const scriptCloseTag         { "</script>"                           };///< Script close tag.
    static constexpr const char* const selfCloseTag           { "/>"                                  };///< HTML self-close tag.
    static constexpr const char* const special                { "., `~!@#$%^&*()-=+[]{}\\|;:'\",<>/?" };///< Punctuation set for token classification.
    static constexpr const char* const state                  { "state"                               };///< State keyword.
    static constexpr const char* const styleCloseTag          { "</style>"                            };///< Style close tag.
    static constexpr const char* const textareaCloseTag       { "</textarea>"                         };///< Textarea close tag.
    static constexpr const char* const urlFunction            { "url"                                 };///< CSS url function name.
    static constexpr const char* const urlOpen                { "url("                                };///< CSS url function open.
    static constexpr const char* const utf8                   { "UTF-8"                               };///< UTF-8 charset name.
    static constexpr const char* const varFunction            { "var"                                 };///< CSS var function name.
    static constexpr const char* const wwwAutolinkPrefix      { "www."                                };///< WWW autolink prefix.
    static constexpr const char* const xml                    { "xml"                                 };///< XML keyword.

/**
 * @brief Character codepoints, token literals, and the character relation maps.
 *
 * Two groups of constants. The juce::juce_wchar constants name Unicode
 * codepoints the parsers branch on; the const char* constants name
 * multi-character token literals. Two maps and one predicate read those
 * constants: `escape` maps each control character to its named escape
 * letter, `enclosure` maps each opening character to the closing character
 * that pairs with it, and `isNumeric` answers whether a codepoint belongs in a numeric literal.
 */
    inline static const jam::HashMap<juce::juce_wchar, juce::juce_wchar> escape
    {
        { backspace,      lowerB },
        { tab,            lowerT },
        { newline,        lowerN },
        { verticalTab,    lowerV },
        { formFeed,       lowerF },
        { carriageReturn, lowerR },
    };

/**
 * @brief Character codepoints, token literals, and the character relation maps.
 *
 * Two groups of constants. The juce::juce_wchar constants name Unicode
 * codepoints the parsers branch on; the const char* constants name
 * multi-character token literals. Two maps and one predicate read those
 * constants: `escape` maps each control character to its named escape
 * letter, `enclosure` maps each opening character to the closing character
 * that pairs with it, and `isNumeric` answers whether a codepoint belongs in a numeric literal.
 */
    inline static const jam::HashMap<juce::juce_wchar, juce::juce_wchar> enclosure
    {
        { openBracket, closeBracket },
        { openBrace,   closeBrace },
        { openParen,   closeParen },
        { doubleQuote, doubleQuote },
        { singleQuote, singleQuote },
        { backtick,    backtick },
    };

    /**
     * @brief Answers whether @p c belongs in a numeric literal.
     * @param c  The codepoint to test.
     * @return   True for a digit, dot, or dash.
     */
    static constexpr bool isNumeric (juce::juce_wchar c) noexcept
    {
        return (c >= zero and c <= nine) or c == dot or c == dash;
    }
};
