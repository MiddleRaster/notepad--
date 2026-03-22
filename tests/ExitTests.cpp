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

Test ExitTests[] = {
    { std::string("Exiting when dirty pops up DialogBox; click 'Don't Save'"), []()
        {
            TestAutomation::MainWindow notepad;
            notepad.GetEditField().MarkAsDirty();
            notepad.TryExit();

            auto messageBox = notepad.GetFileDirtyMessageBox();
            Assert::AreEqual(std::wstring(L"Do you want to save changes to Untitled?"), messageBox.GetStaticMessage(), "Exit dialog prompt text mismatch");
    
            messageBox.PressDontSave();
        }
    },
    { std::string("Exiting when dirty then Save writes file"), []()
        {
            TestAutomation::MainWindow notepad;
            auto editField = notepad.GetEditField();
            const wchar_t* text = L"Hello, World!";
            editField.SetText(text);
            notepad.TryExit();

            auto messageBox = notepad.GetFileDirtyMessageBox();
            messageBox.PressSave();

            std::filesystem::path filePath = FileUtils::GetTempFilename(L"notepad--exit-save.txt");

            auto saveAs = notepad.FindExistingFileSaveAsDialogBox();
            saveAs.SaveFile(filePath);

            Assert::AreEqual(std::wstring(text), FileUtils::ReadFileUtf8(filePath), "Save As file contents mismatch");
            FileUtils::DeleteFileWithRetry(filePath);
        }
    },
    { std::string("Exiting when dirty then Cancel keeps app open"), []()
        {
            TestAutomation::MainWindow notepad;
            auto editField = notepad.GetEditField();
            const wchar_t* text = L"Hello, World!";
            editField.SetText(text);
            notepad.TryExit();

            auto messageBox = notepad.GetFileDirtyMessageBox();
            messageBox.PressCancel();

            Assert::IsTrue(notepad.IsRunning(), "Notepad-- exited after Cancel");
            Assert::AreEqual(std::wstring(text), editField.GetText(), "Edit text changed after Cancel");

            notepad.GetEditField().ClearDirtyFlag();
            notepad.TryExit();
        }
    },
};
