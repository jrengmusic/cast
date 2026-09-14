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
 * @file jam_Files.h
 * @brief Framework asset file names and recognised file extensions.
 */

#pragma once

/**
 * @brief File-extension strings, keyed by language name.
 *
 * Each constant is the bare extension (no leading dot) of one supported
 * source or markup language. Extensions pair with map::languageFamily.
 */
struct Extensions
{
    static constexpr const char* const bash   { "bash"   };///< Bash script.
    static constexpr const char* const c      { "c"      };///< C source.
    static constexpr const char* const cast   { "cast"   };///< CAST template block.
    static constexpr const char* const cmake  { "cmake"  };///< CMake script.
    static constexpr const char* const cpp    { "cpp"    };///< C++ source.
    static constexpr const char* const css    { "css"    };///< Cascading stylesheet.
    static constexpr const char* const go     { "go"     };///< Go source.
    static constexpr const char* const h      { "h"      };///< C/C++ header.
    static constexpr const char* const html   { "html"   };///< HTML markup.
    static constexpr const char* const java   { "java"   };///< Java source.
    static constexpr const char* const js     { "js"     };///< JavaScript source.
    static constexpr const char* const json   { "json"   };///< JSON data.
    static constexpr const char* const jsx    { "jsx"    };///< JSX markup.
    static constexpr const char* const lua    { "lua"    };///< Lua script.
    static constexpr const char* const md     { "md"     };///< Markdown.
    static constexpr const char* const py     { "py"     };///< Python source.
    static constexpr const char* const python { "python" };///< Python source (long form).
    static constexpr const char* const rb     { "rb"     };///< Ruby source.
    static constexpr const char* const rs     { "rs"     };///< Rust source.
    static constexpr const char* const ruby   { "ruby"   };///< Ruby source (long form).
    static constexpr const char* const rust   { "rust"   };///< Rust source (long form).
    static constexpr const char* const sh     { "sh"     };///< Shell script.
    static constexpr const char* const shell  { "shell"  };///< Shell script (long form).
    static constexpr const char* const sql    { "sql"    };///< SQL script.
    static constexpr const char* const svg    { "svg"    };///< SVG image.
    static constexpr const char* const ts     { "ts"     };///< TypeScript source.
    static constexpr const char* const tsx    { "tsx"    };///< TSX markup.
    static constexpr const char* const xml    { "xml"    };///< XML markup.
    static constexpr const char* const yaml   { "yaml"   };///< YAML data.
    static constexpr const char* const yml    { "yml"    };///< YAML data (short form).
};

//==============================================================================

namespace files
{
/*_____________________________________________________________________________*/

/**
 * @brief Framework asset file names — layouts, shaders, icons, stylesheets.
 *
 * Each constant is the literal file name of an embedded resource, resolved
 * against the binary-data / asset search path at load time.
 */

