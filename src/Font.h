#ifndef FONT_H
#define FONT_H

#pragma once
#include <windows.h>
#include <commdlg.h>

struct Font
{
    static void InitializeFont_OnlyCallOnNewlyCreatedEditWindows(HWND edit)
    {
        HDC hdc  = GetDC(edit);
        int dpiY = GetDeviceCaps(hdc, LOGPIXELSY);
                   ReleaseDC(edit, hdc);

        NONCLIENTMETRICS ncm       = {0};
        ncm.cbSize                 = sizeof(ncm);
        SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
        ncm.lfMessageFont.lfHeight = -MulDiv(12, dpiY, 72);
        HFONT hFont                = CreateFontIndirect(&ncm.lfMessageFont);
        SendMessage(edit, WM_SETFONT, (WPARAM)hFont, FALSE);
    }

    static void DisplayChooseFontDialog(HWND hwnd, HWND edit)
    {
        LOGFONT    lf  = {0};
        CHOOSEFONT cf  = {0};
        cf.lStructSize = sizeof(cf);
        cf.hwndOwner   = hwnd;
        cf.lpLogFont   = &lf;
        cf.Flags       = CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT | CF_FORCEFONTEXIST;

        HFONT hCurrentFont = (HFONT)SendMessage(edit, WM_GETFONT, 0, 0);
        if (hCurrentFont != nullptr)
        {
            GetObject(hCurrentFont, sizeof(LOGFONT), &lf);
            ::wcsncpy_s(lf.lfFaceName, LF_FACESIZE, GetFamilyName(lf.lfFaceName).c_str(), _TRUNCATE);
        }
        else
        { // if no font selected, use system metrics
            NONCLIENTMETRICS ncm = {0};
            ncm.cbSize = sizeof(ncm);
            SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
            lf = ncm.lfCaptionFont; // copy the whole struct over

            // fiddle with lfHeight
            HDC hdc       = GetDC(hwnd);
            int dpiY      = GetDeviceCaps(hdc, LOGPIXELSY);
                            ReleaseDC(hwnd, hdc);
            int pointSize = MulDiv(-lf.lfHeight, 72, dpiY);
            cf.iPointSize = pointSize*10;                   // tenths of a point
            lf.lfHeight   = -MulDiv(pointSize, dpiY, 72);   // re-derive so they're consistent
        }
        if (!ChooseFont(&cf))
            return;

        // After ChooseFont returns, lf is already updated by CF_INITTOLOGFONTSTRUCT.
        // Recompute lfHeight from iPointSize to get a canonical value.
        HDC hdc       =  GetDC(hwnd);
        int dpiY      =  GetDeviceCaps(hdc, LOGPIXELSY);
                         ReleaseDC(hwnd, hdc);
        lf.lfHeight   = -MulDiv(cf.iPointSize/10, dpiY, 72);

        HFONT hNewFont = CreateFontIndirect(&lf);
        HFONT hOldFont = (HFONT)SendMessage(edit, WM_GETFONT, 0, 0);
        SendMessage(edit, WM_SETFONT, (WPARAM)hNewFont, TRUE);
        if (hOldFont != NULL)
            DeleteObject(hOldFont);
    }

    static std::wstring GetFamilyName(const std::wstring& fullFaceName)
    {
        std::wstring name = GetFamilyNameByEnuming(fullFaceName);

        // if it ends with Light or Regular, etc. strip that off
        std::wstring badEndings[] =
        {
            L" Display Bold Italic",
            L" Display Bold",
            L" Display Italic",
            L" Display",
            L" Small Caption",
            L" Caption",
            L" Subheading",
            L" Heading",

            L" Thin Italic",
            L" ExtraLight Italic",
            L" Extra Light Italic",
            L" SemiLight Italic",
            L" Semi Light Italic",
            L" Light Oblique",
            L" Light Italic",
            L" Regular Italic",
            L" Medium Italic",
            L" SemiBold Italic",
            L" Semi Bold Italic",
            L" Bold Oblique",
            L" Bold Italic",
            L" ExtraBold Italic",
            L" Extra Bold Italic",
            L" Black Italic",
            L" Oblique",
            L" Italic",

            L" Bold Condensed Italic",
            L" Bold Condensed",
            L" Condensed Italic",
            L" ExtraCondensed Bold Italic",
            L" ExtraCondensed Bold",
            L" ExtraCondensed Italic",
            L" ExtraCondensed",
            L" Light Condensed",
            L" Medium Condensed",
            L" SemiBold Condensed",
            L" ExtraBold Condensed",
            L" Black Condensed",
            L" Condensed",

            L" Regular",
            L" Thin",
            L" ExtraLight",
            L" Extra Light",
            L" SemiLight",
            L" Semi Light",
            L" Light",
            L" Medium",
            L" SemiBold",
            L" Semi Bold",
            L" DemiBold",
            L" Demi Bold",
            L" ExtraBold",
            L" Extra Bold",
            L" Bold",
            L" ExtraBlack",
            L" Ultra Black",
            L" Black",
            L" Heavy",
        };

        for(auto bad : badEndings)
        {
            if (name.ends_with(bad))
                return name.substr(0, name.length() - bad.length());
        }
        return name;
    }
private:
    static std::wstring GetFamilyNameByEnuming(const std::wstring& fullFaceName)
    {
        struct Context
        {
            const std::wstring& fullFaceName;
            std::wstring        familyName;
            bool                matched = false;
        } ctx{fullFaceName};

        LOGFONTW query{};
        query.lfCharSet = DEFAULT_CHARSET;
        query.lfFaceName[0] = L'\0';

        HDC hdc = ::GetDC(nullptr);
        ::EnumFontFamiliesExW(hdc, &query,  [](const LOGFONTW* enumLf, const TEXTMETRICW*, DWORD, LPARAM lParam) -> int
                                            {
                                                auto& ctx = *reinterpret_cast<Context*>(lParam);
                                                auto* elf = reinterpret_cast<const ENUMLOGFONTEXW*>(enumLf);

                                                if (_wcsicmp(elf->elfFullName, ctx.fullFaceName.c_str()) != 0)
                                                    return 1;

                                                std::wstring fullName = elf->elfFullName;
                                                std::wstring style = elf->elfStyle;
                                                std::wstring suffix = L" " + style;

                                                if (!style.empty() &&
                                                    fullName.size() > suffix.size() &&
                                                    fullName.compare(fullName.size() - suffix.size(),
                                                        suffix.size(), suffix) == 0)
                                                    ctx.familyName = fullName.substr(0, fullName.size() - suffix.size());
                                                else
                                                    ctx.familyName = fullName;

                                                ctx.matched = true;
                                                return 0;
                                            }, reinterpret_cast<LPARAM>(&ctx), 0);
        ::ReleaseDC(nullptr, hdc);

        return ctx.matched ? ctx.familyName : fullFaceName;
    }
};

#endif
