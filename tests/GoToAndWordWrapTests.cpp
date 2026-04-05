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

Test WordWrapTests[] = {
    { std::string("The Word Wrap menu item checkbox is unchecked by default"), []()
        {
            TestAutomation::MainWindow main;
            Assert::IsFalse(main.GetMenu().GetFormatMenu().IsMenuItemChecked(IDM_WORDWRAP));
            main.ExitViaMenu();
        }
    },
    { std::string("Toggling the sense of the Word Wrap menu item checkbox changes the style bits of the new edit window"), []()
        {
            TestAutomation::MainWindow main;

            auto edit = main.GetEditField();
            Assert::AreEqual(WS_HSCROLL|ES_AUTOHSCROLL, edit.GetStyle() & (WS_HSCROLL|ES_AUTOHSCROLL), "style should include WS_HSCROLL and ES_AUTOHSCROLL"); // unchecked by default

            main.GetMenu().GetFormatMenu().SelectMenuItem(IDM_WORDWRAP);            // toggle wordwrap
            Poll::While(1s, 1ms, [&]() { return edit.IsWindow(); });                // edit window will be destroyed and re-created
            Poll::Until(1s, 1ms, [&]() { return main.GetEditField().IsWindow(); }); // wait until new edit window is created
            edit = main.GetEditField(); // get new one
            Assert::AreEqual(0, edit.GetStyle() & (WS_HSCROLL | ES_AUTOHSCROLL), "style should NOT include WS_HSCROLL and ES_AUTOHSCROLL");

            main.ExitViaMenu();
        }
    },
};

Test GotoTests[] = {
    { std::string("The 'Go To...' menu option is enabled only when word wrap is off"), []()
        {
            TestAutomation::MainWindow main;

            auto format = main.GetMenu().GetFormatMenu();

            Assert::IsFalse(format.IsMenuItemChecked(IDM_WORDWRAP)); // my default is unchecked
            Assert::IsTrue (main.GetMenu().GetEditMenu(  ).IsMenuItemEnabled(IDM_GOTO), "GoTo menu item should be enabled");

            format.SelectMenuItem(IDM_WORDWRAP); // flip the sense of the word wrap
            Poll::While(1s, 1ms, [&]() { return main.GetMenu().GetEditMenu().IsMenuItemEnabled(IDM_GOTO) == true; });
            Assert::IsFalse(main.GetMenu().GetEditMenu().IsMenuItemEnabled(IDM_GOTO), "GoTo menu item should be disabled");

            main.ExitViaMenu();
        }
    },
    { std::string("Menu can pop up the 'Go To...' dialogbox"), []()
        {
            TestAutomation::MainWindow main;

            main.GetMenu().GetEditMenu().SelectMenuItem(IDM_GOTO);
            auto goTo = main.FindExistingGoToDialog();
            goTo.Cancel();

            main.ExitViaMenu();
        }
    },
    { std::string("If nothing is typed into GoTo's edit field, the OK button press is rejected"), []()
        {
            TestAutomation::MainWindow main;

            auto edit = main.GetEditField();
            edit.SetText(L"123");
            edit.SetCursorPosition(1, 2);
            edit.ClearDirtyFlag();

            main.GetMenu().GetEditMenu().SelectMenuItem(IDM_GOTO);
            auto goTo = main.FindExistingGoToDialog();
            goTo.PressOk();

            Assert::IsTrue(goTo.IsVisible(), "Pressing Ok is rejected when edit field is blank");

            goTo.SetValue(L"0");
            goTo.PressOk();

            Poll::While(1s, 1ms, [&]() { return goTo.IsVisible(); });
            Assert::IsFalse(goTo.IsVisible(), "Pressing Ok is accepted when edit field is not blank");

            DWORD s, e;
            edit.GetCursorPosition(s, e);
            Assert::AreEqual(0, s, "start position should be 0");
            Assert::AreEqual(0, e,   "end position should be 0");

            main.ExitViaMenu();
        }
    },
    { std::string("If the user types a huge number into GoTo's edit field, the cursor moves to the last position"), []()
        {
            TestAutomation::MainWindow main;

            auto edit = main.GetEditField();
            edit.SetText(L"123");
            edit.SetCursorPosition(1, 2);
            edit.ClearDirtyFlag();

            main.GetMenu().GetEditMenu().SelectMenuItem(IDM_GOTO);
            auto goTo = main.FindExistingGoToDialog();
            goTo.SetValue(L"11111111111111111111111");
            goTo.PressOk();

            Poll::While(1s, 1ms, [&]() { return goTo.IsVisible(); });
            Assert::IsFalse(goTo.IsVisible(), "Pressing Ok is accepted when edit field is not blank");

            DWORD s, e;
            edit.GetCursorPosition(s, e);
            Assert::AreEqual(3, s, "start position should be 3");
            Assert::AreEqual(3, e,   "end position should be 3");

            main.ExitViaMenu();
        }
    },
    { std::string("GoTo dialog causes cursor to jump to selected line"), []()
        {
            TestAutomation::MainWindow main;

            auto edit = main.GetEditField();
            edit.AppendText(L"0\n");
            edit.AppendText(L"1\n");
            edit.AppendText(L"2\n");
            edit.AppendText(L"3\n");
            edit.AppendText(L"4\n");
            edit.AppendText(L"5\n");
            edit.AppendText(L"6\n");
            edit.AppendText(L"7\n");
            edit.AppendText(L"8\n");
            edit.AppendText(L"9\n");
            edit.ClearDirtyFlag();

            main.GetMenu().GetEditMenu().SelectMenuItem(IDM_GOTO);
            auto goTo = main.FindExistingGoToDialog();
            goTo.SetValue(L"6");
            goTo.PressOk();

            Poll::While(1s, 1ms, [&]() { return goTo.IsVisible(); });
            Assert::IsFalse(goTo.IsVisible(), "Pressing Ok is accepted when edit field is not blank");

            DWORD s, e;
            edit.GetCursorPosition(s, e);
            Assert::AreEqual(18, s, "start position should be 18");
            Assert::AreEqual(18, e,   "end position should be 18");

            main.ExitViaMenu();
        }
    },
};
