#pragma once

#include <windows.h>
#include "..\src\Resource.h"

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
                if (!IsWindowVisible(hWnd))
                    return TRUE;
                if (GetWindow(hWnd, GW_OWNER) != nullptr)
                    return TRUE;
                self->hwnd = hWnd;
                return FALSE;
            }
        } finder{pid};

        EnumWindows(Finder::EnumProc, reinterpret_cast<LPARAM>(&finder));
        return finder.hwnd;
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

    HWND FindSaveAsDialog(DWORD pid)
    {
        struct Finder
        {
            DWORD pid;
            HWND hwnd{nullptr};
            static bool HasChildClass(HWND root, const wchar_t* className)
            {
                struct ChildFinder
                {
                    const wchar_t* className;
                    bool found{false};
                    static BOOL CALLBACK EnumProc(HWND hWnd, LPARAM lParam)
                    {
                        auto* self = reinterpret_cast<ChildFinder*>(lParam);
                        wchar_t cls[64]{};
                        if (GetClassNameW(hWnd, cls, static_cast<int>(std::size(cls))) != 0)
                        {
                            if (std::wcscmp(cls, self->className) == 0)
                            {
                                self->found = true;
                                return FALSE;
                            }
                        }
                        return TRUE;
                    }
                } finder{className};
                EnumChildWindows(root, ChildFinder::EnumProc, reinterpret_cast<LPARAM>(&finder));
                return finder.found;
            }
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
                if (!HasChildClass(hWnd, L"DUIViewWndClassName") && !HasChildClass(hWnd, L"DirectUIHWND"))
                    return TRUE;
                self->hwnd = hWnd;
                return FALSE;
            }
        } finder{pid};
        EnumWindows(Finder::EnumProc, reinterpret_cast<LPARAM>(&finder));
        return finder.hwnd;
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

    HWND FindSaveAsEdit(HWND dlg)
    {
        auto FindDescendantByClass = [](HWND root, const wchar_t* className, const auto& self) -> HWND
        {
            HWND child = nullptr;
            while ((child = FindWindowExW(root, child, nullptr, nullptr)) != nullptr)
            {
                wchar_t cls[64]{};
                if (GetClassNameW(child, cls, static_cast<int>(std::size(cls))) != 0)
                {
                    if (std::wcscmp(cls, className) == 0)
                        return child;
                }
                if (HWND found = self(child, className, self))
                    return found;
            }
            return nullptr;
        };

        HWND duiview = FindWindowExW(dlg, nullptr, L"DUIViewWndClassName", nullptr);
        if (duiview != nullptr)
        {
            HWND directUI = FindWindowExW(duiview, nullptr, L"DirectUIHWND", nullptr);
            if (directUI != nullptr)
            {
                HWND sink = FindWindowExW(directUI, nullptr, L"FloatNotifySink", nullptr);
                if (sink != nullptr)
                {
                    HWND combo = FindWindowExW(sink, nullptr, L"ComboBox", nullptr);
                    if (combo != nullptr)
                    {
                        if (HWND edit = FindWindowExW(combo, nullptr, L"Edit", nullptr))
                            return edit;
                    }
                }
            }
        }
        return FindDescendantByClass(dlg, L"Edit", FindDescendantByClass);
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
}
