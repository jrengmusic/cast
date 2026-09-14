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
 * @file jam_Text.h
 * @brief Framework-localised English UI and diagnostic strings.
 */

#pragma once

namespace text
{
/*_____________________________________________________________________________*/

/**
 * @brief English user-interface and diagnostic string constants.
 *
 * Every constant is a localisable UI string — dialog prompts, button labels,
 * alert fragments, and validator failure messages. Strings that surround a
 * value are authored as prefix/affix pairs and joined by the caller.
 */
struct English
{
    static constexpr const char* const fft                              { "FFTReal by Laurent de Soras"                                                               };///< Third-party FFT attribution.
    static constexpr const char* const importLicensePrompt              { "Select *.jam license file to authorize"                                                    };///< License file-picker prompt.
    static constexpr const char* const selectDirectory                  { "Select directory"                                                                          };///< Directory-picker prompt.
    static constexpr const char* const selectFile                       { "Select file"                                                                               };///< File-picker prompt.
    static constexpr const char* const defaultForNewInstance            { "default for new instance"                                                                  };///< Default-instance label.
    static constexpr const char* const alertNewerVersionPresetPrefix    { "was made using a newer version of"                                                         };///< Newer-preset alert prefix.
    static constexpr const char* const alertNewerVersionPresetSuffix    { "\nTo use this preset correctly, please install the newest version from: \n"                };///< Newer-preset alert suffix.
    static constexpr const char* const presetAlreadyExistsPrefix        { "Preset"                                                                                    };///< Preset-exists alert prefix.
    static constexpr const char* const fileAlreadyExistsPrefix          { "File"                                                                                      };///< File-exists alert prefix.
    static constexpr const char* const fileAlreadyExistsSuffix          { "already exists."                                                                           };///< File-exists alert suffix.
    static constexpr const char* const askReplace                       { "Do you want to replace it?"                                                                };///< Overwrite confirmation.
    static constexpr const char* const alertUserManualNotFound          { "User Manual not found"                                                                     };///< Missing-manual alert.
    static constexpr const char* const alertIRNameTooLong               { "Impulse Response file name or location is too long"                                        };///< IR path-length alert.
    static constexpr const char* const chooseShorterNameOrLocation      { "Please choose another location with shorter name or location."                             };///< IR path-length remedy.
    static constexpr const char* const alertIRNotRecognized             { "Impulse Response file format not recognised"                                               };///< IR format alert.
    static constexpr const char* const chooseAnotherFile                { "Please choose another file."                                                               };///< IR format remedy.
    static constexpr const char* const pleaseTryAgain                   { "Please try again"                                                                          };///< Generic retry prompt.
    static constexpr const char* const noCigar                          { "Close, but no cigar."                                                                      };///< Near-miss message.
    static constexpr const char* const alertAuthorizationSuccesful      { "Auhorization Successful!"                                                                  };///< Success alert.
    static constexpr const char* const rockNRollPrefix                  { "Enjoy your"                                                                                };///< Success follow-up prefix.
    static constexpr const char* const rockNRollSuffix                  { "\n\nRock 'n Roll!"                                                                         };///< Success follow-up suffix.
    static constexpr const char* const authorizationFailed              { "Auhorization Failed"                                                                       };///< Failure alert.
    static constexpr const char* const tryAgainWithCorrectLicensePrefix { "Please try again with the correct license file:\n\n"                                       };///< Retry prompt prefix.
    static constexpr const char* const tryAgainWithCorrectLicenseSuffix { "\n\nFor assistance, contact"                                                               };///< Retry prompt suffix.
    static constexpr const char* const productInDemoSuffix              { "\n is in demo mode"                                                                        };///< Demo-mode notice suffix.
    static constexpr const char* const alertNoise                       { "4.44 second noise will be generated \nevery minute until authorized."                      };///< Demo-noise notice.
    static constexpr const char* const buttonOk                         { "OK"                                                                                        };///< OK button label.
    static constexpr const char* const buttonYes                        { "YES"                                                                                       };///< Yes button label.
    static constexpr const char* const buttonNo                         { "NO"                                                                                        };///< No button label.
    static constexpr const char* const presetSaved                      { "Saved!"                                                                                    };///< Preset-saved confirmation.
    static constexpr const char* const fileSavedPrefix                  { "Saved at"                                                                                  };///< File-saved prefix.
    static constexpr const char* const luaMissingSuffix                 { "missing"                                                                                   };///< Lua missing-value suffix.
    static constexpr const char* const luaInvalidValueSuffix            { "invalid value"                                                                             };///< Lua invalid-value suffix.
    static constexpr const char* const trademarkText                    { "@companyName@ & @productName@ are trademarks of @legalCompanyName@.\nAll rights reserved." };///< Trademark template.
    static constexpr const char* const saveAs                           { "Save Preset As..."                                                                         };///< Save-as label.
    static constexpr const char* const buyNow                           { "Buy Now!"                                                                                  };///< Purchase label.
    static constexpr const char* const mainMeshNoOp                     { "void mainMesh (inout vec3 position, inout vec3 normal) {}"                                 };///< No-op mesh shader body.
    static constexpr const char* const failNoMatch                      { "value does not match"                                                                      };///< Validator: value mismatch.
    static constexpr const char* const failDuplicate                    { "duplicate \""                                                                              };///< Validator: duplicate prefix.
    static constexpr const char* const failForeignKeyMissing            { "value not found in "                                                                       };///< Validator: foreign-key miss.
    static constexpr const char* const failNotInSet                     { "value not in {"                                                                            };///< Validator: set-membership fail.
    static constexpr const char* const failOutOfRange                   { " outside ["                                                                                };///< Validator: range fail.
    static constexpr const char* const failLocalMissing                 { "local key not in ref"                                                                      };///< Validator: local-key miss.
    static constexpr const char* const failRefMissing                   { "ref key not in local"                                                                      };///< Validator: ref-key miss.
    static constexpr const char* const failGroupOpen                    { "group '"                                                                                   };///< Validator: group-name prefix.
    static constexpr const char* const failGroupClose                   { "' does not have exactly one marked row"                                                    };///< Validator: group arity fail.
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace text
