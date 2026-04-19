#pragma once
#ifndef UIA_H
#define UIA_H

//#include <atlbase.h>

bool ClickMenuItemViaUIA(HWND hwndNotepad, const wchar_t* topMenuName, const wchar_t* itemName);
void ClickPrintDialogCancel(HWND hwndPrint);
HRESULT SelectCustomComboBoxItem(HWND comboBox, const wchar_t* itemName);
void GetDirectUIText(HWND hwndDialog, wchar_t* buf, int bufLen);

class UIAimpl;
class UIA
{
    std::unique_ptr<UIAimpl> impl;
public:
    UIA();
   ~UIA();
    HRESULT Click       (HWND hwnd);
    HRESULT SetText     (HWND hEdit,     const wchar_t* text);
    HRESULT GetText     (HWND hEdit,           wchar_t* buffer, size_t size);
    HRESULT SelectByName(HWND hComboBox, const wchar_t* itemName);
};

#endif