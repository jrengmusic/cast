#include "jam_style.h"

#if JUCE_MODULE_AVAILABLE_jam_gui
#include <jam_gui/jam_gui.h>
#endif

#include "style_manager/jam_StyleManager.cpp"
#include "style_manager/jam_StyleTheme.cpp"

#include "jam_StyleShadow.cpp"

#if JUCE_MODULE_AVAILABLE_jam_vulkan
#include "jam_StyleTogglePush.cpp"
#include "jam_StyleToggleSlide.cpp"
#endif

#include "jam_StyleVariDisplay.cpp"
