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

Test StatusBarTests[] = {
    { std::string("StatusBar is visible by default"), []()
        {
            TestAutomation::MainWindow main;

            auto statusBar = main.GetStatusBar();
            Poll::Until(1s, 1ms, [&]() { return statusBar.GetText() == L"Ln 0, Col 0 | ANSI"; });
            Assert::AreEqual(L"Ln 0, Col 0 | ANSI", statusBar.GetText());
            Assert::IsTrue(statusBar.IsVisible(), "status bar should be visible by default");

            main.ExitViaMenu();
        }
    },
    { std::string("StatusBar updates when cursor position moves"), []()
        {
            TestAutomation::MainWindow main;

            auto edit = main.GetEditField();
            edit.SetText(L"StatusBar updates when cursor position moves");
            edit.SetCursorPosition(20, 20);
            edit.ClearDirtyFlag();

            auto statusBar = main.GetStatusBar();
            Poll::Until(1s, 1ms, [&]() { return statusBar.GetText() == L"Ln 0, Col 20 | ANSI"; });
            Assert::AreEqual(L"Ln 0, Col 20 | ANSI", statusBar.GetText());

            main.ExitViaMenu();
        }
    },
    { std::string("StatusBar is invisible if wordwrap is on"), []()
        {
            TestAutomation::MainWindow main;

            main.GetMenu().GetFormatMenu().SelectMenuItem(IDM_WORDWRAP);
            auto statusBar = main.GetStatusBar();
            Poll::While(1s, 1ms, [&]() { return statusBar.IsVisible(); });
            Assert::IsFalse(statusBar.IsVisible(), "status bar should be invisible when wordwrap is on");

            main.ExitViaMenu();
        }
    },
    { std::string("StatusBar shows current encoding"), []()
        {
            static const wchar_t text[] = L"Here is some Unicode text";
            static const BYTE    bom [] = {0xFF, 0xFE};

            std::filesystem::path path = FileUtils::GetTempFilename(L"Unicode.txt");
            HANDLE hFile = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
            Assert::AreNotEqual(INVALID_HANDLE_VALUE, hFile, "could not open temp file");

            struct FileDeleter
            {
                std::filesystem::path path;
                FileDeleter(const std::filesystem::path& path) : path(path) {}
               ~FileDeleter() { FileUtils::DeleteFileWithRetry(path); }
            } fd(path);

            DWORD written;
            WriteFile(hFile, bom,  sizeof(bom ), &written, nullptr);
            WriteFile(hFile, text, sizeof(text), &written, nullptr);
            CloseHandle(hFile);
            Assert::AreEqual(sizeof(text), written, "could not write all wchars");

            TestAutomation::MainWindow main(path);
            auto statusBar = main.GetStatusBar();
            Poll::Until(1s, 1ms, [&]() { return statusBar.GetText() == L"Ln 0, Col 0 | Unicode"; });
            auto statusBarText = statusBar.GetText();

            main.ExitViaMenu();

            Assert::AreEqual(L"Ln 0, Col 0 | Unicode", statusBarText, "statusBar text should have included encoding");
        }
    },
};
