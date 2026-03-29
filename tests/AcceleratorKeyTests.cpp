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
};