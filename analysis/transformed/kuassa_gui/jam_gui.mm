#include "jam_gui.cpp"

#if JUCE_MAC
#import <Cocoa/Cocoa.h>
#import <CoreGraphics/CoreGraphics.h>
#include "menu/jam_Menu.mm"
#include "native/jam_NativeFileChooser_mac.mm"
#include "windows/jam_StyleWindow.mm"
#include "windows/native/jam_ModalSheet_mac.mm"
#include "desktop/native/jam_BackgroundBlur_mac.mm"
#include "desktop/native/jam_SystemColour_mac.mm"
#endif
