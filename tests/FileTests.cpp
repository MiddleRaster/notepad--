
#include "..\src\PrintEngine.h"

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
            int last = 20;
            for (int attempt=1; attempt<=last; ++attempt)
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
                if (attempt == last) // last try...
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

Test FileSaveAsTests[] = {
    { std::string("Loading a file and selecting File->SaveAs pre-populates SaveAs dialog"), []()
        {
            auto path = FileUtils::CreateTempUtf8File(L"Notepad--.txt", L"Loading a file and selecting File->SaveAs pre-populates SaveAs dialog");

            auto stripExtension = [](const std::wstring& filename)
                                    {
                                        auto dot = filename.rfind(L'.');
                                        if (dot != std::wstring::npos)
                                            return filename.substr(0, dot);
                                        return filename;
                                    };

            TestAutomation::MainWindow main(path);
            auto saveAs       = main.SaveAs();
            Poll::Until(1s, 1ms, [&]() { return stripExtension(saveAs.GetEditFieldText()) == stripExtension(L"Notepad--.txt"); });
            auto prepopulated = saveAs.GetEditFieldText();
            saveAs.Cancel();

            FileUtils::DeleteFileWithRetry(path);
            main.ExitViaMenu();

            Assert::AreEqual(stripExtension(path.filename().c_str()), stripExtension(prepopulated), "filename should have been set in SaveAs dialog's edit field");
        }
    },
    { std::string("Write 5 differently encoded txt files and verify them by reading them back in"), []()
        {
            std::cout << "Just before TestAutomation::COM com; call" << std::endl;
            TestAutomation::COM com; // just in case

            std::cout << "Just before FileDeleter definition" << std::endl;
            struct FileDeleter
            {
                const std::filesystem::path path;
                FileDeleter(const std::filesystem::path& path) : path(path) {}
               ~FileDeleter() { FileUtils::DeleteFileWithRetry(path); }
            };

            std::cout << "Just before main" << std::endl;
            TestAutomation::MainWindow main;
            std::cout << "Just before GetEditField().SetText()" << std::endl;
            main.GetEditField().SetText(L"This is some text");

            std::cout << "Just before creating 5 tempfilenames" << std::endl;
            auto pathUTF8noBOM   = FileUtils::GetTempFilename(L"UTF8noBOM.txt");
            auto pathUTF8withBOM = FileUtils::GetTempFilename(L"UTF8withBOM.txt");
            auto pathANSI        = FileUtils::GetTempFilename(L"ANSI.txt");
            auto pathUTF16       = FileUtils::GetTempFilename(L"UTF16.txt");
            auto pathUTF16BE     = FileUtils::GetTempFilename(L"UTF16BE.txt");
            
            std::cout << "Just before 5 FileDeleter objects' creation" << std::endl;
            FileDeleter fd0(pathUTF8noBOM);
            FileDeleter fd1(pathUTF8withBOM);
            FileDeleter fd2(pathANSI);
            FileDeleter fd3(pathUTF16);
            FileDeleter fd4(pathUTF16BE);

            std::cout << "Just before 5 saveAs/SetEncoding/SaveFile calls" << std::endl;
            { 
                std::cout << "Just before first saveAs call" << std::endl;
                auto saveAs = main.SaveAs(); 

                std::cout << "Just before first SetEncoding call" << std::endl;
                saveAs.SetEncoding(L"UTF-8 no BOM"      );

                std::cout << "Just before first SaveFile call" << std::endl;
                saveAs.SaveFile(pathUTF8noBOM  .c_str());
            }
            std::cout << "Just afer first saveAs.SaveFile call" << std::endl;

            { 
                std::cout << "Just before second saveAs call" << std::endl;
                auto saveAs = main.SaveAs();
                std::cout << "Just before second SetEncoding call" << std::endl;
                saveAs.SetEncoding(L"UTF-8 with BOM"    );
                std::cout << "Just before second SaveFile call" << std::endl;
                saveAs.SaveFile(pathUTF8withBOM.c_str());
            }
            std::cout << "Just afer second saveAs.SaveFile call" << std::endl;

            {
                std::cout << "Just before third saveAs call" << std::endl;
                auto saveAs = main.SaveAs();
                std::cout << "Just before third SetEncoding call" << std::endl;
                saveAs.SetEncoding(L"ANSI"              );
                std::cout << "Just before third SaveFile call" << std::endl;
                saveAs.SaveFile(pathANSI       .c_str());
            }
            std::cout << "Just afer third saveAs.SaveFile call" << std::endl;

            { 
                std::cout << "Just before fourth saveAs call" << std::endl;
                auto saveAs = main.SaveAs();
                std::cout << "Just before fourth SetEncoding call" << std::endl;
                saveAs.SetEncoding(L"Unicode"           );
                std::cout << "Just before fourth SaveFile call" << std::endl;
                saveAs.SaveFile(pathUTF16      .c_str());
            }
            std::cout << "Just afer fourth saveAs.SaveFile call" << std::endl;

            {
                std::cout << "Just before fifth saveAs call" << std::endl;
                auto saveAs = main.SaveAs();
                std::cout << "Just before fifth SetEncoding call" << std::endl;
                saveAs.SetEncoding(L"Unicode big endian");
                std::cout << "Just before fifth SaveFile call" << std::endl;
                saveAs.SaveFile(pathUTF16BE    .c_str());
            }
            std::cout << "Just afer fifth saveAs.SaveFile call" << std::endl;

            std::cout << "Just before ExitViaMenu" << std::endl;
            main.ExitViaMenu();

            std::cout << "Just before Read class definition" << std::endl;
            struct Read
            {
                static std::wstring BOM(const std::filesystem::path& filename)
                {   // read the first 3 bytes:
                    std::ifstream in(filename, std::ios::binary);

                    std::array<unsigned char, 3> b;
                    in.read(reinterpret_cast<char*>(b.data()), 3);

                    if (b[0] == 0xEF)
                    if (b[1] == 0xBB)
                    if (b[2] == 0xBF)
                        return std::format(L"{:02X},{:02X},{:02X}", b[0], b[1], b[2]); // UTF-8 with BOM

                    if (b[0] == 0xFF)
                    if (b[1] == 0xFE)
                        return std::format(L"{:02X},{:02X}",        b[0], b[1]); // UTF-16

                    if (b[0] == 0xFE)
                    if (b[1] == 0xFF)
                        return std::format(L"{:02X},{:02X}",        b[0], b[1]); // UTF-16 be

                    return L"";                                                  // ANSI, UTF-8 no BOM
                }
                static std::wstring File(const std::filesystem::path& filename)
                {
                    TestAutomation::MainWindow main(filename);
                    return main.GetEditField().GetText();
                }
            };

            std::cout << "Just before 5 Asserts regarding BOMs" << std::endl;
            Assert::AreEqual(std::format(L""                                      ), Read::BOM(pathUTF8noBOM),   "UTF-8 no BOM is wrong");
            Assert::AreEqual(std::format(L"{:02X},{:02X},{:02X}", 0xEF, 0xBB, 0xBF), Read::BOM(pathUTF8withBOM), "UTF-8 with BOM is wrong");
            Assert::AreEqual(std::format(L"{:02X},{:02X}",        0xFF, 0xFE      ), Read::BOM(pathUTF16),       "UTF-16 BOM is wrong");
            Assert::AreEqual(std::format(L"{:02X},{:02X}",        0xFe, 0xFf      ), Read::BOM(pathUTF16BE),     "UTF-16be BOM is wrong");
            Assert::AreEqual(std::format(L""                                      ), Read::BOM(pathANSI),        "ANSI (no BOM) is wrong");

            std::cout << "Just before 5 Asserts comparing contents of files" << std::endl;
            Assert::AreEqual(L"This is some text", Read::File(pathUTF8noBOM),   "UTF-8 no BOM encoding is wrong");
            Assert::AreEqual(L"This is some text", Read::File(pathUTF8withBOM), "UTF-8 with BOM encoding is wrong");
            Assert::AreEqual(L"This is some text", Read::File(pathANSI),        "ANSI encoding is wrong");
            Assert::AreEqual(L"This is some text", Read::File(pathUTF16),       "UTF-16 encoding is wrong");
            Assert::AreEqual(L"This is some text", Read::File(pathUTF16BE),     "UTF-16BE encoding is wrong");

            std::cout << "About to exit lambda" << std::endl;
        }
    },
};