#ifndef STATUSBAR_H
#define STATUSBAR_H

#pragma once
#include <windows.h>
#include <commctrl.h>

#include "FileIO.h"

class StatusBar
{
    HWND hStatusBar = nullptr;
public:
    void Create(HWND hWndParent) { hStatusBar = CreateStatusWindow(WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|CCS_BOTTOM, L"", hWndParent, IDC_STATUSBAR); }
    
    void ResizeStatusBar(HWND hWndParent)
    {
        RECT rc;
        GetClientRect(hWndParent, &rc);

        int parts[2];
        parts[0] = rc.right - 125;
        parts[1] = -1;   // auto‑size to the right edge
        SendMessage(hStatusBar, SB_SETPARTS, 2, (LPARAM)parts);
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
    void UpdateStatusBar(HWND edit, FileIO::Encoding encoding)
    {
        DWORD selStart=0, selEnd=0;
                       SendMessage(edit, EM_GETSEL,       (WPARAM)&selStart, (LPARAM)&selEnd);
        LRESULT row  = SendMessage(edit, EM_LINEFROMCHAR, selStart, 0);
        LRESULT line = SendMessage(edit, EM_LINEINDEX,    row,      0);
        std::wstring buf = std::format(L"Ln {}, Col {}", row, selStart-line);
        SendMessageW(hStatusBar, SB_SETTEXT, 0, reinterpret_cast<LPARAM>(buf.c_str()));

        switch (encoding)
        {
        default:
        case FileIO::Encoding::eANSI     : SendMessageW(hStatusBar, SB_SETTEXT, 1, reinterpret_cast<LPARAM>(L"ANSI"              ));    break;
        case FileIO::Encoding::eUNICODE  : SendMessageW(hStatusBar, SB_SETTEXT, 1, reinterpret_cast<LPARAM>(L"Unicode"           ));    break;
        case FileIO::Encoding::eUNICODEbe: SendMessageW(hStatusBar, SB_SETTEXT, 1, reinterpret_cast<LPARAM>(L"Unicode big endian"));    break;
        case FileIO::Encoding::eUTF8     : SendMessageW(hStatusBar, SB_SETTEXT, 1, reinterpret_cast<LPARAM>(L"UTF-8 with BOM"    ));    break;
        case FileIO::Encoding::eUTF8noBOM: SendMessageW(hStatusBar, SB_SETTEXT, 1, reinterpret_cast<LPARAM>(L"UTF-8 no BOM"      ));    break;
        }
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