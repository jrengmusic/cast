/*******************************************************************************
 BEGIN_JUCE_MODULE_DECLARATION
  ID:                           jam_gui
  vendor:                       JRENG! Architectural Modules
  version:                      0.0.1
  name:                         JAM GUI
  description:                  GUI foundation — Window, Modal, Glass
  website:                      https://jrengmusic.com
  license:                      Proprietary
  dependencies:                 juce_events,
                                juce_gui_extra,
                                jam_animation,
                                jam_graphics,
                                jam_data_structures,
                                jam_vulkan,
                                jam_style,
  OSXFrameworks:                Cocoa,
                                CoreGraphics,
                                QuartzCore,
 END_JUCE_MODULE_DECLARATION
*******************************************************************************/
// ================================================================
//  UI ARCHITECTURE MANIFESTO
//  ---------------------------------------------------------------
//  1. Native by Default
//     - Almost everything is just a JUCE component.
//     - No subclassing unless you are defining a contract itself.
//
//  2. Elevation by Mixins
//     - Capabilities (Events, Style, Parameter) are added via CRTP mixins.
//     - Elevation is declarative, chosen at registration time.
//     - Contracts are clear, minimal, and composable.
//
//  3. Data-Driven Topology
//     - Creation, attachment, and binding are all declared in data.
//     - attachTo = model semantics (parameter binding).
//     - bindTo   = UI semantics (interactivity, z-order, grouping).
//
//  4. Selective Polymorphism
//     - Cast only when the contract is known.
//     - Aux components hook themselves by casting to the abstract
//       (e.g. Mouse::Events<Base>), never by poking internals.
//
//  5. Consistency + Freedom
//     - Consistency: enforced by contracts and registrar.
//     - Freedom: any JUCE component can be elevated, any aux can bind.
//     - The Model is the single source of truth.
//
//  Guiding Principle:
//     "Declare capability in data.
//      Elevate with mixins.
//      Bind by contract.
//      Never subclass unless it IS the contract."
//
// ================================================================

/**
 * @file jam_gui.h
 * @brief Module header aggregating all jam_gui submodule includes.
 */

#pragma once
#include <juce_events/juce_events.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include <jam_animation/jam_animation.h>
#include <jam_graphics/jam_graphics.h>
#include <jam_data_structures/jam_data_structures.h>
#include <jam_vulkan/jam_vulkan.h>
#include <jam_style/jam_style.h>

#include "utils/jam_ComponentUtils.h"
#include "utils/jam_SliderUtils.h"
#include "menu/jam_Menu.h"

#if JUCE_WINDOWS
#include <juce_gui_basics/native/juce_ScopedThreadDPIAwarenessSetter_windows.h>
#endif

#include "menu/jam_PopupMenu.h"
#include "windows/jam_MessageBox.h"
#include "mouse/jam_MouseEvents.h"
#include "mouse/jam_MouseEnterParent.h"

// Overlay submodule (depends on jam_style StyleCustom, jam_graphics Rotary, jam_animation Animator)
#include "overlay/jam_OverlayImage.h"
#include "overlay/jam_Shadow.h"
#include "overlay/jam_OverlayDrawShadow.h"
#include "overlay/jam_OverlayToggleShadow.h"
#include "overlay/jam_OverlayRotaryMarker.h"
#include "layout/jam_FlexBox.h"
#include "buttons/jam_ButtonMenu.h"
#include "buttons/jam_ButtonSVG.h"
#include "buttons/jam_ButtonLogo.h"
#include "buttons/jam_ButtonToggleValue.h"

// Layout submodule
#include "layout/jam_OwnedComponent.h"
#include "layout/jam_OwnerComponent.h"
#include "layout/jam_PaneComponent.h"
#include "layout/jam_PaneEdge.h"
#include "layout/jam_MatrixComponent.h"

// Desktop submodule (native OS integration)
#include "desktop/jam_BackgroundBlur.h"
#include "desktop/jam_SystemColour.h"

// Windows submodule (chrome)
#include "windows/jam_StyleWindow.h"

// Windows submodule (depends on desktop/ BackgroundBlur)
#include "windows/jam_Window.h"
#include "windows/jam_ModalSheet.h"
#include "windows/jam_ModalWindow.h"

// Buttons submodule (depends on windows/ above)
#include "buttons/jam_ButtonTab.h"
#include "buttons/jam_ButtonBar.h"
#include "buttons/jam_ButtonOptions.h"
#include "buttons/jam_ButtonGroup.h"
#include "buttons/jam_ButtonDialog.h"

// TabbedComponent (depends on ButtonBar, ButtonSVG above)
#include "layout/jam_TabbedComponent.h"

#include "widgets/jam_SelectorArrows.h"
#include "widgets/jam_ImageComponent.h"
#include "widgets/jam_ComboBox.h"
#include "widgets/jam_Selector.h"
#include "widgets/jam_PresetSelector.h"
#include "widgets/jam_SliderIncDec.h"
#include "widgets/jam_PopupTextBox.h"
#include "widgets/jam_VariComponent.h"
#include "widgets/jam_FrequencyTextBox.h"
#include "widgets/jam_ToggleMod.h"
#include "widgets/jam_FrequencyGrid.h"
#include "widgets/jam_Vignette.h"
#include "widgets/jam_MagnitudePlot.h"
#include "widgets/jam_Analyzer.h"

#include "native/jam_NativeFileChooser.h"
#include "file_chooser/jam_FileChooser.h"
#include "filebrowser/jam_Browser.h"
#include "filebrowser/jam_PathBrowser.h"
#include "filebrowser/jam_SelectorBrowser.h"

// Code editor submodule
// keyboard/jam_CaretComponent.h and code_view/jam_CodeView.h are END-only
// (Sync Ignore, no framework consumer) — included directly by END, not here.
#include "layout/jam_Scrollbar.h"

#include "layout/jam_CollapsibleSectionBase.h"
#include "layout/jam_CollapsibleList.h"
#include "layout/jam_DynamicRow.h"
