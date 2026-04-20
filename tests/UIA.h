#pragma once
#ifndef UIA_H
#define UIA_H

class UIAimpl;
class UIA
{
    std::unique_ptr<UIAimpl> impl;
public:
    UIA();
   ~UIA();
    HRESULT Click       (HWND hwnd);
    HRESULT SetText     (HWND hEdit,     const wchar_t* text);
    HRESULT GetText     (HWND hEdit,           wchar_t* text, size_t size);
    HRESULT SelectByName(HWND hComboBox,                             const wchar_t* itemName);
    bool    ClickMenu   (HWND hNotepad,  const wchar_t* topMenuName, const wchar_t* itemName);
    HRESULT CancelPrint (HWND hwndPrint);
};

#endif