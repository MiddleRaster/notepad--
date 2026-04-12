#ifndef SETTINGS_H
#define SETTINGS_H

#pragma once
#include <string>
#include <windows.h>

#include "..\src\Font.h"

namespace Settings
{
    class Registry
    {
        static constexpr LPCWSTR kSettingsKey = L"Software\\MiddleRaster\\notepad--";

        static bool WriteInt(const std::wstring& valueName, int value)
        {
            HKEY hKey = nullptr;
            if (RegCreateKeyExW(HKEY_CURRENT_USER, kSettingsKey, 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, nullptr) == ERROR_SUCCESS)
            {
                RegSetValueExW(hKey, valueName.c_str(), 0, REG_DWORD, reinterpret_cast<const BYTE*>(&value), sizeof(DWORD));
                RegCloseKey(hKey);
                return true;
            }
            return false;
        }
        static bool WriteString(const std::wstring& valueName, const std::wstring& value)
        {
            HKEY hKey = nullptr;
            if (RegCreateKeyExW(HKEY_CURRENT_USER, kSettingsKey, 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, nullptr) == ERROR_SUCCESS)
            {
                RegSetValueExW(hKey, valueName.c_str(), 0, REG_SZ, reinterpret_cast<const BYTE*>(value.c_str()), static_cast<DWORD>((value.size()+1)*sizeof(wchar_t)));
                RegCloseKey(hKey);
                return true;
            }
            return false;
        }

        static int ReadInt(const std::wstring& valueName, int defaultValue=0)
        {
            HKEY hKey = nullptr;
            if (RegOpenKeyExW(HKEY_CURRENT_USER, kSettingsKey, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
            {
                DWORD type=0, size=sizeof(DWORD);
                RegQueryValueExW(hKey, valueName.c_str(), nullptr, &type, reinterpret_cast<BYTE*>(&defaultValue), &size);
                RegCloseKey(hKey);
            }
            return defaultValue;
        }
        static std::wstring ReadString(const std::wstring& valueName, std::wstring defaultValue=L"")
        {
            HKEY hKey = nullptr;
            if (RegOpenKeyExW(HKEY_CURRENT_USER, kSettingsKey, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
            {
                DWORD type=0, size=0;
                RegQueryValueExW(hKey, valueName.c_str(), nullptr, &type, nullptr, &size);
                if (size > 0)
                {
                    defaultValue.resize(size/sizeof(wchar_t));
                    RegQueryValueExW(hKey, valueName.c_str(), nullptr, &type, reinterpret_cast<BYTE*>(defaultValue.data()), &size);
                    if (!defaultValue.empty() && defaultValue.back() == L'\0')
                        defaultValue.pop_back();
                }
                RegCloseKey(hKey);
            }
            return defaultValue;
        }
    public:
        static std::wstring GetFontName() { return ReadString(L"FaceName",   L"Segoe UI"); }
        static int        GetFontHeight() { return ReadInt   (L"FontHeight", 12); }
        static bool         IsMaximized() { return ReadInt   (L"Maximized",  0) != 0; }
        static RECT    GetWindowExtents()
        {
            RECT r{};
            r.left   = ReadInt(L"Left",   100);
            r.top    = ReadInt(L"Top",    100);
            r.right  = ReadInt(L"Right",  600);
            r.bottom = ReadInt(L"Bottom", 500);
            return r;
        }

        static void SetFontName     (const std::wstring& fontName) { WriteString(L"FaceName",   fontName ); }
        static void SetFontHeight   (      int           lfHeight) { WriteInt   (L"FontHeight", lfHeight ); }
        static void SetMaximized    (      bool          b       ) { WriteInt   (L"Maximized",  b ? 1 : 0); }
        static void SetWindowExtents(const RECT&         r       )
        {
            WriteInt(L"Left",   r.left  );
            WriteInt(L"Top",    r.top   );
            WriteInt(L"Right",  r.right );
            WriteInt(L"Bottom", r.bottom);
        }
    };

    struct FontAndPlacement
    {
        static void Load(HWND hWnd, HWND edit)
        {
            RECT rc        = Registry::GetWindowExtents();
            bool maximized = Registry::IsMaximized();

            WINDOWPLACEMENT wp{};
            wp.length           = sizeof(WINDOWPLACEMENT);
            wp.showCmd          = maximized ? SW_SHOWMAXIMIZED : SW_SHOWNORMAL;
            wp.rcNormalPosition = rc;
            ::SetWindowPlacement(hWnd, &wp);

            // font selection
            std::wstring faceName  = Registry::GetFontName();
            int          pointSize = Registry::GetFontHeight();
            HDC hdc                = ::GetDC(hWnd);
            int lfHeight           = -::MulDiv(pointSize, ::GetDeviceCaps(hdc, LOGPIXELSY), 72);
            ::ReleaseDC(hWnd, hdc);
            LOGFONT lf{};
            lf.lfHeight            = lfHeight;
            lf.lfCharSet           = DEFAULT_CHARSET;
            lf.lfOutPrecision      = OUT_TT_PRECIS;
            lf.lfClipPrecision     = CLIP_DEFAULT_PRECIS;
            lf.lfQuality           = CLEARTYPE_QUALITY;
            lf.lfPitchAndFamily    = DEFAULT_PITCH | FF_DONTCARE;
            ::wcsncpy_s(lf.lfFaceName, LF_FACESIZE, Font::GetFamilyName(faceName).c_str(), _TRUNCATE);

            HFONT font = ::CreateFontIndirect(&lf);

            HFONT hOldFont = (HFONT)SendMessage(edit, WM_GETFONT, 0, 0);
            SendMessage(edit, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
            if (hOldFont != NULL)
                DeleteObject(hOldFont);
        }
        static void Save(HWND hWnd, HWND edit)
        {
            WINDOWPLACEMENT wp{};
            wp.length = sizeof(WINDOWPLACEMENT);
            ::GetWindowPlacement(hWnd, &wp);
            Registry::SetWindowExtents(wp.rcNormalPosition);
            Registry::SetMaximized    (wp.showCmd == SW_SHOWMAXIMIZED);

            HFONT hFont = reinterpret_cast<HFONT>(::SendMessage(edit, WM_GETFONT, 0, 0));
            LOGFONT lf{};
            ::GetObject(hFont, sizeof(LOGFONT), &lf); // lf.lfFaceName is the font name (wchar_t[32])

            HDC hdc       = ::GetDC(edit);
            int pointSize = ::MulDiv(-lf.lfHeight, 72, ::GetDeviceCaps(hdc, LOGPIXELSY)); // lf.lfHeight   is the height in logical units (negative for character height)
            ::ReleaseDC(edit, hdc);
            Registry::SetFontName(Font::GetFamilyName(lf.lfFaceName));
            Registry::SetFontHeight(pointSize);
        }
    };
}
#endif
