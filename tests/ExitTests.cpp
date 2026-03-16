import std;
import tdd20;
import VsTdd20;

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
    HWND FindExitDialog(DWORD pid)
    {
        struct Finder
        {
            DWORD pid;
            HWND hwnd{nullptr};
            static BOOL CALLBACK EnumProc(HWND hWnd, LPARAM lParam)
            {
                auto* self = reinterpret_cast<Finder*>(lParam);
                DWORD windowPid = 0;
                GetWindowThreadProcessId(hWnd, &windowPid);
                if (windowPid != self->pid)
                    return TRUE;
                wchar_t className[64]{};
                if (GetClassNameW(hWnd, className, static_cast<int>(std::size(className))) == 0)
                    return TRUE;
                if (std::wcscmp(className, L"#32770") != 0)
                    return TRUE;
                self->hwnd = hWnd;
                return FALSE;
            }
        } finder{pid};
        EnumWindows(Finder::EnumProc, reinterpret_cast<LPARAM>(&finder));
        return finder.hwnd;
    }

    HWND WaitForExitDialog(DWORD pid, std::chrono::milliseconds timeout)
    {
        const auto deadline = std::chrono::steady_clock::now() + timeout;
        HWND dlg = nullptr;
        while (std::chrono::steady_clock::now() < deadline)
        {
            dlg = FindExitDialog(pid);
            if (dlg != nullptr)
                break;
            std::this_thread::sleep_for(50ms);
        }
        return dlg;
    }

    HWND FindDontSaveButton(HWND dlg)
    {
        HWND button = nullptr;
        while ((button = FindWindowExW(dlg, button, L"Button", nullptr)) != nullptr)
        {
            wchar_t text[64]{};
            GetWindowTextW(button, text, static_cast<int>(std::size(text)));
            if (std::wcscmp(text, L"Don't Save") == 0)
                return button;
        }
        return nullptr;
    }

    void MarkEditDirty(HWND hWnd)
    {
        HWND edit = FindWindowExW(hWnd, nullptr, L"Edit", nullptr);
        Assert::AreNotEqual(nullptr, edit, "Edit control not found");
        for (wchar_t ch : L"Dirty")
            SendMessageW(edit, WM_CHAR, ch, 1);
    }

    void SetEditText(HWND hWnd, const wchar_t* text)
    {
        HWND edit = FindWindowExW(hWnd, nullptr, L"Edit", nullptr);
        Assert::AreNotEqual(nullptr, edit, "Edit control not found");
        Assert::IsTrue(SendMessageW(edit, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(L"")) != 0, "Failed to clear edit field text");
        for (const wchar_t* p = text; *p != L'\0'; ++p)
            SendMessageW(edit, WM_CHAR, *p, 1);
    }

    HWND FindDialogButton(HWND dlg, const wchar_t* label)
    {
        HWND button = nullptr;
        while ((button = FindWindowExW(dlg, button, L"Button", nullptr)) != nullptr)
        {
            wchar_t text[64]{};
            GetWindowTextW(button, text, static_cast<int>(std::size(text)));
            if (std::wcscmp(text, label) == 0)
                return button;
        }
        return nullptr;
    }

    std::filesystem::path SaveAsToTemp(DWORD pid, const wchar_t* fileName)
    {
        wchar_t tempPath[MAX_PATH]{};
        Assert::IsTrue(GetTempPathW(MAX_PATH, tempPath) > 0, "Failed to get temp path");
        std::filesystem::path filePath = std::filesystem::path(tempPath) / fileName;
        DeleteFileW(filePath.c_str());

        HWND saveAs = WaitForSaveAsDialog(pid, 5s);
        Assert::AreNotEqual(nullptr, saveAs, "Save As dialog not found");

        HWND dlgEdit = WaitForSaveAsEdit(saveAs, 1s);
        Assert::AreNotEqual(nullptr, dlgEdit, "Save As edit control not found");
        Assert::IsTrue(SendMessageW(dlgEdit, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(filePath.c_str())) != 0, "Failed to set Save As filename");

        HWND okButton = GetDlgItem(saveAs, IDOK);
        Assert::AreNotEqual(nullptr, okButton, "Save As OK button not found");
        SendMessageW(okButton, BM_CLICK, 0, 0);

        const auto dialogDeadline = std::chrono::steady_clock::now() + 5s;
        while (std::chrono::steady_clock::now() < dialogDeadline && IsWindow(saveAs))
            std::this_thread::sleep_for(50ms);

        const auto fileDeadline = std::chrono::steady_clock::now() + 5s;
        while (std::chrono::steady_clock::now() < fileDeadline && !std::filesystem::exists(filePath))
            std::this_thread::sleep_for(50ms);
        Assert::IsTrue(std::filesystem::exists(filePath), "Save As did not create the file");

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

VsTest ExitTests[] = {
    { std::string("Exiting when dirty pops up DialogBox; click 'Don't Save'"), []()
        {
            ProcessGuard proc([&](PROCESS_INFORMATION& pi) { return LaunchNotepadAndWait(pi); });
            Assert::IsTrue(proc.created, "Failed to launch Notepad--.exe");
            HWND hwnd = WaitForMainWindow(GetProcessId(proc.hProcess), 5s);
            Assert::AreNotEqual(nullptr, hwnd, "Main window did not appear");

            MarkEditDirty(hwnd);

            PostMessageW(hwnd, WM_COMMAND, IDM_EXIT, 0);
            HWND dialog = WaitForExitDialog(GetProcessId(proc.hProcess), 5s);
            Assert::AreNotEqual(nullptr, dialog, "Exit dialog not found");
            HWND prompt = FindWindowExW(dialog, nullptr, L"Static", nullptr);
            Assert::AreNotEqual(nullptr, prompt, "Exit dialog prompt not found");
            wchar_t promptText[128]{};
            GetWindowTextW(prompt, promptText, static_cast<int>(std::size(promptText)));
            Assert::AreEqual(std::wstring(L"Do you want to save the changes?"), std::wstring(promptText), "Exit dialog prompt text mismatch");

            HWND dontSave = FindDontSaveButton(dialog);
            Assert::AreNotEqual(nullptr, dontSave, "Don't Save button not found");
            SendMessageW(dontSave, BM_CLICK, 0, 0);

            Assert::AreEqual(WAIT_OBJECT_0, WaitForSingleObject(proc.hProcess, 5000), "Notepad-- did not exit after Don't Save");
        }
    },
    { std::string("Exiting when dirty then Save writes file"), []()
        {
            ProcessGuard proc([&](PROCESS_INFORMATION& pi) { return LaunchNotepadAndWait(pi); });
            Assert::IsTrue(proc.created, "Failed to launch Notepad--.exe");
            HWND hwnd = WaitForMainWindow(GetProcessId(proc.hProcess), 5s);
            Assert::AreNotEqual(nullptr, hwnd, "Main window did not appear");

            const wchar_t* text = L"Hello, World!";
            SetEditText(hwnd, text);

            PostMessageW(hwnd, WM_COMMAND, IDM_EXIT, 0);
            HWND dialog = WaitForExitDialog(GetProcessId(proc.hProcess), 5s);
            Assert::AreNotEqual(nullptr, dialog, "Exit dialog not found");

            HWND saveButton = FindDialogButton(dialog, L"Save");
            Assert::AreNotEqual(nullptr, saveButton, "Save button not found");
            SendMessageW(saveButton, BM_CLICK, 0, 0);

            std::filesystem::path filePath = SaveAsToTemp(GetProcessId(proc.hProcess), L"notepad--exit-save.txt");
            std::wstring contents = ReadFileUtf8(filePath);
            Assert::AreEqual(std::wstring(text), contents, "Save As file contents mismatch");

            Assert::AreEqual(WAIT_OBJECT_0, WaitForSingleObject(proc.hProcess, 5000), "Notepad-- did not exit after Save");
            DeleteFileWithRetry(filePath);
        }
    },
    { std::string("Exiting when dirty then Cancel keeps app open"), []()
        {
            ProcessGuard proc([&](PROCESS_INFORMATION& pi) { return LaunchNotepadAndWait(pi); });
            Assert::IsTrue(proc.created, "Failed to launch Notepad--.exe");
            HWND hwnd = WaitForMainWindow(GetProcessId(proc.hProcess), 5s);
            Assert::AreNotEqual(nullptr, hwnd, "Main window did not appear");

            const wchar_t* text = L"Hello, World!";
            SetEditText(hwnd, text);

            PostMessageW(hwnd, WM_COMMAND, IDM_EXIT, 0);
            HWND dialog = WaitForExitDialog(GetProcessId(proc.hProcess), 5s);
            Assert::AreNotEqual(nullptr, dialog, "Exit dialog not found");

            HWND cancelButton = FindDialogButton(dialog, L"Cancel");
            Assert::AreNotEqual(nullptr, cancelButton, "Cancel button not found");
            SendMessageW(cancelButton, BM_CLICK, 0, 0);

            Assert::AreEqual(WAIT_TIMEOUT, WaitForSingleObject(proc.hProcess, 0), "Notepad-- exited after Cancel");

            HWND edit = FindWindowExW(hwnd, nullptr, L"Edit", nullptr);
            Assert::AreNotEqual(nullptr, edit, "Edit control not found");
            Assert::AreEqual(std::wstring(text), GetEditText(edit), "Edit text changed after Cancel");

            Assert::AreEqual(nullptr, WaitForSaveAsDialog(GetProcessId(proc.hProcess), 200ms), "Save As dialog appeared after Cancel");

            SendMessageW(edit, EM_SETMODIFY, 0, 0);
            PostMessageW(hwnd, WM_COMMAND, IDM_EXIT, 0);
            Assert::AreEqual(WAIT_OBJECT_0, WaitForSingleObject(proc.hProcess, 5000), "Notepad-- did not exit after Cancel cleanup");
        }
    },
};
