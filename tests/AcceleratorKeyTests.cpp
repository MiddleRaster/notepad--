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

Test AcceleratorKeyTests[] = {
    { std::string("Ctrl-N works"), []()
        {
            TestAutomation::MainWindow main;

            auto edit = main.GetEditField();
            edit.AppendText(L"Ctrl-N works");

            main.EnsureInForeground();
            main.SendKey('N', TestAutomation::MainWindow::Control);

            auto fdmb = main.GetFileDirtyMessageBox();
            fdmb.PressDontSave();

            edit.ClearDirtyFlag();
            main.ExitViaMenu();
        }
    },
    { std::string("Ctrl-O works"), []()
        {
            TestAutomation::MainWindow main;

            main.EnsureInForeground();
            main.SendKey('O', TestAutomation::MainWindow::Control);

            auto open = main.FindExistingOpenDialog();
            open.PressCancel();

            main.GetEditField().ClearDirtyFlag();
            main.ExitViaMenu();
        }
    },
    { std::string("Ctrl-S works"), []()
        {
            std::filesystem::path tempfile = FileUtils::CreateTempUtf8File(L"ControlS.txt", L"Ctrl-S");
            TestAutomation::MainWindow main(tempfile);

            auto edit = main.GetEditField();
            edit.AppendText(L" works");
            Poll::Until(1s, 1ms, [&edit]() { return edit.GetText() == L"Ctrl-S works"; });
            Assert::AreEqual(L"Ctrl-S works", edit.GetText(), "load file and append should have worked");

            main.EnsureInForeground();
            main.SendKey('S', TestAutomation::MainWindow::Control);

            std::this_thread::sleep_for(100ms); // wait for Notepad-- to finish writing to the file. Ugly: everything else I tried opens the file, causing the problem to get worse
            Poll::Until(1s, 1ms, [&tempfile]() { return FileUtils::ReadFileUtf8(tempfile) == L"Ctrl-S works"; });
            Assert::AreEqual(L"Ctrl-S works", FileUtils::ReadFileUtf8(tempfile), "contents not saved to file");

            FileUtils::DeleteFileWithRetry(tempfile);
            main.ExitViaMenu();
        }
    },
    { std::string("Ctrl-Shift-S works"), []()
        {
            std::filesystem::path tempfile = FileUtils::CreateTempUtf8File(L"ControlShiftS.txt", L"Ctrl-Shift-S works");
            TestAutomation::MainWindow main(tempfile);
            Assert::AreEqual(L"Ctrl-Shift-S works", main.GetEditField().GetText(), "load file failed");

            main.EnsureInForeground();
            main.SendKey('S', TestAutomation::MainWindow::BothControlAndShift);

            auto saveAs = main.FindExistingFileSaveAsDialogBox();
            saveAs.Cancel();

            FileUtils::DeleteFileWithRetry(tempfile);
            main.ExitViaMenu();
        }
    },
#if _DEBUG
    { std::string("Ctrl-P works"), []()
        {
            TestAutomation::MainWindow main;
            auto edit = main.GetEditField();
            edit.SetText(L"Ctrl-P works");
            edit.ClearDirtyFlag();

            main.EnsureInForeground();
            main.SendKey('P', TestAutomation::MainWindow::Control);

            auto print = main.FindExistingPrintDialog();
            print.Cancel();

            main.ExitViaMenu();
        }
    },
#endif
    { std::string("Ctrl-Z works"), []()
        {
            TestAutomation::MainWindow main;
            auto edit = main.GetEditField();
            edit.SetText(L"Ctrl-Z");
            edit.AppendText(L" works.");

            Poll::Until(1s, 1ms, [&edit]() { return edit.GetText() == L"Ctrl-Z works."; });
            Assert::AreEqual(L"Ctrl-Z works.", edit.GetText(), "should have appended text by now");

            main.EnsureInForeground();
            main.SendKey('Z', TestAutomation::MainWindow::Control);

            Poll::Until(1s, 1ms, [&edit]() { return edit.GetText() == L"Ctrl-Z works"; });
            Assert::AreEqual(L"Ctrl-Z works", edit.GetText(), "should have removed last char '.'");

            edit.ClearDirtyFlag();
            Poll::While(1s, 1ms, [&edit]() { return edit.IsDirty(); });
            main.ExitViaMenu();
        }
    },
    { std::string("Ctrl-V works"), []()
        {
            std::wstring text{L"Ctrl-V works"};
            TestAutomation::Clipboard::SetClipboardText(text);
            TestAutomation::MainWindow main;

            main.EnsureInForeground();
            main.SendKey('V', TestAutomation::MainWindow::Control);

            auto edit = main.GetEditField();
            Poll::Until(1s, 1ms, [&edit]() { return edit.GetText() == L"Ctrl-V works"; });
            Assert::AreEqual(L"Ctrl-V works", edit.GetText(), "should have pasted");

            edit.ClearDirtyFlag();
            Poll::While(1s, 1ms, [&edit]() { return edit.IsDirty(); });
            main.ExitViaMenu();
        }
    },
    { std::string("Del key works"), []()
        {
            TestAutomation::MainWindow main;
            auto edit = main.GetEditField();
            edit.SetText(L"Del key works");
            edit.SetCursorPosition(3, 7);

            main.EnsureInForeground();
            main.SendKey(static_cast<WORD>(VK_DELETE), TestAutomation::MainWindow::KeyStates::None);
            Poll::Until(1s, 1ms, [&edit]() { return edit.GetText() == L"Del works"; });

            edit.ClearDirtyFlag();
            Poll::While(1s, 1ms, [&edit]() { return edit.IsDirty(); });

            Assert::AreEqual(L"Del works", edit.GetText(), "should have deleted selected text");

            main.ExitViaMenu();
        }
    },
    { std::string("Ctrl-A works"), []()
        {
            TestAutomation::MainWindow main;
            auto edit = main.GetEditField();
            edit.SetText(L"Ctrl-A works");

            DWORD s, e;
            edit.GetCursorPosition(s, e);

            main.EnsureInForeground();
            main.SendKey('A', TestAutomation::MainWindow::Control);
            Poll::Until(1s, 1ms, [&]()  {
                                            DWORD ss,ee;
                                            edit.GetCursorPosition(ss, ee);
                                            return (s != ss) || (e != ee);
                                        });
            edit.ClearDirtyFlag();
            Poll::While(1s, 1ms, [&edit]() { return edit.IsDirty(); });

            edit.GetCursorPosition(s, e);
            Assert::AreEqual( 0, s, "start cursor position should be at beginning");
            Assert::AreEqual(12, e,   "end cursor position should be at the end");

            main.ExitViaMenu();
        }
    },
    { std::string("Ctrl-F works"), []()
        {
            TestAutomation::MainWindow main;
            auto edit = main.GetEditField();
            edit.SetText(L"Ctrl-F works");

            main.EnsureInForeground();
            main.SendKey('F', TestAutomation::MainWindow::Control);

            edit.ClearDirtyFlag();
            Poll::While(1s, 1ms, [&edit]() { return edit.IsDirty(); });
            Poll::Until(1s, 1ms, [&main]() { try { auto find = main.FindExistingFindDialog(); return true; }
                                             catch (...) { return false; } });
            auto find = main.FindExistingFindDialog();
            find.Cancel();

            main.ExitViaMenu();
        }
    },
    { std::string("The F3 key works"), []()
        {
            TestAutomation::MainWindow main;
            auto edit = main.GetEditField();
            edit.SetText(L"The F3 key works");
            edit.ClearDirtyFlag();
            Poll::While(1s, 1ms, [&edit]() { return edit.IsDirty(); });

            edit.SetCursorPosition(0, 0);
            Poll::Until(1s, 1ms, [&]()  {   DWORD ss, ee;
                                            edit.GetCursorPosition(ss, ee);
                                            return (0 == ss) && (0 == ee);
                                        });

            auto editMenu = main.GetMenu().GetEditMenu();
            editMenu.SelectMenuItem(IDM_FIND);
            auto find = main.FindExistingFindDialog();
            find.SetEditFieldText(L"k");
            find.FindNext();
            Poll::While(1s, 1ms, [&]()  {   DWORD ss, ee;
                                            edit.GetCursorPosition(ss, ee);
                                            return (0 == ss) && (0 == ee);
                                        });
            DWORD s, e;
            edit.GetCursorPosition(s, e);
            Assert::AreEqual(7, s, "should have found first 'k' at starting position 7");
            Assert::AreEqual(8, e, "should have found first 'k' at ending position 7");

            // find again via F3
            main.EnsureInForeground();
            main.SendKey((WORD)VK_F3, TestAutomation::MainWindow::KeyStates::None);

            Poll::While(1s, 1ms, [&]()  {   DWORD ss, ee;
                                            edit.GetCursorPosition(ss, ee);
                                            return (7 == ss) && (8 == ee);
                                        });
            edit.GetCursorPosition(s, e);
            Assert::AreEqual(14, s, "should have found second 'k' at starting position 14");
            Assert::AreEqual(15, e, "should have found second 'k' at ending position 15");

            main.ExitViaMenu();
        }
    },
    { std::string("If nothing selected yet, the F3 key puts up the Find dialog box"), []()
        {
            TestAutomation::MainWindow main;
            auto edit = main.GetEditField();
            edit.SetText(L"If nothing selected yet, the F3 key puts up the Find dialog box");
            edit.ClearDirtyFlag();
            Poll::While(1s, 1ms, [&edit]() { return edit.IsDirty(); });

            edit.SetCursorPosition(0, 0);
            Poll::Until(1s, 1ms, [&]() {   DWORD ss, ee;
                                            edit.GetCursorPosition(ss, ee);
                                            return (0 == ss) && (0 == ee);
                                        });

            main.EnsureInForeground();
            main.SendKey((WORD)VK_F3, TestAutomation::MainWindow::KeyStates::None);

            auto find = main.FindExistingFindDialog();
            find.Cancel();

            main.ExitViaMenu();
        }
    },
    { std::string("Ctrl-H displays Replace dialogbox"), []()
        {
            TestAutomation::MainWindow main;
            auto edit = main.GetEditField();
            edit.SetText(L"Ctrl-H displays Replace dialogbox");
            edit.ClearDirtyFlag();
            Poll::While(1s, 1ms, [&edit]() { return edit.IsDirty(); });

            main.EnsureInForeground();
            main.SendKey('H', TestAutomation::MainWindow::Control);

            auto replace = main.FindExistingReplaceDialog();
            replace.Cancel();

            main.ExitViaMenu();
        }
    },
    { std::string("Ctrl-G displays GoToLine dialogbox"), []()
        {
            TestAutomation::MainWindow main;
            auto edit = main.GetEditField();
            edit.SetText(L"Ctrl-G displays GoToLine dialogbox");
            edit.ClearDirtyFlag();
            Poll::While(1s, 1ms, [&edit]() { return edit.IsDirty(); });

            main.EnsureInForeground();
            main.SendKey('G', TestAutomation::MainWindow::Control);

            auto goTo = main.FindExistingGoToDialog();
            goTo.Cancel();

            main.ExitViaMenu();
        }
    },
};