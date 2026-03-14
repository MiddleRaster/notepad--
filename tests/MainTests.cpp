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

            HMENU menu = GetMenu(hwnd); // Trigger File -> Exit via command ID.
            Assert::IsTrue(menu != nullptr, "Menu handle was null");
            SendMessageW(hwnd, WM_INITMENU, reinterpret_cast<WPARAM>(menu), 0);
            SendMessageW(hwnd, WM_COMMAND, IDM_EXIT, 0);

            Assert::AreEqual(WAIT_OBJECT_0, WaitForSingleObject(proc.hProcess, 5000), "Notepad-- did not exit after IDM_EXIT");
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

