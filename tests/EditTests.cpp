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

Test EditTests[] = {
    { std::string("When firing up Notepad--, the Edit->Undo menu item is grayed out"), []()
        {
            TestAutomation::MainWindow main;

            auto editMenu = main.GetMenu().GetEditMenu();
            Assert::IsFalse(editMenu.IsMenuItemEnabled(IDM_UNDO));

            main.ExitViaMenu();
        }
    },
    { std::string("After typing some text, the Edit->Undo menu item is not grayed out"), []()
        {
            TestAutomation::MainWindow main;
            auto edit = main.GetEditField();
            edit.SetText(L"After typing some text, the Edit->Undo menu item is not grayed out");
            auto editMenu = main.GetMenu().GetEditMenu();
            edit.ClearDirtyFlag();
            Assert::IsTrue(editMenu.IsMenuItemEnabled(IDM_UNDO));
            main.ExitViaMenu();
        }
    },
    { std::string("After typing a char, undo reverts to blank"), []()
        {
            TestAutomation::MainWindow main;
            auto edit = main.GetEditField();
            edit.SetText(L"X");
            Assert::AreEqual(L"X", edit.GetText(), "should have single X character");

            main.Undo();
            edit.ClearDirtyFlag();
            Poll::While(100ms, 1ms, [&edit]() { return edit.GetText() == L"X"; });

            Assert::AreEqual(L"", edit.GetText(), "should have applied Undo");

            main.ExitViaMenu();
        }
    },
    { std::string("type a char, undo, type a char, undo => blank"), []()
        {
            TestAutomation::MainWindow main;
            auto edit = main.GetEditField();
            edit.SetText(L"A");
            Poll::While(100ms, 1ms, [&edit]() { return edit.GetText() == L"A"; });
            Assert::AreEqual(L"A", edit.GetText(), "should have single A character");
            main.Undo();

            edit.SetText(L"B");
            Poll::While(100ms, 1ms, [&edit]() { return edit.GetText() == L"B"; });
            Assert::AreEqual(L"B", edit.GetText(), "should have single B character");
            main.Undo();

            edit.ClearDirtyFlag();
            Assert::AreEqual(L"", edit.GetText(), "should have applied Undo");
            main.ExitViaMenu();
        }
    },
};
