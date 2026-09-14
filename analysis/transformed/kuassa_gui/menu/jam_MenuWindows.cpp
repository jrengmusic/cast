#if JUCE_WINDOWS
#include <windows.h>
#include <commctrl.h>

namespace jam::menu
{
/*____________________________________________________________________________*/

static HMENU buildHMENU (const juce::PopupMenu& juceMenu)
{
    HMENU hMenu { CreatePopupMenu() };
    UINT index { 0 };

    juce::PopupMenu::MenuItemIterator iter (juceMenu, false);

    while (iter.next())
    {
        const auto& item { iter.getItem() };

        MENUITEMINFOW mii {};
        mii.cbSize = sizeof (MENUITEMINFOW);

        if (item.isSeparator)
        {
            mii.fMask = MIIM_FTYPE;
            mii.fType = MFT_SEPARATOR;
            InsertMenuItemW (hMenu, index, TRUE, &mii);
        }
        else if (item.isSectionHeader)
        {
            const juce::String text { item.text };
            mii.fMask      = MIIM_STRING | MIIM_STATE;
            mii.fState     = MFS_GRAYED;
            mii.dwTypeData = const_cast<LPWSTR> (text.toWideCharPointer());
            InsertMenuItemW (hMenu, index, TRUE, &mii);
        }
        else if (item.subMenu != nullptr)
        {
            const juce::String text { item.text };
            mii.fMask      = MIIM_STRING | MIIM_SUBMENU | MIIM_STATE;
            mii.hSubMenu   = buildHMENU (*item.subMenu);
            mii.fState     = item.isEnabled ? MFS_ENABLED : MFS_GRAYED;
            mii.dwTypeData = const_cast<LPWSTR> (text.toWideCharPointer());
            InsertMenuItemW (hMenu, index, TRUE, &mii);
        }
        else
        {
            const juce::String sentenceCase { item.text.substring (0, 1).toUpperCase()
                                              + item.text.substring (1).toLowerCase() };
            mii.fMask      = MIIM_ID | MIIM_STRING | MIIM_STATE;
            mii.wID        = static_cast<UINT> (item.itemID);
            mii.fState     = item.isEnabled ? MFS_ENABLED : MFS_GRAYED;

            if (item.isTicked)
                mii.fState |= MFS_CHECKED;

            mii.dwTypeData = const_cast<LPWSTR> (sentenceCase.toWideCharPointer());
            InsertMenuItemW (hMenu, index, TRUE, &mii);
        }

        ++index;
    }

    // Strip trailing separator — JUCE and NSMenu silently drop them,
    // Win32 HMENU renders them. Remove if present.
    if (index > 0)
    {
        MENUITEMINFOW last {};
        last.cbSize = sizeof (MENUITEMINFOW);
        last.fMask  = MIIM_FTYPE;

        if (GetMenuItemInfoW (hMenu, index - 1, TRUE, &last))
        {
            if ((last.fType & MFT_SEPARATOR) != 0)
                RemoveMenu (hMenu, index - 1, MF_BYPOSITION);
        }
    }

    return hMenu;
}

static constexpr DWORD_PTR menuWidthSentinel { 0xCA01 };
static constexpr UINT_PTR  menuWidthSubclassId { 1 };

static LRESULT CALLBACK menuWidthSubclassProc (HWND hwnd, UINT msg, WPARAM wp, LPARAM lp,
                                                UINT_PTR id, DWORD_PTR refData)
{
    if (msg == WM_MEASUREITEM)
    {
        auto* mis { reinterpret_cast<MEASUREITEMSTRUCT*> (lp) };

        if (mis->CtlType == ODT_MENU and mis->itemData == menuWidthSentinel)
        {
            mis->itemWidth  = static_cast<UINT> (refData);
            mis->itemHeight = 0;
            return TRUE;
        }
    }

    if (msg == WM_DRAWITEM)
    {
        auto* dis { reinterpret_cast<DRAWITEMSTRUCT*> (lp) };

        if (dis->CtlType == ODT_MENU and dis->itemData == menuWidthSentinel)
            return TRUE;
    }

    return DefSubclassProc (hwnd, msg, wp, lp);
}

static int showAndCleanup (HMENU hMenu, HWND hwnd, int x, int y, int minWidth = 0)
{
    const auto prevCtx { SetThreadDpiAwarenessContext (
        DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2) };

    if (minWidth > 0)
    {
        MENUITEMINFOW spacer {};
        spacer.cbSize     = sizeof (MENUITEMINFOW);
        spacer.fMask      = MIIM_FTYPE | MIIM_DATA;
        spacer.fType      = MFT_OWNERDRAW | MFT_SEPARATOR;
        spacer.dwItemData = menuWidthSentinel;
        InsertMenuItemW (hMenu, 0, TRUE, &spacer);

        SetWindowSubclass (hwnd, menuWidthSubclassProc, menuWidthSubclassId,
                           static_cast<DWORD_PTR> (minWidth));
    }

    const HWND rootHwnd { GetAncestor (hwnd, GA_ROOT) };
    SetForegroundWindow (rootHwnd);

    const auto selected { static_cast<int> (TrackPopupMenuEx (
        hMenu,
        TPM_RETURNCMD | TPM_NONOTIFY | TPM_LEFTALIGN | TPM_TOPALIGN | TPM_NOANIMATION,
        x, y,
        hwnd,
        nullptr)) };

    PostMessage (hwnd, WM_NULL, 0, 0);

    if (minWidth > 0)
        RemoveWindowSubclass (hwnd, menuWidthSubclassProc, menuWidthSubclassId);

    SetThreadDpiAwarenessContext (prevCtx);

    DestroyMenu (hMenu);

    return selected;
}

void showNative (void* windowHandle,
                 const std::map<int, juce::String>& map,
                 std::function<void (int)> onSelect)
{
    HWND hwnd { static_cast<HWND> (windowHandle) };
    HMENU hMenu { CreatePopupMenu() };
    UINT index { 0 };

    for (const auto& [tag, value] : map)
    {
        const juce::String text { value };
        MENUITEMINFOW mii {};
        mii.cbSize     = sizeof (MENUITEMINFOW);
        mii.fMask      = MIIM_ID | MIIM_STRING | MIIM_STATE;
        mii.wID        = static_cast<UINT> (tag);
        mii.fState     = MFS_ENABLED;
        mii.dwTypeData = const_cast<LPWSTR> (text.toWideCharPointer());
        InsertMenuItemW (hMenu, index, TRUE, &mii);
        ++index;
    }

    POINT cursorPos {};
    GetCursorPos (&cursorPos);

    const int result { showAndCleanup (hMenu, hwnd, cursorPos.x, cursorPos.y) };
    onSelect (result);
}

void showNative (void* windowHandle,
                 const juce::PopupMenu& menu,
                 std::function<void (int)> onSelect)
{
    HWND hwnd { static_cast<HWND> (windowHandle) };
    HMENU hMenu { buildHMENU (menu) };

    POINT cursorPos {};
    GetCursorPos (&cursorPos);

    const int result { showAndCleanup (hMenu, hwnd, cursorPos.x, cursorPos.y) };
    onSelect (result);
}

void showNativeAt (void* windowHandle,
                   const juce::PopupMenu& menu,
                   juce::Rectangle<int> targetBounds,
                   std::function<void (int)> onSelect)
{
    HWND hwnd { static_cast<HWND> (windowHandle) };

    POINT screenTopLeft { targetBounds.getX(), targetBounds.getBottom() };
    POINT screenTopRight { targetBounds.getRight(), targetBounds.getBottom() };
    ClientToScreen (hwnd, &screenTopLeft);
    ClientToScreen (hwnd, &screenTopRight);

    const int screenWidth { screenTopRight.x - screenTopLeft.x };

    HMENU hMenu { buildHMENU (menu) };

    const int result { showAndCleanup (hMenu, hwnd, screenTopLeft.x, screenTopLeft.y, screenWidth) };
    onSelect (result);
}

void showNativeOptions (void* windowHandle,
                        const std::map<int, juce::String>& map,
                        int selectedKey,
                        std::function<void (int)> onSelect,
                        const juce::String& header)
{
    HWND hwnd { static_cast<HWND> (windowHandle) };
    HMENU hMenu { CreatePopupMenu() };
    UINT index { 0 };

    if (header.isNotEmpty())
    {
        MENUITEMINFOW headerItem {};
        headerItem.cbSize     = sizeof (MENUITEMINFOW);
        headerItem.fMask      = MIIM_STRING | MIIM_STATE;
        headerItem.fState     = MFS_GRAYED;
        headerItem.dwTypeData = const_cast<LPWSTR> (header.toWideCharPointer());
        InsertMenuItemW (hMenu, index, TRUE, &headerItem);
        ++index;

        MENUITEMINFOW sepItem {};
        sepItem.cbSize = sizeof (MENUITEMINFOW);
        sepItem.fMask  = MIIM_FTYPE;
        sepItem.fType  = MFT_SEPARATOR;
        InsertMenuItemW (hMenu, index, TRUE, &sepItem);
        ++index;
    }

    for (const auto& [tag, value] : map)
    {
        const juce::String text { value };
        MENUITEMINFOW mii {};
        mii.cbSize     = sizeof (MENUITEMINFOW);
        mii.fMask      = MIIM_ID | MIIM_STRING | MIIM_STATE;
        mii.wID        = static_cast<UINT> (tag);
        mii.fState     = MFS_ENABLED;

        if (tag == selectedKey)
            mii.fState |= MFS_CHECKED;

        mii.dwTypeData = const_cast<LPWSTR> (text.toWideCharPointer());
        InsertMenuItemW (hMenu, index, TRUE, &mii);
        ++index;
    }

    POINT cursorPos {};
    GetCursorPos (&cursorPos);

    const int result { showAndCleanup (hMenu, hwnd, cursorPos.x, cursorPos.y) };
    onSelect (result);
}

/*___________________________END OF NAMESPACE_________________________________*/
} // namespace jam::menu

#endif // JUCE_WINDOWS
