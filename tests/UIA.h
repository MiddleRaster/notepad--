#pragma once
#ifndef UIA_H
#define UIA_H

//#include <atlbase.h>

bool ClickMenuItemViaUIA(HWND hwndNotepad, const wchar_t* topMenuName, const wchar_t* itemName);
void ClickPrintDialogCancel(HWND hwndPrint);
HRESULT SelectCustomComboBoxItem(HWND comboBox, const wchar_t* itemName);
HRESULT PushCustomizedFileSaveDialogOkButton(HWND hwndSaveButton);
void GetDirectUIText(HWND hwndDialog, wchar_t* buf, int bufLen);

#endif