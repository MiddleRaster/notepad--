#ifndef PRINTENGINE_H
#define PRINTENGINE_H

#pragma once
#include <windows.h>
#include <string>

#include "MockableFunction.h"


struct Empty
{
    MOCKABLE_FUNCTION(TextOutW)
    MOCKABLE_FUNCTION(GetDeviceCaps)
    MOCKABLE_FUNCTION(EndPage)
};

template <typename Base>
struct PrintEngineT : private Base
{
    using Base::TextOutW;
    using Base::GetDeviceCaps;
    using Base::EndPage;

    static int CalcLineBreak(HDC hdc, const wchar_t* text, int len, int pageWidthPx)
    {
        int lo = 1, hi = len, fit = 1;
        while (lo <= hi)
        {
            int  mid = (lo + hi) / 2;
            SIZE size{};
            GetTextExtentPoint32W(hdc, text, mid, &size);
            if (size.cx <= pageWidthPx) {
                fit = mid; 
                lo = mid + 1; 
            } else
                hi = mid - 1;
        }

        if (fit < len) // If we didn't reach end of segment, back up to the last space or hyphen, etc.
        {
            std::wstring_view sv(text, fit);
            auto pos = sv.find_last_of(L" -\t/");
            if (pos != std::wstring_view::npos)
                fit = static_cast<int>(pos) + 1;
        }
        return fit;
    }

    static void PrintToHdc(HDC hdc, const std::wstring& text)
    {
        TEXTMETRICW tm{};
        GetTextMetricsW(hdc, &tm);
        int lineH      = tm.tmHeight + tm.tmExternalLeading;
        int marginX    = GetDeviceCaps(hdc, LOGPIXELSX)/2;  // 0.5" margin in pixels
        int marginY    = GetDeviceCaps(hdc, LOGPIXELSY)/2;
        int pageW      = GetDeviceCaps(hdc, HORZRES) - 2*marginX;
        int pageH      = GetDeviceCaps(hdc, VERTRES) - 2*marginY;
        int linesPerPg = lineH > 0 ? pageH/lineH : 1;

        int pos = 0;
        int n = static_cast<int>(text.size());
        int lineOnPage = 0;

        StartPage(hdc);
        while (pos < n)
        {
            int nlPos = static_cast<int>(text.find(L'\n', pos));
            int logEnd = (nlPos == static_cast<int>(std::wstring::npos)) ? n : nlPos;

            int seg = pos;
            while (seg < logEnd || seg == pos)
            {
                int fit = CalcLineBreak(hdc, text.c_str() + seg, logEnd - seg, pageW);
                TextOutW(hdc, marginX, marginY + lineOnPage*lineH, text.c_str() + seg, fit);
                seg += fit;

                if (++lineOnPage >= linesPerPg)
                {
                    lineOnPage = 0;
                    EndPage(hdc);
                    StartPage(hdc);
                }
                if (seg >= logEnd) break;
            }
            pos = logEnd + 1;
        }
        EndPage(hdc);
    }
};
using PrintEngine = PrintEngineT<Empty>;

#endif
