#include "..\src\PrintEngine.h"

import std;
import tdd20;

#include <windows.h>
#include "Automation.h"

using namespace std::chrono_literals;
using namespace TDD20;

namespace TDD20
{
    template <> inline std::string ToString(const std::nullptr_t&) { return "0x0"; }
}

class Blitter
{
    HDC        hdc, memdc;
    HBITMAP    bmp, oldBmp;
    HFONT     font, oldFont;
public:
    Blitter()
    {
        hdc = CreateDC(L"DISPLAY", nullptr, nullptr, nullptr);
        memdc = CreateCompatibleDC(hdc);
        bmp = CreateCompatibleBitmap(hdc, 1920, 1080);
        oldBmp = (HBITMAP)SelectObject(memdc, bmp);
        font = CreateFontW(-16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, L"Courier New");
        oldFont = (HFONT)SelectObject(memdc, font);

        // set up memdc (optional, but makes it easier to see)
        RECT r{ 0,0,1920,1080 };
        FillRect(memdc, &r, (HBRUSH)GetStockObject(WHITE_BRUSH));
    }
    ~Blitter()
    {
        SelectObject(memdc, oldFont);
        DeleteObject(font);
        SelectObject(memdc, oldBmp);
        DeleteObject(bmp);
        DeleteDC(memdc);
        DeleteDC(hdc);
    }
    void BitBlt() { ::BitBlt(hdc, 0, 0, 1920, 1080, memdc, 0, 0, SRCCOPY); } // for visual inspection
    HDC GetDC() const { return memdc; }
};
struct TextOutParams
{
    int x, y;
    std::wstring string;
    int c;
};
static int count;
static TextOutParams params[2];
struct TestBase : Empty
{
    static int GetDeviceCaps(HDC hdc, int index)
    {
        if (index == HORZRES)
            return 1920;
        return ::GetDeviceCaps(hdc, index);
    }
    static BOOL TextOutW(HDC hdc, int x, int y, LPCWSTR lpString, int c)
    {
        if (count < 2)
            params[count] = { x,y,std::wstring(lpString,c),c }; // so it doesn't crash
        ++count;
        ::TextOutW(hdc, x, y, lpString, c); // call the real one so I can see something on-screen
        return TRUE;
    }
};

