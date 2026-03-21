#ifndef FIND_WINDOWS_H
#define FIND_WINDOWS_H

#include <windows.h>

namespace WindowFinder
{
    struct Is
    {
        static bool Visible(HWND hwnd) { return !!IsWindowVisible(hwnd); }
        static bool UnOwned(HWND hwnd) { return GetWindow(hwnd, GW_OWNER) == nullptr; }
    };
    namespace Has
    {
        struct Pid
        {
            DWORD pid;
            bool operator()(HWND hwnd) const
            {
                DWORD dw = 0;
                GetWindowThreadProcessId(hwnd, &dw);
                return pid == dw;
            }
        };
        struct ClassName
        {
            const std::wstring className;
            bool operator()(HWND hwnd) const
            {
                wchar_t cls[256]{};
                GetClassNameW(hwnd, cls, static_cast<int>(std::size(cls)));
                return std::wcscmp(cls, className.c_str()) == 0;
            }
        };
        class NotStyle
        {
            DWORD notThisStyle{};
        public:
            NotStyle(DWORD dw) : notThisStyle(dw) {}
            bool operator()(HWND hwnd) const
            {
                DWORD style = GetWindowLong(hwnd, GWL_STYLE);
                if (style & notThisStyle)
                    return false;
                return true;
            }
        };
    }

    HWND FindDesiredChildWindow(HWND parent, auto... preds)
    {
        auto combined = [...ps = preds](HWND hwnd) { return (ps(hwnd) && ...); };
        struct Finder
        {
            decltype(combined) pred;
            HWND hwnd{};
        } finder{combined};

        EnumChildWindows(parent,[](HWND hWnd, LPARAM lParam) -> BOOL
                                {
                                    auto* finder = reinterpret_cast<Finder*>(lParam);
                                    if (!finder->pred(hWnd))
                                        return TRUE;
                                    finder->hwnd = hWnd;
                                    return FALSE;
                                }, reinterpret_cast<LPARAM>(&finder));
        return finder.hwnd;
    }
}
#endif