    inline const juce::String addDisabled            { juce::String::fromUTF8 ("add_disabled.svg")              };///< Add-button disabled icon.
    inline const juce::String addDown                { juce::String::fromUTF8 ("add_down.svg")                  };///< Add-button pressed icon.
    inline const juce::String addNormal              { juce::String::fromUTF8 ("add_normal.svg")                };///< Add-button idle icon.
    inline const juce::String addOver                { juce::String::fromUTF8 ("add_over.svg")                  };///< Add-button hover icon.
    inline const juce::String backgroundCombineFrag  { juce::String::fromUTF8 ("background_combine.frag.spv")   };///< Background combine fragment shader.
    inline const juce::String backgroundFrag         { juce::String::fromUTF8 ("background.frag.spv")           };///< Background fragment shader.
    inline const juce::String calibrationFrag        { juce::String::fromUTF8 ("calibration.frag.spv")          };///< Calibration fragment shader.
    inline const juce::String calibrationVert        { juce::String::fromUTF8 ("calibration.vert.spv")          };///< Calibration vertex shader.
    inline const juce::String closeDisabled          { juce::String::fromUTF8 ("close_disabled.svg")            };///< Close-button disabled icon.
    inline const juce::String closeDown              { juce::String::fromUTF8 ("close_down.svg")                };///< Close-button pressed icon.
    inline const juce::String closeNormal            { juce::String::fromUTF8 ("close_normal.svg")              };///< Close-button idle icon.
    inline const juce::String closeOver              { juce::String::fromUTF8 ("close_over.svg")                };///< Close-button hover icon.
    inline const juce::String componentLayout        { juce::String::fromUTF8 ("component.md")                  };///< Component descriptor tables.
    inline const juce::String defaultSettings        { juce::String::fromUTF8 ("DefaultSettings.xml")           };///< Embedded default user-settings template.
    inline const juce::String fillRectFrag           { juce::String::fromUTF8 ("fill_rect.frag.spv")            };///< Rect-fill fragment shader.
    inline const juce::String fillRectVert           { juce::String::fromUTF8 ("fill_rect.vert.spv")            };///< Rect-fill vertex shader.
    inline const juce::String glyphEmojiFrag         { juce::String::fromUTF8 ("glyph_emoji.frag.spv")          };///< Emoji glyph fragment shader.
    inline const juce::String glyphMonoFrag          { juce::String::fromUTF8 ("glyph_mono.frag.spv")           };///< Mono glyph fragment shader.
    inline const juce::String gradientFillFrag       { juce::String::fromUTF8 ("gradient_fill.frag.spv")        };///< Gradient-fill fragment shader.
    inline const juce::String imageAlphaMaskFrag     { juce::String::fromUTF8 ("image_alpha_mask.frag.spv")     };///< Image alpha-mask fragment shader.
    inline const juce::String imageFrag              { juce::String::fromUTF8 ("image.frag.spv")                };///< Image fragment shader.
    inline const juce::String incDecDownDown         { juce::String::fromUTF8 ("inc_dec_down_down.svg")         };///< Inc/dec down-pressed icon.
    inline const juce::String incDecDownNormal       { juce::String::fromUTF8 ("inc_dec_down_normal.svg")       };///< Inc/dec down-idle icon.
    inline const juce::String incDecDownOver         { juce::String::fromUTF8 ("inc_dec_down_over.svg")         };///< Inc/dec down-hover icon.
    inline const juce::String incDecUpDown           { juce::String::fromUTF8 ("inc_dec_up_down.svg")           };///< Inc/dec up-pressed icon.
    inline const juce::String incDecUpNormal         { juce::String::fromUTF8 ("inc_dec_up_normal.svg")         };///< Inc/dec up-idle icon.
    inline const juce::String incDecUpOver           { juce::String::fromUTF8 ("inc_dec_up_over.svg")           };///< Inc/dec up-hover icon.
    inline const juce::String instancedMaskedVert    { juce::String::fromUTF8 ("instanced_masked.vert.spv")     };///< Instanced masked vertex shader.
    inline const juce::String instancedRectVert      { juce::String::fromUTF8 ("instanced_rect.vert.spv")       };///< Instanced rect vertex shader.
    inline const juce::String instancedVert          { juce::String::fromUTF8 ("instanced.vert.spv")            };///< Instanced vertex shader.
    inline const juce::String logo                   { juce::String::fromUTF8 ("logo.svg")                      };///< Framework logo mark.
    inline const juce::String manualSuffix           { juce::String::fromUTF8 (" Manual.pdf")                   };///< User-manual file-name suffix.
    inline const juce::String maskedImageFrag        { juce::String::fromUTF8 ("masked_image.frag.spv")         };///< Masked-image fragment shader.
    inline const juce::String matteChokeComp         { juce::String::fromUTF8 ("matte_choke.comp.spv")          };///< Matte-choke compute shader.
    inline const juce::String matteFeatherComp       { juce::String::fromUTF8 ("matte_feather.comp.spv")        };///< Matte-feather compute shader.
    inline const juce::String mermaidStyleSheet      { juce::String::fromUTF8 ("mermaid.css")                   };///< Mermaid diagram stylesheet.
    inline const juce::String meshDefaultFrag        { juce::String::fromUTF8 ("mesh_default.frag.spv")         };///< Default mesh fragment shader.
    inline const juce::String meshDefaultVertex      { juce::String::fromUTF8 ("mesh_default.vert")             };///< Default mesh vertex shader.
    inline const juce::String meshEdgeFrag           { juce::String::fromUTF8 ("mesh_edge.frag.spv")            };///< Edge mesh fragment shader.
    inline const juce::String meshEdgeVertex         { juce::String::fromUTF8 ("mesh_edge.vert")                };///< Edge mesh vertex shader.
    inline const juce::String paneCornerMenu         { juce::String::fromUTF8 ("pane_corner_menu.svg")          };///< Pane corner-menu icon.
    inline const juce::String parametersLayout       { juce::String::fromUTF8 ("parameters.md")                 };///< Parameter descriptor tables.
    inline const juce::String postProcessCombineFrag { juce::String::fromUTF8 ("post_process_combine.frag.spv") };///< Post-process combine fragment shader.
    inline const juce::String selectorArrows         { juce::String::fromUTF8 ("selector_arrows.svg")           };///< Selector arrows icon.
    inline const juce::String settingExtension       { juce::String::fromUTF8 (".setting")                      };///< User settings file extension.
    inline const juce::String settingsDirectory      { juce::String::fromUTF8 ("Settings")                      };///< User settings directory name.
    inline const juce::String shaderPassVert         { juce::String::fromUTF8 ("shader_pass.vert.spv")          };///< Shader-pass vertex shader.
    inline const juce::String shaderToyChannelMacro  { juce::String::fromUTF8 ("shaderToyChannelMacro.frag")    };///< Shadertoy channel macro.
    inline const juce::String shaderToySceneMacro    { juce::String::fromUTF8 ("shaderToySceneMacro.frag")      };///< Shadertoy scene macro.
    inline const juce::String shadertoyWrapper       { juce::String::fromUTF8 ("shadertoy_wrapper.frag")        };///< Shadertoy wrapper fragment.
    inline const juce::String stackBlurBufferComp    { juce::String::fromUTF8 ("stack_blur_buffer.comp.spv")    };///< Stack-blur buffer compute shader.
    inline const juce::String stackBlurTextureComp   { juce::String::fromUTF8 ("stack_blur_texture.comp.spv")   };///< Stack-blur texture compute shader.
    inline const juce::String standby                { juce::String::fromUTF8 ("standby.svg")                   };///< Standby icon.
    inline const juce::String straightAlphaFrag      { juce::String::fromUTF8 ("straight_alpha.frag.spv")       };///< Straight-alpha fragment shader.
    inline const juce::String styleSheet             { juce::String::fromUTF8 ("style.css")                     };///< Application stylesheet.
    inline const juce::String tiledImageFrag         { juce::String::fromUTF8 ("tiled_image.frag.spv")          };///< Tiled-image fragment shader.

/**______________________________END OF NAMESPACE______________________________*/
}// namespace files