Test FilePrintTests[] = {
//#if _DEBUG
    { std::string("File->Print pops up Print Dialog (then WM_CLOSE)"), []()
        {
            TestAutomation::MainWindow main;
            auto edit = main.GetEditField();
            edit.SetText(L"File->Print pops up Print Dialog(then WM_CLOSE)");
            edit.ClearDirtyFlag();

            auto print = main.Print();
            print.Cancel();

            main.ExitViaMenu();
        }
    },
//#endif
    { std::string("Just measuring my screen (for now) including margins"), []()
        {
            HDC hdc = GetDC(nullptr);
            PrintEngine::PrintToHdc(hdc, std::wstring(129, L'W') + L'X');
            ReleaseDC(nullptr, hdc);
        }
    },
    { std::string("No spaces just breaks the word at the right spot"), []()
        {
            count = 0;
            params[0] = {};
            params[1] = {};

            Blitter blitter;
            PrintEngineT<TestBase>::PrintToHdc(blitter.GetDC(), std::wstring(182, L'W') + L'X');
            blitter.BitBlt();

            Assert::AreEqual(2, count, "should have been one line break => two lines");

            Assert::AreEqual(48, params[0].x, "should be same as margin");
            Assert::AreEqual(48, params[0].y, "should be same as margin");
            Assert::AreEqual(182, params[0].c, "a full line is 182 characters");

            Assert::AreEqual(48, params[1].x, "should be same as margin");
            Assert::AreEqual(66, params[1].y, "should be margin plus height of a line");
            Assert::AreEqual(1, params[1].c, "one character on the next line");
            Assert::AreEqual(L"X", params[1].string, "the string should consist of a single 'X'");
        }
    },
    { std::string("Line breaks at the last space"), []()
        {
            count = 0;
            params[0] = {};
            params[1] = {};

            std::wstring text(170, L'W');
            text = text + L" ABCDEFGHIJKL";

            Blitter blitter;
            PrintEngineT<TestBase>::PrintToHdc(blitter.GetDC(), text);
            blitter.BitBlt();

            Assert::AreEqual(2, count, "should have been one line break => two lines");

            Assert::AreEqual(48, params[0].x, "should be same as margin");
            Assert::AreEqual(48, params[0].y, "should be same as margin");
            Assert::AreEqual(171, params[0].c, "the space is at position 170");

            Assert::AreEqual(48, params[1].x, "should be same as margin");
            Assert::AreEqual(66, params[1].y, "should be margin plus height of a line");
            Assert::AreEqual(12, params[1].c, "should be 12 chars after the ' '");
            Assert::AreEqual(L"ABCDEFGHIJKL", params[1].string, "remainder should be ABCDEFGHIJKL");
        }
    },
    { std::string("Line breaks at hyphenated word"), []()
        {
            count = 0;
            params[0] = {};
            params[1] = {};

            std::wstring text(170, L'W');
            text = text + L"-ABCDEFGHIJKL";

            Blitter blitter;
            PrintEngineT<TestBase>::PrintToHdc(blitter.GetDC(), text);
            blitter.BitBlt();

            Assert::AreEqual(2, count, "should have been one line break => two lines");

            Assert::AreEqual(48, params[0].x, "should be same as margin");
            Assert::AreEqual(48, params[0].y, "should be same as margin");
            Assert::AreEqual(171, params[0].c, "the hyphen is at position 170");
            Assert::AreEqual(L'-', params[0].string[170], "there should be - right here");

            Assert::AreEqual(48, params[1].x, "should be same as margin");
            Assert::AreEqual(66, params[1].y, "should be margin plus height of a line");
            Assert::AreEqual(12, params[1].c, "should be 12 chars after the ' '");
            Assert::AreEqual(L"ABCDEFGHIJKL", params[1].string, "remainder should be ABCDEFGHIJKL");
        }
    },
    { std::string("Really long URLs break properly"), []()
        {
            count = 0;
            params[0] = {};
            params[1] = {};

            std::wstring text(147, L'W');
            text = L"https://www.google.com/" + text + L"/ABCDEFGHIJKL.html";

            Blitter blitter;
            PrintEngineT<TestBase>::PrintToHdc(blitter.GetDC(), text);
            blitter.BitBlt();

            Assert::AreEqual(2, count, "should have been one line break => two lines");
            Assert::AreEqual(171, params[0].c,           "the slash is at position 170");
            Assert::AreEqual(L'/', params[0].string[170], "there should be / right here");

            Assert::AreEqual(L"ABCDEFGHIJKL.html", params[1].string);
        }
    },
    { std::string("Part of tab running off past margin on the right is ok"), []()
        {
            count = 0;
            params[0] = {};
            params[1] = {};

            std::wstring text(180, L'W');
            text = text + L"\tABCDEFGHIJKL";

            Blitter blitter;
            PrintEngineT<TestBase>::PrintToHdc(blitter.GetDC(), text);
            blitter.BitBlt();

            Assert::AreEqual(2, count, "should have been one line break => two lines");
            Assert::AreEqual(181, params[0].c,            "the tab is at position 170");
            Assert::AreEqual(L'\t', params[0].string[180], "there should be tab right here");

            Assert::AreEqual(L"ABCDEFGHIJKL", params[1].string);
        }
    },
    { std::string("each page of a multi-page doc has at most 54 lines"), []()
        {
            std::wstring text;
            for (int j = 0; j < 107; ++j) // (1080 - 48 - 48)/(66-48) = 54.66666
            for (int i = 0; i < 182 / 5; ++i)
                text += L"abcd ";

            static int pages;
            pages = 0;
            struct TestBase : Empty
            {
                static int GetDeviceCaps(HDC hdc, int index)
                {
                    switch (index) {
                    case HORZRES: return 1920;
                    case VERTRES: return 1080;
                    default:      return ::GetDeviceCaps(hdc, index);
                    }
                }
                static int EndPage(HDC) { return ++pages; }
                static BOOL GetTextMetricsW(HDC, TEXTMETRICW* tm)
                {
                    *tm = TEXTMETRICW{};
                    tm->tmHeight = 18;
                    return TRUE;
                }
            };

            Blitter blitter;
            PrintEngineT<TestBase>::PrintToHdc(blitter.GetDC(), text);
            blitter.BitBlt();

            Assert::AreEqual(2, pages, "107 lines of text is 2 pages, exactly");
        }
    },
};

