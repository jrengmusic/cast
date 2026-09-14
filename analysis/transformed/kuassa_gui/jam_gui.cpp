#include "jam_gui.h"

// Layout submodule
#include "layout/jam_MatrixComponent.cpp"
#include "layout/jam_TabbedComponent.cpp"

// Buttons submodule
#include "buttons/jam_ButtonSVG.cpp"
#include "buttons/jam_ButtonBar.cpp"
#include "buttons/jam_ButtonGroup.cpp"

// Widgets submodule
#include "widgets/jam_FrequencyTextBox.cpp"

// Desktop submodule (native OS integration; each TU self-guarded per platform)
#include "desktop/native/jam_BackgroundBlur_windows.cpp"
#include "desktop/native/jam_SystemColour_windows.cpp"

// Windows submodule (chrome; Windows-native dispatch self-guarded #if JUCE_WINDOWS)
#include "windows/jam_StyleWindow.cpp"

// Windows submodule
#include "windows/jam_Window.cpp"
#include "windows/jam_ModalWindow.cpp"

// Menu submodule (Windows-native dispatch; self-guarded #if JUCE_WINDOWS)
#include "menu/jam_MenuWindows.cpp"

// Components
#include "native/jam_NativeFileChooser_windows.cpp"
#include "windows/jam_MessageBox.cpp"

#include "layout/jam_Scrollbar.cpp"
// code_view/jam_CodeView.cpp is END-only (Sync Ignore, no framework consumer)
// — compiled directly by END, not here.

#include "layout/jam_CollapsibleList.cpp"
#include "layout/jam_DynamicRow.cpp"
