#pragma once

#include "WindowFinder.h"
#include "..\src\Resource.h"

namespace WindowFinder::Has
{
    struct EitherChildClass
    {
        const wchar_t* className1;
        const wchar_t* className2;
        bool operator()(HWND hwnd) const
        {
            return (nullptr != WindowFinder::FindDesiredChildWindow(hwnd, WindowFinder::Has::ClassName{className1})) ||
                   (nullptr != WindowFinder::FindDesiredChildWindow(hwnd, WindowFinder::Has::ClassName{className2})) ;
        }
    };
}

namespace
{
    using namespace TDD20;

    class ProcessGuard
    {
        ProcessGuard           (                    ) = delete;
        ProcessGuard           (const ProcessGuard& ) = delete;
        ProcessGuard& operator=(const ProcessGuard& ) = delete;
        ProcessGuard           (      ProcessGuard&&) = delete;
        ProcessGuard& operator=(      ProcessGuard&&) = delete;

        PROCESS_INFORMATION pi{};
    public:
        const HANDLE& hProcess;
        const HANDLE& hThread;
        const bool created;

        template <typename LaunchFn> explicit ProcessGuard(LaunchFn&& launch) : hProcess(pi.hProcess), hThread(pi.hThread), created(launch(pi)) {}
       ~ProcessGuard()
        {
            if (created)
                if (WaitForSingleObject(hProcess, 1000) == WAIT_TIMEOUT)
                    TerminateProcess(hProcess, 1);
            if (hThread)
                CloseHandle(hThread);
            if (hProcess)
                CloseHandle(hProcess);
        }
    };

    std::wstring GetModuleDirectory()
    {
        std::wstring path(MAX_PATH, L'\0');
        DWORD len = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
        while (len == path.size())
        {
            path.resize(path.size() * 2, L'\0');
            len = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
        }
        path.resize(len);
        std::filesystem::path p(path);
        return p.parent_path().wstring();
    }

    std::wstring GetNotepadExePath()
    {   // tests.exe lives alongside Notepad--.exe under <repo>\x64\<config>
        std::filesystem::path p(GetModuleDirectory());
        p /= L"Notepad--.exe";
        return p.wstring();
    }

    HWND FindMainWindowForProcess(DWORD pid)
    {
        return WindowFinder::FindDesiredChildWindow(nullptr, WindowFinder::Has::Pid{pid}, WindowFinder::Is::Visible, WindowFinder::Is::UnOwned);
    }

    HWND WaitForMainWindow(DWORD pid, std::chrono::milliseconds timeout)
    {
        const auto deadline = std::chrono::steady_clock::now() + timeout;
        HWND hwnd = nullptr;
        while (std::chrono::steady_clock::now() < deadline)
        {
            hwnd = FindMainWindowForProcess(pid);
            if (hwnd != nullptr)
                break;
            std::this_thread::sleep_for(std::chrono::milliseconds{50});
        }
        return hwnd;
    }

    void TriggerExitViaPopupMenu(HWND hwnd)
    {
        HMENU menu = GetMenu(hwnd);
        Assert::IsTrue(menu != nullptr, "Menu handle was null");
        if (menu == nullptr)
            return;

        HMENU fileMenu = GetSubMenu(menu, 0);
        Assert::IsTrue(fileMenu != nullptr, "File menu handle was null");
        if (fileMenu == nullptr)
            return;

        RECT itemRect{};
        Assert::IsTrue(GetMenuItemRect(hwnd, menu, 0, &itemRect) != 0, "Failed to get menu item rect");

        std::thread invokeThread([&]()
        {
            std::this_thread::sleep_for(std::chrono::milliseconds{100});
            PostMessageW(hwnd, WM_COMMAND, IDM_EXIT, 0);
        });

        TrackPopupMenu(fileMenu, TPM_LEFTALIGN | TPM_TOPALIGN, itemRect.left, itemRect.bottom, 0, hwnd, nullptr);
        invokeThread.join();
    }

    void CloseAppViaFileExit(HWND hwnd, HANDLE process)
    {
        HMENU menu = GetMenu(hwnd);
        Assert::IsTrue(menu != nullptr, "Menu handle was null");
        SendMessageW(hwnd, WM_INITMENU, reinterpret_cast<WPARAM>(menu), 0);
        SendMessageW(hwnd, WM_COMMAND, IDM_EXIT, 0);
        Assert::AreEqual(WAIT_OBJECT_0, WaitForSingleObject(process, 5000), "Notepad-- did not exit after IDM_EXIT");
    }

