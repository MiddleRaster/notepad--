import std;
import tdd20;

#include <windows.h>
#include "..\src\Resource.h"
#include "WinAutomation.h"

using namespace std::chrono_literals;
using namespace TDD20;

namespace TDD20
{
    template <> inline std::string ToString(const std::nullptr_t&) { return "0x0"; }
}

Test FileTests[] = {
    { std::string("File->Open loads file into edit control"), []()
        {
            for (int attempt=0; attempt<3; ++attempt)
            try {
                const wchar_t* text = L"Hello from file open!";
                std::filesystem::path filePath = CreateTempUtf8File(L"notepad--open.txt", text);

                TestAutomation::MainWindow proc;
                auto openDlg = proc.FileOpen();

                std::this_thread::sleep_for(100ms); // Without this delay, the dialog reports "File not found" even though the file exists and WM_SETTEXT succeeds.

                openDlg.OpenFile(filePath);
                auto edit = proc.GetEditField();
                Assert::AreEqual(std::wstring(text), edit.GetText(), "Edit text did not match file contents");
                Assert::IsTrue(DeleteFileW(filePath.c_str()) != 0, "Failed to delete temp file");

                proc.TryExit();
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
