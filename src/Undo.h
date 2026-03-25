#ifndef UNDO_H
#define UNDO_H

#pragma once
#include <windows.h>
#include <string>
#include <deque>

class Undo
{
    std::deque<std::wstring> undo;
    void UpdateUndo(const std::wstring& current)
    {
        undo.push_back(current);
        while(undo.size() > 2)
            undo.pop_front();
    }
public:
    void UpdateUndo(HWND edit)
    {
        const LRESULT length = SendMessageW(edit, WM_GETTEXTLENGTH, 0, 0);
        std::wstring text(static_cast<size_t>(length) + 1, L'\0');
        SendMessageW(edit, WM_GETTEXT, static_cast<WPARAM>(text.size()), reinterpret_cast<LPARAM>(text.data()));
        text.resize(static_cast<size_t>(length));
        
        UpdateUndo(text);
    }
    bool CanUndo() const { return undo.size() > 1; }
    void Apply(HWND edit)
    {
        auto text = undo.front();
        undo.clear();
        ::SetWindowTextW(edit, text.c_str());
        UpdateUndo(text);
    }
};

#endif