    bool LaunchNotepadAndWait(PROCESS_INFORMATION& pi)
    {
        const std::wstring exePath = GetNotepadExePath();
        Assert::IsTrue(std::filesystem::exists(exePath), "Notepad--.exe not found");

        STARTUPINFOW si{};
        si.cb = sizeof(si);
        std::wstring cmdLine = L"\"" + exePath + L"\"";
        const bool created = CreateProcessW(exePath.c_str(), cmdLine.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi) != FALSE;
        Assert::IsTrue(created, "Failed to launch Notepad--.exe");

        if (created)
            WaitForInputIdle(pi.hProcess, 5000);

        return created;
    }

    bool LaunchNotepadWithArgsAndWait(PROCESS_INFORMATION& pi, const wchar_t* args)
    {
        const std::wstring exePath = GetNotepadExePath();
        Assert::IsTrue(std::filesystem::exists(exePath), "Notepad--.exe not found");

        STARTUPINFOW si{};
        si.cb = sizeof(si);
        std::wstring cmdLine = L"\"" + exePath + L"\"";
        if (args != nullptr && args[0] != L'\0')
        {
            cmdLine += L" ";
            cmdLine += L"\"";
            cmdLine += args;
            cmdLine += L"\"";
        }
        const bool created = CreateProcessW(exePath.c_str(), cmdLine.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi) != FALSE;
        Assert::IsTrue(created, "Failed to launch Notepad--.exe");

        if (created)
            WaitForInputIdle(pi.hProcess, 5000);

        return created;
    }

    HWND FindSaveAsDialog(DWORD pid)
    {
        return WindowFinder::FindDesiredChildWindow(nullptr, WindowFinder::Has::Pid{pid}, WindowFinder::Has::ClassName{L"#32770"}, WindowFinder::Has::EitherChildClass{L"DUIViewWndClassName", L"DirectUIHWND"});
    }

    HWND WaitForSaveAsDialog(DWORD pid, std::chrono::milliseconds timeout)
    {
        const auto deadline = std::chrono::steady_clock::now() + timeout;
        HWND dlg = nullptr;
        while (std::chrono::steady_clock::now() < deadline)
        {
            dlg = FindSaveAsDialog(pid);
            if (dlg != nullptr)
                break;
            std::this_thread::sleep_for(std::chrono::milliseconds{50});
        }
        return dlg;
    }

    HWND WaitForOpenFileDialog(DWORD pid, std::chrono::milliseconds timeout)
    {
        return WaitForSaveAsDialog(pid, timeout);
    }

    HWND FindSaveAsEdit(HWND dlg)
    {
        if (HWND duiview  = WindowFinder::FindDesiredChildWindow(dlg,      WindowFinder::Has::ClassName{L"DUIViewWndClassName"}))
        if (HWND directUI = WindowFinder::FindDesiredChildWindow(duiview,  WindowFinder::Has::ClassName{L"DirectUIHWND"       }))
        if (HWND sink     = WindowFinder::FindDesiredChildWindow(directUI, WindowFinder::Has::ClassName{L"FloatNotifySink"    }))
        if (HWND combo    = WindowFinder::FindDesiredChildWindow(sink,     WindowFinder::Has::ClassName{L"ComboBox"           }))
        if (HWND edit     = WindowFinder::FindDesiredChildWindow(combo,    WindowFinder::Has::ClassName{L"Edit"               }))
            return edit;
        return WindowFinder::FindDesiredChildWindow(dlg, WindowFinder::Has::ClassName{L"Edit"});
    }

    HWND WaitForSaveAsEdit(HWND dlg, std::chrono::milliseconds timeout)
    {
        const auto deadline = std::chrono::steady_clock::now() + timeout;
        HWND edit = nullptr;
        while (std::chrono::steady_clock::now() < deadline)
        {
            edit = FindSaveAsEdit(dlg);
            if (edit != nullptr)
                break;
            std::this_thread::sleep_for(std::chrono::milliseconds{10});
        }
        return edit;
    }

    HWND FindOpenFileEdit(HWND dlg)
    {
        if (HWND comboEx = WindowFinder::FindDesiredChildWindow(dlg,     WindowFinder::Has::ClassName{L"ComboBoxEx32"}))
        if (HWND combo   = WindowFinder::FindDesiredChildWindow(comboEx, WindowFinder::Has::ClassName{L"ComboBox"}))
        if (HWND edit    = WindowFinder::FindDesiredChildWindow(combo,   WindowFinder::Has::ClassName{L"Edit"}))
            return edit;
        return FindSaveAsEdit(dlg);
    }

