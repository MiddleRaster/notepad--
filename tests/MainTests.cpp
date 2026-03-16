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

VsTest MainTests[] = {
    { std::string("Launch and Exit Notepad--"), []()
        {
            ProcessGuard proc([&](PROCESS_INFORMATION& pi) { return LaunchNotepadAndWait(pi); });
            Assert::IsTrue(proc.created, "Failed to launch Notepad--.exe");
            HWND hwnd = WaitForMainWindow(GetProcessId(proc.hProcess), 5s);
            Assert::AreNotEqual(nullptr, hwnd, "Main window did not appear");
            CloseAppViaFileExit(hwnd, proc.hProcess);
        } },
    { std::string{"Launch and Exit 2"}, []()
        {
            ProcessGuard proc([&](PROCESS_INFORMATION& pi) { return LaunchNotepadAndWait(pi); });
            Assert::IsTrue(proc.created, "Failed to launch Notepad--.exe");
            HWND hwnd = WaitForMainWindow(GetProcessId(proc.hProcess), 5s);
            Assert::AreNotEqual(nullptr, hwnd, "Main window did not appear");
            TriggerExitViaPopupMenu(hwnd);
            Assert::AreEqual(WAIT_OBJECT_0, WaitForSingleObject(proc.hProcess, 5000), "Notepad-- did not exit after menu selection");
        }
    },
    { std::string{"Upon launch title bar says Untitled"}, []()
        {
            ProcessGuard proc([&](PROCESS_INFORMATION& pi) { return LaunchNotepadAndWait(pi); });
            Assert::IsTrue(proc.created, "Failed to launch Notepad--.exe");
            HWND hwnd = WaitForMainWindow(GetProcessId(proc.hProcess), 5s);
            Assert::AreNotEqual(nullptr, hwnd, "Main window did not appear");

            wchar_t title[256]{};
            Assert::IsTrue(GetWindowTextW(hwnd, title, static_cast<int>(std::size(title))) > 0, "Failed to read window title");
            Assert::AreEqual(std::wstring(L"Untitled"), std::wstring(title), "Unexpected window title");

            TriggerExitViaPopupMenu(hwnd);
            Assert::AreEqual(WAIT_OBJECT_0, WaitForSingleObject(proc.hProcess, 5000), "Notepad-- did not exit after menu selection");
        }
    },
    { std::string("Launch with file path loads file into edit control"), []()
        {
            const wchar_t* text = L"Hello from file!";
            std::filesystem::path filePath = CreateTempUtf8File(L"notepad--cli.txt", text);

            ProcessGuard proc([&](PROCESS_INFORMATION& pi) { return LaunchNotepadWithArgsAndWait(pi, filePath.c_str()); });
            Assert::IsTrue(proc.created, "Failed to launch Notepad--.exe");
            HWND hwnd = WaitForMainWindow(GetProcessId(proc.hProcess), 5s);
            Assert::AreNotEqual(nullptr, hwnd, "Main window did not appear");

            HWND edit = FindWindowExW(hwnd, nullptr, L"Edit", nullptr);
            Assert::AreNotEqual(nullptr, edit, "Edit control not found");
            Assert::AreEqual(std::wstring(text), GetEditText(edit), "Edit text did not match file contents");

            wchar_t title[256]{};
            Assert::IsTrue(GetWindowTextW(hwnd, title, static_cast<int>(std::size(title))) > 0, "Failed to read window title");
            Assert::AreEqual(std::wstring(L"notepad--cli.txt"), std::wstring(title), "Unexpected window title after opening file");

            CloseAppViaFileExit(hwnd, proc.hProcess);

            Assert::IsTrue(DeleteFileW(filePath.c_str()) != 0, "Failed to delete temp file");
        }
    },
    { std::string("Launch with bad path shows error"), []()
        {
            auto FindMessageBoxForProcess = [](DWORD pid) -> HWND
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
            };

            auto WaitForMessageBox = [&](DWORD pid, std::chrono::milliseconds timeout) -> HWND
            {
                const auto deadline = std::chrono::steady_clock::now() + timeout;
                HWND dlg = nullptr;
                while (std::chrono::steady_clock::now() < deadline)
                {
                    dlg = FindMessageBoxForProcess(pid);
                    if (dlg != nullptr)
                        break;
                    std::this_thread::sleep_for(50ms);
                }
                return dlg;
            };

            const wchar_t* badPath = L"Z:\\this\\path\\does\\not\\exist.txt";
            ProcessGuard proc([&](PROCESS_INFORMATION& pi) { return LaunchNotepadWithArgsAndWait(pi, badPath); });
            Assert::IsTrue(proc.created, "Failed to launch Notepad--.exe");

            HWND msg = WaitForMessageBox(GetProcessId(proc.hProcess), 1s);
            Assert::AreNotEqual(nullptr, msg, "Error message box not shown for bad file path");

            HWND okButton = GetDlgItem(msg, IDOK);
            if (okButton != nullptr)
                SendMessageW(okButton, BM_CLICK, 0, 0);
            else
                SendMessageW(msg, WM_CLOSE, 0, 0);

            HWND hwnd = WaitForMainWindow(GetProcessId(proc.hProcess), 5s);
            Assert::AreNotEqual(nullptr, hwnd, "Main window did not appear");
            CloseAppViaFileExit(hwnd, proc.hProcess);
        }
    },
};

