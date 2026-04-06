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
            GetObject(hCurrentFont, sizeof(LOGFONT), &lf);
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
};

#endif
