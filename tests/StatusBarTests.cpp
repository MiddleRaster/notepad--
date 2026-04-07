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

Test StatusBarTests[] = {
    { std::string("StatusBar is visible by default"), []()
        {
            TestAutomation::MainWindow main;

            auto statusBar = main.GetStatusBar();
            Poll::Until(1s, 1ms, [&]() { return statusBar.GetText() != L""; });
            Assert::AreEqual(L"Ln 0, Col 0", statusBar.GetText());
            Assert::IsTrue(statusBar.IsVisible(), "status bar should be visible by default");

            main.ExitViaMenu();
        }
    },
    { std::string("StatusBar updates when cursor position moves"), []()
        {
            TestAutomation::MainWindow main;

            auto edit = main.GetEditField();
            edit.SetText(L"StatusBar updates when cursor position moves");
            edit.SetCursorPosition(20, 20);
            edit.ClearDirtyFlag();

            auto statusBar = main.GetStatusBar();
            Poll::Until(1s, 1ms, [&]() { return statusBar.GetText() == L"Ln 0, Col 20"; });
            Assert::AreEqual(L"Ln 0, Col 20", statusBar.GetText());

            main.ExitViaMenu();
        }
    },
    { std::string("StatusBar is invisible if wordwrap is on"), []()
        {
            TestAutomation::MainWindow main;

            main.GetMenu().GetFormatMenu().SelectMenuItem(IDM_WORDWRAP);
            auto statusBar = main.GetStatusBar();
            Poll::While(1s, 1ms, [&]() { return statusBar.IsVisible(); });
            Assert::IsFalse(statusBar.IsVisible(), "status bar should be visible by default");

            main.ExitViaMenu();
        }
    },
};
