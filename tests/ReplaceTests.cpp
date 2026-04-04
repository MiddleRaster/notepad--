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

Test ReplaceTests[] = {
    { std::string("The Edit->Replace menu item not is enabled if no text in edit field"), []()
        {
            TestAutomation::MainWindow main;
            Assert::IsFalse(main.GetMenu().GetEditMenu().IsMenuItemEnabled(IDM_REPLACE), "the Edit->Replace menu item should be disabled");
        }
    },
    { std::string("The Edit->Replace menu item is enabled whenever there is text in the edit field"), []()
        {
            TestAutomation::MainWindow main;

            auto edit = main.GetEditField();
            edit.SetText(L"The Edit->Replace menu item is enabled whenever there is text in the edit field");
            edit.ClearDirtyFlag();
            Poll::While(1s, 1ms, [&edit]() { return edit.IsDirty(); });

            Assert::IsTrue(main.GetMenu().GetEditMenu().IsMenuItemEnabled(IDM_REPLACE), "the Edit->Replace menu item should be enabled");
        }
    },
    { std::string("User can replace the next item by using the menus to select '&Replace'"), []()
        {
            TestAutomation::MainWindow main;
            auto edit = main.GetEditField();
            edit.SetText(L"User can replace the next item by using the menus to select '&Replace'");
            edit.ClearDirtyFlag();
            Poll::While(1s, 1ms, [&edit]() { return edit.IsDirty(); });

            auto editMenu = main.GetMenu().GetEditMenu();
            editMenu.SelectMenuItem(IDM_REPLACE);

            auto replace = main.FindExistingReplaceDialog();
            replace.Cancel();

            main.ExitViaMenu();
        }
    },
    { std::string("Given text in the edit field, 'aaa', when the user selects an 'a', he can replace it with 'b'"), []()
        {
            TestAutomation::MainWindow main;
            auto edit = main.GetEditField();
            edit.SetText(L"aaa");
            edit.SetCursorPosition(0,0);
            edit.ClearDirtyFlag();
            Poll::While(1s, 1ms, [&edit]() { return edit.IsDirty(); });

            auto editMenu = main.GetMenu().GetEditMenu();
            editMenu.SelectMenuItem(IDM_REPLACE);
            auto replace = main.FindExistingReplaceDialog();

            replace.SetFindText   (L"a");
            replace.SetReplaceText(L"b");
            replace.Replace();

            Poll::Until(1s, 1ms, [&]() { return edit.GetText() == L"baa"; });
            Assert::AreEqual(L"baa", edit.GetText(), "first 'a' should have been replace with 'b'");

            Assert::IsTrue(edit.IsDirty(), "edit field should be dirty");

            edit.ClearDirtyFlag();
            Poll::While(1s, 1ms, [&edit]() { return edit.IsDirty(); });
            main.ExitViaMenu();
        }
    },
    { std::string("Cannot replace past end of text:  messagebox pops up"), []()
        {
            TestAutomation::MainWindow main;
            auto edit = main.GetEditField();
            edit.SetText(L"aa");
            edit.SetCursorPosition(0, 0);
            edit.ClearDirtyFlag();
            Poll::While(1s, 1ms, [&edit]() { return edit.IsDirty(); });

            auto editMenu = main.GetMenu().GetEditMenu();
            editMenu.SelectMenuItem(IDM_REPLACE);
            auto replace = main.FindExistingReplaceDialog();

            replace.SetFindText   (L"a");
            replace.SetReplaceText(L"b");
            replace.Replace(); // this one will work
            Poll::Until(1s, 1ms, [&]() { return edit.GetText() == L"ba"; });
            Assert::AreEqual(L"ba", edit.GetText(), "one 'a' should have been replaced with a 'b'");

            replace.Replace(); // so will this one
            Poll::Until(1s, 1ms, [&]() { return edit.GetText() == L"bb"; });
            Assert::AreEqual(L"bb", edit.GetText(), "a second 'a' should have been replaced with a 'b'");

            replace.Replace(); // this one will pop up a messagebox

            auto mb = main.FindExistingUnfoundMessageBox();
            Assert::AreEqual(L"Cannot find 'a'", mb.GetText(), "messagebox's static text is wrong");
            mb.PushOkButton();

            edit.ClearDirtyFlag();
            Poll::While(1s, 1ms, [&edit]() { return edit.IsDirty(); });
            replace.Cancel();
            main.ExitViaMenu();
        }
    },
    { std::string("Whole word replacing works"), []()
        {
            TestAutomation::MainWindow main;

            auto edit = main.GetEditField();
            edit.SetText(L" aa aaaaa");
            edit.SetCursorPosition(0, 0);

            edit.ClearDirtyFlag();
            Poll::While(1s, 1ms, [&edit]() { return edit.IsDirty(); });

            auto editMenu = main.GetMenu().GetEditMenu();
            editMenu.SelectMenuItem(IDM_REPLACE);
            auto replace = main.FindExistingReplaceDialog();

            replace.SetFindText   (L"aa");
            replace.SetReplaceText(L"bb");
            replace.SelectWholeWordCheckbox(true);
            replace.Replace();

            Poll::Until(1s, 1ms, [&]() { return edit.GetText() == L" bb aaaaa"; });
            Assert::AreEqual(L" bb aaaaa", edit.GetText(), "first 'aa' should have been replaced by 'bb");

            // next replace should fail
            replace.Replace();
            auto mb = main.FindExistingUnfoundMessageBox();

            Assert::AreEqual(L"Cannot find 'aa'", mb.GetText(), "second search for whole word 'aa' should fail");
            mb.PushOkButton();

            edit.ClearDirtyFlag();
            Poll::While(1s, 1ms, [&edit]() { return edit.IsDirty(); });
            main.ExitViaMenu();
        }
    },
    { std::string("When replacing, the replacement text is selected"), []()
        {
            TestAutomation::MainWindow main;

            auto edit = main.GetEditField();
            edit.SetText(L"abcdef");
            edit.SetCursorPosition(0, 0);

            edit.ClearDirtyFlag();
            Poll::While(1s, 1ms, [&edit]() { return edit.IsDirty(); });

            auto editMenu = main.GetMenu().GetEditMenu();
            editMenu.SelectMenuItem(IDM_REPLACE);
            auto replace = main.FindExistingReplaceDialog();

            replace.SetFindText(L"cd");
            replace.SetReplaceText(L"***");
            replace.SelectWholeWordCheckbox(false);
            replace.Replace();

            Poll::Until(1s, 1ms, [&]() { return edit.GetText() == L"ab***ef"; });
            Assert::AreEqual(L"ab***ef", edit.GetText(), "the middle 'cd' should have been replaced by '***");

            Poll::While(1s, 1ms, [&edit]()  {
                                                DWORD ss, ee;
                                                edit.GetCursorPosition(ss, ee);
                                                return ss == 0 && ee == 0;
                                            });
            DWORD s=0, e=0;
            edit.GetCursorPosition(s, e);
            Assert::AreEqual(2, s, "start selection position should be 2");
            Assert::AreEqual(5, e,   "end selection position should be 5");

            edit.ClearDirtyFlag();
            Poll::While(1s, 1ms, [&edit]() { return edit.IsDirty(); });
            replace.Cancel();
            main.ExitViaMenu();
        }
    },
    { std::string("Pressing ReplaceAll button works"), []()
        {
            TestAutomation::MainWindow main;

            auto edit = main.GetEditField();
            edit.SetText(L"aaaaaa");
            edit.SetCursorPosition(3, 3);

            edit.ClearDirtyFlag();
            Poll::While(1s, 1ms, [&edit]() { return edit.IsDirty(); });

            auto editMenu = main.GetMenu().GetEditMenu();
            editMenu.SelectMenuItem(IDM_REPLACE);
            auto replace = main.FindExistingReplaceDialog();

            replace.SetFindText   (L"a");
            replace.SetReplaceText(L"b");
            replace.ReplaceAll();

            Poll::While(1s, 1ms, [&]() { return edit.GetText() == L"aaaaaa"; });
            Assert::AreEqual(L"bbbbbb", edit.GetText(), "ReplaceAll should have replaced all 'a' chars with 'b' chars");

            Poll::Until(1s, 1ms, [&edit]()  {   DWORD ss, ee;
                                                edit.GetCursorPosition(ss, ee);
                                                return ss == 0 && ee == 0;
                                            });
            DWORD s=0, e=0;
            edit.GetCursorPosition(s, e);
            Assert::AreEqual(0, s, "start selection position should be reset to 0");
            Assert::AreEqual(0, e,   "end selection position should be reset to 0");
            Assert::IsTrue(edit.IsDirty(), "edit field should be dirty");

            edit.ClearDirtyFlag();
            Poll::While(1s, 1ms, [&edit]() { return edit.IsDirty(); });
            replace.Cancel();
            main.ExitViaMenu();
        }
    },
};
