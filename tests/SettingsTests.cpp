#include "..\src\Settings.h"

import std;
import tdd20;

#include "Automation.h"

using namespace std::chrono_literals;
using namespace TDD20;

namespace TDD20
{
    template <> inline std::string ToString(const std::nullptr_t&) { return "0x0"; }
}

class RegistryRestorer
{
    std::wstring fontName;
    int          fontHeight;
    bool         isMaximized;
    RECT         windowExtents;
public:
    RegistryRestorer() : fontName     (Settings::Registry::GetFontName())
                       , fontHeight   (Settings::Registry::GetFontHeight())
                       , isMaximized  (Settings::Registry::IsMaximized())
                       , windowExtents(Settings::Registry::GetWindowExtents())
    {}
   ~RegistryRestorer()
    {
       Settings::Registry::SetFontName(fontName);
       Settings::Registry::SetFontHeight(fontHeight);
       Settings::Registry::SetMaximized(isMaximized);
       Settings::Registry::SetWindowExtents(windowExtents);
    }
};

Test SettingsTests[] = {
    { std::string("RegistryRestorer compiles"), []()
        {
            RegistryRestorer rr;
        }
    },
    { std::string("Closing notepad saves window position"), []()
        {
            RegistryRestorer rr;
            {
                TestAutomation::MainWindow main;
                main.MoveWindow(150, 151, 600, 400);
                main.ExitViaMenu();
            }
            RECT r{Settings::Registry::GetWindowExtents()};

            Assert::AreEqual(150, r.left,     "x position should have been written to registry");
            Assert::AreEqual(151, r.top,      "y position should have been written to registry");
            Assert::AreEqual(600, r.right-r.left,  "width should have been written to registry");
            Assert::AreEqual(400, r.bottom-r.top, "height should have been written to registry");
        }
    },
    { std::string("Initial window placement is read from registry"), []()
        {
            RegistryRestorer rr;
            {
                TestAutomation::MainWindow main;
                main.MoveWindow(0, 1, 500, 300);
                main.ExitViaMenu();
            }
            {
                TestAutomation::MainWindow main;
                
                WINDOWPLACEMENT wp{};
                wp.length = sizeof(WINDOWPLACEMENT);
                ::GetWindowPlacement(main.hwnd, &wp);

                main.ExitViaMenu();

                Assert::AreEqual(    0, wp.rcNormalPosition.left,     "left position should have been read from registry");
                Assert::AreEqual(    1, wp.rcNormalPosition.top,       "top position should have been read from registry");
                Assert::AreEqual(300+1, wp.rcNormalPosition.bottom, "bottom position should have been read from registry");
                Assert::AreEqual(500+0, wp.rcNormalPosition.right,   "right position should have been read from registry");
            }
        }
    },
    { std::string("Maximized window placement is read from registry"), []()
        {
            RegistryRestorer rr;
            {
                TestAutomation::MainWindow main;
                ShowWindow(main.hwnd, SW_SHOWMAXIMIZED);
                main.ExitViaMenu();
            }
            {
                TestAutomation::MainWindow main;
                
                WINDOWPLACEMENT wp{};
                wp.length = sizeof(WINDOWPLACEMENT);
                ::GetWindowPlacement(main.hwnd, &wp);

                main.ExitViaMenu();

                Assert::AreEqual(SW_SHOWMAXIMIZED, wp.showCmd, "notepad-- should have shown up maximized");
            }
        }
    },
    { std::string("Current font is written and read from the registry"), []()
        {
            RegistryRestorer rr;
            {
                TestAutomation::MainWindow main;
                main.GetMenu().GetFormatMenu().SelectMenuItem(IDM_FONT);
                auto font = main.FindExistingChooseFontDialog();
                font.SetFontSize(20);
                Poll::Until(1s, 1ms, [&]()  {
                                                font.SetFont(L"Consolas");
                                                return font.GetFontName() == L"Consolas";
                                            });
                font.PressOK();
                main.ExitViaMenu();

                Poll::Until(1s, 1ms, []() { return L"Consolas" == Settings::Registry::GetFontName(); });
                Assert::AreEqual(L"Consolas", Settings::Registry::GetFontName(), "after exiting, font name should have been written to registry");
            }
            {
                TestAutomation::MainWindow main;
            //  main.GetMenu().GetFormatMenu().SelectMenuItem(IDM_FONT);

                HDC  hdc = ::GetDC(nullptr);
                int  dpiY = ::GetDeviceCaps(hdc, LOGPIXELSY);
                ::ReleaseDC(nullptr, hdc);

                std::wstring fontName;
                LONG fontSize;
                Poll::Until(1s, 1ms, [&]()  {
                                                main.GetEditField().GetFontNameAndSize(fontName, fontSize);
                                                int  pointSize = ::MulDiv(-fontSize, 72, dpiY);
                                                return pointSize == 20;
                                            });
                main.ExitViaMenu();

                int  pointSize = ::MulDiv(-fontSize, 72, dpiY);
                Assert::AreEqual(20,         pointSize, "font size should be read from registry");
                Assert::AreEqual(L"Consolas", fontName, "font name should be read from registry");
            }
        }
    },
};
