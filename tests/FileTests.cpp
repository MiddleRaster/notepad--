import std;
import tdd20;

#include <windows.h>
#include "..\src\Resource.h"
#include "Automation.h"

using namespace std::chrono_literals;
using namespace TDD20;

namespace TDD20
{
    template <> inline std::string ToString(const std::nullptr_t&) { return "0x0"; }
}

Test FileNewTests[] = {
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
            auto fileDirtyMessageBox = main.GetFileDirtyMessageBox();
            fileDirtyMessageBox.PressCancel();

            Assert::AreEqual(L"File->New opens 'do you want to save the changes?' dialog box if dirty:  cancel case", edit.GetText());
            
            main.GetEditField().ClearDirtyFlag();
            main.ExitViaMenu();
        }
    },
    { std::string("File->New opens 'do you want to save the changes?' dialog box if dirty:  no case"), []()
        {
            TestAutomation::MainWindow main;

            auto edit = main.GetEditField();
            edit.SetText(L"File->New opens 'do you want to save the changes?' dialog box if dirty:  no case");

            main.FileNew();
            auto fileDirtyMessageBox = main.GetFileDirtyMessageBox();
            fileDirtyMessageBox.PressDontSave();

            Assert::AreEqual(L"", edit.GetText());

            main.GetEditField().ClearDirtyFlag();
            main.ExitViaMenu();
        }
    },
    { std::string("File->New opens 'do you want to save the changes?' dialog box if dirty:  yes case"), []()
        {
            TestAutomation::MainWindow main;

            auto edit = main.GetEditField();
            edit.SetText(L"File->New opens 'do you want to save the changes?' dialog box if dirty:  yes case");

            main.FileNew();
            auto fileDirtyMessageBox = main.GetFileDirtyMessageBox();
            fileDirtyMessageBox.PressSave();

            auto saveAsDlg = main.FindExistingFileSaveAsDialogBox();
            saveAsDlg.Cancel();

            Poll::Until(1s, 1ms, [&edit]() { return edit.GetText().length() == 0; });

            Assert::AreEqual(L"", edit.GetText());

            main.GetEditField().ClearDirtyFlag();
            main.ExitViaMenu();
        }
    },
};

