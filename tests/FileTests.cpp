import std;
import tdd20;

#include <windows.h>
#include "..\src\Resource.h"
#include "WinAutomation.h"

using namespace std::chrono_literals;
using namespace TDD20;

namespace TDD20
{
    template <> inline std::string ToString(const std::nullptr_t&) { return "0x0"; }
}

namespace Poll
{
    void Until(std::chrono::milliseconds timeout, std::chrono::milliseconds step, auto&& pred)
    {
        const auto deadline = std::chrono::steady_clock::now() + timeout;
        while (std::chrono::steady_clock::now() < deadline)
        {
            if (pred())
                return;
            std::this_thread::sleep_for(step);
        }
    }
}

Test FileTests[] = {
    { std::string("File->New does nothing if not dirty"), []()
        {
            TestAutomation::MainWindow main;
            main.FileNew();
            main.ExitViaMenu();
        }
    },
    { std::string("File->New opens 'do you want to save the changes?' dialog box if dirty:  cancel case"), []()
        {
            TestAutomation::MainWindow main;

            auto edit = main.GetEditField();
            edit.SetText(L"File->New opens 'do you want to save the changes?' dialog box if dirty:  cancel case");

            main.FileNew();
            auto fileDirtyDialog = main.GetFileDirtyDialog();
            fileDirtyDialog.PressCancel();

            Assert::AreEqual(L"File->New opens 'do you want to save the changes?' dialog box if dirty:  cancel case", edit.GetText());
            
            main.ClearDirtyFlag();
            main.ExitViaMenu();
        }
    },
    { std::string("File->New opens 'do you want to save the changes?' dialog box if dirty:  no case"), []()
        {
            TestAutomation::MainWindow main;

            auto edit = main.GetEditField();
            edit.SetText(L"File->New opens 'do you want to save the changes?' dialog box if dirty:  no case");

            main.FileNew();
            auto fileDirtyDialog = main.GetFileDirtyDialog();
            fileDirtyDialog.ClickDontSave();

            Assert::AreEqual(L"", edit.GetText());

            main.ClearDirtyFlag();
            main.ExitViaMenu();
        }
    },
    { std::string("File->New opens 'do you want to save the changes?' dialog box if dirty:  yes case"), []()
        {
            TestAutomation::MainWindow main;

            auto edit = main.GetEditField();
            edit.SetText(L"File->New opens 'do you want to save the changes?' dialog box if dirty:  yes case");

            main.FileNew();
            auto fileDirtyDialog = main.GetFileDirtyDialog();
            fileDirtyDialog.PressSave();

            auto saveAsDlg = main.FindExistingFileSaveAsDialogBox();
            saveAsDlg.Cancel();

            Poll::Until(1s, 1ms, [&edit]() { return edit.GetText().length() == 0; });

            Assert::AreEqual(L"", edit.GetText());

            main.ClearDirtyFlag();
            main.ExitViaMenu();
        }
    },

    { std::string("File->Open loads file into edit control"), []()
        {
            for (int attempt=0; attempt<3; ++attempt)
            try {
                const wchar_t* text = L"Hello from file open!";
                std::filesystem::path filePath = FileUtils::CreateTempUtf8File(L"notepad--open.txt", text);

                TestAutomation::MainWindow proc;
                auto openDlg = proc.FileOpen();

                std::this_thread::sleep_for(100ms); // Without this delay, the dialog reports "File not found" even though the file exists and WM_SETTEXT succeeds.

                openDlg.OpenFile(filePath);
                auto edit = proc.GetEditField();
                Assert::AreEqual(std::wstring(text), edit.GetText(), "Edit text did not match file contents");
                Assert::IsTrue(DeleteFileW(filePath.c_str()) != 0, "Failed to delete temp file");

                proc.TryExit();
                return;
            }
            catch (AssertException&)
            {
                if (attempt == 2) // last try...
                    throw;
            }
        }
    },
};
