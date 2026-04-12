#include <windows.h>
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

Test FontTests[] = {
    { std::string("Can open Choose Font dialog"), []()
        {
            TestAutomation::MainWindow main;
            main.GetMenu().GetFormatMenu().SelectMenuItem(IDM_FONT);
            auto font = main.FindExistingChooseFontDialog();
            font.Cancel();
            main.ExitViaMenu();
        }
    },
    { std::string("Opening the Font dialog and clicking OK changes nothing"), []()
        {
            TestAutomation::MainWindow main;
            auto edit = main.GetEditField();

            std::wstring oldFontName;
            LONG         oldFontSize{};
            edit.GetFontNameAndSize(oldFontName, oldFontSize);

            main.GetMenu().GetFormatMenu().SelectMenuItem(IDM_FONT);
            auto font = main.FindExistingChooseFontDialog();
            font.PressOK();

            Poll::While(1s, 1ms, [&]() { return font.IsVisible(); });
            std::wstring newFontName;
            LONG         newFontSize{};
            edit.GetFontNameAndSize(newFontName, newFontSize);

            Assert::AreEqual(oldFontName, newFontName, "font names should not have changed");
            Assert::AreEqual(oldFontSize, newFontSize, "font sizes should not have changed");

            main.ExitViaMenu();
        }
    },
    { std::string("Increasing the font size works"), []()
        {
            TestAutomation::MainWindow main;
            auto edit = main.GetEditField();

            std::wstring oldFontName;
            LONG         oldFontSize{};
            edit.GetFontNameAndSize(oldFontName, oldFontSize);

            main.GetMenu().GetFormatMenu().SelectMenuItem(IDM_FONT);
            auto font = main.FindExistingChooseFontDialog();

            int fontSize;
            Poll::Until(1s, 1ms, [&]()  {
                                            try { fontSize = font.GetFontSize(); }
                                            catch (...) { return false; }
                                            return true;
                                        });
            font.SetFontSize(fontSize+1);
            font.PressOK();

            Poll::While(1s, 1ms, [&]() { return font.IsVisible(); });
            std::wstring newFontName;
            LONG         newFontSize{};
            edit.GetFontNameAndSize(newFontName, newFontSize);
            main.ExitViaMenu();

            Poll::While(1s, 1ms, [&]() { return Settings::Registry::GetFontHeight() == fontSize  ; }); // wait for Notepad-- to start writing its settings
            Poll::While(1s, 1ms, [&]() { return Settings::Registry::GetFontHeight() == fontSize+1; }); // wiat for Notepad-- to finish writing its settings
            Settings::Registry::SetFontHeight(fontSize); // change it back so we don't keep growing the text height

            Assert::AreEqual(oldFontName,   newFontName, "font names should not have changed");
            Assert::AreEqual(oldFontSize-1, newFontSize, "font size should have changed"); // oldFontSize was -16. After the change, it's -17 (rounded, close enough).
        }
    },
};
