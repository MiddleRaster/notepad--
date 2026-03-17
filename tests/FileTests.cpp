//import std;
//import tdd20;
//import VsTdd20;
//
//#include <windows.h>
//#include <commdlg.h> // trying Copilot's way
//#include <UIAutomation.h>
//#include <wrl/client.h>

// 1. Win32 headers FIRST
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
//#include <commdlg.h>

// 2. THEN your modules
import std;
import tdd20;
import VsTdd20;

#include "..\src\Resource.h"
#include "WinAutomation.h"

using namespace std::chrono_literals;
using namespace TDD20;

namespace TDD20
{
    template <> inline std::string ToString(const std::nullptr_t&) { return "0x0"; }
}

VsTest FileTests[] = {
    { std::string("File->Open loads file into edit control"), []()
        {
            for (int attempt=0; attempt<3; ++attempt)
            try {
                const wchar_t* text = L"Hello from file open!";
                std::filesystem::path filePath = CreateTempUtf8File(L"notepad--open.txt", text);

                ProcessGuard proc([&](PROCESS_INFORMATION& pi) { return LaunchNotepadAndWait(pi); });
                Assert::IsTrue(proc.created, "Failed to launch Notepad--.exe");
                HWND hwnd = WaitForMainWindow(GetProcessId(proc.hProcess), 1s);
                Assert::AreNotEqual(nullptr, hwnd, "Main window did not appear");
                PostMessageW(hwnd, WM_COMMAND, IDM_OPEN, 0);
                HWND openDlg = WaitForOpenFileDialog(GetProcessId(proc.hProcess), 1s);
                Assert::AreNotEqual(nullptr, openDlg, "File Open dialog not found");

                std::this_thread::sleep_for(100ms); // Without this delay, the dialog reports "File not found" even though the file exists and WM_SETTEXT succeeds.

                HWND dlgEdit = WaitForOpenFileEdit(openDlg, 1s);
                Assert::AreNotEqual(nullptr, dlgEdit, "File Open edit control not found");
                Assert::IsTrue(SendMessageW(dlgEdit, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(filePath.c_str())) != 0, "Failed to set File Open filename");

                HWND okButton = GetDlgItem(openDlg, IDOK);
                Assert::AreNotEqual(nullptr, okButton, "File Open OK button not found");
                SendMessageW(okButton, BM_CLICK, 0, 0);

                const auto dialogDeadline = std::chrono::steady_clock::now() + 1s;
                while (std::chrono::steady_clock::now() < dialogDeadline && IsWindow(openDlg))
                    std::this_thread::sleep_for(50ms);

                HWND edit = FindWindowExW(hwnd, nullptr, L"Edit", nullptr);
                Assert::AreNotEqual(nullptr, edit, "Edit control not found");
                Assert::AreEqual(std::wstring(text), GetEditText(edit), "Edit text did not match file contents");

                CloseAppViaFileExit(hwnd, proc.hProcess);

                Assert::IsTrue(DeleteFileW(filePath.c_str()) != 0, "Failed to delete temp file");
                return;
            }
            catch (AssertException&)
            {
                if (attempt == 2) // last try...
                    throw;
            }
        }
    },
};
