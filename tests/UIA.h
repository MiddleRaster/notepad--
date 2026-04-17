#pragma once
#ifndef UIA_H
#define UIA_H

bool ClickMenuItemViaUIA(HWND hwndNotepad, const wchar_t* topMenuName, const wchar_t* itemName);
void ClickPrintDialogCancel(HWND hwndPrint);
HRESULT SelectCustomComboBoxItem(HWND comboBox, const wchar_t* itemName);

#endif