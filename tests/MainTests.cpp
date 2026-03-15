import std;
import tdd20;
import VsTdd20;

#include <windows.h>
#include "WinAutomation.h"

using namespace std::chrono_literals;
using namespace TDD20;

namespace
{
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
}

VsTest MainTests[] = {
    { std::string("Launch and Exit Notepad--"), []()
        {
            ProcessGuard proc([&](PROCESS_INFORMATION& pi)
            {
                const std::wstring exePath = GetNotepadExePath();
                Assert::IsTrue(std::filesystem::exists(exePath), "Notepad--.exe not found");

                STARTUPINFOW si{};
                si.cb = sizeof(si);
                std::wstring cmdLine = L"\"" + exePath + L"\"";
                return CreateProcessW(exePath.c_str(), cmdLine.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi) != FALSE;
            });
            Assert::IsTrue(proc.created, "Failed to launch Notepad--.exe");

            WaitForInputIdle(proc.hProcess, 5000);
            HWND hwnd = WaitForMainWindow(GetProcessId(proc.hProcess), 5s);
            Assert::IsTrue(hwnd != nullptr, "Main window did not appear");

            CloseAppViaFileExit(hwnd, proc.hProcess);
        } },
    { std::string{"Launch and Exit 2"}, []()
        {
            ProcessGuard proc([&](PROCESS_INFORMATION& pi)
            {
                const std::wstring exePath = GetNotepadExePath();
                Assert::IsTrue(std::filesystem::exists(exePath), "Notepad--.exe not found");

                STARTUPINFOW si{};
                si.cb = sizeof(si);
                std::wstring cmdLine = L"\"" + exePath + L"\"";
                return CreateProcessW(exePath.c_str(), cmdLine.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi) != FALSE;
            });
            Assert::IsTrue(proc.created, "Failed to launch Notepad--.exe");

            WaitForInputIdle(proc.hProcess, 5000);
            HWND hwnd = WaitForMainWindow(GetProcessId(proc.hProcess), 5s);
            Assert::IsTrue(hwnd != nullptr, "Main window did not appear");

            TriggerExitViaPopupMenu(hwnd);
            Assert::AreEqual(WAIT_OBJECT_0, WaitForSingleObject(proc.hProcess, 5000), "Notepad-- did not exit after menu selection");
        }
    },
};

VsTest EditFieldTests[] = {
    { std::string("Edit Field Round Trip"), []()
        {
            ProcessGuard proc([&](PROCESS_INFORMATION& pi)
            {
                const std::wstring exePath = GetNotepadExePath();
                Assert::IsTrue(std::filesystem::exists(exePath), "Notepad--.exe not found");

                STARTUPINFOW si{};
                si.cb = sizeof(si);
                std::wstring cmdLine = L"\"" + exePath + L"\"";
                return CreateProcessW(exePath.c_str(), cmdLine.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi) != FALSE;
            });
            Assert::IsTrue(proc.created, "Failed to launch Notepad--.exe");

            WaitForInputIdle(proc.hProcess, 5000);
            HWND hwnd = WaitForMainWindow(GetProcessId(proc.hProcess), 5s);
            Assert::IsTrue(hwnd != nullptr, "Main window did not appear");

            HWND edit = FindWindowExW(hwnd, nullptr, L"Edit", nullptr);
            Assert::IsTrue(edit != nullptr, "Edit control not found");

            if (edit != nullptr)
            {
                const wchar_t* text = L"Hello, World!";
                Assert::IsTrue(SendMessageW(edit, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(text)) != 0, "Failed to set edit field text");

                std::wstring buffer(256, L'\0');
                const LRESULT copied = SendMessageW(edit, WM_GETTEXT, static_cast<WPARAM>(buffer.size()), reinterpret_cast<LPARAM>(buffer.data()));
                Assert::IsTrue(copied > 0, "Failed to read edit field text");
                buffer.resize(static_cast<size_t>(copied));
                Assert::AreEqual(std::wstring(text), buffer, "Edit field text mismatch");
            }
            CloseAppViaFileExit(hwnd, proc.hProcess);
        }
    },
};