    HWND WaitForOpenFileEdit(HWND dlg, std::chrono::milliseconds timeout)
    {
        const auto deadline = std::chrono::steady_clock::now() + timeout;
        HWND edit = nullptr;
        while (std::chrono::steady_clock::now() < deadline)
        {
            edit = FindOpenFileEdit(dlg);
            if (edit != nullptr)
                break;
            std::this_thread::sleep_for(std::chrono::milliseconds{10});
        }
        return edit;
    }

    std::filesystem::path CreateTempUtf8File(const wchar_t* fileName, const wchar_t* text)
    {
        wchar_t tempPath[MAX_PATH]{};
        Assert::IsTrue(GetTempPathW(MAX_PATH, tempPath) > 0, "Failed to get temp path");
        std::filesystem::path filePath = std::filesystem::path(tempPath) / fileName;
        DeleteFileW(filePath.c_str());

        const int utf8Size = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
        Assert::IsTrue(utf8Size > 0, "Failed to size UTF-8 buffer");
        std::vector<char> utf8(static_cast<size_t>(utf8Size - 1) + 3);
        utf8[0] = static_cast<char>(0xEF);
        utf8[1] = static_cast<char>(0xBB);
        utf8[2] = static_cast<char>(0xBF);
        WideCharToMultiByte(CP_UTF8, 0, text, -1, utf8.data() + 3, utf8Size - 1, nullptr, nullptr);

        HANDLE file = CreateFileW(filePath.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        Assert::IsTrue(file != INVALID_HANDLE_VALUE, "Failed to create temp file");
        DWORD written = 0;
        WriteFile(file, utf8.data(), static_cast<DWORD>(utf8.size()), &written, nullptr);
        CloseHandle(file);
        return filePath;
    }

    std::wstring GetEditText(HWND edit)
    {
        const LRESULT length = SendMessageW(edit, WM_GETTEXTLENGTH, 0, 0);
        std::wstring buffer(static_cast<size_t>(length) + 1, L'\0');
        SendMessageW(edit, WM_GETTEXT, static_cast<WPARAM>(buffer.size()), reinterpret_cast<LPARAM>(buffer.data()));
        buffer.resize(static_cast<size_t>(length));
        return buffer;
    }
}

namespace WindowUtils
{
    inline HWND WaitForWindow(std::chrono::milliseconds timeout, auto&& pred)
    {
        const auto deadline = std::chrono::steady_clock::now() + timeout;
        while (std::chrono::steady_clock::now() < deadline)
        {
            if (HWND hwnd = pred())
                return hwnd;
            std::this_thread::sleep_for(50ms);
        }
        return nullptr;
    }
}

namespace TestAutomation
{
    using namespace std::chrono_literals;

    class FileDirtyDialog
    {
        HWND FindDialogButton(const wchar_t* label)
        {
            HWND button = nullptr;
            while ((button = FindWindowExW(dialog, button, L"Button", nullptr)) != nullptr)
            {
                wchar_t text[64]{};
                GetWindowTextW(button, text, static_cast<int>(std::size(text)));
                if (std::wcscmp(text, label) == 0)
                    return button;
            }
            return nullptr;
        }
        HWND FindDontSaveButton()
        {
            HWND button = nullptr;
            while ((button = FindWindowExW(dialog, button, L"Button", nullptr)) != nullptr)
            {
                wchar_t text[64]{};
                GetWindowTextW(button, text, static_cast<int>(std::size(text)));
                if (std::wcscmp(text, L"Don't Save") == 0)
                    return button;
            }
            return nullptr;
        }

        HWND dialog;
    public:
        FileDirtyDialog(DWORD pid)
        {
            dialog = WindowUtils::WaitForWindow(5s, [&pid]() { return WindowFinder::FindDesiredChildWindow(nullptr, WindowFinder::Has::Pid{ pid }, WindowFinder::Has::ClassName{ L"#32770" }); });
            Assert::AreNotEqual(nullptr, dialog, "Exit dialog not found");
        }
        std::wstring GetStaticMessage() const
        {
            HWND prompt = FindWindowExW(dialog, nullptr, L"Static", nullptr);
            Assert::AreNotEqual(nullptr, prompt, "Exit dialog prompt not found");
            wchar_t promptText[128]{};
            GetWindowTextW(prompt, promptText, static_cast<int>(std::size(promptText)));
            return promptText;
        }
        void ClickDontSave()
        {
            HWND dontSave = FindDontSaveButton();
            Assert::AreNotEqual(nullptr, dontSave, "Don't Save button not found");
            SendMessageW(dontSave, BM_CLICK, 0, 0);
        }
        void PressSave()
        {
            HWND saveButton = FindDialogButton(L"Save");
            Assert::AreNotEqual(nullptr, saveButton, "Save button not found");
            SendMessageW(saveButton, BM_CLICK, 0, 0);
        }
        void PressCancel()
        {
            HWND cancelButton = FindDialogButton(L"Cancel");
            Assert::AreNotEqual(nullptr, cancelButton, "Cancel button not found");
            SendMessageW(cancelButton, BM_CLICK, 0, 0);
        }
    };