VsTest EditFieldTests[] = {
    { std::string("Edit Field Round Trip"), []()
        {
            ProcessGuard proc([&](PROCESS_INFORMATION& pi) { return LaunchNotepadAndWait(pi); });
            Assert::IsTrue(proc.created, "Failed to launch Notepad--.exe");
            HWND hwnd = WaitForMainWindow(GetProcessId(proc.hProcess), 5s);
            Assert::AreNotEqual(nullptr, hwnd, "Main window did not appear");

            HWND edit = FindWindowExW(hwnd, nullptr, L"Edit", nullptr);
            Assert::AreNotEqual(nullptr, edit, "Edit control not found");
            const wchar_t* text = L"Hello, World!";
            Assert::IsTrue(SendMessageW(edit, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(text)) != 0, "Failed to set edit field text");

            Assert::AreEqual(std::wstring(text), GetEditText(edit), "Edit field text mismatch");

            CloseAppViaFileExit(hwnd, proc.hProcess);
        }
    },
    { std::string("Set some text, then SaveAs"), []()
        {
            ProcessGuard proc([&](PROCESS_INFORMATION& pi) { return LaunchNotepadAndWait(pi); });
            Assert::IsTrue(proc.created, "Failed to launch Notepad--.exe");
            HWND hwnd = WaitForMainWindow(GetProcessId(proc.hProcess), 5s);
            Assert::AreNotEqual(nullptr, hwnd, "Main window did not appear");

            HWND edit = FindWindowExW(hwnd, nullptr, L"Edit", nullptr);
            Assert::AreNotEqual(nullptr, edit, "Edit control not found");
            const wchar_t* text = L"Hello, World!";
            Assert::IsTrue(SendMessageW(edit, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(text)) != 0, "Failed to set edit field text");

            wchar_t tempPath[MAX_PATH]{};
            Assert::IsTrue(GetTempPathW(MAX_PATH, tempPath) > 0, "Failed to get temp path");
            std::filesystem::path filePath = std::filesystem::path(tempPath) / L"notepad--test.txt";
            DeleteFileW(filePath.c_str());

            PostMessageW(hwnd, WM_COMMAND, IDM_SAVEAS, 0);
            HWND saveAs = WaitForSaveAsDialog(GetProcessId(proc.hProcess), 5s);
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
            Assert::AreEqual(std::wstring(text), wide, "Save As file contents mismatch");

            wchar_t title[256]{};
            Assert::IsTrue(GetWindowTextW(hwnd, title, static_cast<int>(std::size(title))) > 0, "Failed to read window title");
            Assert::AreEqual(std::wstring(L"notepad--test.txt"), std::wstring(title), "Unexpected window title after Save As");

            CloseAppViaFileExit(hwnd, proc.hProcess);

            Assert::IsTrue(DeleteFileW(filePath.c_str()), "Failed to delete test file");
        }
    },
};
