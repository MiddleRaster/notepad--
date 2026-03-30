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
    { std::string("If there is nothing selected, the Edit>Copy menu item is grayed out"), []()
        {
            TestAutomation::MainWindow main;
            auto edit = main.GetEditField();
            edit.SetText(L"ABCDEFG");
            edit.ClearDirtyFlag();
            Poll::While(100ms, 1ms, [&edit]() { return edit.IsDirty(); });

            auto editMenu = main.GetMenu().GetEditMenu();
            Assert::IsFalse(editMenu.IsMenuItemEnabled(IDM_COPY));

            main.ExitViaMenu();
        }
    },
    { std::string("If some text is selected, the Edit>Copy menu item is enabled"), []()
        {
            TestAutomation::MainWindow main;
            auto edit = main.GetEditField();
            edit.SetText(L"ABCDEFG");
            edit.ClearDirtyFlag();
            Poll::While(100ms, 1ms, [&edit]() { return edit.IsDirty(); });

            edit.SetCursorPosition(2, 4);
            auto editMenu = main.GetMenu().GetEditMenu();
            Assert::IsTrue(editMenu.IsMenuItemEnabled(IDM_COPY));

            main.ExitViaMenu();
        }
    },
    { std::string("If there is nothing selected, the Edit>Cut menu item is grayed out"), []()
        {
            TestAutomation::MainWindow main;
            auto edit = main.GetEditField();
            edit.SetText(L"ABCDEFG");
            edit.ClearDirtyFlag();
            Poll::While(100ms, 1ms, [&edit]() { return edit.IsDirty(); });

            auto editMenu = main.GetMenu().GetEditMenu();
            Assert::IsFalse(editMenu.IsMenuItemEnabled(IDM_CUT));

            main.ExitViaMenu();
        }
    },
    { std::string("If some text is selected, the Edit>Cut menu item is enabled"), []()
        {
            TestAutomation::MainWindow main;
            auto edit = main.GetEditField();
            edit.SetText(L"ABCDEFG");
            edit.ClearDirtyFlag();
            Poll::While(100ms, 1ms, [&edit]() { return edit.IsDirty(); });

            edit.SetCursorPosition(2, 4);
            auto editMenu = main.GetMenu().GetEditMenu();
            Assert::IsTrue(editMenu.IsMenuItemEnabled(IDM_CUT));

            main.ExitViaMenu();
        }
    },
    { std::string("If some text is selected and the user selects Edit->Copy, the text ends up on the clipboard"), []()
        {
            std::wstring droids{L"These aren't the droids you're looking for."};

            int last = 6;
            for(int i=1; i<=last; ++i) {
                try {
                    TestAutomation::Clipboard::SetClipboardText(droids.c_str());

                    TestAutomation::MainWindow main;
                    auto edit = main.GetEditField();
                    edit.SetText(L"ABCDEFG");
                    edit.ClearDirtyFlag();
                    Poll::While(100ms, 1ms, [&edit]() { return edit.IsDirty(); });

                    edit.SetCursorPosition(2, 4);
                    auto editMenu = main.GetMenu().GetEditMenu();

                    if (i&1)
                        editMenu.SelectMenuItem(IDM_COPY);
                    else
                        editMenu.ClickMenuItem(L"Edit", L"Copy\tCtrl+C");

                    Poll::Until(1s, 1ms, [&droids]() { return TestAutomation::Clipboard::GetClipboardText() == L"CD"; });
                    Assert::AreEqual(L"CD", TestAutomation::Clipboard::GetClipboardText(), "the selection should have been copied to the clipboard");

                    main.ExitViaMenu();
                    return;
                }
                catch (AssertException&)
                {
                    if (i == last) // last try...
                        throw;
                }
            }
        }
    },
    { std::string("If some text is selected and the user selects Edit->Cut, the text ends up on the clipboard and is removed from the edit field"), []()
        {
            std::wstring droids{L"These aren't the droids you're looking for."};

            // try it both ways: if the first succeeds, return
            int last = 6;
            for (int i=1; i<=6; ++i) {
                try {
                    TestAutomation::Clipboard::SetClipboardText(droids.c_str());

                    TestAutomation::MainWindow main;
                    auto edit = main.GetEditField();
                    edit.SetText(L"ABCDEFG");

                    edit.SetCursorPosition(2, 4);
                    auto editMenu = main.GetMenu().GetEditMenu();

                    if (i&1)
                        editMenu.SelectMenuItem(IDM_CUT);
                    else
                        editMenu.ClickMenuItem(L"Edit", L"Cut\tCtrl+X");

                    Poll::Until(1s, 1ms, [&droids]() { return TestAutomation::Clipboard::GetClipboardText() == L"CD"; });
                    edit.ClearDirtyFlag();
                    Poll::While(1s, 1ms, [&edit]() { return edit.IsDirty(); });

                    Assert::AreEqual(L"CD", TestAutomation::Clipboard::GetClipboardText(), "the selection should have been copied to the clipboard");
                    Assert::AreEqual(L"ABEFG", edit.GetText(), "two characters should have been cut from the edit field's text");

                    main.ExitViaMenu();
                    return;
                }
                catch (AssertException&)
                {
                    if (i == last) // last try...
                        throw;
                }
            }
        }
    },
    { std::string("If there is no text on the clipboard, then the Edit->Paste menu item is grayed out"), []()
        {
            TestAutomation::Clipboard::EmptyClipboard();
            Poll::While(1s, 1ms, []() { return TestAutomation::Clipboard::GetClipboardText() == L""; });
            TestAutomation::MainWindow main;

            auto editMenu = main.GetMenu().GetEditMenu();
            Assert::IsFalse(editMenu.IsMenuItemEnabled(IDM_PASTE));

            main.ExitViaMenu();
        }
    },
    { std::string("If there is text on the clipboard and the user selects Edit>Paste, then the clipboard text is pasted at current cursor position"), []()
        {
            std::wstring text{L"Paste works"};

            // try it both ways: if the first succeeds, return
            int last = 6;
            for (int i=1; i<=6; ++i) {
                try {
                    TestAutomation::Clipboard::SetClipboardText(text.c_str());

                    TestAutomation::MainWindow main;
                    auto edit = main.GetEditField();
                    edit.SetText(L"XX");
                    edit.SetCursorPosition(1, 1);
             
                    auto editMenu = main.GetMenu().GetEditMenu();

                    if (i&1)
                        editMenu.SelectMenuItem(IDM_PASTE);
                    else
                        editMenu.ClickMenuItem(L"Edit", L"Paste\tCtrl+V");

                    Poll::While(1s, 1ms, [&edit]() { return edit.GetText() == L"XX"; });
                    edit.ClearDirtyFlag();
                    Poll::While(1s, 1ms, [&edit]() { return edit.IsDirty(); });

                    Assert::AreEqual(L"XPaste worksX", edit.GetText(), "should have been pasted at position 1");

                    main.ExitViaMenu();
                    return;
                }
                catch (AssertException&)
                {
                    if (i == last) // last try...
                        throw;
                }
            }
        }
    },
};