    class EditField
    {
        HWND edit;
    public:
        EditField(HWND hwnd) : edit(hwnd) {}
        void SetText(const std::wstring& text)
        {
            Assert::IsTrue(SendMessageW(edit, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(L"")) != 0, "Failed to clear edit field text");
            for (auto c : text)
                SendMessageW(edit, WM_CHAR, c, 1);
        }
        std::wstring GetText() const
        {
            const LRESULT length = SendMessageW(edit, WM_GETTEXTLENGTH, 0, 0);
            std::wstring buffer(static_cast<size_t>(length) + 1, L'\0');
            SendMessageW(edit, WM_GETTEXT, static_cast<WPARAM>(buffer.size()), reinterpret_cast<LPARAM>(buffer.data()));
            buffer.resize(static_cast<size_t>(length));
            return buffer;
        }
    };

    class MainWindow
    {
        ProcessGuard proc;
        HWND hwnd{};
    public:
        MainWindow() : proc([&](PROCESS_INFORMATION& pi) { return LaunchNotepadAndWait(pi); })
        {
            Assert::IsTrue(proc.created, "Failed to launch Notepad--.exe");
            hwnd = WaitForMainWindow(GetProcessId(proc.hProcess), 1s);
            Assert::AreNotEqual(nullptr, hwnd, "Main window did not appear");
        }
        ~MainWindow()
        {
            Assert::AreEqual(WAIT_OBJECT_0, WaitForSingleObject(proc.hProcess, 5000), "Notepad-- did not exit after Don't Save");
        }
        bool IsRunning() const
        {
            return IsWindow(hwnd);
        }
        void TryExit()
        {
            PostMessageW(hwnd, WM_COMMAND, IDM_EXIT, 0);
        }
        void MarkAsDirty(const std::wstring& dirty = L"Dirty")
        {
            HWND edit = FindWindowExW(hwnd, nullptr, L"Edit", nullptr);
            Assert::AreNotEqual(nullptr, edit, "Edit control not found");
            for (wchar_t ch : dirty)
                SendMessageW(edit, WM_CHAR, ch, 1);
        }
        void ClearDirtyFlag()
        {
            HWND edit = FindWindowExW(hwnd, nullptr, L"Edit", nullptr);
            SendMessageW(edit, EM_SETMODIFY, 0, 0);
        }
        FileDirtyDialog WaitForExitDialog()
        {
            return {GetProcessId(proc.hProcess)};
        }
        EditField GetEditField()
        {
            HWND edit = FindWindowExW(hwnd, nullptr, L"Edit", nullptr);
            Assert::AreNotEqual(nullptr, edit, "Edit control not found");
            return {edit};
        }
        void SaveAs(const std::filesystem::path& fileName)
        {
            HWND saveAs = WaitForSaveAsDialog(GetProcessId(proc.hProcess), 5s);
            Assert::AreNotEqual(nullptr, saveAs, "Save As dialog not found");

            HWND dlgEdit = WaitForSaveAsEdit(saveAs, 1s);
            Assert::AreNotEqual(nullptr, dlgEdit, "Save As edit control not found");
            Assert::IsTrue(SendMessageW(dlgEdit, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(fileName.c_str())) != 0, "Failed to set Save As filename");

            HWND okButton = GetDlgItem(saveAs, IDOK);
            Assert::AreNotEqual(nullptr, okButton, "Save As OK button not found");
            SendMessageW(okButton, BM_CLICK, 0, 0);

            const auto dialogDeadline = std::chrono::steady_clock::now() + 5s;
            while (std::chrono::steady_clock::now() < dialogDeadline && IsWindow(saveAs))
                std::this_thread::sleep_for(50ms);

            const auto fileDeadline = std::chrono::steady_clock::now() + 5s;
            while (std::chrono::steady_clock::now() < fileDeadline && !std::filesystem::exists(fileName))
                std::this_thread::sleep_for(50ms);
            Assert::IsTrue(std::filesystem::exists(fileName), "Save As did not create the file");
        }
    };
}
