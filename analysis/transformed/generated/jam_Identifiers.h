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
 * @file jam_Identifiers.h
 * @brief Identifier and name-string constants for the jam framework vocabulary.
 */

#pragma once

namespace Id
{
/*_____________________________________________________________________________*/

/**
 * @brief Identifier and name-string vocabulary shared across jam modules.
 *
 * juce::Identifier entries are provenance/stamp keys and vocabulary words;
 * juce::String entries name string-valued constants such as company and
 * product identity fields.
 */

inline const juce::Identifier a                               { juce::String::fromUTF8 ("a")                                                                                         };
inline const juce::Identifier aFlat                           { juce::String::fromUTF8 ("aFlat")                                                                                     };
inline const juce::Identifier aSharp                          { juce::String::fromUTF8 ("aSharp")                                                                                    };
inline const juce::Identifier aax                             { juce::String::fromUTF8 ("aax")                                                                                       };
inline const juce::Identifier about                           { juce::String::fromUTF8 ("about")                                                                                     };
inline const juce::Identifier aboutBox                        { juce::String::fromUTF8 ("aboutBox")                                                                                  };
inline const juce::Identifier absolute                        { juce::String::fromUTF8 ("absolute")                                                                                  };
inline const juce::Identifier accDescr                        { juce::String::fromUTF8 ("accDescr")                                                                                  };
inline const juce::Identifier accTitle                        { juce::String::fromUTF8 ("accTitle")                                                                                  };
inline const juce::Identifier acrylic                         { juce::String::fromUTF8 ("acrylic")                                                                                   };
inline const juce::Identifier action                          { juce::String::fromUTF8 ("action")                                                                                    };
inline const juce::Identifier activate                        { juce::String::fromUTF8 ("activate")                                                                                  };
inline const juce::Identifier active                          { juce::String::fromUTF8 ("active")                                                                                    };
inline const juce::Identifier activeScreen                    { juce::String::fromUTF8 ("activeScreen")                                                                              };
inline const juce::Identifier actor                           { juce::String::fromUTF8 ("actor")                                                                                     };
inline const juce::Identifier actorBox                        { juce::String::fromUTF8 ("actorBox")                                                                                  };
inline const juce::Identifier actors                          { juce::String::fromUTF8 ("actors")                                                                                    };
inline const juce::Identifier address                         { juce::String::fromUTF8 ("address")                                                                                   };
inline const juce::Identifier after                           { juce::String::fromUTF8 ("after")                                                                                     };
inline const juce::Identifier alias                           { juce::String::fromUTF8 ("alias")                                                                                     };
inline const juce::Identifier alignment                       { juce::String::fromUTF8 ("alignment")                                                                                 };
inline const juce::Identifier all                             { juce::String::fromUTF8 ("all")                                                                                       };
inline const juce::Identifier allStops                        { juce::String::fromUTF8 ("allStops")                                                                                  };
inline const juce::Identifier alog1                           { juce::String::fromUTF8 ("alog1")                                                                                     };
inline const juce::Identifier alog10                          { juce::String::fromUTF8 ("alog10")                                                                                    };
inline const juce::Identifier alog15                          { juce::String::fromUTF8 ("alog15")                                                                                    };
inline const juce::Identifier alog2                           { juce::String::fromUTF8 ("alog2")                                                                                     };
inline const juce::Identifier alog20                          { juce::String::fromUTF8 ("alog20")                                                                                    };
inline const juce::Identifier alog25                          { juce::String::fromUTF8 ("alog25")                                                                                    };
inline const juce::Identifier alog3                           { juce::String::fromUTF8 ("alog3")                                                                                     };
inline const juce::Identifier alog30                          { juce::String::fromUTF8 ("alog30")                                                                                    };
inline const juce::Identifier alog35                          { juce::String::fromUTF8 ("alog35")                                                                                    };
inline const juce::Identifier alog4                           { juce::String::fromUTF8 ("alog4")                                                                                     };
inline const juce::Identifier alog40                          { juce::String::fromUTF8 ("alog40")                                                                                    };
inline const juce::Identifier alog45                          { juce::String::fromUTF8 ("alog45")                                                                                    };
inline const juce::Identifier alog5                           { juce::String::fromUTF8 ("alog5")                                                                                     };
inline const juce::Identifier alt                             { juce::String::fromUTF8 ("alt")                                                                                       };
inline const juce::Identifier alternate                       { juce::String::fromUTF8 ("alternate")                                                                                 };
inline const juce::Identifier alternateScreen                 { juce::String::fromUTF8 ("alternateScreen")                                                                           };
inline const juce::Identifier alternateScreenBuffer           { juce::String::fromUTF8 ("alternateScreenBuffer")                                                                     };
inline const juce::Identifier alternateScreenClear            { juce::String::fromUTF8 ("alternateScreenClear")                                                                      };
inline const juce::Identifier amp                             { juce::String::fromUTF8 ("amp")                                                                                       };
inline const juce::Identifier analyzer                        { juce::String::fromUTF8 ("analyzer")                                                                                  };
inline const juce::Identifier analyzerMode                    { juce::String::fromUTF8 ("analyzer_mode")                                                                             };
inline const juce::Identifier animation1                      { juce::String::fromUTF8 ("animation1")                                                                                };
inline const juce::Identifier anyTag                          { juce::String::fromUTF8 ("anyTag")                                                                                    };
inline const juce::Identifier apos                            { juce::String::fromUTF8 ("apos")                                                                                      };
inline const juce::Identifier appearance                      { juce::String::fromUTF8 ("appearance")                                                                                };
inline const juce::Identifier applicationCursor               { juce::String::fromUTF8 ("applicationCursor")                                                                         };
inline const juce::Identifier applicationKeypad               { juce::String::fromUTF8 ("applicationKeypad")                                                                         };
inline const juce::Identifier applicationSupport              { juce::String::fromUTF8 ("Application Support")                                                                       };///< macOS Application Support directory label.
inline const juce::Identifier architectureBeta                { juce::String::fromUTF8 ("architecture-beta")                                                                         };///< Mermaid architecture-beta diagram keyword.
inline const juce::Identifier area                            { juce::String::fromUTF8 ("area")                                                                                      };
inline const juce::Identifier arrowEnd                        { juce::String::fromUTF8 ("arrowEnd")                                                                                  };
inline const juce::Identifier arrowStart                      { juce::String::fromUTF8 ("arrowStart")                                                                                };
inline const juce::Identifier arrowhead                       { juce::String::fromUTF8 ("arrowhead")                                                                                 };
inline const juce::Identifier arrowheadLength                 { juce::String::fromUTF8 ("arrowheadLength")                                                                           };
inline const juce::Identifier arrowheadWidth                  { juce::String::fromUTF8 ("arrowheadWidth")                                                                            };
inline const juce::Identifier article                         { juce::String::fromUTF8 ("article")                                                                                   };
inline const juce::Identifier as                              { juce::String::fromUTF8 ("as")                                                                                        };
inline const juce::Identifier ascii                           { juce::String::fromUTF8 ("ascii")                                                                                     };
inline const juce::Identifier aside                           { juce::String::fromUTF8 ("aside")                                                                                     };
inline const juce::Identifier assigned                        { juce::String::fromUTF8 ("assigned")                                                                                  };
inline const juce::Identifier asteriskDot                     { juce::String::fromUTF8 ("*.")                                                                                        };///< CSS `*.` substring selector operator.
inline const juce::Identifier async                           { juce::String::fromUTF8 ("async")                                                                                     };
inline const juce::Identifier atKeyword                       { juce::String::fromUTF8 ("atKeyword")                                                                                 };
inline const juce::Identifier attribute                       { juce::String::fromUTF8 ("attribute")                                                                                 };
inline const juce::Identifier attributeAssign                 { juce::String::fromUTF8 ("=")                                                                                         };///< CSS attribute `=` assign operator.
inline const juce::Identifier attributeComment                { juce::String::fromUTF8 ("attributeComment")                                                                          };
inline const juce::Identifier attributeKeys                   { juce::String::fromUTF8 ("attributeKeys")                                                                             };
inline const juce::Identifier attributeName                   { juce::String::fromUTF8 ("attributeName")                                                                             };
inline const juce::Identifier attributeType                   { juce::String::fromUTF8 ("attributeType")                                                                             };
inline const juce::Identifier attributes                      { juce::String::fromUTF8 ("attributes")                                                                                };
inline const juce::Identifier atxHeading                      { juce::String::fromUTF8 ("atxHeading")                                                                                };
inline const juce::Identifier au                              { juce::String::fromUTF8 ("au")                                                                                        };
inline const juce::Identifier autoRepeat                      { juce::String::fromUTF8 ("autoRepeat")                                                                                };
inline const juce::Identifier autoWrap                        { juce::String::fromUTF8 ("autoWrap")                                                                                  };
inline const juce::Identifier autolink                        { juce::String::fromUTF8 ("autolink")                                                                                  };
inline const juce::Identifier autonumber                      { juce::String::fromUTF8 ("autonumber")                                                                                };
inline const juce::Identifier auv3                            { juce::String::fromUTF8 ("auv3")                                                                                      };
inline const juce::Identifier await                           { juce::String::fromUTF8 ("await")                                                                                     };
inline const juce::Identifier axisBottom                      { juce::String::fromUTF8 ("axisBottom")                                                                                };
inline const juce::Identifier axisFormat                      { juce::String::fromUTF8 ("axisFormat")                                                                                };
inline const juce::Identifier axisLeft                        { juce::String::fromUTF8 ("axisLeft")                                                                                  };
inline const juce::Identifier axisRight                       { juce::String::fromUTF8 ("axisRight")                                                                                 };
inline const juce::Identifier axisTop                         { juce::String::fromUTF8 ("axisTop")                                                                                   };
inline const juce::Identifier b                               { juce::String::fromUTF8 ("b")                                                                                         };
inline const juce::Identifier bFlat                           { juce::String::fromUTF8 ("bFlat")                                                                                     };
inline const juce::Identifier background                      { juce::String::fromUTF8 ("background")                                                                                };
inline const juce::Identifier backgroundBlur                  { juce::String::fromUTF8 ("backgroundBlur")                                                                            };
inline const juce::Identifier backgroundFirst                 { juce::String::fromUTF8 ("backgroundFirst")                                                                           };
inline const juce::Identifier backgroundLast                  { juce::String::fromUTF8 ("backgroundLast")                                                                            };
inline const juce::Identifier badString                       { juce::String::fromUTF8 ("badString")                                                                                 };
inline const juce::Identifier badUrl                          { juce::String::fromUTF8 ("badUrl")                                                                                    };
inline const juce::Identifier bang                            { juce::String::fromUTF8 ("bang")                                                                                      };
inline const juce::Identifier bannerClose                     { juce::String::fromUTF8 ("bannerClose")                                                                               };
inline const juce::Identifier bannerOpen                      { juce::String::fromUTF8 ("bannerOpen")                                                                                };
inline const juce::Identifier bar                             { juce::String::fromUTF8 ("bar")                                                                                       };
inline const juce::Identifier bars                            { juce::String::fromUTF8 ("bars")                                                                                      };
inline const juce::Identifier base                            { juce::String::fromUTF8 ("base")                                                                                      };
inline const juce::Identifier basefont                        { juce::String::fromUTF8 ("basefont")                                                                                  };
inline const juce::Identifier begin                           { juce::String::fromUTF8 ("begin")                                                                                     };
inline const juce::Identifier bell                            { juce::String::fromUTF8 ("bell")                                                                                      };
inline const juce::Identifier bevel                           { juce::String::fromUTF8 ("bevel")                                                                                     };
inline const juce::Identifier biRel                           { juce::String::fromUTF8 ("BiRel")                                                                                     };///< C4 bidirectional relation keyword.
inline const juce::Identifier blink                           { juce::String::fromUTF8 ("blink")                                                                                     };
inline const juce::Identifier blinkingBar                     { juce::String::fromUTF8 ("blinkingBar")                                                                               };
inline const juce::Identifier blinkingBlock                   { juce::String::fromUTF8 ("blinkingBlock")                                                                             };
inline const juce::Identifier blinkingUnderline               { juce::String::fromUTF8 ("blinkingUnderline")                                                                         };
inline const juce::Identifier blockArrow                      { juce::String::fromUTF8 ("blockArrow")                                                                                };
inline const juce::Identifier blockBeta                       { juce::String::fromUTF8 ("block-beta")                                                                                };///< Mermaid block-beta diagram keyword.
inline const juce::Identifier blockClose                      { juce::String::fromUTF8 ("blockClose")                                                                                };
inline const juce::Identifier blockCommentClose               { juce::String::fromUTF8 ("*/")                                                                                        };///< C `*/` block-comment close.
inline const juce::Identifier blockCommentOpen                { juce::String::fromUTF8 ("/*")                                                                                        };///< C `/*` block-comment open.
inline const juce::Identifier blockOpen                       { juce::String::fromUTF8 ("blockOpen")                                                                                 };
inline const juce::Identifier blockTag                        { juce::String::fromUTF8 ("blockTag")                                                                                  };
inline const juce::Identifier blockquote                      { juce::String::fromUTF8 ("blockquote")                                                                                };
inline const juce::Identifier blur                            { juce::String::fromUTF8 ("blur")                                                                                      };
inline const juce::Identifier blurBehind                      { juce::String::fromUTF8 ("blurBehind")                                                                                };
inline const juce::Identifier body                            { juce::String::fromUTF8 ("body")                                                                                      };
inline const juce::Identifier bold                            { juce::String::fromUTF8 ("bold")                                                                                      };
inline const juce::Identifier border                          { juce::String::fromUTF8 ("border")                                                                                    };
inline const juce::Identifier boolean                         { juce::String::fromUTF8 ("bool")                                                                                      };///< Audio parameter `bool` kind.
inline const juce::Identifier bottom                          { juce::String::fromUTF8 ("bottom")                                                                                    };
inline const juce::Identifier bottomLeft                      { juce::String::fromUTF8 ("bottom-left")                                                                               };///< CSS flex `bottom-left` anchor.
inline const juce::Identifier bottomRight                     { juce::String::fromUTF8 ("bottom-right")                                                                              };///< CSS flex `bottom-right` anchor.
inline const juce::Identifier boundary                        { juce::String::fromUTF8 ("Boundary")                                                                                  };///< C4 generic boundary keyword.
inline const juce::Identifier bounds                          { juce::String::fromUTF8 ("bounds")                                                                                    };
inline const juce::Identifier box                             { juce::String::fromUTF8 ("box")                                                                                       };
inline const juce::Identifier br                              { juce::String::fromUTF8 ("br")                                                                                        };
inline const juce::Identifier bracketedPaste                  { juce::String::fromUTF8 ("bracketedPaste")                                                                            };
inline const juce::Identifier branch                          { juce::String::fromUTF8 ("branch")                                                                                    };
inline const juce::Identifier brightBackgroundFirst           { juce::String::fromUTF8 ("brightBackgroundFirst")                                                                     };
inline const juce::Identifier brightBackgroundLast            { juce::String::fromUTF8 ("brightBackgroundLast")                                                                      };
inline const juce::Identifier brightForegroundFirst           { juce::String::fromUTF8 ("brightForegroundFirst")                                                                     };
inline const juce::Identifier brightForegroundLast            { juce::String::fromUTF8 ("brightForegroundLast")                                                                      };
inline const juce::Identifier browser                         { juce::String::fromUTF8 ("browser")                                                                                   };
inline const juce::Identifier bt                              { juce::String::fromUTF8 ("bt")                                                                                        };
inline const juce::Identifier buffer                          { juce::String::fromUTF8 ("Buffer")                                                                                    };///< C4 buffer element keyword.
inline const juce::Identifier button                          { juce::String::fromUTF8 ("button")                                                                                    };
inline const juce::Identifier buttonDialog                    { juce::String::fromUTF8 ("buttonDialog")                                                                              };
inline const juce::Identifier buttonGroup                     { juce::String::fromUTF8 ("buttonGroup")                                                                               };
inline const juce::Identifier buttonMenu                      { juce::String::fromUTF8 ("buttonMenu")                                                                                };
inline const juce::Identifier buttonOptions                   { juce::String::fromUTF8 ("buttonOptions")                                                                             };
inline const juce::Identifier buyNow                          { juce::String::fromUTF8 ("buyNow")                                                                                    };
inline const juce::Identifier bypass                          { juce::String::fromUTF8 ("bypass")                                                                                    };
inline const juce::Identifier cSharp                          { juce::String::fromUTF8 ("cSharp")                                                                                    };
inline const juce::Identifier c4Component                     { juce::String::fromUTF8 ("C4Component")                                                                               };///< C4 component element keyword.
inline const juce::Identifier c4Container                     { juce::String::fromUTF8 ("C4Container")                                                                               };///< C4 container element keyword.
inline const juce::Identifier c4Context                       { juce::String::fromUTF8 ("C4Context")                                                                                 };///< C4 context diagram keyword.
inline const juce::Identifier c4Deployment                    { juce::String::fromUTF8 ("C4Deployment")                                                                              };///< C4 deployment diagram keyword.
inline const juce::Identifier c4Dynamic                       { juce::String::fromUTF8 ("C4Dynamic")                                                                                 };///< C4 dynamic diagram keyword.
inline const juce::Identifier cache                           { juce::String::fromUTF8 ("cache")                                                                                     };
inline const juce::Identifier call                            { juce::String::fromUTF8 ("call")                                                                                      };
inline const juce::Identifier canvasMargin                    { juce::String::fromUTF8 ("canvasMargin")                                                                              };
inline const juce::Identifier caption                         { juce::String::fromUTF8 ("caption")                                                                                   };
inline const juce::Identifier captions                        { juce::String::fromUTF8 ("captions")                                                                                  };
inline const juce::Identifier cdata                           { juce::String::fromUTF8 ("cdata")                                                                                     };
inline const juce::Identifier cdataClose                      { juce::String::fromUTF8 ("]]>")                                                                                       };///< XML `]]>` CDATA close.
inline const juce::Identifier cdataOpen                       { juce::String::fromUTF8 ("<![CDATA[")                                                                                 };///< XML `<![CDATA[` CDATA open.
inline const juce::Identifier center                          { juce::String::fromUTF8 ("center")                                                                                    };
inline const juce::Identifier centre                          { juce::String::fromUTF8 ("centre")                                                                                    };
inline const juce::Identifier channelMacros                   { juce::String::fromUTF8 ("channelMacros")                                                                             };
inline const juce::Identifier character                       { juce::String::fromUTF8 ("character")                                                                                 };
inline const juce::Identifier characterReference              { juce::String::fromUTF8 ("characterReference")                                                                        };
inline const juce::Identifier charset                         { juce::String::fromUTF8 ("charset")                                                                                   };
inline const juce::Identifier charsetRule                     { juce::String::fromUTF8 ("charsetRule")                                                                               };
inline const juce::Identifier checkboxSize                    { juce::String::fromUTF8 ("checkboxSize")                                                                              };
inline const juce::Identifier checked                         { juce::String::fromUTF8 ("checked")                                                                                   };
inline const juce::Identifier checkout                        { juce::String::fromUTF8 ("checkout")                                                                                  };
inline const juce::Identifier cherryPick                      { juce::String::fromUTF8 ("cherry-pick")                                                                               };///< Mermaid git cherry-pick commit kind.
inline const juce::Identifier children                        { juce::String::fromUTF8 ("children")                                                                                  };
inline const juce::Identifier choice                          { juce::String::fromUTF8 ("choice")                                                                                    };
inline const juce::Identifier choices                         { juce::String::fromUTF8 ("choices")                                                                                   };
inline const juce::Identifier circle                          { juce::String::fromUTF8 ("circle")                                                                                    };
inline const juce::Identifier circleCross                     { juce::String::fromUTF8 ("circleCross")                                                                               };
inline const juce::Identifier clampToBorder                   { juce::String::fromUTF8 ("clamp_to_border")                                                                           };///< Vulkan `clamp_to_border` sampler mode.
inline const juce::Identifier clampToEdge                     { juce::String::fromUTF8 ("clamp_to_edge")                                                                             };///< Vulkan `clamp_to_edge` sampler mode.
inline const juce::Identifier classDef                        { juce::String::fromUTF8 ("classDef")                                                                                  };
inline const juce::Identifier classDependency                 { juce::String::fromUTF8 ("classDependency")                                                                           };
inline const juce::Identifier classDiagram                    { juce::String::fromUTF8 ("classDiagram")                                                                              };
inline const juce::Identifier classMembers                    { juce::String::fromUTF8 ("classMembers")                                                                              };
inline const juce::Identifier classMethods                    { juce::String::fromUTF8 ("classMethods")                                                                              };
inline const juce::Identifier classSelector                   { juce::String::fromUTF8 ("classSelector")                                                                             };
inline const juce::Identifier classStereotype                 { juce::String::fromUTF8 ("classStereotype")                                                                           };
inline const juce::Identifier click                           { juce::String::fromUTF8 ("click")                                                                                     };
inline const juce::Identifier close                           { juce::String::fromUTF8 ("close")                                                                                     };
inline const juce::Identifier cloud                           { juce::String::fromUTF8 ("cloud")                                                                                     };
inline const juce::Identifier code                            { juce::String::fromUTF8 ("code")                                                                                      };
inline const juce::Identifier codeBlock                       { juce::String::fromUTF8 ("codeBlock")                                                                                 };
inline const juce::Identifier codeBlockPadding                { juce::String::fromUTF8 ("codeBlockPadding")                                                                          };
inline const juce::Identifier codeClass                       { juce::String::fromUTF8 ("class")                                                                                     };
inline const juce::Identifier codePadding                     { juce::String::fromUTF8 ("codePadding")                                                                               };
inline const juce::Identifier codeSpan                        { juce::String::fromUTF8 ("codeSpan")                                                                                  };
inline const juce::Identifier codepointPrefix                 { juce::String::fromUTF8 ("U+")                                                                                        };///< U+ codepoint literal prefix.
inline const juce::Identifier col                             { juce::String::fromUTF8 ("col")                                                                                       };
inline const juce::Identifier colgroup                        { juce::String::fromUTF8 ("colgroup")                                                                                  };
inline const juce::Identifier color                           { juce::String::fromUTF8 ("color")                                                                                     };
inline const juce::Identifier colour                          { juce::String::fromUTF8 ("colour")                                                                                    };
inline const juce::Identifier colours                         { juce::String::fromUTF8 ("colours")                                                                                   };
inline const juce::Identifier columnMode                      { juce::String::fromUTF8 ("columnMode")                                                                                };
inline const juce::Identifier columns                         { juce::String::fromUTF8 ("columns")                                                                                   };
inline const juce::Identifier commandStart                    { juce::String::fromUTF8 ("commandStart")                                                                              };
inline const juce::Identifier comment                         { juce::String::fromUTF8 ("comment")                                                                                   };
inline const juce::Identifier commentClose                    { juce::String::fromUTF8 ("-->")                                                                                       };///< HTML `-->` comment close.
inline const juce::Identifier commentOpen                     { juce::String::fromUTF8 ("<!--")                                                                                      };///< HTML `<!--` comment open.
inline const juce::Identifier commit                          { juce::String::fromUTF8 ("commit")                                                                                    };
inline const juce::Identifier common                          { juce::String::fromUTF8 ("common")                                                                                    };
inline const juce::Identifier commonSource                    { juce::String::fromUTF8 ("commonSource")                                                                              };
inline const juce::Identifier companyName                     { juce::String::fromUTF8 ("companyName")                                                                               };
inline const juce::Identifier component                       { juce::String::fromUTF8 ("Component")                                                                                 };///< C4 component element keyword.
inline const juce::Identifier componentDb                     { juce::String::fromUTF8 ("ComponentDb")                                                                               };///< C4 component-database element keyword.
inline const juce::Identifier componentDbExt                  { juce::String::fromUTF8 ("ComponentDb_Ext")                                                                           };///< C4 external component-database keyword.
inline const juce::Identifier componentExt                    { juce::String::fromUTF8 ("Component_Ext")                                                                             };///< C4 external component keyword.
inline const juce::Identifier componentQueue                  { juce::String::fromUTF8 ("ComponentQueue")                                                                            };///< C4 component-queue element keyword.
inline const juce::Identifier componentQueueExt               { juce::String::fromUTF8 ("ComponentQueue_Ext")                                                                        };///< C4 external component-queue keyword.
inline const juce::Identifier container                       { juce::String::fromUTF8 ("Container")                                                                                 };///< C4 container element keyword.
inline const juce::Identifier containerBoundary               { juce::String::fromUTF8 ("Container_Boundary")                                                                        };///< C4 container boundary keyword.
inline const juce::Identifier containerDb                     { juce::String::fromUTF8 ("ContainerDb")                                                                               };///< C4 container-database element keyword.
inline const juce::Identifier containerDbExt                  { juce::String::fromUTF8 ("ContainerDb_Ext")                                                                           };///< C4 external container-database keyword.
inline const juce::Identifier containerExt                    { juce::String::fromUTF8 ("Container_Ext")                                                                             };///< C4 external container keyword.
inline const juce::Identifier containerQueue                  { juce::String::fromUTF8 ("ContainerQueue")                                                                            };///< C4 container-queue element keyword.
inline const juce::Identifier containerQueueExt               { juce::String::fromUTF8 ("ContainerQueue_Ext")                                                                        };///< C4 external container-queue keyword.
inline const juce::Identifier contains                        { juce::String::fromUTF8 ("contains")                                                                                  };
inline const juce::Identifier content                         { juce::String::fromUTF8 ("content")                                                                                   };
inline const juce::Identifier contrast                        { juce::String::fromUTF8 ("contrast")                                                                                  };
inline const juce::Identifier copies                          { juce::String::fromUTF8 ("copies")                                                                                    };
inline const juce::Identifier copyright                       { juce::String::fromUTF8 ("copyright")                                                                                 };
inline const juce::Identifier copyrightSymbol                 { juce::String::fromUTF8 ("\xc2\xa9")                                                                                  };///< Copyright sign character.
inline const juce::Identifier cpu                             { juce::String::fromUTF8 ("CPU")                                                                                       };///< CPU device label.
inline const juce::Identifier create                          { juce::String::fromUTF8 ("create")                                                                                    };
inline const juce::Identifier crit                            { juce::String::fromUTF8 ("crit")                                                                                      };
inline const juce::Identifier critical                        { juce::String::fromUTF8 ("critical")                                                                                  };
inline const juce::Identifier cross                           { juce::String::fromUTF8 ("cross")                                                                                     };
inline const juce::Identifier crowsFootMany                   { juce::String::fromUTF8 ("crowsFootMany")                                                                             };
inline const juce::Identifier crowsFootOne                    { juce::String::fromUTF8 ("crowsFootOne")                                                                              };
inline const juce::Identifier crowsFootZeroMany               { juce::String::fromUTF8 ("crowsFootZeroMany")                                                                         };
inline const juce::Identifier crowsFootZeroOne                { juce::String::fromUTF8 ("crowsFootZeroOne")                                                                          };
inline const juce::Identifier csiIntroducer                   { juce::String::fromUTF8 ("csiIntroducer")                                                                             };
inline const juce::Identifier cssClass                        { juce::String::fromUTF8 ("cssClass")                                                                                  };
inline const juce::Identifier cssCloseBrace                   { juce::String::fromUTF8 ("closeBrace")                                                                                };///< CSS `closeBrace` token name.
inline const juce::Identifier cssCloseBracket                 { juce::String::fromUTF8 ("closeBracket")                                                                              };///< CSS `closeBracket` token name.
inline const juce::Identifier cssCloseParen                   { juce::String::fromUTF8 ("closeParen")                                                                                };///< CSS `closeParen` token name.
inline const juce::Identifier cssColon                        { juce::String::fromUTF8 ("colon")                                                                                     };///< CSS `colon` token name.
inline const juce::Identifier cssComma                        { juce::String::fromUTF8 ("comma")                                                                                     };///< CSS `comma` token name.
inline const juce::Identifier cssHash                         { juce::String::fromUTF8 ("hash")                                                                                      };///< CSS `hash` token name.
inline const juce::Identifier cssOpenBrace                    { juce::String::fromUTF8 ("openBrace")                                                                                 };///< CSS `openBrace` token name.
inline const juce::Identifier cssOpenBracket                  { juce::String::fromUTF8 ("openBracket")                                                                               };///< CSS `openBracket` token name.
inline const juce::Identifier cssOpenParen                    { juce::String::fromUTF8 ("openParen")                                                                                 };///< CSS `openParen` token name.
inline const juce::Identifier cssSemicolon                    { juce::String::fromUTF8 ("semicolon")                                                                                 };///< CSS `semicolon` token name.
inline const juce::Identifier curly                           { juce::String::fromUTF8 ("curly")                                                                                     };
inline const juce::Identifier currentColumn                   { juce::String::fromUTF8 ("currentColumn")                                                                             };
inline const juce::Identifier cursor                          { juce::String::fromUTF8 ("cursor")                                                                                    };
inline const juce::Identifier cursorBack                      { juce::String::fromUTF8 ("cursorBack")                                                                                };
inline const juce::Identifier cursorBackwardTabulation        { juce::String::fromUTF8 ("cursorBackwardTabulation")                                                                  };
inline const juce::Identifier cursorColor                     { juce::String::fromUTF8 ("cursorColor")                                                                               };
inline const juce::Identifier cursorDown                      { juce::String::fromUTF8 ("cursorDown")                                                                                };
inline const juce::Identifier cursorForward                   { juce::String::fromUTF8 ("cursorForward")                                                                             };
inline const juce::Identifier cursorForwardTabulation         { juce::String::fromUTF8 ("cursorForwardTabulation")                                                                   };
inline const juce::Identifier cursorHorizontalAbsolute        { juce::String::fromUTF8 ("cursorHorizontalAbsolute")                                                                  };
inline const juce::Identifier cursorNextLine                  { juce::String::fromUTF8 ("cursorNextLine")                                                                            };
inline const juce::Identifier cursorPosition                  { juce::String::fromUTF8 ("cursorPosition")                                                                            };
inline const juce::Identifier cursorPreviousLine              { juce::String::fromUTF8 ("cursorPreviousLine")                                                                        };
inline const juce::Identifier cursorShape                     { juce::String::fromUTF8 ("cursorShape")                                                                               };
inline const juce::Identifier cursorUp                        { juce::String::fromUTF8 ("cursorUp")                                                                                  };
inline const juce::Identifier cursorVisible                   { juce::String::fromUTF8 ("cursorVisible")                                                                             };
inline const juce::Identifier customPropertyPrefix            { juce::String::fromUTF8 ("customPropertyPrefix")                                                                      };
inline const juce::Identifier customStylesheet                { juce::String::fromUTF8 ("customStylesheet")                                                                          };
inline const juce::Identifier cwd                             { juce::String::fromUTF8 ("cwd")                                                                                       };
inline const juce::Identifier cx                              { juce::String::fromUTF8 ("cx")                                                                                        };
inline const juce::Identifier cy                              { juce::String::fromUTF8 ("cy")                                                                                        };
inline const juce::Identifier cyl                             { juce::String::fromUTF8 ("cyl")                                                                                       };
inline const juce::Identifier d                               { juce::String::fromUTF8 ("d")                                                                                         };
inline const juce::Identifier dB                              { juce::String::fromUTF8 ("dB")                                                                                        };
inline const juce::Identifier dFlat                           { juce::String::fromUTF8 ("dFlat")                                                                                     };
inline const juce::Identifier dSharp                          { juce::String::fromUTF8 ("dSharp")                                                                                    };
inline const juce::Identifier dark                            { juce::String::fromUTF8 ("dark")                                                                                      };
inline const juce::Identifier dashed                          { juce::String::fromUTF8 ("dashed")                                                                                    };
inline const juce::Identifier data                            { juce::String::fromUTF8 ("data")                                                                                      };
inline const juce::Identifier dataButton                      { juce::String::fromUTF8 ("data-button")                                                                               };///< HTML `data-button` attribute.
inline const juce::Identifier dataContent                     { juce::String::fromUTF8 ("data-content")                                                                              };///< HTML `data-content` attribute.
inline const juce::Identifier dataParameter                   { juce::String::fromUTF8 ("data-parameter")                                                                            };///< HTML `data-parameter` attribute.
inline const juce::Identifier dateFormat                      { juce::String::fromUTF8 ("dateFormat")                                                                                };
inline const juce::Identifier daw                             { juce::String::fromUTF8 ("daw")                                                                                       };
inline const juce::Identifier dblCirc                         { juce::String::fromUTF8 ("dblCirc")                                                                                   };
inline const juce::Identifier dd                              { juce::String::fromUTF8 ("dd")                                                                                        };
inline const juce::Identifier deactivate                      { juce::String::fromUTF8 ("deactivate")                                                                                };
inline const juce::Identifier debugger                        { juce::String::fromUTF8 ("debugger")                                                                                  };
inline const juce::Identifier decLineDraw                     { juce::String::fromUTF8 ("decLineDraw")                                                                               };
inline const juce::Identifier decSaveCursor                   { juce::String::fromUTF8 ("decSaveCursor")                                                                             };
inline const juce::Identifier declaration                     { juce::String::fromUTF8 ("declaration")                                                                               };
inline const juce::Identifier declarationClose                { juce::String::fromUTF8 (">")                                                                                         };///< HTML `>` declaration close.
inline const juce::Identifier declarationOpen                 { juce::String::fromUTF8 ("<!")                                                                                        };///< HTML `<!` declaration open.
inline const juce::Identifier decoration                      { juce::String::fromUTF8 ("decoration")                                                                                };
inline const juce::Identifier decrqssAlternateScreen          { juce::String::fromUTF8 ("decrqssAlternateScreen")                                                                    };
inline const juce::Identifier decrqssAlternateScreenBuffer    { juce::String::fromUTF8 ("decrqssAlternateScreenBuffer")                                                              };
inline const juce::Identifier decrqssAlternateScreenClear     { juce::String::fromUTF8 ("decrqssAlternateScreenClear")                                                               };
inline const juce::Identifier decrqssAutoWrap                 { juce::String::fromUTF8 ("decrqssAutoWrap")                                                                           };
inline const juce::Identifier decrqssCursorPosition           { juce::String::fromUTF8 ("decrqssCursorPosition")                                                                     };
inline const juce::Identifier decrqssCursorVisible            { juce::String::fromUTF8 ("decrqssCursorVisible")                                                                      };
inline const juce::Identifier decrqssOriginMode               { juce::String::fromUTF8 ("decrqssOriginMode")                                                                         };
inline const juce::Identifier decrqssSyncOutput               { juce::String::fromUTF8 ("decrqssSyncOutput")                                                                         };
inline const juce::Identifier def                             { juce::String::fromUTF8 ("def")                                                                                       };
inline const juce::Identifier defaultBackground               { juce::String::fromUTF8 ("defaultBackground")                                                                         };
inline const juce::Identifier defaultForeground               { juce::String::fromUTF8 ("defaultForeground")                                                                         };
inline const juce::Identifier defaultPreset                   { juce::String::fromUTF8 ("defaultPreset")                                                                             };
inline const juce::Identifier defaultPresets                  { juce::String::fromUTF8 ("Default Presets")                                                                           };///< Default presets folder label.
inline const juce::Identifier defaultShape                    { juce::String::fromUTF8 ("defaultShape")                                                                              };
inline const juce::Identifier defaultUnderline                { juce::String::fromUTF8 ("defaultUnderline")                                                                          };
inline const juce::Identifier defaultValue                    { juce::String::fromUTF8 ("default")                                                                                   };
inline const juce::Identifier del                             { juce::String::fromUTF8 ("del")                                                                                       };
inline const juce::Identifier deleteLine                      { juce::String::fromUTF8 ("deleteLine")                                                                                };
inline const juce::Identifier delim                           { juce::String::fromUTF8 ("delim")                                                                                     };
inline const juce::Identifier deploymentNode                  { juce::String::fromUTF8 ("Deployment_Node")                                                                           };///< C4 deployment-node keyword.
inline const juce::Identifier derives                         { juce::String::fromUTF8 ("derives")                                                                                   };
inline const juce::Identifier desc                            { juce::String::fromUTF8 ("desc")                                                                                      };
inline const juce::Identifier descr                           { juce::String::fromUTF8 ("descr")                                                                                     };
inline const juce::Identifier designConstraint                { juce::String::fromUTF8 ("designConstraint")                                                                          };
inline const juce::Identifier designConstraintLabel           { juce::String::fromUTF8 ("Design Constraint")                                                                         };///< Requirement design-constraint stereotype label.
inline const juce::Identifier desktop                         { juce::String::fromUTF8 ("desktop")                                                                                   };
inline const juce::Identifier desktopNotify                   { juce::String::fromUTF8 ("desktopNotify")                                                                             };
inline const juce::Identifier destroy                         { juce::String::fromUTF8 ("destroy")                                                                                   };
inline const juce::Identifier details                         { juce::String::fromUTF8 ("details")                                                                                   };
inline const juce::Identifier deviceAttributes                { juce::String::fromUTF8 ("deviceAttributes")                                                                          };
inline const juce::Identifier deviceStatusReport              { juce::String::fromUTF8 ("deviceStatusReport")                                                                        };
inline const juce::String     diagnosticSeparator             { juce::String::fromUTF8 (": ")                                                                                        };///< Diagnostic message field separator.
inline const juce::Identifier dialog                          { juce::String::fromUTF8 ("dialog")                                                                                    };
inline const juce::Identifier diamond                         { juce::String::fromUTF8 ("diamond")                                                                                   };
inline const juce::Identifier diamondFilled                   { juce::String::fromUTF8 ("diamondFilled")                                                                             };
inline const juce::Identifier dim                             { juce::String::fromUTF8 ("dim")                                                                                       };
inline const juce::Identifier dimension                       { juce::String::fromUTF8 ("dimension")                                                                                 };
inline const juce::Identifier dir                             { juce::String::fromUTF8 ("dir")                                                                                       };
inline const juce::Identifier direction                       { juce::String::fromUTF8 ("direction")                                                                                 };
inline const juce::Identifier disabled                        { juce::String::fromUTF8 ("disabled")                                                                                  };
inline const juce::Identifier disabledOn                      { juce::String::fromUTF8 ("disabledOn")                                                                                };
inline const juce::Identifier display                         { juce::String::fromUTF8 ("display")                                                                                   };
inline const juce::Identifier div                             { juce::String::fromUTF8 ("div")                                                                                       };
inline const juce::Identifier dividers                        { juce::String::fromUTF8 ("dividers")                                                                                  };
inline const juce::Identifier dl                              { juce::String::fromUTF8 ("dl")                                                                                        };
inline const juce::Identifier docRef                          { juce::String::fromUTF8 ("docref")                                                                                    };///< Requirement docref attribute keyword.
inline const juce::Identifier docRefLabel                     { juce::String::fromUTF8 ("Doc Ref")                                                                                   };///< Requirement Doc Ref attribute label.
inline const juce::Identifier doctype                         { juce::String::fromUTF8 ("doctype")                                                                                   };
inline const juce::Identifier document                        { juce::String::fromUTF8 ("document")                                                                                  };
inline const juce::Identifier documentPath                    { juce::String::fromUTF8 ("documentPath")                                                                              };
inline const juce::Identifier documents                       { juce::String::fromUTF8 ("documents")                                                                                 };
inline const juce::Identifier dollar                          { juce::String::fromUTF8 ("dollar")                                                                                    };
inline const juce::Identifier done                            { juce::String::fromUTF8 ("done")                                                                                      };
inline const juce::Identifier dotted                          { juce::String::fromUTF8 ("dotted")                                                                                    };
inline const juce::Identifier doubleCircle                    { juce::String::fromUTF8 ("dbl-circ")                                                                                  };///< Mermaid double-circle node shape spelling.
inline const juce::Identifier doubleColon                     { juce::String::fromUTF8 ("::")                                                                                        };///< CAST `::` delimiter.
inline const juce::Identifier doubleDash                      { juce::String::fromUTF8 ("--")                                                                                        };///< Double-dash delimiter.
inline const juce::Identifier doubleLine                      { juce::String::fromUTF8 ("doubleLine")                                                                                };
inline const juce::Identifier doublePercent                   { juce::String::fromUTF8 ("%%")                                                                                        };///< Mermaid `%%` comment marker.
inline const juce::Identifier doubleUnderline                 { juce::String::fromUTF8 ("doubleUnderline")                                                                           };
inline const juce::Identifier down                            { juce::String::fromUTF8 ("down")                                                                                      };
inline const juce::Identifier downOn                          { juce::String::fromUTF8 ("downOn")                                                                                    };
inline const juce::Identifier downloads                       { juce::String::fromUTF8 ("Downloads")                                                                                 };///< Downloads directory label.
inline const juce::Identifier drag                            { juce::String::fromUTF8 ("drag")                                                                                      };
inline const juce::Identifier drainComplete                   { juce::String::fromUTF8 ("drainComplete")                                                                             };
inline const juce::Identifier dt                              { juce::String::fromUTF8 ("dt")                                                                                        };
inline const juce::Identifier duration                        { juce::String::fromUTF8 ("duration")                                                                                  };
inline const juce::Identifier e                               { juce::String::fromUTF8 ("e")                                                                                         };
inline const juce::Identifier edge                            { juce::String::fromUTF8 ("edge")                                                                                      };
inline const juce::Identifier edgeLabelPadding                { juce::String::fromUTF8 ("edgeLabelPadding")                                                                          };
inline const juce::Identifier edgeTable                       { juce::String::fromUTF8 ("edgeTable")                                                                                 };
inline const juce::Identifier edges                           { juce::String::fromUTF8 ("edges")                                                                                     };
inline const juce::Identifier editor                          { juce::String::fromUTF8 ("editor")                                                                                    };
inline const juce::Identifier eFlat                           { juce::String::fromUTF8 ("eFlat")                                                                                     };
inline const juce::Identifier element                         { juce::String::fromUTF8 ("element")                                                                                   };
inline const juce::Identifier elementLabel                    { juce::String::fromUTF8 ("Element")                                                                                   };///< Requirement element stereotype label.
inline const juce::Identifier elementTag                      { juce::String::fromUTF8 ("elementTag")                                                                                };
inline const juce::Identifier elif                            { juce::String::fromUTF8 ("elif")                                                                                      };
inline const juce::Identifier ellipse                         { juce::String::fromUTF8 ("ellipse")                                                                                   };
inline const juce::Identifier em                              { juce::String::fromUTF8 ("em")                                                                                        };
inline const juce::Identifier email                           { juce::String::fromUTF8 ("email")                                                                                     };
inline const juce::Identifier embed                           { juce::String::fromUTF8 ("embed")                                                                                     };
inline const juce::Identifier embolden                        { juce::String::fromUTF8 ("embolden")                                                                                  };
inline const juce::Identifier emphasisDelimiter               { juce::String::fromUTF8 ("emphasisDelimiter")                                                                         };
inline const juce::Identifier encoding                        { juce::String::fromUTF8 ("encoding")                                                                                  };
inline const juce::Identifier end                             { juce::String::fromUTF8 ("end")                                                                                       };
inline const juce::Identifier endDecoration                   { juce::String::fromUTF8 ("endDecoration")                                                                             };
inline const juce::Identifier endLabel                        { juce::String::fromUTF8 ("endLabel")                                                                                  };
inline const juce::Identifier endOfFile                       { juce::String::fromUTF8 ("endOfFile")                                                                                 };
inline const juce::Identifier endTag                          { juce::String::fromUTF8 ("endTag")                                                                                    };
inline const juce::Identifier endTagOpen                      { juce::String::fromUTF8 ("</")                                                                                        };///< HTML `</` end-tag open.
inline const juce::Identifier enterprise                      { juce::String::fromUTF8 ("Enterprise")                                                                                };///< C4 enterprise element keyword.
inline const juce::Identifier enterpriseBoundary              { juce::String::fromUTF8 ("Enterprise_Boundary")                                                                       };///< C4 enterprise boundary keyword.
inline const juce::Identifier erDiagram                       { juce::String::fromUTF8 ("erDiagram")                                                                                 };
inline const juce::Identifier erEdgeDashLength                { juce::String::fromUTF8 ("erEdgeDashLength")                                                                          };
inline const juce::Identifier erEdgeStrokeWidth               { juce::String::fromUTF8 ("erEdgeStrokeWidth")                                                                         };
inline const juce::Identifier erEntityPadding                 { juce::String::fromUTF8 ("erEntityPadding")                                                                           };
inline const juce::Identifier erMarkerBarHeight               { juce::String::fromUTF8 ("erMarkerBarHeight")                                                                         };
inline const juce::Identifier erMarkerBarSpacing              { juce::String::fromUTF8 ("erMarkerBarSpacing")                                                                        };
inline const juce::Identifier erMarkerCircleRadius            { juce::String::fromUTF8 ("erMarkerCircleRadius")                                                                      };
inline const juce::Identifier erMarkerFootLength              { juce::String::fromUTF8 ("erMarkerFootLength")                                                                        };
inline const juce::Identifier erMarkerFootSpread              { juce::String::fromUTF8 ("erMarkerFootSpread")                                                                        };
inline const juce::Identifier erMinEntityHeight               { juce::String::fromUTF8 ("erMinEntityHeight")                                                                         };
inline const juce::Identifier erMinEntityWidth                { juce::String::fromUTF8 ("erMinEntityWidth")                                                                          };
inline const juce::Identifier erNodeSeparation                { juce::String::fromUTF8 ("erNodeSeparation")                                                                          };
inline const juce::Identifier erRankSeparation                { juce::String::fromUTF8 ("erRankSeparation")                                                                          };
inline const juce::Identifier eraseCharacter                  { juce::String::fromUTF8 ("eraseCharacter")                                                                            };
inline const juce::Identifier eraseInDisplay                  { juce::String::fromUTF8 ("eraseInDisplay")                                                                            };
inline const juce::Identifier eraseInLine                     { juce::String::fromUTF8 ("eraseInLine")                                                                               };
inline const juce::Identifier errorDirective                  { juce::String::fromUTF8 ("#error")                                                                                    };///< C `#error` preprocessor directive.
inline const juce::Identifier escapedDoubleQuote              { juce::String::fromUTF8 ("\"")                                                                                        };
inline const juce::Identifier escapedPipe                     { juce::String::fromUTF8 ("\\|")                                                                                       };///< Table-cell escaped `\|` pipe.
inline const juce::Identifier evaluation                      { juce::String::fromUTF8 ("evaluation")                                                                                };
inline const juce::Identifier event                           { juce::String::fromUTF8 ("event")                                                                                     };
inline const juce::Identifier except                          { juce::String::fromUTF8 ("except")                                                                                    };
inline const juce::Identifier excludes                        { juce::String::fromUTF8 ("excludes")                                                                                  };
inline const juce::String     existsIn                        { juce::String::fromUTF8 ("exists in")                                                                                 };///< Validator exists-in message.
inline const juce::Identifier expression                      { juce::String::fromUTF8 ("expression")                                                                                };
inline const juce::Identifier extendedBackground              { juce::String::fromUTF8 ("extendedBackground")                                                                        };
inline const juce::Identifier extendedForeground              { juce::String::fromUTF8 ("extendedForeground")                                                                        };
inline const juce::Identifier extendedUnderlineColor          { juce::String::fromUTF8 ("extendedUnderlineColor")                                                                    };
inline const juce::Identifier extends                         { juce::String::fromUTF8 ("extends")                                                                                   };
inline const juce::Identifier externalComponent               { juce::String::fromUTF8 ("External Component")                                                                        };///< C4 external component stereotype.
inline const juce::Identifier externalContainer               { juce::String::fromUTF8 ("External Container")                                                                        };///< C4 external container stereotype.
inline const juce::Identifier externalPerson                  { juce::String::fromUTF8 ("External Person")                                                                           };///< C4 external person stereotype.
inline const juce::Identifier externalSystem                  { juce::String::fromUTF8 ("External System")                                                                           };///< C4 external system stereotype.
inline const juce::Identifier f                               { juce::String::fromUTF8 ("f")                                                                                         };
inline const juce::Identifier fSharp                          { juce::String::fromUTF8 ("fSharp")                                                                                    };
inline const juce::Identifier fader                           { juce::String::fromUTF8 ("fader")                                                                                     };
inline const juce::Identifier faderOverlay                    { juce::String::fromUTF8 ("faderOverlay")                                                                              };
inline const juce::Identifier family                          { juce::String::fromUTF8 ("family")                                                                                    };
inline const juce::Identifier feedback                        { juce::String::fromUTF8 ("Feedback")                                                                                  };///< Feedback pass label.
inline const juce::Identifier feedbackPass                    { juce::String::fromUTF8 ("feedback_pass")                                                                             };///< Feedback pass shader tag.
inline const juce::Identifier fencedCode                      { juce::String::fromUTF8 ("fencedCode")                                                                                };
inline const juce::Identifier fieldset                        { juce::String::fromUTF8 ("fieldset")                                                                                  };
inline const juce::Identifier figcaption                      { juce::String::fromUTF8 ("figcaption")                                                                                };
inline const juce::Identifier figure                          { juce::String::fromUTF8 ("figure")                                                                                    };
inline const juce::Identifier file                            { juce::String::fromUTF8 ("file")                                                                                      };
inline const juce::Identifier fill                            { juce::String::fromUTF8 ("fill")                                                                                      };
inline const juce::Identifier fillOpacity                     { juce::String::fromUTF8 ("fill-opacity")                                                                              };///< SVG `fill-opacity` attribute.
inline const juce::Identifier filterLinear                    { juce::String::fromUTF8 ("filter_linear")                                                                             };///< Vulkan `filter_linear` sampler mode.
inline const juce::Identifier finalViewport                   { juce::String::fromUTF8 ("FinalViewport")                                                                             };///< Final viewport label.
inline const juce::Identifier finally                         { juce::String::fromUTF8 ("finally")                                                                                   };
inline const juce::Identifier flex                            { juce::String::fromUTF8 ("flex")                                                                                      };
inline const juce::Identifier flexBasis                       { juce::String::fromUTF8 ("flex-basis")                                                                                };///< CSS `flex-basis` property.
inline const juce::Identifier flexDirection                   { juce::String::fromUTF8 ("flex-direction")                                                                            };///< CSS `flex-direction` property.
inline const juce::Identifier flexGap                         { juce::String::fromUTF8 ("flex-gap")                                                                                  };///< CSS `gap` property, resolved for flex layout.
inline const juce::Identifier flexEnd                         { juce::String::fromUTF8 ("flex-end")                                                                                  };///< CSS `flex-end` value.
inline const juce::Identifier flexGrow                        { juce::String::fromUTF8 ("flex-grow")                                                                                 };///< CSS `flex-grow` property.
inline const juce::Identifier flexShrink                      { juce::String::fromUTF8 ("flex-shrink")                                                                               };///< CSS `flex-shrink` property.
inline const juce::Identifier floatFramebuffer                { juce::String::fromUTF8 ("float_framebuffer")                                                                         };///< Vulkan `float_framebuffer` format tag.
inline const juce::Identifier floatingPoint                   { juce::String::fromUTF8 ("float")                                                                                     };///< Audio floating-point parameter kind.
inline const juce::Identifier flowchart                       { juce::String::fromUTF8 ("flowchart")                                                                                 };
inline const juce::Identifier flowchartCircleEmptyMinSize     { juce::String::fromUTF8 ("flowchartCircleEmptyMinSize")                                                               };
inline const juce::Identifier flowchartDoubleCircleInnerInset { juce::String::fromUTF8 ("flowchartDoubleCircleInnerInset")                                                           };
inline const juce::Identifier flowchartForkJoinCornerRadius   { juce::String::fromUTF8 ("flowchartForkJoinCornerRadius")                                                             };
inline const juce::Identifier focus                           { juce::String::fromUTF8 ("focus")                                                                                     };
inline const juce::Identifier focusEvents                     { juce::String::fromUTF8 ("focusEvents")                                                                               };
inline const juce::Identifier focusIn                         { juce::String::fromUTF8 ("focusIn")                                                                                   };
inline const juce::Identifier focusOut                        { juce::String::fromUTF8 ("focusOut")                                                                                  };
inline const juce::Identifier focusedPane                     { juce::String::fromUTF8 ("focused_pane")                                                                              };///< Terminal focused-pane tag.
inline const juce::Identifier focusedTab                      { juce::String::fromUTF8 ("focused_tab")                                                                               };///< Terminal focused-tab tag.
inline const juce::Identifier font                            { juce::String::fromUTF8 ("font")                                                                                      };
inline const juce::Identifier fontDefault                     { juce::String::fromUTF8 ("fontDefault")                                                                               };
inline const juce::Identifier fontFace                        { juce::String::fromUTF8 ("font-face")                                                                                 };///< CSS `font-face` at-rule name.
inline const juce::Identifier fontFaceRule                    { juce::String::fromUTF8 ("fontFaceRule")                                                                              };
inline const juce::Identifier fontFamily                      { juce::String::fromUTF8 ("font-family")                                                                               };///< CSS `font-family` property.
inline const juce::Identifier fontFirst                       { juce::String::fromUTF8 ("fontFirst")                                                                                 };
inline const juce::Identifier fontLast                        { juce::String::fromUTF8 ("fontLast")                                                                                  };
inline const juce::Identifier fontSize                        { juce::String::fromUTF8 ("font-size")                                                                                 };///< CSS `font-size` property.
inline const juce::Identifier fontWeight                      { juce::String::fromUTF8 ("fontWeight")                                                                                };
inline const juce::Identifier fonts                           { juce::String::fromUTF8 ("fonts")                                                                                     };
inline const juce::Identifier footer                          { juce::String::fromUTF8 ("footer")                                                                                    };
inline const juce::Identifier foregroundFirst                 { juce::String::fromUTF8 ("foregroundFirst")                                                                           };
inline const juce::Identifier foregroundLast                  { juce::String::fromUTF8 ("foregroundLast")                                                                            };
inline const juce::Identifier forkJoin                        { juce::String::fromUTF8 ("fork")                                                                                      };///< Mermaid fork/join node shape spelling.
inline const juce::Identifier form                            { juce::String::fromUTF8 ("form")                                                                                      };
inline const juce::Identifier format                          { juce::String::fromUTF8 ("format")                                                                                    };
inline const juce::Identifier formats                         { juce::String::fromUTF8 ("formats")                                                                                   };
inline const juce::Identifier fragment                        { juce::String::fromUTF8 ("fragment")                                                                                  };
inline const juce::Identifier frame                           { juce::String::fromUTF8 ("frame")                                                                                     };
inline const juce::Identifier frameCount                      { juce::String::fromUTF8 ("FrameCount")                                                                                };///< Shader FrameCount uniform.
inline const juce::Identifier frameCountMod                   { juce::String::fromUTF8 ("frame_count_mod")                                                                           };///< Shader frame-count modulo uniform.
inline const juce::Identifier frameDirection                  { juce::String::fromUTF8 ("FrameDirection")                                                                            };///< Shader frame-direction uniform.
inline const juce::Identifier frameset                        { juce::String::fromUTF8 ("frameset")                                                                                  };
inline const juce::Identifier freetype                        { juce::String::fromUTF8 ("freetype")                                                                                  };
inline const juce::Identifier from                            { juce::String::fromUTF8 ("from")                                                                                      };
inline const juce::Identifier fromUtF8Prefix                  { juce::String::fromUTF8 ("juce::String::fromUTF8 (\"")                                                                };
inline const juce::Identifier fromUtF8Suffix                  { juce::String::fromUTF8 ("\")")                                                                                       };
inline const juce::Identifier fromSide                        { juce::String::fromUTF8 ("fromSide")                                                                                  };
inline const juce::Identifier ftpAutolinkPrefix               { juce::String::fromUTF8 ("ftp://")                                                                                    };///< FTP autolink prefix.
inline const juce::Identifier function                        { juce::String::fromUTF8 ("function")                                                                                  };
inline const juce::Identifier functionalRequirement           { juce::String::fromUTF8 ("functionalRequirement")                                                                     };
inline const juce::Identifier functionalRequirementLabel      { juce::String::fromUTF8 ("Functional Requirement")                                                                    };///< Requirement functional stereotype label.
inline const juce::Identifier g                               { juce::String::fromUTF8 ("g")                                                                                         };
inline const juce::Identifier gFlat                           { juce::String::fromUTF8 ("gFlat")                                                                                     };
inline const juce::Identifier gSharp                          { juce::String::fromUTF8 ("gSharp")                                                                                    };
inline const juce::Identifier g0                              { juce::String::fromUTF8 ("g0")                                                                                        };
inline const juce::Identifier g1                              { juce::String::fromUTF8 ("g1")                                                                                        };
inline const juce::Identifier g2                              { juce::String::fromUTF8 ("g2")                                                                                        };
inline const juce::Identifier g3                              { juce::String::fromUTF8 ("g3")                                                                                        };
inline const juce::Identifier gamma                           { juce::String::fromUTF8 ("gamma")                                                                                     };
inline const juce::Identifier gantt                           { juce::String::fromUTF8 ("gantt")                                                                                     };
inline const juce::Identifier gap                             { juce::String::fromUTF8 ("gap")                                                                                       };
inline const juce::Identifier gitGraph                        { juce::String::fromUTF8 ("gitGraph")                                                                                  };
inline const juce::Identifier glassFxClear                    { juce::String::fromUTF8 ("glassFXClear")                                                                              };///< macOS clear glass effect.
inline const juce::Identifier glassFxRegular                  { juce::String::fromUTF8 ("glassFXRegular")                                                                            };///< macOS regular glass effect.
inline const juce::Identifier global                          { juce::String::fromUTF8 ("global")                                                                                    };
inline const juce::Identifier goToFolder                      { juce::String::fromUTF8 ("Go to Preset Folder")                                                                       };///< Go-to-folder button label.
inline const juce::Identifier gpu                             { juce::String::fromUTF8 ("GPU")                                                                                       };///< GPU device label.
inline const juce::Identifier gradient                        { juce::String::fromUTF8 ("gradient")                                                                                  };
inline const juce::Identifier gradientUnits                   { juce::String::fromUTF8 ("gradientUnits")                                                                             };
inline const juce::Identifier graph                           { juce::String::fromUTF8 ("graph")                                                                                     };
inline const juce::Identifier graphemeClustering              { juce::String::fromUTF8 ("graphemeClustering")                                                                        };
inline const juce::Identifier graphics                        { juce::String::fromUTF8 ("graphics")                                                                                  };
inline const juce::Identifier group                           { juce::String::fromUTF8 ("group")                                                                                     };
inline const juce::Identifier groupButton                     { juce::String::fromUTF8 ("groupButton")                                                                               };
inline const juce::Identifier groupPadding                    { juce::String::fromUTF8 ("groupPadding")                                                                              };
inline const juce::Identifier groupTitleHeight                { juce::String::fromUTF8 ("groupTitleHeight")                                                                          };
inline const juce::Identifier gt                              { juce::String::fromUTF8 ("gt")                                                                                        };
inline const juce::Identifier h1                              { juce::String::fromUTF8 ("h1")                                                                                        };
inline const juce::Identifier h2                              { juce::String::fromUTF8 ("h2")                                                                                        };
inline const juce::Identifier h3                              { juce::String::fromUTF8 ("h3")                                                                                        };
inline const juce::Identifier h4                              { juce::String::fromUTF8 ("h4")                                                                                        };
inline const juce::Identifier h5                              { juce::String::fromUTF8 ("h5")                                                                                        };
inline const juce::Identifier h6                              { juce::String::fromUTF8 ("h6")                                                                                        };
inline const juce::Identifier head                            { juce::String::fromUTF8 ("head")                                                                                      };
inline const juce::Identifier header                          { juce::String::fromUTF8 ("header")                                                                                    };
inline const juce::Identifier headerRow                       { juce::String::fromUTF8 ("#tr")                                                                                       };///< Grid-table `#tr` header marker.
inline const juce::Identifier heading                         { juce::String::fromUTF8 ("heading")                                                                                   };
inline const juce::Identifier height                          { juce::String::fromUTF8 ("height")                                                                                    };
inline const juce::Identifier help                            { juce::String::fromUTF8 ("help")                                                                                      };
inline const juce::Identifier hertz                           { juce::String::fromUTF8 ("Hz")                                                                                        };///< Hz unit label.
inline const juce::Identifier hex                             { juce::String::fromUTF8 ("hex")                                                                                       };
inline const juce::Identifier hexEscapePrefix                 { juce::String::fromUTF8 ("\\x")                                                                                       };///< Hex `\x` escape prefix.
inline const juce::Identifier hexPrefix                       { juce::String::fromUTF8 ("0x")                                                                                        };///< Hex `0x` literal prefix.
inline const juce::Identifier hidden                          { juce::String::fromUTF8 ("hidden")                                                                                    };
inline const juce::Identifier high                            { juce::String::fromUTF8 ("High")                                                                                      };///< Requirement high risk level.
inline const juce::Identifier highlight                       { juce::String::fromUTF8 ("highlight")                                                                                 };
inline const juce::Identifier horizontal                      { juce::String::fromUTF8 ("horizontal")                                                                                };
inline const juce::Identifier horizontalPositionAbsolute      { juce::String::fromUTF8 ("horizontalPositionAbsolute")                                                                };
inline const juce::Identifier horizontalPositionRelative      { juce::String::fromUTF8 ("horizontalPositionRelative")                                                                };
inline const juce::Identifier horizontalTabSet                { juce::String::fromUTF8 ("horizontalTabSet")                                                                          };
inline const juce::Identifier horizontalVerticalPosition      { juce::String::fromUTF8 ("horizontalVerticalPosition")                                                                };
inline const juce::Identifier hr                              { juce::String::fromUTF8 ("hr")                                                                                        };
inline const juce::Identifier href                            { juce::String::fromUTF8 ("href")                                                                                      };
inline const juce::Identifier html                            { juce::String::fromUTF8 ("html")                                                                                      };
inline const juce::Identifier htmlBlock                       { juce::String::fromUTF8 ("htmlBlock")                                                                                 };
inline const juce::Identifier htmlTagDiv                      { juce::String::fromUTF8 ("div")                                                                                       };///< HTML `<div>` tag name.
inline const juce::Identifier httpAutolinkPrefix              { juce::String::fromUTF8 ("http://")                                                                                   };///< HTTP autolink prefix.
inline const juce::Identifier httpsAutolinkPrefix             { juce::String::fromUTF8 ("https://")                                                                                  };///< HTTPS autolink prefix.
inline const juce::Identifier huge                            { juce::String::fromUTF8 ("huge")                                                                                      };
inline const juce::Identifier hyperlink                       { juce::String::fromUTF8 ("hyperlink")                                                                                 };
inline const juce::Identifier hz                              { juce::String::fromUTF8 ("hz")                                                                                        };
inline const juce::Identifier iChannel                        { juce::String::fromUTF8 ("iChannel")                                                                                  };
inline const juce::Identifier iScene                          { juce::String::fromUTF8 ("iScene")                                                                                    };
inline const juce::Identifier icon                            { juce::String::fromUTF8 ("icon")                                                                                      };
inline const juce::Identifier id                              { juce::String::fromUTF8 ("id")                                                                                        };
inline const juce::Identifier idLabel                         { juce::String::fromUTF8 ("ID")                                                                                        };///< Requirement ID attribute label.
inline const juce::Identifier idNamespace                     { juce::String::fromUTF8 ("Id::")                                                                                      };///< CAST `Id::` namespace prefix.
inline const juce::Identifier ident                           { juce::String::fromUTF8 ("ident")                                                                                     };
inline const juce::Identifier iframe                          { juce::String::fromUTF8 ("iframe")                                                                                    };
inline const juce::Identifier image                           { juce::String::fromUTF8 ("image")                                                                                     };
inline const juce::Identifier imageDecoded                    { juce::String::fromUTF8 ("imageDecoded")                                                                              };
inline const juce::Identifier imageFader                      { juce::String::fromUTF8 ("imageFader")                                                                                };
inline const juce::Identifier imageKnob                       { juce::String::fromUTF8 ("imageKnob")                                                                                 };
inline const juce::Identifier imageOpen                       { juce::String::fromUTF8 ("imageOpen")                                                                                 };
inline const juce::Identifier imageOverlay                    { juce::String::fromUTF8 ("imageOverlay")                                                                              };
inline const juce::Identifier imageToggle                     { juce::String::fromUTF8 ("imageToggle")                                                                               };
inline const juce::Identifier img                             { juce::String::fromUTF8 ("img")                                                                                       };
inline const juce::Identifier implements                      { juce::String::fromUTF8 ("implements")                                                                                };
inline const juce::Identifier import                          { juce::String::fromUTF8 ("import")                                                                                    };
inline const juce::Identifier importLicense                   { juce::String::fromUTF8 ("importLicense")                                                                             };
inline const juce::Identifier importRule                      { juce::String::fromUTF8 ("importRule")                                                                                };
inline const juce::Identifier important                       { juce::String::fromUTF8 ("important")                                                                                 };
inline const juce::Identifier in                              { juce::String::fromUTF8 ("in")                                                                                        };
inline const juce::Identifier includeDirective                { juce::String::fromUTF8 ("#include")                                                                                  };///< C `#include` preprocessor directive.
inline const juce::Identifier includes                        { juce::String::fromUTF8 ("includes")                                                                                  };
inline const juce::Identifier indent                          { juce::String::fromUTF8 ("indent")                                                                                    };
inline const juce::Identifier index                           { juce::String::fromUTF8 ("index")                                                                                     };
inline const juce::Identifier info                            { juce::String::fromUTF8 ("info")                                                                                      };
inline const juce::Identifier init                            { juce::String::fromUTF8 ("init")                                                                                      };
inline const juce::Identifier input                           { juce::String::fromUTF8 ("input")                                                                                     };
inline const juce::Identifier insertCharacter                 { juce::String::fromUTF8 ("insertCharacter")                                                                           };
inline const juce::Identifier insertLine                      { juce::String::fromUTF8 ("insertLine")                                                                                };
inline const juce::Identifier insertMode                      { juce::String::fromUTF8 ("insertMode")                                                                                };
inline const juce::Identifier inset                           { juce::String::fromUTF8 ("inset")                                                                                     };
inline const juce::Identifier instanceof                      { juce::String::fromUTF8 ("instanceof")                                                                                };
inline const juce::Identifier integer                         { juce::String::fromUTF8 ("int")                                                                                       };///< Audio integer parameter kind.
inline const juce::Identifier interfaceRequirement            { juce::String::fromUTF8 ("interfaceRequirement")                                                                      };
inline const juce::Identifier interfaceRequirementLabel       { juce::String::fromUTF8 ("Interface Requirement")                                                                     };///< Requirement interface stereotype label.
inline const juce::Identifier interval                        { juce::String::fromUTF8 ("interval")                                                                                  };
inline const juce::Identifier inverse                         { juce::String::fromUTF8 ("inverse")                                                                                   };
inline const juce::Identifier invisible                       { juce::String::fromUTF8 ("invisible")                                                                                 };
inline const juce::Identifier is                              { juce::String::fromUTF8 ("is")                                                                                        };
inline const juce::Identifier isClose                         { juce::String::fromUTF8 ("isClose")                                                                                   };
inline const juce::Identifier isDirty                         { juce::String::fromUTF8 ("isDirty")                                                                                   };
inline const juce::Identifier isIncrement                     { juce::String::fromUTF8 ("isIncrement")                                                                               };
inline const juce::Identifier isOpen                          { juce::String::fromUTF8 ("isOpen")                                                                                    };
inline const juce::Identifier italic                          { juce::String::fromUTF8 ("italic")                                                                                    };
inline const juce::Identifier item                            { juce::String::fromUTF8 ("item")                                                                                      };
inline const juce::Identifier iterm2Image                     { juce::String::fromUTF8 ("iterm2Image")                                                                               };
inline const juce::Identifier jam                             { juce::String::fromUTF8 ("jam")                                                                                       };
inline const juce::Identifier journey                         { juce::String::fromUTF8 ("journey")                                                                                   };
inline const juce::Identifier jpeg                            { juce::String::fromUTF8 ("jpeg")                                                                                      };
inline const juce::Identifier jpg                             { juce::String::fromUTF8 ("jpg")                                                                                       };
inline const juce::Identifier juceNamespace                   { juce::String::fromUTF8 ("juce::")                                                                                    };///< CAST `juce::` namespace prefix.
inline const juce::Identifier jsonArray                       { juce::String::fromUTF8 ("JSON_ARRAY")                                                                                };///< JSON array marker.
inline const juce::Identifier junction                        { juce::String::fromUTF8 ("junction")                                                                                  };
inline const juce::Identifier justifyContent                  { juce::String::fromUTF8 ("justify-content")                                                                           };///< CSS `justify-content` property.
inline const juce::Identifier kanban                          { juce::String::fromUTF8 ("kanban")                                                                                    };
inline const juce::Identifier kerning                         { juce::String::fromUTF8 ("kerning")                                                                                   };
inline const juce::Identifier keyboardFlags                   { juce::String::fromUTF8 ("keyboardFlags")                                                                             };
inline const juce::Identifier keyboardProtocol                { juce::String::fromUTF8 ("keyboardProtocol")                                                                          };
inline const juce::Identifier keyboardResetGivenFlags         { juce::String::fromUTF8 ("keyboardResetGivenFlags")                                                                   };
inline const juce::Identifier keyboardSetAllFlags             { juce::String::fromUTF8 ("keyboardSetAllFlags")                                                                       };
inline const juce::Identifier keyboardSetGivenFlags           { juce::String::fromUTF8 ("keyboardSetGivenFlags")                                                                     };
inline const juce::Identifier keyword                         { juce::String::fromUTF8 ("keyword")                                                                                   };
inline const juce::Identifier khz                             { juce::String::fromUTF8 ("khz")                                                                                       };
inline const juce::Identifier kiloHertz                       { juce::String::fromUTF8 ("KHz")                                                                                       };///< kHz unit label.
inline const juce::Identifier knob                            { juce::String::fromUTF8 ("knob")                                                                                      };
inline const juce::Identifier label                           { juce::String::fromUTF8 ("label")                                                                                     };
inline const juce::Identifier labels                          { juce::String::fromUTF8 ("labels")                                                                                    };
inline const juce::Identifier lambda                          { juce::String::fromUTF8 ("lambda")                                                                                    };
inline const juce::Identifier landscape                       { juce::String::fromUTF8 ("landscape")                                                                                 };
inline const juce::Identifier large                           { juce::String::fromUTF8 ("large")                                                                                     };
inline const juce::Identifier layout                          { juce::String::fromUTF8 ("layout")                                                                                    };
inline const juce::Identifier layoutCell                      { juce::String::fromUTF8 ("layoutCell")                                                                                };
inline const juce::Identifier leanL                           { juce::String::fromUTF8 ("leanL")                                                                                     };
inline const juce::Identifier leanR                           { juce::String::fromUTF8 ("leanR")                                                                                     };
inline const juce::Identifier left                            { juce::String::fromUTF8 ("left")                                                                                      };
inline const juce::Identifier leftOf                          { juce::String::fromUTF8 ("leftOf")                                                                                    };
inline const juce::Identifier legalCompanyName                { juce::String::fromUTF8 ("legalCompanyName")                                                                          };
inline const juce::Identifier legend                          { juce::String::fromUTF8 ("legend")                                                                                    };
inline const juce::Identifier let                             { juce::String::fromUTF8 ("let")                                                                                       };
inline const juce::Identifier level                           { juce::String::fromUTF8 ("level")                                                                                     };
inline const juce::Identifier level1                          { juce::String::fromUTF8 ("level1")                                                                                    };
inline const juce::Identifier level2                          { juce::String::fromUTF8 ("level2")                                                                                    };
inline const juce::Identifier level3                          { juce::String::fromUTF8 ("level3")                                                                                    };
inline const juce::Identifier level4                          { juce::String::fromUTF8 ("level4")                                                                                    };
inline const juce::Identifier level5                          { juce::String::fromUTF8 ("level5")                                                                                    };
inline const juce::Identifier level6                          { juce::String::fromUTF8 ("level6")                                                                                    };
inline const juce::Identifier li                              { juce::String::fromUTF8 ("li")                                                                                        };
inline const juce::Identifier licenses                        { juce::String::fromUTF8 ("Licenses")                                                                                  };///< Licenses label.
inline const juce::Identifier light                           { juce::String::fromUTF8 ("light")                                                                                     };
inline const juce::Identifier line                            { juce::String::fromUTF8 ("line")                                                                                      };
inline const juce::Identifier lineBreak                       { juce::String::fromUTF8 ("lineBreak")                                                                                 };
inline const juce::Identifier linear                          { juce::String::fromUTF8 ("linear")                                                                                    };
inline const juce::Identifier linearGradient                  { juce::String::fromUTF8 ("linearGradient")                                                                            };
inline const juce::Identifier lineHeight                      { juce::String::fromUTF8 ("lineHeight")                                                                                };
inline const juce::Identifier link                            { juce::String::fromUTF8 ("link")                                                                                      };
inline const juce::Identifier linkClose                       { juce::String::fromUTF8 ("linkClose")                                                                                 };
inline const juce::Identifier linkOpen                        { juce::String::fromUTF8 ("linkOpen")                                                                                  };
inline const juce::Identifier linkStyle                       { juce::String::fromUTF8 ("linkStyle")                                                                                 };
inline const juce::Identifier links                           { juce::String::fromUTF8 ("links")                                                                                     };
inline const juce::Identifier listIndent                      { juce::String::fromUTF8 ("listIndent")                                                                                };
inline const juce::Identifier listItem                        { juce::String::fromUTF8 ("listItem")                                                                                  };
inline const juce::Identifier literalBreak                    { juce::String::fromUTF8 ("\" \"")                                                                                     };
inline const juce::Identifier log1                            { juce::String::fromUTF8 ("log1")                                                                                      };
inline const juce::Identifier log10                           { juce::String::fromUTF8 ("log10")                                                                                     };
inline const juce::Identifier log15                           { juce::String::fromUTF8 ("log15")                                                                                     };
inline const juce::Identifier log2                            { juce::String::fromUTF8 ("log2")                                                                                      };
inline const juce::Identifier log20                           { juce::String::fromUTF8 ("log20")                                                                                     };
inline const juce::Identifier log25                           { juce::String::fromUTF8 ("log25")                                                                                     };
inline const juce::Identifier log3                            { juce::String::fromUTF8 ("log3")                                                                                      };
inline const juce::Identifier log30                           { juce::String::fromUTF8 ("log30")                                                                                     };
inline const juce::Identifier log35                           { juce::String::fromUTF8 ("log35")                                                                                     };
inline const juce::Identifier log4                            { juce::String::fromUTF8 ("log4")                                                                                      };
inline const juce::Identifier log40                           { juce::String::fromUTF8 ("log40")                                                                                     };
inline const juce::Identifier log45                           { juce::String::fromUTF8 ("log45")                                                                                     };
inline const juce::Identifier log5                            { juce::String::fromUTF8 ("log5")                                                                                      };
inline const juce::Identifier loop                            { juce::String::fromUTF8 ("loop")                                                                                      };
inline const juce::Identifier low                             { juce::String::fromUTF8 ("Low")                                                                                       };///< Requirement low risk level.
inline const juce::Identifier lr                              { juce::String::fromUTF8 ("lr")                                                                                        };
inline const juce::Identifier lt                              { juce::String::fromUTF8 ("lt")                                                                                        };
inline const juce::Identifier luaArrayClose                   { juce::String::fromUTF8 (" }")                                                                                        };///< Lua array ` }` close.
inline const juce::Identifier luaArrayOpen                    { juce::String::fromUTF8 ("{ ")                                                                                        };///< Lua array `{ ` open.
inline const juce::Identifier luaArrayPropertyClose           { juce::String::fromUTF8 (" },")                                                                                       };///< Lua array property ` },` close.
inline const juce::Identifier luaArrayPropertyOpen            { juce::String::fromUTF8 ("= {")                                                                                       };///< Lua array property `= {` open.
inline const juce::Identifier luaArraySeparator               { juce::String::fromUTF8 (", ")                                                                                        };///< Lua array `, ` separator.
inline const juce::Identifier luaClose                        { juce::String::fromUTF8 ("}")                                                                                         };///< Lua table `}` close.
inline const juce::Identifier luaComma                        { juce::String::fromUTF8 (",")                                                                                         };///< Lua `,` separator.
inline const juce::Identifier luaReturnOpen                   { juce::String::fromUTF8 ("return {")                                                                                  };///< Lua `return {` open.
inline const juce::Identifier luaTableClose                   { juce::String::fromUTF8 ("},")                                                                                        };///< Lua table `},` close.
inline const juce::Identifier luaTableOpen                    { juce::String::fromUTF8 (" = {")                                                                                      };///< Lua table ` = {` open.
inline const juce::Identifier lv2                             { juce::String::fromUTF8 ("lv2")                                                                                       };
inline const juce::Identifier mac                             { juce::String::fromUTF8 ("mac")                                                                                       };
inline const juce::Identifier mailtoAutolinkPrefix            { juce::String::fromUTF8 ("mailto:")                                                                                   };///< mailto autolink prefix.
inline const juce::Identifier main                            { juce::String::fromUTF8 ("main")                                                                                      };
inline const juce::Identifier mainMesh                        { juce::String::fromUTF8 ("mainMesh")                                                                                  };
inline const juce::Identifier mapNamespace                    { juce::String::fromUTF8 ("map::")                                                                                     };///< CAST `map::` namespace prefix.
inline const juce::Identifier marginTop                       { juce::String::fromUTF8 ("margin-top")                                                                                };///< CSS `margin-top` property.
inline const juce::Identifier marker                          { juce::String::fromUTF8 ("marker")                                                                                    };
inline const juce::Identifier markup                          { juce::String::fromUTF8 ("markup")                                                                                    };
inline const juce::Identifier markupProcessingClose           { juce::String::fromUTF8 ("?>")                                                                                        };///< Markup `?>` processing close.
inline const juce::Identifier markupProcessingOpen            { juce::String::fromUTF8 ("<?")                                                                                        };///< Markup `<?` processing open.
inline const juce::Identifier master                          { juce::String::fromUTF8 ("master")                                                                                    };
inline const juce::String     matches                         { juce::String::fromUTF8 ("matches")                                                                                   };
inline const juce::Identifier max                             { juce::String::fromUTF8 ("max")                                                                                       };
inline const juce::Identifier maxChannelCount                 { juce::String::fromUTF8 ("maxChannelCount")                                                                           };
inline const juce::Identifier maxContent                      { juce::String::fromUTF8 ("max-content")                                                                               };///< CSS `max-content` value.
inline const juce::Identifier maxLabel                        { juce::String::fromUTF8 ("maxLabel")                                                                                  };
inline const juce::Identifier media                           { juce::String::fromUTF8 ("media")                                                                                     };
inline const juce::Identifier mediaRule                       { juce::String::fromUTF8 ("mediaRule")                                                                                 };
inline const juce::Identifier medium                          { juce::String::fromUTF8 ("medium")                                                                                    };
inline const juce::Identifier menu                            { juce::String::fromUTF8 ("menu")                                                                                      };
inline const juce::Identifier menuitem                        { juce::String::fromUTF8 ("menuitem")                                                                                  };
inline const juce::Identifier merge                           { juce::String::fromUTF8 ("merge")                                                                                     };
inline const juce::Identifier mermaid                         { juce::String::fromUTF8 ("mermaid")                                                                                   };
inline const juce::Identifier mesh                            { juce::String::fromUTF8 ("mesh")                                                                                      };
inline const juce::Identifier meshShader                      { juce::String::fromUTF8 ("mesh_shader")                                                                               };///< Vulkan mesh-shader stage tag.
inline const juce::Identifier meta                            { juce::String::fromUTF8 ("meta")                                                                                      };
inline const juce::Identifier metric                          { juce::String::fromUTF8 ("metric")                                                                                    };
inline const juce::Identifier metrics                         { juce::String::fromUTF8 ("metrics")                                                                                   };
inline const juce::Identifier middle                          { juce::String::fromUTF8 ("middle")                                                                                    };
inline const juce::Identifier milestone                       { juce::String::fromUTF8 ("milestone")                                                                                 };
inline const juce::Identifier min                             { juce::String::fromUTF8 ("min")                                                                                       };
inline const juce::Identifier mindmap                         { juce::String::fromUTF8 ("mindmap")                                                                                   };
inline const juce::Identifier mindmapDefault                  { juce::String::fromUTF8 ("mindmapDefault")                                                                            };
inline const juce::Identifier mini                            { juce::String::fromUTF8 ("mini")                                                                                      };
inline const juce::Identifier minLabel                        { juce::String::fromUTF8 ("minLabel")                                                                                  };
inline const juce::Identifier minus                           { juce::String::fromUTF8 ("minus")                                                                                     };
inline const juce::Identifier mipmap                          { juce::String::fromUTF8 ("mipmap")                                                                                    };
inline const juce::Identifier mipmapInput                     { juce::String::fromUTF8 ("mipmap_input")                                                                              };///< Vulkan mipmap-input sampler tag.
inline const juce::Identifier mirroredRepeat                  { juce::String::fromUTF8 ("mirrored_repeat")                                                                           };///< Vulkan `mirrored_repeat` sampler mode.
inline const juce::Identifier mode                            { juce::String::fromUTF8 ("mode")                                                                                      };
inline const juce::Identifier modeReportReset                 { juce::String::fromUTF8 ("modeReportReset")                                                                           };
inline const juce::Identifier modes                           { juce::String::fromUTF8 ("modes")                                                                                     };
inline const juce::Identifier mouseAll                        { juce::String::fromUTF8 ("mouseAll")                                                                                  };
inline const juce::Identifier mouseClick                      { juce::String::fromUTF8 ("mouseClick")                                                                                };
inline const juce::Identifier mouseDrag                       { juce::String::fromUTF8 ("mouseDrag")                                                                                 };
inline const juce::Identifier mouseHighlight                  { juce::String::fromUTF8 ("mouseHighlight")                                                                            };
inline const juce::Identifier mouseSgr                        { juce::String::fromUTF8 ("mouseSgr")                                                                                  };
inline const juce::Identifier mouseTracking                   { juce::String::fromUTF8 ("mouseTracking")                                                                             };
inline const juce::Identifier mvp                             { juce::String::fromUTF8 ("MVP")                                                                                       };///< MVP matrix uniform label.
inline const juce::Identifier name                            { juce::String::fromUTF8 ("name")                                                                                      };
inline const juce::Identifier namespaceRule                   { juce::String::fromUTF8 ("namespaceRule")                                                                             };
inline const juce::Identifier native                          { juce::String::fromUTF8 ("native")                                                                                    };
inline const juce::Identifier nav                             { juce::String::fromUTF8 ("nav")                                                                                       };
inline const juce::Identifier nearest                         { juce::String::fromUTF8 ("nearest")                                                                                   };
inline const juce::Identifier newLineMode                     { juce::String::fromUTF8 ("newLineMode")                                                                               };
inline const juce::Identifier nextLine                        { juce::String::fromUTF8 ("nextLine")                                                                                  };
inline const juce::Identifier noBlink                         { juce::String::fromUTF8 ("noBlink")                                                                                   };
inline const juce::Identifier noBoldDim                       { juce::String::fromUTF8 ("noBoldDim")                                                                                 };
inline const juce::Identifier noHidden                        { juce::String::fromUTF8 ("noHidden")                                                                                  };
inline const juce::Identifier noInverse                       { juce::String::fromUTF8 ("noInverse")                                                                                 };
inline const juce::Identifier noItalic                        { juce::String::fromUTF8 ("noItalic")                                                                                  };
inline const juce::Identifier noOverline                      { juce::String::fromUTF8 ("noOverline")                                                                                };
inline const juce::Identifier noStrike                        { juce::String::fromUTF8 ("noStrike")                                                                                  };
inline const juce::Identifier noSubscript                     { juce::String::fromUTF8 ("noSubscript")                                                                               };
inline const juce::Identifier noSuperscript                   { juce::String::fromUTF8 ("noSuperscript")                                                                             };
inline const juce::Identifier noUnderline                     { juce::String::fromUTF8 ("noUnderline")                                                                               };
inline const juce::Identifier node                            { juce::String::fromUTF8 ("node")                                                                                      };
inline const juce::Identifier nodeL                           { juce::String::fromUTF8 ("Node_L")                                                                                    };///< C4 left node keyword.
inline const juce::Identifier nodePadding                     { juce::String::fromUTF8 ("nodePadding")                                                                               };
inline const juce::Identifier nodeR                           { juce::String::fromUTF8 ("Node_R")                                                                                    };///< C4 right node keyword.
inline const juce::Identifier nodeSeparation                  { juce::String::fromUTF8 ("nodeSeparation")                                                                            };
inline const juce::Identifier nodes                           { juce::String::fromUTF8 ("nodes")                                                                                     };
inline const juce::Identifier noframes                        { juce::String::fromUTF8 ("noframes")                                                                                  };
inline const juce::Identifier none                            { juce::String::fromUTF8 ("none")                                                                                      };
inline const juce::Identifier nonlocal                        { juce::String::fromUTF8 ("nonlocal")                                                                                  };
inline const juce::Identifier noodle                          { juce::String::fromUTF8 ("noodle")                                                                                    };
inline const juce::Identifier normal                          { juce::String::fromUTF8 ("normal")                                                                                    };
inline const juce::Identifier normalKeypad                    { juce::String::fromUTF8 ("normalKeypad")                                                                              };
inline const juce::Identifier normalOn                        { juce::String::fromUTF8 ("normalOn")                                                                                  };
inline const juce::Identifier notRecognised                   { juce::String::fromUTF8 ("notRecognised")                                                                             };
inline const juce::Identifier note                            { juce::String::fromUTF8 ("note")                                                                                      };
inline const juce::Identifier notifyTitleBody                 { juce::String::fromUTF8 ("notifyTitleBody")                                                                           };
inline const juce::Identifier null                            { juce::String::fromUTF8 ("null")                                                                                      };
inline const juce::Identifier numChannels                     { juce::String::fromUTF8 ("numChannels")                                                                               };
inline const juce::Identifier number                          { juce::String::fromUTF8 ("number")                                                                                    };
inline const juce::Identifier numbers                         { juce::String::fromUTF8 ("numbers")                                                                                   };
inline const juce::Identifier numeric                         { juce::String::fromUTF8 ("numeric")                                                                                   };
inline const juce::Identifier numericKeypad                   { juce::String::fromUTF8 ("numericKeypad")                                                                             };
inline const juce::Identifier odd                             { juce::String::fromUTF8 ("odd")                                                                                       };
inline const juce::Identifier ode                             { juce::String::fromUTF8 ("ode")                                                                                       };
inline const juce::Identifier of                              { juce::String::fromUTF8 ("of")                                                                                        };
inline const juce::Identifier off                             { juce::String::fromUTF8 ("off")                                                                                       };
inline const juce::Identifier offset                          { juce::String::fromUTF8 ("offset")                                                                                    };
inline const juce::Identifier ol                              { juce::String::fromUTF8 ("ol")                                                                                        };
inline const juce::Identifier on                              { juce::String::fromUTF8 ("on")                                                                                        };
inline const juce::Identifier onState                         { juce::String::fromUTF8 ("onState")                                                                                   };
inline const juce::String     oneOf                           { juce::String::fromUTF8 ("one of")                                                                                    };///< Validator one-of message.
inline const juce::String     onePerGroup                     { juce::String::fromUTF8 ("one per group")                                                                             };///< Validator one-per-group message.
inline const juce::Identifier opacity                         { juce::String::fromUTF8 ("opacity")                                                                                   };
inline const juce::Identifier open                            { juce::String::fromUTF8 ("open")                                                                                      };
inline const juce::Identifier openTriangle                    { juce::String::fromUTF8 ("openTriangle")                                                                              };
inline const juce::Identifier operators                       { juce::String::fromUTF8 ("<>")                                                                                        };///< CAST `< >` operator pair.
inline const juce::Identifier opt                             { juce::String::fromUTF8 ("opt")                                                                                       };
inline const juce::Identifier optgroup                        { juce::String::fromUTF8 ("optgroup")                                                                                  };
inline const juce::Identifier option                          { juce::String::fromUTF8 ("option")                                                                                    };
inline const juce::Identifier order                           { juce::String::fromUTF8 ("order")                                                                                     };
inline const juce::Identifier orientation                     { juce::String::fromUTF8 ("orientation")                                                                               };
inline const juce::Identifier originMode                      { juce::String::fromUTF8 ("originMode")                                                                                };
inline const juce::Identifier original                        { juce::String::fromUTF8 ("Original")                                                                                  };///< Original history label.
inline const juce::Identifier originalHistory                 { juce::String::fromUTF8 ("OriginalHistory")                                                                           };///< Original history label.
inline const juce::Identifier out                             { juce::String::fromUTF8 ("out")                                                                                       };
inline const juce::Identifier output                          { juce::String::fromUTF8 ("output")                                                                                    };
inline const juce::Identifier outputEnd                       { juce::String::fromUTF8 ("outputEnd")                                                                                 };
inline const juce::Identifier outputStart                     { juce::String::fromUTF8 ("outputStart")                                                                               };
inline const juce::Identifier over                            { juce::String::fromUTF8 ("over")                                                                                      };
inline const juce::Identifier overOn                          { juce::String::fromUTF8 ("overOn")                                                                                    };
inline const juce::Identifier overline                        { juce::String::fromUTF8 ("overline")                                                                                  };
inline const juce::Identifier oversampling                    { juce::String::fromUTF8 ("oversampling")                                                                              };
inline const juce::Identifier p                               { juce::String::fromUTF8 ("p")                                                                                         };
inline const juce::Identifier packet                          { juce::String::fromUTF8 ("packet")                                                                                    };
inline const juce::Identifier packetBeta                      { juce::String::fromUTF8 ("packet-beta")                                                                               };///< Mermaid packet-beta diagram keyword.
inline const juce::Identifier padding                         { juce::String::fromUTF8 ("padding")                                                                                   };
inline const juce::Identifier page                            { juce::String::fromUTF8 ("page")                                                                                      };
inline const juce::Identifier pageA                           { juce::String::fromUTF8 ("pageA")                                                                                     };
inline const juce::Identifier pageB                           { juce::String::fromUTF8 ("pageB")                                                                                     };
inline const juce::Identifier pageC                           { juce::String::fromUTF8 ("pageC")                                                                                     };
inline const juce::Identifier pageRule                        { juce::String::fromUTF8 ("pageRule")                                                                                  };
inline const juce::Identifier palette256                      { juce::String::fromUTF8 ("palette256")                                                                                };
inline const juce::Identifier panel                           { juce::String::fromUTF8 ("panel")                                                                                     };
inline const juce::Identifier panelHeight                     { juce::String::fromUTF8 ("panel_height")                                                                              };///< Panel height metric key.
inline const juce::Identifier panelTop                        { juce::String::fromUTF8 ("panelTop")                                                                                  };
inline const juce::Identifier par                             { juce::String::fromUTF8 ("par")                                                                                       };
inline const juce::Identifier paragraph                       { juce::String::fromUTF8 ("paragraph")                                                                                 };
inline const juce::Identifier paragraphSpacing                { juce::String::fromUTF8 ("paragraphSpacing")                                                                          };
inline const juce::Identifier parallelogram                   { juce::String::fromUTF8 ("lean-r")                                                                                    };///< Mermaid lean-right parallelogram spelling.
inline const juce::Identifier parallelogramAlt                { juce::String::fromUTF8 ("lean-l")                                                                                    };///< Mermaid lean-left parallelogram spelling.
inline const juce::Identifier param                           { juce::String::fromUTF8 ("param")                                                                                     };
inline const juce::Identifier parameter                       { juce::String::fromUTF8 ("parameter")                                                                                 };
inline const juce::Identifier parent                          { juce::String::fromUTF8 ("parent")                                                                                    };
inline const juce::String     parity                          { juce::String::fromUTF8 ("parity")                                                                                    };
inline const juce::Identifier participant                     { juce::String::fromUTF8 ("participant")                                                                               };
inline const juce::Identifier pass                            { juce::String::fromUTF8 ("pass")                                                                                      };
inline const juce::Identifier passFeedback                    { juce::String::fromUTF8 ("PassFeedback")                                                                              };///< Feedback render-pass label.
inline const juce::Identifier passOutput                      { juce::String::fromUTF8 ("PassOutput")                                                                                };///< Output render-pass label.
inline const juce::Identifier passSource                      { juce::String::fromUTF8 ("passSource")                                                                                };
inline const juce::Identifier path                            { juce::String::fromUTF8 ("path")                                                                                      };
inline const juce::Identifier pathBrowser                     { juce::String::fromUTF8 ("pathBrowser")                                                                               };
inline const juce::Identifier percentage                      { juce::String::fromUTF8 ("percentage")                                                                                };
inline const juce::Identifier performanceRequirement          { juce::String::fromUTF8 ("performanceRequirement")                                                                    };
inline const juce::Identifier performanceRequirementLabel     { juce::String::fromUTF8 ("Performance Requirement")                                                                   };///< Requirement performance stereotype label.
inline const juce::Identifier permanentlyReset                { juce::String::fromUTF8 ("permanentlyReset")                                                                          };
inline const juce::Identifier permanentlySet                  { juce::String::fromUTF8 ("permanentlySet")                                                                            };
inline const juce::Identifier person                          { juce::String::fromUTF8 ("Person")                                                                                    };///< C4 person element keyword.
inline const juce::Identifier personExt                       { juce::String::fromUTF8 ("Person_Ext")                                                                                };///< C4 external person keyword.
inline const juce::Identifier physicalRequirement             { juce::String::fromUTF8 ("physicalRequirement")                                                                       };
inline const juce::Identifier physicalRequirementLabel        { juce::String::fromUTF8 ("Physical Requirement")                                                                      };///< Requirement physical stereotype label.
inline const juce::Identifier pie                             { juce::String::fromUTF8 ("pie")                                                                                       };
inline const juce::Identifier pluginWrapper                   { juce::String::fromUTF8 ("pluginWrapper")                                                                             };
inline const juce::Identifier plus                            { juce::String::fromUTF8 ("plus")                                                                                      };
inline const juce::Identifier png                             { juce::String::fromUTF8 ("png")                                                                                       };
inline const juce::Identifier polygon                         { juce::String::fromUTF8 ("polygon")                                                                                   };
inline const juce::Identifier popupTextBox                    { juce::String::fromUTF8 ("popupTextBox")                                                                              };
inline const juce::Identifier portrait                        { juce::String::fromUTF8 ("portrait")                                                                                  };
inline const juce::Identifier position                        { juce::String::fromUTF8 ("position")                                                                                  };
inline const juce::Identifier pragma                          { juce::String::fromUTF8 ("#pragma")                                                                                   };///< C `#pragma` preprocessor directive.
inline const juce::Identifier pre                             { juce::String::fromUTF8 ("pre")                                                                                       };
inline const juce::Identifier preCloseTag                     { juce::String::fromUTF8 ("</pre>")                                                                                    };///< HTML `</pre>` close tag.
inline const juce::Identifier preset                          { juce::String::fromUTF8 ("preset")                                                                                    };
inline const juce::Identifier presetSelector                  { juce::String::fromUTF8 ("presetSelector")                                                                            };
inline const juce::Identifier presets                         { juce::String::fromUTF8 ("Presets")                                                                                   };///< Presets label.
inline const juce::Identifier previewFile                     { juce::String::fromUTF8 ("previewFile")                                                                               };
inline const juce::Identifier primary                         { juce::String::fromUTF8 ("primary")                                                                                   };
inline const juce::Identifier priority                        { juce::String::fromUTF8 ("priority")                                                                                  };
inline const juce::Identifier privateMarker                   { juce::String::fromUTF8 ("privateMarker")                                                                             };
inline const juce::Identifier processingInstruction           { juce::String::fromUTF8 ("processingInstruction")                                                                     };
inline const juce::Identifier product                         { juce::String::fromUTF8 ("product")                                                                                   };
inline const juce::Identifier productName                     { juce::String::fromUTF8 ("productName")                                                                               };
inline const juce::Identifier prompt                          { juce::String::fromUTF8 ("prompt")                                                                                    };
inline const juce::Identifier promptRow                       { juce::String::fromUTF8 ("promptRow")                                                                                 };
inline const juce::Identifier promptStart                     { juce::String::fromUTF8 ("promptStart")                                                                               };
inline const juce::Identifier proportional                    { juce::String::fromUTF8 ("proportional")                                                                              };
inline const juce::Identifier proportions                     { juce::String::fromUTF8 ("proportions")                                                                               };
inline const juce::Identifier protocolSeparator               { juce::String::fromUTF8 ("://")                                                                                       };///< URL `://` separator.
inline const juce::Identifier punctuation                     { juce::String::fromUTF8 ("punctuation")                                                                               };
inline const juce::Identifier px                              { juce::String::fromUTF8 ("px")                                                                                        };
inline const juce::Identifier quadrantChart                   { juce::String::fromUTF8 ("quadrantChart")                                                                             };
inline const juce::Identifier quadrant1                       { juce::String::fromUTF8 ("quadrant-1")                                                                                };///< Mermaid quadrant-1 label.
inline const juce::Identifier quadrant2                       { juce::String::fromUTF8 ("quadrant-2")                                                                                };///< Mermaid quadrant-2 label.
inline const juce::Identifier quadrant3                       { juce::String::fromUTF8 ("quadrant-3")                                                                                };///< Mermaid quadrant-3 label.
inline const juce::Identifier quadrant4                       { juce::String::fromUTF8 ("quadrant-4")                                                                                };///< Mermaid quadrant-4 label.
inline const juce::Identifier quot                            { juce::String::fromUTF8 ("quot")                                                                                      };
inline const juce::Identifier quote                           { juce::String::fromUTF8 ("quote")                                                                                     };
inline const juce::Identifier quoteBarWidth                   { juce::String::fromUTF8 ("quoteBarWidth")                                                                             };
inline const juce::Identifier quoteIndent                     { juce::String::fromUTF8 ("quoteIndent")                                                                               };
inline const juce::Identifier quotes                          { juce::String::fromUTF8 ("quotes")                                                                                    };
inline const juce::Identifier r                               { juce::String::fromUTF8 ("r")                                                                                         };
inline const juce::Identifier raise                           { juce::String::fromUTF8 ("raise")                                                                                     };
inline const juce::String     range                           { juce::String::fromUTF8 ("range")                                                                                     };
inline const juce::Identifier rankSeparation                  { juce::String::fromUTF8 ("rankSeparation")                                                                            };
inline const juce::Identifier rapidBlink                      { juce::String::fromUTF8 ("rapidBlink")                                                                                };
inline const juce::Identifier rasterizer                      { juce::String::fromUTF8 ("rasterizer")                                                                                };
inline const juce::Identifier rawHtml                         { juce::String::fromUTF8 ("rawHtml")                                                                                   };
inline const juce::Identifier rawText                         { juce::String::fromUTF8 ("rawText")                                                                                   };
inline const juce::Identifier recent                          { juce::String::fromUTF8 ("recent")                                                                                    };
inline const juce::Identifier rect                            { juce::String::fromUTF8 ("rect")                                                                                      };
inline const juce::Identifier referenceDefinition             { juce::String::fromUTF8 ("referenceDefinition")                                                                       };
inline const juce::Identifier refines                         { juce::String::fromUTF8 ("refines")                                                                                   };
inline const juce::Identifier region                          { juce::String::fromUTF8 ("region")                                                                                    };
inline const juce::Identifier regionClose                     { juce::String::fromUTF8 ("regionClose")                                                                               };
inline const juce::Identifier regionOpen                      { juce::String::fromUTF8 ("regionOpen")                                                                                };
inline const juce::Identifier rel                             { juce::String::fromUTF8 ("Rel")                                                                                       };///< C4 generic relation keyword.
inline const juce::Identifier relD                            { juce::String::fromUTF8 ("Rel_D")                                                                                     };///< C4 down relation keyword.
inline const juce::Identifier relL                            { juce::String::fromUTF8 ("Rel_L")                                                                                     };///< C4 left relation keyword.
inline const juce::Identifier relR                            { juce::String::fromUTF8 ("Rel_R")                                                                                     };///< C4 right relation keyword.
inline const juce::Identifier relU                            { juce::String::fromUTF8 ("Rel_U")                                                                                     };///< C4 up relation keyword.
inline const juce::Identifier relBack                         { juce::String::fromUTF8 ("Rel_Back")                                                                                  };///< C4 back relation keyword.
inline const juce::Identifier relIndex                        { juce::String::fromUTF8 ("RelIndex")                                                                                  };///< C4 index relation keyword.
inline const juce::Identifier renderer                        { juce::String::fromUTF8 ("renderer")                                                                                  };
inline const juce::Identifier repeat                          { juce::String::fromUTF8 ("repeat")                                                                                    };
inline const juce::Identifier repeatCharacter                 { juce::String::fromUTF8 ("repeatCharacter")                                                                           };
inline const juce::Identifier reportCellPixels                { juce::String::fromUTF8 ("reportCellPixels")                                                                          };
inline const juce::Identifier reportCursorPosition            { juce::String::fromUTF8 ("reportCursorPosition")                                                                      };
inline const juce::Identifier reportTextChars                 { juce::String::fromUTF8 ("reportTextChars")                                                                           };
inline const juce::Identifier reportTextPixels                { juce::String::fromUTF8 ("reportTextPixels")                                                                          };
inline const juce::Identifier requestMode                     { juce::String::fromUTF8 ("requestMode")                                                                               };
inline const juce::Identifier requirement                     { juce::String::fromUTF8 ("requirement")                                                                               };
inline const juce::Identifier requirementDiagram              { juce::String::fromUTF8 ("requirementDiagram")                                                                        };
inline const juce::Identifier requirementLabel                { juce::String::fromUTF8 ("Requirement")                                                                               };///< Requirement stereotype label.
inline const juce::Identifier reset                           { juce::String::fromUTF8 ("reset")                                                                                     };
inline const juce::Identifier resetCursorColor                { juce::String::fromUTF8 ("resetCursorColor")                                                                          };
inline const juce::Identifier resetMode                       { juce::String::fromUTF8 ("resetMode")                                                                                 };
inline const juce::Identifier resetToInitialState             { juce::String::fromUTF8 ("resetToInitialState")                                                                       };
inline const juce::Identifier restoreCursor                   { juce::String::fromUTF8 ("restoreCursor")                                                                             };
inline const juce::Identifier reverse                         { juce::String::fromUTF8 ("reverse")                                                                                   };
inline const juce::Identifier reverseIndex                    { juce::String::fromUTF8 ("reverseIndex")                                                                              };
inline const juce::Identifier reverseVideo                    { juce::String::fromUTF8 ("reverseVideo")                                                                              };
inline const juce::Identifier rgb24                           { juce::String::fromUTF8 ("rgb24")                                                                                     };
inline const juce::Identifier right                           { juce::String::fromUTF8 ("right")                                                                                     };
inline const juce::Identifier rightOf                         { juce::String::fromUTF8 ("rightOf")                                                                                   };
inline const juce::Identifier risk                            { juce::String::fromUTF8 ("risk")                                                                                      };
inline const juce::Identifier riskLabel                       { juce::String::fromUTF8 ("Risk")                                                                                      };///< Requirement Risk attribute label.
inline const juce::Identifier rl                              { juce::String::fromUTF8 ("rl")                                                                                        };
inline const juce::Identifier root                            { juce::String::fromUTF8 ("root")                                                                                      };
inline const juce::Identifier rootSelector                    { juce::String::fromUTF8 (":root")                                                                                     };///< CSS `:root` selector.
inline const juce::Identifier round                           { juce::String::fromUTF8 ("round")                                                                                     };
inline const juce::Identifier rounded                         { juce::String::fromUTF8 ("rounded")                                                                                   };
inline const juce::Identifier row                             { juce::String::fromUTF8 ("row")                                                                                       };
inline const juce::Identifier rulePadding                     { juce::String::fromUTF8 ("rulePadding")                                                                               };
inline const juce::Identifier ruleThickness                   { juce::String::fromUTF8 ("ruleThickness")                                                                             };
inline const juce::Identifier runs                            { juce::String::fromUTF8 ("runs")                                                                                      };
inline const juce::Identifier rx                              { juce::String::fromUTF8 ("rx")                                                                                        };
inline const juce::Identifier ry                              { juce::String::fromUTF8 ("ry")                                                                                        };
inline const juce::Identifier sampleRate                      { juce::String::fromUTF8 ("samplerate")                                                                                };
inline const juce::Identifier sankeyBeta                      { juce::String::fromUTF8 ("sankey-beta")                                                                               };///< Mermaid sankey-beta diagram keyword.
inline const juce::Identifier satisfies                       { juce::String::fromUTF8 ("satisfies")                                                                                 };
inline const juce::Identifier save                            { juce::String::fromUTF8 ("Save Preset")                                                                               };///< Save-preset button label.
inline const juce::Identifier saveAs                          { juce::String::fromUTF8 ("saveAs")                                                                                    };
inline const juce::Identifier saveCursor                      { juce::String::fromUTF8 ("saveCursor")                                                                                };
inline const juce::Identifier savedKeyboardFlags              { juce::String::fromUTF8 ("savedKeyboardFlags")                                                                        };
inline const juce::Identifier savedKeyboardFlagsCount         { juce::String::fromUTF8 ("savedKeyboardFlagsCount")                                                                   };
inline const juce::Identifier scale                           { juce::String::fromUTF8 ("scale")                                                                                     };
inline const juce::Identifier scaleX                          { juce::String::fromUTF8 ("scale_x")                                                                                   };///< Shader scale-x uniform.
inline const juce::Identifier scaleY                          { juce::String::fromUTF8 ("scale_y")                                                                                   };///< Shader scale-y uniform.
inline const juce::Identifier scaleType                       { juce::String::fromUTF8 ("scale_type")                                                                                };///< Shader scale-type uniform.
inline const juce::Identifier scaleTypeX                      { juce::String::fromUTF8 ("scale_type_x")                                                                              };///< Shader scale-type-x uniform.
inline const juce::Identifier scaleTypeY                      { juce::String::fromUTF8 ("scale_type_y")                                                                              };///< Shader scale-type-y uniform.
inline const juce::Identifier sceneMacro                      { juce::String::fromUTF8 ("sceneMacro")                                                                                };
inline const juce::Identifier score                           { juce::String::fromUTF8 ("score")                                                                                     };
inline const juce::Identifier scrambledText                   { juce::String::fromUTF8 ("scrambledText")                                                                             };
inline const juce::Identifier screenAlignmentTest             { juce::String::fromUTF8 ("screenAlignmentTest")                                                                       };
inline const juce::Identifier screenDirty                     { juce::String::fromUTF8 ("screenDirty")                                                                               };
inline const juce::Identifier script                          { juce::String::fromUTF8 ("script")                                                                                    };
inline const juce::Identifier scriptCloseTag                  { juce::String::fromUTF8 ("</script>")                                                                                 };///< HTML `</script>` close tag.
inline const juce::Identifier scrollDown                      { juce::String::fromUTF8 ("scrollDown")                                                                                };
inline const juce::Identifier scrollUp                        { juce::String::fromUTF8 ("scrollUp")                                                                                  };
inline const juce::Identifier search                          { juce::String::fromUTF8 ("search")                                                                                    };
inline const juce::Identifier section                         { juce::String::fromUTF8 ("section")                                                                                   };
inline const juce::Identifier select                          { juce::String::fromUTF8 ("select")                                                                                    };
inline const juce::Identifier selectGraphicRendition          { juce::String::fromUTF8 ("selectGraphicRendition")                                                                    };
inline const juce::Identifier selector                        { juce::String::fromUTF8 ("selector")                                                                                  };
inline const juce::Identifier selfCloseTag                    { juce::String::fromUTF8 ("/>")                                                                                        };///< HTML `/>` self-close tag.
inline const juce::Identifier selfClosing                     { juce::String::fromUTF8 ("selfClosing")                                                                               };
inline const juce::Identifier sequenceActivationWidth         { juce::String::fromUTF8 ("sequenceActivationWidth")                                                                   };
inline const juce::Identifier sequenceActorHeight             { juce::String::fromUTF8 ("sequenceActorHeight")                                                                       };
inline const juce::Identifier sequenceActorMargin             { juce::String::fromUTF8 ("sequenceActorMargin")                                                                       };
inline const juce::Identifier sequenceActorWidth              { juce::String::fromUTF8 ("sequenceActorWidth")                                                                        };
inline const juce::Identifier sequenceBoxMargin               { juce::String::fromUTF8 ("sequenceBoxMargin")                                                                         };
inline const juce::Identifier sequenceDiagram                 { juce::String::fromUTF8 ("sequenceDiagram")                                                                           };
inline const juce::Identifier sequenceLifelineStrokeWidth     { juce::String::fromUTF8 ("sequenceLifelineStrokeWidth")                                                               };
inline const juce::Identifier sequenceMessageDashLength       { juce::String::fromUTF8 ("sequenceMessageDashLength")                                                                 };
inline const juce::Identifier sequenceMessageMargin           { juce::String::fromUTF8 ("sequenceMessageMargin")                                                                     };
inline const juce::Identifier sequenceMessageStrokeWidth      { juce::String::fromUTF8 ("sequenceMessageStrokeWidth")                                                                };
inline const juce::Identifier sequenceNoteMargin              { juce::String::fromUTF8 ("sequenceNoteMargin")                                                                        };
inline const juce::Identifier serifId                         { juce::String::fromUTF8 ("serif:id")                                                                                  };///< SVG `serif:id` attribute.
inline const juce::Identifier service                         { juce::String::fromUTF8 ("service")                                                                                   };
inline const juce::Identifier set                             { juce::String::fromUTF8 ("set")                                                                                       };
inline const juce::Identifier setClipboard                    { juce::String::fromUTF8 ("setClipboard")                                                                              };
inline const juce::Identifier setCursorColor                  { juce::String::fromUTF8 ("setCursorColor")                                                                            };
inline const juce::Identifier setCursorStyle                  { juce::String::fromUTF8 ("setCursorStyle")                                                                            };
inline const juce::Identifier setCwd                          { juce::String::fromUTF8 ("setCwd")                                                                                    };
inline const juce::Identifier setDefaultBackground            { juce::String::fromUTF8 ("setDefaultBackground")                                                                      };
inline const juce::Identifier setDefaultForeground            { juce::String::fromUTF8 ("setDefaultForeground")                                                                      };
inline const juce::Identifier setMode                         { juce::String::fromUTF8 ("setMode")                                                                                   };
inline const juce::Identifier setPaletteEntry                 { juce::String::fromUTF8 ("setPaletteEntry")                                                                           };
inline const juce::Identifier setScrollingRegion              { juce::String::fromUTF8 ("setScrollingRegion")                                                                        };
inline const juce::Identifier setTitleOnly                    { juce::String::fromUTF8 ("setTitleOnly")                                                                              };
inline const juce::Identifier setWindowTitle                  { juce::String::fromUTF8 ("setWindowTitle")                                                                            };
inline const juce::Identifier settings                        { juce::String::fromUTF8 ("settings")                                                                                  };
inline const juce::Identifier sevenSegment                    { juce::String::fromUTF8 ("sevenSegment")                                                                              };
inline const juce::Identifier shader                          { juce::String::fromUTF8 ("shader")                                                                                    };
inline const juce::Identifier shaders                         { juce::String::fromUTF8 ("shaders")                                                                                   };
inline const juce::Identifier shape                           { juce::String::fromUTF8 ("shape")                                                                                     };///< Structure-line ordinal stamp.
inline const juce::Identifier shellExited                     { juce::String::fromUTF8 ("shellExited")                                                                               };
inline const juce::Identifier shellIntegration                { juce::String::fromUTF8 ("shellIntegration")                                                                          };
inline const juce::Identifier showData                        { juce::String::fromUTF8 ("showData")                                                                                  };
inline const juce::Identifier sideB                           { juce::String::fromUTF8 ("B")                                                                                         };///< Architecture bottom side label.
inline const juce::Identifier sideL                           { juce::String::fromUTF8 ("L")                                                                                         };///< Architecture left side label.
inline const juce::Identifier sideR                           { juce::String::fromUTF8 ("R")                                                                                         };///< Architecture right side label.
inline const juce::Identifier sideT                           { juce::String::fromUTF8 ("T")                                                                                         };///< Architecture top side label.
inline const juce::Identifier single                          { juce::String::fromUTF8 ("single")                                                                                    };
inline const juce::Identifier size                            { juce::String::fromUTF8 ("size")                                                                                      };
inline const juce::Identifier skew                            { juce::String::fromUTF8 ("skew")                                                                                      };
inline const juce::Identifier slangp                          { juce::String::fromUTF8 ("slangp")                                                                                    };
inline const juce::Identifier sliderIncDec                    { juce::String::fromUTF8 ("sliderIncDec")                                                                              };
inline const juce::Identifier solid                           { juce::String::fromUTF8 ("solid")                                                                                     };
inline const juce::Identifier source                          { juce::String::fromUTF8 ("source")                                                                                    };
inline const juce::Identifier spaceAround                     { juce::String::fromUTF8 ("space-around")                                                                              };///< CSS `space-around` value.
inline const juce::Identifier spaceBetween                    { juce::String::fromUTF8 ("space-between")                                                                             };///< CSS `space-between` value.
inline const juce::Identifier special                         { juce::String::fromUTF8 ("special")                                                                                   };
inline const juce::Identifier square                          { juce::String::fromUTF8 ("square")                                                                                    };
inline const juce::Identifier src                             { juce::String::fromUTF8 ("src")                                                                                       };
inline const juce::Identifier srgbFramebuffer                 { juce::String::fromUTF8 ("srgb_framebuffer")                                                                          };///< Vulkan sRGB-framebuffer format tag.
inline const juce::Identifier stadium                         { juce::String::fromUTF8 ("stadium")                                                                                   };
inline const juce::Identifier stage                           { juce::String::fromUTF8 ("stage")                                                                                     };
inline const juce::Identifier standalone                      { juce::String::fromUTF8 ("Standalone")                                                                                };///< Standalone plugin wrapper label.
inline const juce::Identifier start                           { juce::String::fromUTF8 ("start")                                                                                     };
inline const juce::Identifier startDecoration                 { juce::String::fromUTF8 ("startDecoration")                                                                           };
inline const juce::Identifier startLabel                      { juce::String::fromUTF8 ("startLabel")                                                                                };
inline const juce::Identifier startTag                        { juce::String::fromUTF8 ("startTag")                                                                                  };
inline const juce::Identifier stateClusterStrokeWidth         { juce::String::fromUTF8 ("stateClusterStrokeWidth")                                                                   };
inline const juce::Identifier stateCornerRadius               { juce::String::fromUTF8 ("stateCornerRadius")                                                                         };
inline const juce::Identifier stateDescription                { juce::String::fromUTF8 ("stateDescription")                                                                          };
inline const juce::Identifier stateDiagram                    { juce::String::fromUTF8 ("stateDiagram")                                                                              };
inline const juce::Identifier stateDiagramV2                  { juce::String::fromUTF8 ("stateDiagram-v2")                                                                           };///< Mermaid stateDiagram-v2 keyword.
inline const juce::Identifier stateEdgeStrokeWidth            { juce::String::fromUTF8 ("stateEdgeStrokeWidth")                                                                      };
inline const juce::Identifier stateEnd                        { juce::String::fromUTF8 ("stateEnd")                                                                                  };
inline const juce::Identifier stateFontSize                   { juce::String::fromUTF8 ("stateFontSize")                                                                             };
inline const juce::Identifier stateNodeStrokeWidth            { juce::String::fromUTF8 ("stateNodeStrokeWidth")                                                                      };
inline const juce::Identifier stateStart                      { juce::String::fromUTF8 ("stateStart")                                                                                };
inline const juce::Identifier stateTerminalInnerSize          { juce::String::fromUTF8 ("stateTerminalInnerSize")                                                                    };
inline const juce::Identifier stateTerminalSize               { juce::String::fromUTF8 ("stateTerminalSize")                                                                         };
inline const juce::Identifier stateTerminalStrokeWidth        { juce::String::fromUTF8 ("stateTerminalStrokeWidth")                                                                  };
inline const juce::Identifier status                          { juce::String::fromUTF8 ("status")                                                                                    };
inline const juce::Identifier steadyBar                       { juce::String::fromUTF8 ("steadyBar")                                                                                 };
inline const juce::Identifier steadyBlock                     { juce::String::fromUTF8 ("steadyBlock")                                                                               };
inline const juce::Identifier steadyUnderline                 { juce::String::fromUTF8 ("steadyUnderline")                                                                           };
inline const juce::Identifier stop                            { juce::String::fromUTF8 ("stop")                                                                                      };
inline const juce::Identifier stopColor                       { juce::String::fromUTF8 ("stop-color")                                                                                };///< SVG `stop-color` attribute.
inline const juce::Identifier stopOpacity                     { juce::String::fromUTF8 ("stop-opacity")                                                                              };///< SVG `stop-opacity` attribute.
inline const juce::Identifier strike                          { juce::String::fromUTF8 ("strike")                                                                                    };
inline const juce::Identifier string                          { juce::String::fromUTF8 ("string")                                                                                    };
inline const juce::Identifier stroke                          { juce::String::fromUTF8 ("stroke")                                                                                    };
inline const juce::Identifier strokeLinecap                   { juce::String::fromUTF8 ("stroke-linecap")                                                                            };///< SVG `stroke-linecap` attribute.
inline const juce::Identifier strokeLinejoin                  { juce::String::fromUTF8 ("stroke-linejoin")                                                                           };///< SVG `stroke-linejoin` attribute.
inline const juce::Identifier strokeWidth                     { juce::String::fromUTF8 ("stroke-width")                                                                              };///< SVG `stroke-width` attribute.
inline const juce::Identifier strong                          { juce::String::fromUTF8 ("strong")                                                                                    };
inline const juce::Identifier style                           { juce::String::fromUTF8 ("style")                                                                                     };
inline const juce::Identifier styleCloseTag                   { juce::String::fromUTF8 ("</style>")                                                                                  };///< HTML `</style>` close tag.
inline const juce::Identifier styleRule                       { juce::String::fromUTF8 ("styleRule")                                                                                 };
inline const juce::Identifier subgraph                        { juce::String::fromUTF8 ("subgraph")                                                                                  };
inline const juce::Identifier subroutine                      { juce::String::fromUTF8 ("subroutine")                                                                                };
inline const juce::Identifier subscript                       { juce::String::fromUTF8 ("subscript")                                                                                 };
inline const juce::Identifier summary                         { juce::String::fromUTF8 ("summary")                                                                                   };
inline const juce::Identifier super                           { juce::String::fromUTF8 ("super")                                                                                     };
inline const juce::Identifier superscript                     { juce::String::fromUTF8 ("superscript")                                                                               };
inline const juce::Identifier support                         { juce::String::fromUTF8 ("support")                                                                                   };
inline const juce::Identifier svg                             { juce::String::fromUTF8 ("svg")                                                                                       };
inline const juce::Identifier syncOutput                      { juce::String::fromUTF8 ("syncOutput")                                                                                };
inline const juce::Identifier syncOutputActive                { juce::String::fromUTF8 ("syncOutputActive")                                                                          };
inline const juce::Identifier system                          { juce::String::fromUTF8 ("System")                                                                                    };///< C4 software-system element keyword.
inline const juce::Identifier systemAccent                    { juce::String::fromUTF8 ("system-accent")                                                                             };///< CSS `system-accent` colour.
inline const juce::Identifier systemBoundary                  { juce::String::fromUTF8 ("System_Boundary")                                                                           };///< C4 system boundary keyword.
inline const juce::Identifier systemDb                        { juce::String::fromUTF8 ("SystemDb")                                                                                  };///< C4 system-database element keyword.
inline const juce::Identifier systemDbExt                     { juce::String::fromUTF8 ("SystemDb_Ext")                                                                              };///< C4 external system-database keyword.
inline const juce::Identifier systemExt                       { juce::String::fromUTF8 ("System_Ext")                                                                                };///< C4 external system keyword.
inline const juce::Identifier systemHighlightedText           { juce::String::fromUTF8 ("system-highlighted-text")                                                                   };///< CSS `system-highlighted-text` colour.
inline const juce::Identifier systemQueue                     { juce::String::fromUTF8 ("SystemQueue")                                                                               };///< C4 system-queue element keyword.
inline const juce::Identifier systemQueueExt                  { juce::String::fromUTF8 ("SystemQueue_Ext")                                                                           };///< C4 external system-queue keyword.
inline const juce::Identifier systemSeparator                 { juce::String::fromUTF8 ("system-separator")                                                                          };///< CSS `system-separator` colour.
inline const juce::Identifier systemText                      { juce::String::fromUTF8 ("system-text")                                                                               };///< CSS `system-text` colour.
inline const juce::Identifier systemWindow                    { juce::String::fromUTF8 ("system-window")                                                                             };///< CSS `system-window` colour.
inline const juce::Identifier table                           { juce::String::fromUTF8 ("table")                                                                                     };
inline const juce::Identifier tableCell                       { juce::String::fromUTF8 ("tableCell")                                                                                 };
inline const juce::Identifier tableCellPadding                { juce::String::fromUTF8 ("tableCellPadding")                                                                          };
inline const juce::Identifier tableLineThickness              { juce::String::fromUTF8 ("tableLineThickness")                                                                        };
inline const juce::Identifier tableRow                        { juce::String::fromUTF8 ("tableRow")                                                                                  };
inline const juce::Identifier tabulationClear                 { juce::String::fromUTF8 ("tabulationClear")                                                                           };
inline const juce::Identifier tag                             { juce::String::fromUTF8 ("tag")                                                                                       };
inline const juce::Identifier tail                            { juce::String::fromUTF8 ("tail")                                                                                      };
inline const juce::Identifier taper                           { juce::String::fromUTF8 ("taper")                                                                                     };
inline const juce::Identifier task                            { juce::String::fromUTF8 ("task")                                                                                      };
inline const juce::Identifier tb                              { juce::String::fromUTF8 ("tb")                                                                                        };
inline const juce::Identifier tbody                           { juce::String::fromUTF8 ("tbody")                                                                                     };
inline const juce::Identifier td                              { juce::String::fromUTF8 ("td")                                                                                        };
inline const juce::Identifier techn                           { juce::String::fromUTF8 ("techn")                                                                                     };
inline const juce::Identifier text                            { juce::String::fromUTF8 ("text")                                                                                      };
inline const juce::Identifier textAlign                       { juce::String::fromUTF8 ("text-align")                                                                                };///< CSS `text-align` property.
inline const juce::Identifier textLabel                       { juce::String::fromUTF8 ("Text")                                                                                      };///< Requirement Text attribute label.
inline const juce::Identifier textProcessor                   { juce::String::fromUTF8 ("textProcessor")                                                                             };
inline const juce::Identifier textTransform                   { juce::String::fromUTF8 ("text-transform")                                                                            };///< CSS `text-transform` property.
inline const juce::Identifier textarea                        { juce::String::fromUTF8 ("textarea")                                                                                  };
inline const juce::Identifier textareaCloseTag                { juce::String::fromUTF8 ("</textarea>")                                                                               };///< HTML `</textarea>` close tag.
inline const juce::Identifier textures                        { juce::String::fromUTF8 ("textures")                                                                                  };
inline const juce::Identifier texturesLinear                  { juce::String::fromUTF8 ("texturesLinear")                                                                            };
inline const juce::Identifier texturesNearest                 { juce::String::fromUTF8 ("texturesNearest")                                                                           };
inline const juce::Identifier tfoot                           { juce::String::fromUTF8 ("tfoot")                                                                                     };
inline const juce::Identifier th                              { juce::String::fromUTF8 ("th")                                                                                        };
inline const juce::Identifier thead                           { juce::String::fromUTF8 ("thead")                                                                                     };
inline const juce::Identifier thematicBreak                   { juce::String::fromUTF8 ("thematicBreak")                                                                             };
inline const juce::Identifier thick                           { juce::String::fromUTF8 ("thick")                                                                                     };
inline const juce::Identifier ticket                          { juce::String::fromUTF8 ("ticket")                                                                                    };
inline const juce::Identifier tight                           { juce::String::fromUTF8 ("tight")                                                                                     };
inline const juce::Identifier time                            { juce::String::fromUTF8 ("time")                                                                                      };
inline const juce::Identifier timeline                        { juce::String::fromUTF8 ("timeline")                                                                                  };
inline const juce::Identifier title                           { juce::String::fromUTF8 ("title")                                                                                     };
inline const juce::Identifier to                              { juce::String::fromUTF8 ("to")                                                                                        };
inline const juce::Identifier toComment                       { juce::String::fromUTF8 ("toComment")                                                                                 };
inline const juce::Identifier toCommentBlock                  { juce::String::fromUTF8 ("toCommentBlock")                                                                            };
inline const juce::Identifier toSide                          { juce::String::fromUTF8 ("toSide")                                                                                    };
inline const juce::Identifier toStringSuffix                  { juce::String::fromUTF8 (".toString()")                                                                               };///< CAST `.toString()` suffix.
inline const juce::Identifier toUnicode                       { juce::String::fromUTF8 ("to unicode")                                                                                };///< To-unicode label.
inline const juce::Identifier todayMarker                     { juce::String::fromUTF8 ("todayMarker")                                                                               };
inline const juce::Identifier toggle                          { juce::String::fromUTF8 ("toggle")                                                                                    };
inline const juce::Identifier toggleMod                       { juce::String::fromUTF8 ("toggleMod")                                                                                 };
inline const juce::Identifier toggleOverlay                   { juce::String::fromUTF8 ("toggleOverlay")                                                                             };
inline const juce::Identifier toggleValue                     { juce::String::fromUTF8 ("toggleValue")                                                                               };
inline const juce::Identifier tokenAlignas                    { juce::String::fromUTF8 ("alignas")                                                                                   };
inline const juce::Identifier tokenAlignof                    { juce::String::fromUTF8 ("alignof")                                                                                   };
inline const juce::Identifier tokenAnd                        { juce::String::fromUTF8 ("and")                                                                                       };
inline const juce::Identifier tokenAndEq                      { juce::String::fromUTF8 ("and_eq")                                                                                    };
inline const juce::Identifier tokenAsm                        { juce::String::fromUTF8 ("asm")                                                                                       };
inline const juce::Identifier tokenAssert                     { juce::String::fromUTF8 ("assert")                                                                                    };
inline const juce::Identifier tokenAuto                       { juce::String::fromUTF8 ("auto")                                                                                      };
inline const juce::Identifier tokenBitand                     { juce::String::fromUTF8 ("bitand")                                                                                    };
inline const juce::Identifier tokenBitor                      { juce::String::fromUTF8 ("bitor")                                                                                     };
inline const juce::Identifier tokenBreak                      { juce::String::fromUTF8 ("break")                                                                                     };
inline const juce::Identifier tokenCase                       { juce::String::fromUTF8 ("case")                                                                                      };
inline const juce::Identifier tokenCatch                      { juce::String::fromUTF8 ("catch")                                                                                     };
inline const juce::Identifier tokenChar                       { juce::String::fromUTF8 ("char")                                                                                      };
inline const juce::Identifier tokenChar16T                    { juce::String::fromUTF8 ("char16_t")                                                                                  };
inline const juce::Identifier tokenChar32T                    { juce::String::fromUTF8 ("char32_t")                                                                                  };
inline const juce::Identifier tokenChar8T                     { juce::String::fromUTF8 ("char8_t")                                                                                   };
inline const juce::Identifier tokenClass                      { juce::String::fromUTF8 ("class")                                                                                     };
inline const juce::Identifier tokenCoAwait                    { juce::String::fromUTF8 ("co_await")                                                                                  };
inline const juce::Identifier tokenCoReturn                   { juce::String::fromUTF8 ("co_return")                                                                                 };
inline const juce::Identifier tokenCoYield                    { juce::String::fromUTF8 ("co_yield")                                                                                  };
inline const juce::Identifier tokenCompl                      { juce::String::fromUTF8 ("compl")                                                                                     };
inline const juce::Identifier tokenConcept                    { juce::String::fromUTF8 ("concept")                                                                                   };
inline const juce::Identifier tokenConst                      { juce::String::fromUTF8 ("const")                                                                                     };
inline const juce::Identifier tokenConstCast                  { juce::String::fromUTF8 ("const_cast")                                                                                };
inline const juce::Identifier tokenConsteval                  { juce::String::fromUTF8 ("consteval")                                                                                 };
inline const juce::Identifier tokenConstexpr                  { juce::String::fromUTF8 ("constexpr")                                                                                 };
inline const juce::Identifier tokenConstinit                  { juce::String::fromUTF8 ("constinit")                                                                                 };
inline const juce::Identifier tokenContinue                   { juce::String::fromUTF8 ("continue")                                                                                  };
inline const juce::Identifier tokenDecltype                   { juce::String::fromUTF8 ("decltype")                                                                                  };
inline const juce::Identifier tokenDefault                    { juce::String::fromUTF8 ("default")                                                                                   };
inline const juce::Identifier tokenDelete                     { juce::String::fromUTF8 ("delete")                                                                                    };
inline const juce::Identifier tokenDo                         { juce::String::fromUTF8 ("do")                                                                                        };
inline const juce::Identifier tokenDouble                     { juce::String::fromUTF8 ("double")                                                                                    };
inline const juce::Identifier tokenDynamicCast                { juce::String::fromUTF8 ("dynamic_cast")                                                                              };
inline const juce::Identifier tokenElse                       { juce::String::fromUTF8 ("else")                                                                                      };
inline const juce::Identifier tokenEnum                       { juce::String::fromUTF8 ("enum")                                                                                      };
inline const juce::Identifier tokenExplicit                   { juce::String::fromUTF8 ("explicit")                                                                                  };
inline const juce::Identifier tokenExport                     { juce::String::fromUTF8 ("export")                                                                                    };
inline const juce::Identifier tokenExtern                     { juce::String::fromUTF8 ("extern")                                                                                    };
inline const juce::Identifier tokenFalse                      { juce::String::fromUTF8 ("false")                                                                                     };
inline const juce::Identifier tokenFinal                      { juce::String::fromUTF8 ("final")                                                                                     };
inline const juce::Identifier tokenFor                        { juce::String::fromUTF8 ("for")                                                                                       };
inline const juce::Identifier tokenFriend                     { juce::String::fromUTF8 ("friend")                                                                                    };
inline const juce::Identifier tokenGoto                       { juce::String::fromUTF8 ("goto")                                                                                      };
inline const juce::Identifier tokenIf                         { juce::String::fromUTF8 ("if")                                                                                        };
inline const juce::Identifier tokenInline                     { juce::String::fromUTF8 ("inline")                                                                                    };
inline const juce::Identifier tokenInterface                  { juce::String::fromUTF8 ("interface")                                                                                 };
inline const juce::Identifier tokenLong                       { juce::String::fromUTF8 ("long")                                                                                      };
inline const juce::Identifier tokenModule                     { juce::String::fromUTF8 ("module")                                                                                    };
inline const juce::Identifier tokenMutable                    { juce::String::fromUTF8 ("mutable")                                                                                   };
inline const juce::Identifier tokenNamespace                  { juce::String::fromUTF8 ("namespace")                                                                                 };
inline const juce::Identifier tokenNew                        { juce::String::fromUTF8 ("new")                                                                                       };
inline const juce::Identifier tokenNoexcept                   { juce::String::fromUTF8 ("noexcept")                                                                                  };
inline const juce::Identifier tokenNot                        { juce::String::fromUTF8 ("not")                                                                                       };
inline const juce::Identifier tokenNotEq                      { juce::String::fromUTF8 ("not_eq")                                                                                    };
inline const juce::Identifier tokenNullptr                    { juce::String::fromUTF8 ("nullptr")                                                                                   };
inline const juce::Identifier tokenOperator                   { juce::String::fromUTF8 ("operator")                                                                                  };
inline const juce::Identifier tokenOr                         { juce::String::fromUTF8 ("or")                                                                                        };
inline const juce::Identifier tokenOrEq                       { juce::String::fromUTF8 ("or_eq")                                                                                     };
inline const juce::Identifier tokenOverride                   { juce::String::fromUTF8 ("override")                                                                                  };
inline const juce::Identifier tokenPrivate                    { juce::String::fromUTF8 ("private")                                                                                   };
inline const juce::Identifier tokenProtected                  { juce::String::fromUTF8 ("protected")                                                                                 };
inline const juce::Identifier tokenPublic                     { juce::String::fromUTF8 ("public")                                                                                    };
inline const juce::Identifier tokenRegister                   { juce::String::fromUTF8 ("register")                                                                                  };
inline const juce::Identifier tokenReinterpretCast            { juce::String::fromUTF8 ("reinterpret_cast")                                                                          };
inline const juce::Identifier tokenRequires                   { juce::String::fromUTF8 ("requires")                                                                                  };
inline const juce::Identifier tokenReturn                     { juce::String::fromUTF8 ("return")                                                                                    };
inline const juce::Identifier tokenShort                      { juce::String::fromUTF8 ("short")                                                                                     };
inline const juce::Identifier tokenSigned                     { juce::String::fromUTF8 ("signed")                                                                                    };
inline const juce::Identifier tokenSizeof                     { juce::String::fromUTF8 ("sizeof")                                                                                    };
inline const juce::Identifier tokenSmall                      { juce::String::fromUTF8 ("small")                                                                                     };
inline const juce::Identifier tokenStatic                     { juce::String::fromUTF8 ("static")                                                                                    };
inline const juce::Identifier tokenStaticAssert               { juce::String::fromUTF8 ("static_assert")                                                                             };
inline const juce::Identifier tokenStaticCast                 { juce::String::fromUTF8 ("static_cast")                                                                               };
inline const juce::Identifier tokenStruct                     { juce::String::fromUTF8 ("struct")                                                                                    };
inline const juce::Identifier tokenSwitch                     { juce::String::fromUTF8 ("switch")                                                                                    };
inline const juce::Identifier tokenTemplate                   { juce::String::fromUTF8 ("template")                                                                                  };
inline const juce::Identifier tokenThis                       { juce::String::fromUTF8 ("this")                                                                                      };
inline const juce::Identifier tokenThreadLocal                { juce::String::fromUTF8 ("thread_local")                                                                              };
inline const juce::Identifier tokenThrow                      { juce::String::fromUTF8 ("throw")                                                                                     };
inline const juce::Identifier tokenTrue                       { juce::String::fromUTF8 ("true")                                                                                      };
inline const juce::Identifier tokenTry                        { juce::String::fromUTF8 ("try")                                                                                       };
inline const juce::Identifier tokenTypedef                    { juce::String::fromUTF8 ("typedef")                                                                                   };
inline const juce::Identifier tokenTypeid                     { juce::String::fromUTF8 ("typeid")                                                                                    };
inline const juce::Identifier tokenTypename                   { juce::String::fromUTF8 ("typename")                                                                                  };
inline const juce::Identifier tokenUnion                      { juce::String::fromUTF8 ("union")                                                                                     };
inline const juce::Identifier tokenUnsigned                   { juce::String::fromUTF8 ("unsigned")                                                                                  };
inline const juce::Identifier tokenUsing                      { juce::String::fromUTF8 ("using")                                                                                     };
inline const juce::Identifier tokenVirtual                    { juce::String::fromUTF8 ("virtual")                                                                                   };
inline const juce::Identifier tokenVoid                       { juce::String::fromUTF8 ("void")                                                                                      };
inline const juce::Identifier tokenVolatile                   { juce::String::fromUTF8 ("volatile")                                                                                  };
inline const juce::Identifier tokenWcharT                     { juce::String::fromUTF8 ("wchar_t")                                                                                   };
inline const juce::Identifier tokenWhile                      { juce::String::fromUTF8 ("while")                                                                                     };
inline const juce::Identifier tokenXor                        { juce::String::fromUTF8 ("xor")                                                                                       };
inline const juce::Identifier tokenXorEq                      { juce::String::fromUTF8 ("xor_eq")                                                                                    };
inline const juce::Identifier tokens                          { juce::String::fromUTF8 ("tokens")                                                                                    };
inline const juce::Identifier top                             { juce::String::fromUTF8 ("top")                                                                                       };
inline const juce::Identifier topLeft                         { juce::String::fromUTF8 ("top-left")                                                                                  };///< CSS flex `top-left` anchor.
inline const juce::Identifier topRight                        { juce::String::fromUTF8 ("top-right")                                                                                 };///< CSS flex `top-right` anchor.
inline const juce::Identifier tr                              { juce::String::fromUTF8 ("tr")                                                                                        };
inline const juce::Identifier traces                          { juce::String::fromUTF8 ("traces")                                                                                    };
inline const juce::Identifier track                           { juce::String::fromUTF8 ("track")                                                                                     };
inline const juce::Identifier trademark                       { juce::String::fromUTF8 ("trademark")                                                                                 };
inline const juce::String     trademarkText                   { juce::String::fromUTF8 ("@companyName@ & @productName@ are trademarks of @legalCompanyName@.\nAll rights reserved.") };
inline const juce::Identifier transparent                     { juce::String::fromUTF8 ("transparent")                                                                               };
inline const juce::Identifier trapB                           { juce::String::fromUTF8 ("trapB")                                                                                     };
inline const juce::Identifier trapT                           { juce::String::fromUTF8 ("trapT")                                                                                     };
inline const juce::Identifier trapezoid                       { juce::String::fromUTF8 ("trap-b")                                                                                    };///< Mermaid trap-bottom trapezoid spelling.
inline const juce::Identifier trapezoidAlt                    { juce::String::fromUTF8 ("trap-t")                                                                                    };///< Mermaid trap-top trapezoid spelling.
inline const juce::String     tripleColon                     { juce::String::fromUTF8 (":::")                                                                                       };///< CAST `:::` delimiter.
inline const juce::Identifier type                            { juce::String::fromUTF8 ("type")                                                                                      };
inline const juce::Identifier typeLabel                       { juce::String::fromUTF8 ("Type")                                                                                      };///< Requirement Type attribute label.
inline const juce::Identifier typeof                          { juce::String::fromUTF8 ("typeof")                                                                                    };
inline const juce::Identifier ul                              { juce::String::fromUTF8 ("ul")                                                                                        };
inline const juce::Identifier UIScale                         { juce::String::fromUTF8 ("UI_scale")                                                                                  };
inline const juce::Identifier UISize                          { juce::String::fromUTF8 ("UI_size")                                                                                   };
inline const juce::Identifier undefined                       { juce::String::fromUTF8 ("Undefined")                                                                                 };///< Plugin wrapper undefined label.
inline const juce::Identifier underline                       { juce::String::fromUTF8 ("underline")                                                                                 };
inline const juce::String     unique                          { juce::String::fromUTF8 ("unique")                                                                                    };
inline const juce::Identifier unit                            { juce::String::fromUTF8 ("unit")                                                                                      };
inline const juce::Identifier unity                           { juce::String::fromUTF8 ("unity")                                                                                     };
inline const juce::Identifier unknownRule                     { juce::String::fromUTF8 ("unknownRule")                                                                               };
inline const juce::Identifier until                           { juce::String::fromUTF8 ("until")                                                                                     };
inline const juce::Identifier up                              { juce::String::fromUTF8 ("up")                                                                                        };
inline const juce::Identifier uppercase                       { juce::String::fromUTF8 ("uppercase")                                                                                 };
inline const juce::Identifier url                             { juce::String::fromUTF8 ("url")                                                                                       };
inline const juce::Identifier urlOpen                         { juce::String::fromUTF8 ("url(")                                                                                      };///< CSS `url(` function open.
inline const juce::Identifier user                            { juce::String::fromUTF8 ("user")                                                                                      };
inline const juce::Identifier userManual                      { juce::String::fromUTF8 ("userManual")                                                                                };
inline const juce::Identifier userManuals                     { juce::String::fromUTF8 ("User Manuals")                                                                              };///< User manuals label.
inline const juce::Identifier userPresets                     { juce::String::fromUTF8 ("User Presets")                                                                              };///< User presets folder label.
inline const juce::Identifier userSpaceOnUse                  { juce::String::fromUTF8 ("userSpaceOnUse")                                                                            };
inline const juce::Identifier utf8                            { juce::String::fromUTF8 ("utf8")                                                                                      };
inline const juce::Identifier value                           { juce::String::fromUTF8 ("value")                                                                                     };
inline const juce::Identifier values                          { juce::String::fromUTF8 ("values")                                                                                    };
inline const juce::Identifier var                             { juce::String::fromUTF8 ("var")                                                                                       };
inline const juce::Identifier variComponent                   { juce::String::fromUTF8 ("variComponent")                                                                             };
inline const juce::Identifier variDisplay                     { juce::String::fromUTF8 ("variDisplay")                                                                               };
inline const juce::Identifier variDisplayOverlay              { juce::String::fromUTF8 ("variDisplayOverlay")                                                                        };
inline const juce::Identifier verificationLabel               { juce::String::fromUTF8 ("Verification")                                                                              };///< Requirement Verification attribute label.
inline const juce::Identifier verifies                        { juce::String::fromUTF8 ("verifies")                                                                                  };
inline const juce::Identifier verifyMethod                    { juce::String::fromUTF8 ("verifymethod")                                                                              };///< Requirement verifymethod attribute keyword.
inline const juce::Identifier version                         { juce::String::fromUTF8 ("version")                                                                                   };
inline const juce::Identifier versionHint                     { juce::String::fromUTF8 ("versionHint")                                                                               };
inline const juce::Identifier versionPrefix                   { juce::String::fromUTF8 ("Ver.")                                                                                      };///< Version `Ver.` prefix.
inline const juce::Identifier versionString                   { juce::String::fromUTF8 ("versionString")                                                                             };
inline const juce::Identifier vertex                          { juce::String::fromUTF8 ("vertex")                                                                                    };
inline const juce::Identifier vertical                        { juce::String::fromUTF8 ("vertical")                                                                                  };
inline const juce::Identifier verticalPositionAbsolute        { juce::String::fromUTF8 ("verticalPositionAbsolute")                                                                  };
inline const juce::Identifier verticalPositionRelative        { juce::String::fromUTF8 ("verticalPositionRelative")                                                                  };
inline const juce::Identifier veryHigh                        { juce::String::fromUTF8 ("Very High")                                                                                 };///< Kanban very-high priority label.
inline const juce::Identifier veryLow                         { juce::String::fromUTF8 ("Very Low")                                                                                  };///< Kanban very-low priority label.
inline const juce::Identifier vst                             { juce::String::fromUTF8 ("vst")                                                                                       };
inline const juce::Identifier vst3                            { juce::String::fromUTF8 ("vst3")                                                                                      };
inline const juce::Identifier view                            { juce::String::fromUTF8 ("view")                                                                                      };
inline const juce::Identifier viewBox                         { juce::String::fromUTF8 ("viewBox")                                                                                   };
inline const juce::Identifier viewEditor                      { juce::String::fromUTF8 ("viewEditor")                                                                                };
inline const juce::Identifier viewport                        { juce::String::fromUTF8 ("viewport")                                                                                  };
inline const juce::Identifier visitJReng                      { juce::String::fromUTF8 ("visitJReng")                                                                                };
inline const juce::Identifier visualFxWindowBackground        { juce::String::fromUTF8 ("visualFXWindowBackground")                                                                  };///< macOS visual-effect window background.
inline const juce::Identifier wbr                             { juce::String::fromUTF8 ("wbr")                                                                                       };
inline const juce::Identifier website                         { juce::String::fromUTF8 ("website")                                                                                   };
inline const juce::Identifier weight                          { juce::String::fromUTF8 ("weight")                                                                                    };
inline const juce::Identifier whitespace                      { juce::String::fromUTF8 ("whitespace")                                                                                };
inline const juce::Identifier width                           { juce::String::fromUTF8 ("width")                                                                                     };
inline const juce::Identifier win                             { juce::String::fromUTF8 ("win")                                                                                       };
inline const juce::Identifier win32InputMode                  { juce::String::fromUTF8 ("win32InputMode")                                                                            };
inline const juce::Identifier window                          { juce::String::fromUTF8 ("window")                                                                                    };
inline const juce::Identifier windowOps                       { juce::String::fromUTF8 ("windowOps")                                                                                 };
inline const juce::Identifier with                            { juce::String::fromUTF8 ("with")                                                                                      };
inline const juce::Identifier wrapMode                        { juce::String::fromUTF8 ("wrap_mode")                                                                                 };///< Shader `wrap_mode` uniform.
inline const juce::Identifier wwwAutolinkPrefix               { juce::String::fromUTF8 ("www.")                                                                                      };///< www autolink prefix.
inline const juce::Identifier x                               { juce::String::fromUTF8 ("x")                                                                                         };
inline const juce::Identifier xAxis                           { juce::String::fromUTF8 ("x-axis")                                                                                    };///< Mermaid x-axis label.
inline const juce::Identifier xAxisCategories                 { juce::String::fromUTF8 ("xAxisCategories")                                                                           };
inline const juce::Identifier xAxisLabel                      { juce::String::fromUTF8 ("xAxisLabel")                                                                                };
inline const juce::Identifier x1                              { juce::String::fromUTF8 ("x1")                                                                                        };
inline const juce::Identifier x2                              { juce::String::fromUTF8 ("x2")                                                                                        };
inline const juce::Identifier x4                              { juce::String::fromUTF8 ("x4")                                                                                        };
inline const juce::Identifier x8                              { juce::String::fromUTF8 ("x8")                                                                                        };
inline const juce::Identifier xychart                         { juce::String::fromUTF8 ("xychart")                                                                                   };
inline const juce::Identifier xychartBeta                     { juce::String::fromUTF8 ("xychart-beta")                                                                              };///< Mermaid xychart-beta diagram keyword.
inline const juce::Identifier y                               { juce::String::fromUTF8 ("y")                                                                                         };
inline const juce::Identifier yAxis                           { juce::String::fromUTF8 ("y-axis")                                                                                    };///< Mermaid y-axis label.
inline const juce::Identifier yAxisLabel                      { juce::String::fromUTF8 ("yAxisLabel")                                                                                };
inline const juce::Identifier yAxisMax                        { juce::String::fromUTF8 ("yAxisMax")                                                                                  };
inline const juce::Identifier yAxisMin                        { juce::String::fromUTF8 ("yAxisMin")                                                                                  };
inline const juce::Identifier y1                              { juce::String::fromUTF8 ("y1")                                                                                        };
inline const juce::Identifier y2                              { juce::String::fromUTF8 ("y2")                                                                                        };
inline const juce::Identifier year                            { juce::String::fromUTF8 ("year")                                                                                      };
inline const juce::Identifier yield                           { juce::String::fromUTF8 ("yield")                                                                                     };
inline const juce::Identifier z                               { juce::String::fromUTF8 ("z")                                                                                         };///< Component table z column: juce child index.
inline const juce::String     company                         { juce::String::fromUTF8 ("JRENG")                                                                                     };///< Company identity string.
inline const juce::String     companyCopyright                { juce::String::fromUTF8 ("\xc2\xa9 2025. JRENG. All rights reserved.")                                                };///< Company copyright string.
inline const juce::String     companyEmail                    { juce::String::fromUTF8 ("info@jrengmusic.com")                                                                       };///< Company email address.
inline const juce::String     companyWebsite                  { juce::String::fromUTF8 ("https://jrengmusic.com")                                                                    };///< Company website URL.
inline const juce::String     bundleDomain                    { juce::String::fromUTF8 ("com.jreng")                                                                                 };///< Application bundle domain.
inline const juce::String     manufacturerCode                { juce::String::fromUTF8 ("Jrng")                                                                                      };///< Plugin manufacturer code.
inline const juce::String     shopUrl                         { juce::String::fromUTF8 ("https://jrengmusic.com/shop")                                                               };///< Company shop URL.
inline const juce::String     downloadsUrl                    { juce::String::fromUTF8 ("jrengmusic.com/downloads")                                                                  };///< Company downloads URL.
inline const juce::String     supportEmail                    { juce::String::fromUTF8 ("support@jrengmusic.com")                                                                    };///< Company support email.
inline const juce::String     licenseFileExtension            { juce::String::fromUTF8 (".jam")                                                                                      };///< License file extension.
inline const juce::String     productsUrl                     { juce::String::fromUTF8 ("https://jrengmusic.com/products/")                                                          };///< Company products URL.

/**______________________________END OF NAMESPACE______________________________*/
}// namespace Id