Test FileOpenTests[] = {
    { std::string("File->Open loads file into edit control"), []()
        {
            for (int attempt=0; attempt<3; ++attempt)
            try {
                const wchar_t* text = L"File->Open loads file into edit control";
                std::filesystem::path filePath = FileUtils::CreateTempUtf8File(L"notepad--open.txt", text);

                TestAutomation::MainWindow proc;
                proc.FileOpen();
                auto openDlg = proc.FindExistingOpenDialog();

                // Without this delay, the dialog reports "File not found" even though the file exists and WM_SETTEXT succeeds.
                std::this_thread::sleep_for(std::chrono::milliseconds(100*(attempt+1))); // longer and longer delay...

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
    { std::string("File->Open if dirty displays file dirty messageBox:  Cancel case"),[]()
        {
            TestAutomation::MainWindow main;
            main.GetEditField().SetText(L"File->Open if dirty displays file dirty messageBox:  Cancel case");

            main.FileOpen();

            auto msgBox = main.GetFileDirtyMessageBox();
            msgBox.PressDontSave();

            auto openDlg = main.FindExistingOpenDialog();
            openDlg.PressCancel();

            Assert::AreEqual("", main.GetEditField().GetText(), "after File->Open and cancel, edit field should be blank.");

            main.ExitViaMenu();
        }
    },
    { std::string("File->Open if dirty displays file dirty messageBox:  Save case  "),[]()
        {
            TestAutomation::MainWindow main;
            main.GetEditField().SetText(L"File->Open if dirty displays file dirty messageBox:  Save case  ");

            main.FileOpen();
            auto msgBox = main.GetFileDirtyMessageBox();
            msgBox.PressSave();

            auto saveAs = main.FindExistingFileSaveAsDialogBox();

            auto path = FileUtils::GetTempFilename(L"FileOpenSaveAs.txt");
            saveAs.SaveFile(path);

            auto contents = FileUtils::ReadFileUtf8(path);
            FileUtils::DeleteFileWithRetry(path);
            Assert::AreEqual(L"File->Open if dirty displays file dirty messageBox:  Save case  ", contents, "file contents didn't match edit field");

            auto openDlg = main.FindExistingOpenDialog();
            openDlg.PressCancel();

            Assert::AreEqual("", main.GetEditField().GetText(), "should be blank");

            main.ExitViaMenu();
        }
    },
    { std::string("File->Open if dirty displays file dirty messageBox:  No case    "),[]()
        {
            TestAutomation::MainWindow main;
            main.GetEditField().SetText(L"File->Open if dirty displays file dirty messageBox:  No case    ");

            main.FileOpen();
            auto msgBox = main.GetFileDirtyMessageBox();
            msgBox.PressDontSave();

            auto openDlg = main.FindExistingOpenDialog();
            openDlg.PressCancel();

            Assert::AreEqual("", main.GetEditField().GetText(), "after File->Open when dirty and the user selects 'No' and when the OpenDialog show, he selects cancel, then edit field should be blank.");

            main.ExitViaMenu();
        }
    },
};

Test FileSaveTests[] = {
    { std::string("File->Save when untitled and clean displays SaveAs dialog"), []()
        {
            TestAutomation::MainWindow main;
            main.Save();
            auto saveAsDlg = main.FindExistingFileSaveAsDialogBox();

            auto tempFile = FileUtils::GetTempFilename(L"FileSave.txt");
            saveAsDlg.SaveFile(tempFile);

            auto contents = FileUtils::ReadFileUtf8(tempFile);
            FileUtils::DeleteFileWithRetry(tempFile);

            Assert::AreEqual(L"", contents, "there should be nothing in the file");
            Assert::AreEqual(L"FileSave.txt", main.GetTitle(), "title bar should be updated with new name but wasn't");

            main.ExitViaMenu();
        }
    },
    { std::string("File->Save when untitled and dirty displays SaveAs dialog"), []()
        {
            const wchar_t* text = L"File->Save when untitled and dirty displays SaveAs dialog";

            TestAutomation::MainWindow main;
            main.GetEditField().SetText(text);

            main.Save();
            auto saveAsDlg = main.FindExistingFileSaveAsDialogBox();

            auto tempFile = FileUtils::GetTempFilename(L"FileSave.txt");
            saveAsDlg.SaveFile(tempFile);

            auto contents = FileUtils::ReadFileUtf8(tempFile);
            FileUtils::DeleteFileWithRetry(tempFile);

            Assert::AreEqual(text, contents, "contents of the file should be the same as before");
            Assert::AreEqual(L"FileSave.txt", main.GetTitle(), "title bar should be updated with new name but wasn't");

            main.ExitViaMenu();
        }
    },
    { std::string("File->Save when   titled and clean is no-op"), []()
        {
            TestAutomation::MainWindow main;
            main.Save();
            auto saveAsDlg = main.FindExistingFileSaveAsDialogBox();

            auto tempFile = FileUtils::GetTempFilename(L"FileSave.txt");
            saveAsDlg.SaveFile(tempFile);

            // at this point, titled and clean
            main.Save();

            auto contents = FileUtils::ReadFileUtf8(tempFile);
            FileUtils::DeleteFileWithRetry(tempFile);
            Assert::AreEqual(L"", contents, "there should be nothing in the file");
            Assert::AreEqual(L"FileSave.txt", main.GetTitle(), "title bar should be updated with new name but wasn't");

            main.ExitViaMenu();
        }
    },
    { std::string("File->Save when   titled and dirty saves to existing file"), []()
        {
            TestAutomation::MainWindow main;

            main.Save();
            auto saveAsDlg = main.FindExistingFileSaveAsDialogBox();

            auto tempFile = FileUtils::GetTempFilename(L"FileSave.txt");
            saveAsDlg.SaveFile(tempFile);

            std::this_thread::sleep_for(100ms); // there's a race: after notepad-- writes the file, it then changes the title; only then can we read it
            Assert::AreEqual(L"FileSave.txt", main.GetTitle(), "title bar should have been updated with filename");

            const wchar_t* text = L"File->Save when   titled and dirty saves to existing file";
            main.GetEditField().SetText(text);

            // at this point, titled and dirty:  Save just saves to existing file
            main.Save();

            std::this_thread::sleep_for(250ms); // there's a race between when notepad-- writes the file and we read it.

            std::wstring contents = FileUtils::ReadFileUtf8(tempFile);
            FileUtils::DeleteFileWithRetry(tempFile);

            Assert::AreEqual(text, contents, "new contents should have been written to file");
            Assert::AreEqual(L"FileSave.txt", main.GetTitle(), "title bar should not have changed");
            
            main.ExitViaMenu();
        }
    },
};

Test FilePrintTests[] = {
#if _DEBUG
    { std::string("File->Print pops up Print Dialog (then WM_CLOSE)"), []()
        {
            TestAutomation::MainWindow main;
            auto edit = main.GetEditField();
            edit.SetText(L"Hi");
            edit.ClearDirtyFlag();

            auto print = main.Print();
            print.Cancel();

            main.ExitViaMenu();
        }
    },
#endif
    { std::string("Placeholder to Release builds (for now)"), []()
        {
        }
    },
};
