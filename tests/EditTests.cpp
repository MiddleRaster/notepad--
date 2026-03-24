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

Test EditTests[] = {
    { std::string("When firing up Notepad--, the Edit->Undo menu item is grayed out"), []()
        {
            TestAutomation::MainWindow main;

            auto editMenu = main.GetMenu().GetEditMenu();
            Assert::IsFalse(editMenu.IsMenuItemEnabled(IDM_UNDO));

            main.ExitViaMenu();
        }
    },
};
