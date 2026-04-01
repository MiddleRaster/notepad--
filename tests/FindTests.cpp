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

Test FindTests[] = {
    { std::string("The Edit->Find menu item is always enabled"), []()
        {
            TestAutomation::MainWindow main;
            Assert::IsTrue(main.GetMenu().GetEditMenu().IsMenuItemEnabled(IDM_FIND), "the Edit->Find menu item should be enabled");
        }
    },
    { std::string("After the user selects Ctrl-F, the Find dialog box pops up"), []()
        {
            std::wstring text{L"After the user selects Ctrl-F, the Find dialog box pops up"};

            TestAutomation::MainWindow main;
            auto edit = main.GetEditField();
            edit.SetText(text);
            edit.ClearDirtyFlag();
            Poll::While(1s, 1ms, [&edit]() { return edit.IsDirty(); });

            auto editMenu = main.GetMenu().GetEditMenu();
            editMenu.SelectMenuItem(IDM_FIND);

            auto find = main.FindExistingFindDialog();
            find.Cancel();

            main.ExitViaMenu();
        }
    },
    { std::string("Given text in the edit field, 'aaa aaa', when the user selects the first 3 chars and uses Edit->Find, he can find the second instance of 'aaa'"), []()
        {
            std::wstring text{L"aaa aaa"};

            TestAutomation::MainWindow main;
            auto edit = main.GetEditField();
            edit.SetText(text);

            DWORD s=0, e=3;
            edit.SetCursorPosition(s, e);
            Poll::While(1s, 1ms, [&]()  {   DWORD ss,ee;
                                            edit.GetCursorPosition(ss, ee);
                                            return (s != ss) || (e != ee);
                                        });
            edit.ClearDirtyFlag();
            Poll::While(1s, 1ms, [&edit]() { return edit.IsDirty(); });

            auto editMenu = main.GetMenu().GetEditMenu();
            editMenu.SelectMenuItem(IDM_FIND);
            auto find = main.FindExistingFindDialog();

            // 'aaa' is in the Find dialog's edit field
            Assert::AreEqual(L"aaa", find.GetEditFieldText(), "the find dialog's edit field should have been prepopulated");

            // hit next
            find.FindNext();

            // selection should change to next one
            Poll::Until(1s, 1ms, [&]()  {   DWORD ss,ee;
                                            edit.GetCursorPosition(ss, ee);
                                            return (s != ss) || (e != ee);
                                        });
            // cursor position moves to 4, 7
            edit.GetCursorPosition(s, e);
            Assert::AreEqual(4, s, "start cursor position should have moved to 4");
            Assert::AreEqual(7, e,   "end cursor position should have moved to 7");

            main.ExitViaMenu();
        }
    },
    { std::string("Cannot search past end of text:  messagebox pops up"), []()
        {
            TestAutomation::MainWindow main;
            auto edit = main.GetEditField();
            edit.SetText(L"abcdefg");

            DWORD s=0, e=1;
            edit.SetCursorPosition(s, e);
            Poll::While(1s, 1ms, [&]() {   DWORD ss,ee;
                                            edit.GetCursorPosition(ss, ee);
                                            return (s != ss) || (e != ee);
                                        });
            edit.ClearDirtyFlag();
            Poll::While(1s, 1ms, [&edit]() { return edit.IsDirty(); });

            auto editMenu = main.GetMenu().GetEditMenu();
            editMenu.SelectMenuItem(IDM_FIND);
            auto find = main.FindExistingFindDialog();

            // 'a' is in the Find dialog's edit field
            Assert::AreEqual(L"a", find.GetEditFieldText(), "the find dialog's edit field should have been prepopulated");

            // FindNext which will fail to find another 'a'
            find.FindNext();

            auto mb = main.FindExistingUnfoundMessageBox();
            Assert::AreEqual(L"Cannot find 'a'", mb.GetText(), "messagebox's static text is wrong");
            mb.PushOkButton();

            find.Cancel();
            main.ExitViaMenu();
        }
    },
};
