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

Test WordWrapTests[] = {
    { std::string("The Word Wrap menu item checkbox is unchecked by default"), []()
        {
            TestAutomation::MainWindow main;
            Assert::IsFalse(main.GetMenu().GetFormatMenu().IsMenuItemChecked(IDM_WORDWRAP));
            main.ExitViaMenu();
        }
    },
    { std::string("Toggling the sense of the Word Wrap menu item checkbox changes the style bits of the new edit window"), []()
        {
            TestAutomation::MainWindow main;

            auto edit = main.GetEditField();
            Assert::AreEqual(WS_HSCROLL|ES_AUTOHSCROLL, edit.GetStyle() & (WS_HSCROLL|ES_AUTOHSCROLL), "style should include WS_HSCROLL and ES_AUTOHSCROLL"); // unchecked by default

            main.GetMenu().GetFormatMenu().SelectMenuItem(IDM_WORDWRAP);            // toggle wordwrap
            Poll::While(1s, 1ms, [&]() { return edit.IsWindow(); });                // edit window will be destroyed and re-created
            Poll::Until(1s, 1ms, [&]() { return main.GetEditField().IsWindow(); }); // wait until new edit window is created
            edit = main.GetEditField(); // get new one
            Assert::AreEqual(0, edit.GetStyle() & (WS_HSCROLL | ES_AUTOHSCROLL), "style should NOT include WS_HSCROLL and ES_AUTOHSCROLL");

            main.ExitViaMenu();
        }
    },
};

Test GotoTests[] = {
    { std::string("The 'Go To...' menu option is enabled only when word wrap is off"), []()
        {
            TestAutomation::MainWindow main;

            auto format = main.GetMenu().GetFormatMenu();

            Assert::IsFalse(format.IsMenuItemChecked(IDM_WORDWRAP)); // my default is unchecked
            Assert::IsTrue (main.GetMenu().GetEditMenu(  ).IsMenuItemEnabled(IDM_GOTO), "GoTo menu item should be enabled");

            format.SelectMenuItem(IDM_WORDWRAP); // flip the sense of the word wrap
            Poll::While(1s, 1ms, [&]() { return main.GetMenu().GetEditMenu().IsMenuItemEnabled(IDM_GOTO) == true; });
            Assert::IsFalse(main.GetMenu().GetEditMenu().IsMenuItemEnabled(IDM_GOTO), "GoTo menu item should be disabled");

            main.ExitViaMenu();
        }
    },
};
