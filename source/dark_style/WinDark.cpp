#include <QPalette>
#include <QSettings>
#include <QStyleFactory>
#include <QApplication>

#include "WinDark.h"

namespace dark_style_windows {

/**
 * @brief isDarkTheme
 * @return bool
 */
bool isDarkTheme()
{
#ifdef Q_OS_WIN
    QSettings settings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                       QSettings::NativeFormat);
    if(settings.value("AppsUseLightTheme")==0){
        return true;
    }
#endif
    return false;
}

/**
 * @brief setDark_Titlebar
 * @param hwnd
 * @details Use for each window like
 *    if(winDark::isDarkTheme()) winDark::setDark_Titlebar(reinterpret_cast<HWND>(winId()));
 */
void setDark_Titlebar(HWND hwnd)
{
#ifdef Q_OS_WIN
    HMODULE hUxtheme = LoadLibraryExW(L"uxtheme.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
    HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
    fnAllowDarkModeForWindow AllowDarkModeForWindow
        = reinterpret_cast<fnAllowDarkModeForWindow>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(133)));
    fnSetPreferredAppMode SetPreferredAppMode
        = reinterpret_cast<fnSetPreferredAppMode>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(135)));
    fnSetWindowCompositionAttribute SetWindowCompositionAttribute
        = reinterpret_cast<fnSetWindowCompositionAttribute>(GetProcAddress(hUser32, "SetWindowCompositionAttribute"));

    SetPreferredAppMode(AllowDark);
    BOOL dark = TRUE;
    AllowDarkModeForWindow(hwnd, dark);
    WINDOWCOMPOSITIONATTRIBDATA data = {
        WCA_USEDARKMODECOLORS,
        &dark,
        sizeof(dark)
    };
    SetWindowCompositionAttribute(hwnd, &data);
#endif
}

}
