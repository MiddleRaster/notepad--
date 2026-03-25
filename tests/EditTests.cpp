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

Test FocusTest[] = {
    { std::string("Whenever Notepad-- is in the foreground, the edit window has focus"), []()
        {
            TestAutomation::MainWindow main;

            auto edit = main.GetEditField();
            Assert::IsTrue(edit.HasFocus(), "the edit window should have focus");

            main.ExitViaMenu();
        }
    },
};

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
            Poll::While(100ms, 1ms, [&edit]() { return edit.GetText() == L"X"; });
            Assert::AreEqual(L"", edit.GetText(), "should have undone single X character");

            edit.ClearDirtyFlag();
            Poll::While(100ms, 1ms, [&edit]() { return edit.IsDirty(); });
            Assert::IsFalse(edit.IsDirty(), "edit field should not be dirty at this point");
            
            main.ExitViaMenu();
        }
    },
    { std::string("type a char, undo, type a char, undo => blank"), []()
        {
            TestAutomation::MainWindow main;
            auto edit = main.GetEditField();

            edit.SetText(L"A");
            Assert::AreEqual(L"A", edit.GetText(), "should have single A character");
            main.Undo();
            Poll::While(100ms, 1ms, [&edit]() { return edit.GetText() == L"A"; });
            Assert::AreEqual(L"", edit.GetText(), "should have undone single A character");

            edit.SetText(L"B");
            Assert::AreEqual(L"B", edit.GetText(), "should have single B character");
            main.Undo();
            Poll::While(100ms, 1ms, [&edit]() { return edit.GetText() == L"B"; });
            Assert::AreEqual(L"", edit.GetText(), "should have undone single B character");

            edit.ClearDirtyFlag();
            Poll::While(100ms, 1ms, [&edit]() { return edit.IsDirty(); });
            Assert::IsFalse(edit.IsDirty(), "edit field should not be dirty at this point");

            main.ExitViaMenu();
        }
    },
    { std::string("After an undo, the caret is at the end of the remaining string"), []()
        {
            TestAutomation::MainWindow main;
            auto edit = main.GetEditField();

            edit.SetText(L"ABCDEFG");
            Assert::AreEqual(L"ABCDEFG", edit.GetText(), "should have ABCDEFG");

            edit.AppendText(L"X");
            main.Undo();
            Poll::While(100ms, 1ms, [&edit]() { return edit.GetText() == L"ABCDEFG"; });
            Assert::AreEqual(L"ABCDEFG", edit.GetText(), "should have undone the single X character");

            DWORD start, end;
            edit.GetCursorPosition(start, end);

            Assert::AreEqual(start, end, "cursor start and end should be the same");
            Assert::AreEqual(7, start, "cursor should be at the end: 7");

            edit.ClearDirtyFlag();
            Poll::While(100ms, 1ms, [&edit]() { return edit.IsDirty(); });
            Assert::IsFalse(edit.IsDirty(), "edit field should not be dirty at this point");

            main.ExitViaMenu();
        }
    },
};
