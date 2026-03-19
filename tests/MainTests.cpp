import std;
import tdd20;

#include <windows.h>
#include "WinAutomation.h"

using namespace std::chrono_literals;
using namespace TDD20;

namespace TDD20
{
    template <> inline std::string ToString(const std::nullptr_t&) { return "0x0"; }
}

Test MainTests[] = {
    { std::string("Launch and Exit Notepad--"), []()
        {
            TestAutomation::MainWindow proc;
            proc.ExitViaMenu();
        } },
    { std::string{"Launch and Exit 2"}, []()
        {
            TestAutomation::MainWindow proc;
            proc.ExitViaMenuPopUp();
        }
    },
    { std::string{"Upon launch title bar says Untitled"}, []()
        {
            TestAutomation::MainWindow proc;
            Assert::AreEqual(std::wstring(L"Untitled"), proc.GetTitle(), "Unexpected window title");
            proc.ExitViaMenuPopUp();
        }
    },
    { std::string("Launch with file path loads file into edit control"), []()
        {
            const wchar_t* text = L"Hello from file!";
            std::filesystem::path filePath = FileUtils::CreateTempUtf8File(L"notepad--cli.txt", text);

            TestAutomation::MainWindow proc(filePath);
            Assert::AreEqual(std::wstring(text), proc.GetEditField().GetText(), "Edit text did not match file contents");
            Assert::AreEqual(std::wstring(L"notepad--cli.txt"), proc.GetTitle(), "Unexpected window title after opening file");

            proc.ExitViaMenu();
            Assert::IsTrue(DeleteFileW(filePath.c_str()) != 0, "Failed to delete temp file");
        }
    },
    { std::string("Launch with bad path shows error"), []()
        {
            TestAutomation::MainWindow proc(L"Z:\\this\\path\\does\\not\\exist.txt");

            auto msgBox = proc.GetModalMessageBox();
            msgBox.PressOk();

            Assert::IsTrue(proc.IsRunning(), "Notepad-- process is not running");
            proc.ExitViaMenu();
        }
    },
};

Test EditFieldTests[] = {
    { std::string("Edit Field Round Trip"), []()
        {
            TestAutomation::MainWindow proc;
            auto edit = proc.GetEditField();

            const wchar_t* text = L"Hello, World!";
            edit.SetText(text); // this DOES set the dirty flag (unlike sending WM_SETTEXT)
            Assert::AreEqual(std::wstring(text), edit.GetText(), "Edit field text mismatch");

            proc.ClearDirtyFlag();
            proc.ExitViaMenu();
        }
    },
    { std::string("Set some text, then SaveAs"), []()
        {
            TestAutomation::MainWindow proc;
            auto edit = proc.GetEditField();

            const wchar_t* text = L"Hello, World!";
            edit.SetText(text);
            
            auto saveAs = proc.SaveAs();
            std::filesystem::path tempPath = FileUtils::GetTempFilename(L"notepad--test.txt");
            saveAs.SaveFile(tempPath);
            Assert::AreEqual(std::wstring(text), FileUtils::ReadFileUtf8(tempPath), "Save As file contents mismatch");
            Assert::AreEqual(std::wstring(L"notepad--test.txt"), proc.GetTitle(), "Unexpected window title after Save As");

            proc.ExitViaMenu();
            Assert::IsTrue(DeleteFileW(tempPath.c_str()), "Failed to delete test file");
        }
    },
};
