#ifndef FIND_WINDOWS_H
#define FIND_WINDOWS_H

#include <windows.h>

namespace WindowFinder
{
    struct Is
    {
        static bool Visible(HWND hwnd)
        {
            return !!IsWindowVisible(hwnd);
        }
        static bool UnOwned(HWND hwnd)
        {
            return GetWindow(hwnd, GW_OWNER) == nullptr;
        }
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
    }

    template <typename P>                 auto Predicates(P p)           { return p; }
    template <typename P, typename... Ps> auto Predicates(P p, Ps... ps) { return [p, rest = Predicates(ps...)](HWND hwnd) { return p(hwnd) && rest(hwnd); }; }

    template<typename Pred> HWND FindDesiredChildWindow(HWND parent, Pred&& pred)
    {
        struct Finder
        {
            Pred& pred;
            HWND hwnd{nullptr};
            static BOOL CALLBACK EnumProc(HWND hWnd, LPARAM lParam)
            {
                auto* This = reinterpret_cast<Finder*>(lParam);
                if (!This->pred(hWnd))
                    return TRUE;
                This->hwnd = hWnd;
                return FALSE;
            }
        } finder{pred};
        EnumChildWindows(parent, Finder::EnumProc, reinterpret_cast<LPARAM>(&finder));
        return finder.hwnd;
    }
    template <typename... Ps> HWND FindDesiredChildWindow(HWND parent, Ps... preds) { return FindDesiredChildWindow(parent, Predicates(preds...)); }
}
#endif



