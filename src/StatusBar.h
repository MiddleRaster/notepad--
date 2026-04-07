#ifndef STATUSBAR_H
#define STATUSBAR_H

#pragma once
#include <windows.h>
#include <commctrl.h>

class StatusBar
{
    HWND hStatusBar = nullptr;
public:
    void Create(HWND hWndParent) { hStatusBar = CreateStatusWindow(WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|CCS_BOTTOM, L"", hWndParent, IDC_STATUSBAR); }
    
    void ResizeStatusBar(HWND hWndParent)
    {
        RECT rc;
        GetClientRect(hWndParent, &rc);

        // Example: three parts, last one stretches
        //int parts[3];
        //parts[0] = rc.right - 200;
        //parts[1] = rc.right - 100;
        //parts[2] = -1;   // auto‑size to the right edge

        int parts[1] = {-1};
        SendMessage(hStatusBar, SB_SETPARTS, 1, (LPARAM)parts);
        SendMessage(hStatusBar, WM_SIZE, 0, 0);
    }
    int GetHeight() const
    {
        if (!hStatusBar || !IsWindowVisible(hStatusBar))
            return 0;

        RECT rc;
        GetWindowRect(hStatusBar, &rc);
        return rc.bottom - rc.top;
    }
    void UpdateStatusBar(HWND edit)
    {
        DWORD selStart=0, selEnd=0;
                       SendMessage(edit, EM_GETSEL,       (WPARAM)&selStart, (LPARAM)&selEnd);
        LRESULT row  = SendMessage(edit, EM_LINEFROMCHAR, selStart, 0);
        LRESULT line = SendMessage(edit, EM_LINEINDEX,    row,      0);

        std::wstring buf = std::format(L"Ln {}, Col {}", row, selStart-line);
        SendMessageW(hStatusBar, SB_SETTEXT, 0, (LPARAM)buf.c_str());
    }
    void OnWordWrap(bool wrapped)
    {
        if (wrapped)
            ShowWindow(hStatusBar, SW_HIDE);
        else
            ShowWindow(hStatusBar, SW_SHOW);
        SendMessage(hStatusBar, WM_SIZE, 0, 0);
    }
};

#endif