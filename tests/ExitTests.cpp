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

namespace
{
    std::filesystem::path GetTempFilename(const wchar_t* fileName)
    {
        wchar_t tempPath[MAX_PATH]{};
        Assert::IsTrue(GetTempPathW(MAX_PATH, tempPath) > 0, "Failed to get temp path");
        std::filesystem::path filePath = std::filesystem::path(tempPath) / fileName;
        DeleteFileW(filePath.c_str());
        return filePath;
    }

    std::wstring ReadFileUtf8(const std::filesystem::path& filePath)
    {
        std::ifstream in;
        for (int i=0; i<20; ++i)
        {
            in.open(filePath, std::ios::binary);
            if (!in.is_open())
                std::this_thread::sleep_for(50ms);
            else
                break;
        }
        Assert::IsTrue(in.is_open(), "Failed to open saved file");
        std::string bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        in.close();
        if (bytes.size() >= 3 && static_cast<unsigned char>(bytes[0]) == 0xEF && static_cast<unsigned char>(bytes[1]) == 0xBB && static_cast<unsigned char>(bytes[2]) == 0xBF)
            bytes.erase(0, 3);

        const int wideSize = MultiByteToWideChar(CP_UTF8, 0, bytes.data(), static_cast<int>(bytes.size()), nullptr, 0);
        Assert::IsTrue(wideSize >= 0, "Failed to convert saved file to UTF-16");
        std::wstring wide(static_cast<size_t>(wideSize), L'\0');
        MultiByteToWideChar(CP_UTF8, 0, bytes.data(), static_cast<int>(bytes.size()), wide.data(), wideSize);
        return wide;
    }

    void DeleteFileWithRetry(const std::filesystem::path& filePath)
    {
        for (int i=0; i<20; ++i)
        {
            if (DeleteFileW(filePath.c_str()) != 0)
                return;
            std::this_thread::sleep_for(50ms);
        }
        Assert::IsTrue(false, "Failed to delete test file");
    }
}

Test ExitTests[] = {
    { std::string("Exiting when dirty pops up DialogBox; click 'Don't Save'"), []()
        {
            TestAutomation::MainWindow notepad;
            notepad.MarkAsDirty();
            notepad.TryExit();

            auto dialog = notepad.WaitForExitDialog();
            Assert::AreEqual(std::wstring(L"Do you want to save the changes?"), dialog.GetStaticMessage(), "Exit dialog prompt text mismatch");

            dialog.ClickDontSave();
        }
    },
    { std::string("Exiting when dirty then Save writes file"), []()
        {
            TestAutomation::MainWindow notepad;
            auto editField = notepad.GetEditField();
            const wchar_t* text = L"Hello, World!";
            editField.SetText(text);
            notepad.TryExit();

            auto dialog = notepad.WaitForExitDialog();
            dialog.PressSave();

            std::filesystem::path filePath = GetTempFilename(L"notepad--exit-save.txt");
            notepad.SaveAs(filePath);
            Assert::AreEqual(std::wstring(text), ReadFileUtf8(filePath), "Save As file contents mismatch");
            DeleteFileWithRetry(filePath);
        }
    },
    { std::string("Exiting when dirty then Cancel keeps app open"), []()
        {
            TestAutomation::MainWindow notepad;
            auto editField = notepad.GetEditField();
            const wchar_t* text = L"Hello, World!";
            editField.SetText(text);
            notepad.TryExit();

            auto dialog = notepad.WaitForExitDialog();
            dialog.PressCancel();

            Assert::IsTrue(notepad.IsRunning(), "Notepad-- exited after Cancel");
            Assert::AreEqual(std::wstring(text), editField.GetText(), "Edit text changed after Cancel");

            notepad.ClearDirtyFlag();
            notepad.TryExit();
        }
    },
};
