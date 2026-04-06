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

Test FontTests[] = {
    { std::string("Can open Choose Font dialog"), []()
        {
            TestAutomation::MainWindow main;
            main.GetMenu().GetFormatMenu().SelectMenuItem(IDM_FONT);
            auto font = main.FindExistingChooseFontDialog();
            font.Cancel();
            main.ExitViaMenu();
        }
    },
};
