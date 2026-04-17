#include <atlbase.h>
#include <UIAutomation.h>

#pragma comment(lib, "UIAutomationCore.lib")

HRESULT SelectCustomComboBoxItem(HWND hComboBox, const wchar_t* itemName)
{
    CComPtr<IUIAutomation> pAutomation;
    HRESULT hr = pAutomation.CoCreateInstance(CLSID_CUIAutomation);
    if (FAILED(hr)) return hr;

    CComPtr<IUIAutomationElement> pCombo;
    hr = pAutomation->ElementFromHandle(hComboBox, &pCombo);
    if (FAILED(hr)) return hr;

    // Expand via UIA: triggers deferred item loading without a stray popup
    CComPtr<IUIAutomationExpandCollapsePattern> pExpand;
    pCombo->GetCurrentPatternAs(UIA_ExpandCollapsePatternId, IID_PPV_ARGS(&pExpand));
    if (pExpand)
        pExpand->Expand();
    else
        SendMessage(hComboBox, CB_SHOWDROPDOWN, TRUE, 0);

    // Fetch name + SelectionItemPattern in one cross-process round-trip
    CComPtr<IUIAutomationCacheRequest> pCache;
    pAutomation->CreateCacheRequest(&pCache);
    pCache->AddProperty(UIA_NamePropertyId);
    pCache->AddPattern(UIA_SelectionItemPatternId);

    CComVariant                     varItemName(itemName);
    CComPtr<IUIAutomationCondition> pItemCondition;
    pAutomation->CreatePropertyCondition(UIA_NamePropertyId, varItemName, &pItemCondition);

    CComPtr<IUIAutomationElement> pTargetItem;
    hr = pCombo->FindFirstBuildCache(TreeScope_Descendants, pItemCondition, pCache, &pTargetItem);
    if (FAILED(hr) || !pTargetItem) return E_FAIL;

    CComPtr<IUIAutomationSelectionItemPattern> pSelectionPattern;
    hr = pTargetItem->GetCachedPatternAs(UIA_SelectionItemPatternId, IID_PPV_ARGS(&pSelectionPattern));
    if (SUCCEEDED(hr) && pSelectionPattern)
        hr = pSelectionPattern->Select();
    return hr;
}

bool ClickMenuItemViaUIA(HWND hwndNotepad, const wchar_t* topMenuName, const wchar_t* itemName)
{
    CComPtr<IUIAutomation> uia;
    if (FAILED(uia.CoCreateInstance(CLSID_CUIAutomation)))
        return false;

    CComPtr<IUIAutomationElement> window;
    if (FAILED(uia->ElementFromHandle(hwndNotepad, &window)) || !window)
        return false;

    CComPtr<IUIAutomationCondition> topCond;
    if (FAILED(uia->CreatePropertyCondition(UIA_NamePropertyId, CComVariant(topMenuName), &topCond)))
        return false;

    CComPtr<IUIAutomationElement> topMenu;
    if (FAILED(window->FindFirst(TreeScope_Descendants, topCond, &topMenu)) || !topMenu)
        return false;

    // --- Open the top-level menu ---
    bool opened = false;

    CComPtr<IUIAutomationInvokePattern> invoke;
    if (SUCCEEDED(topMenu->GetCurrentPatternAs(UIA_InvokePatternId, IID_PPV_ARGS(&invoke))) && invoke)
    {
        invoke->Invoke();
        opened = true;
    }
    if (!opened)
    {
        CComPtr<IUIAutomationExpandCollapsePattern> expand;
        if (SUCCEEDED(topMenu->GetCurrentPatternAs(UIA_ExpandCollapsePatternId, IID_PPV_ARGS(&expand))) && expand)
        {
            expand->Expand();
            opened = true;
        }
    }
    if (!opened)
    {
        CComPtr<IUIAutomationLegacyIAccessiblePattern> legacy;
        if (SUCCEEDED(topMenu->GetCurrentPatternAs(UIA_LegacyIAccessiblePatternId, IID_PPV_ARGS(&legacy))) && legacy)
        {
            legacy->DoDefaultAction();
            opened = true;
        }
    }
    if (!opened)
        return false;

    // --- Wait for the popup menu and find the target item ---
    CComPtr<IUIAutomationCondition> itemCond;
    if (FAILED(uia->CreatePropertyCondition(UIA_NamePropertyId, CComVariant(itemName), &itemCond)))
        return false;

    CComPtr<IUIAutomationCondition> popupCond;
    if (FAILED(uia->CreatePropertyCondition(UIA_ClassNamePropertyId, CComVariant(L"#32768"), &popupCond)))
        return false;

    CComPtr<IUIAutomationElement> popup;
    for (int i = 0; i < 40 && !popup; ++i)
    {
        CComPtr<IUIAutomationElement> desktop;
        if (SUCCEEDED(uia->GetRootElement(&desktop)) && desktop)
            desktop->FindFirst(TreeScope_Descendants, popupCond, &popup);

        if (popup)
        {
            int  pid{};
            DWORD ourPid{};
            popup->get_CurrentProcessId(&pid);
            GetWindowThreadProcessId(hwndNotepad, &ourPid);
            if (static_cast<DWORD>(pid) != ourPid)
                popup.Release();
        }

        if (!popup)
            Sleep(25);
    }
    if (!popup)
        return false;

    CComPtr<IUIAutomationElement> targetItem;
    if (FAILED(popup->FindFirst(TreeScope_Descendants, itemCond, &targetItem)) || !targetItem)
        return false;

    // --- Invoke the menu item ---
    CComPtr<IUIAutomationInvokePattern> invoke2;
    if (SUCCEEDED(targetItem->GetCurrentPatternAs(UIA_InvokePatternId, IID_PPV_ARGS(&invoke2))) && invoke2)
    {
        invoke2->Invoke();
        return true;
    }
    CComPtr<IUIAutomationLegacyIAccessiblePattern> legacy2;
    if (SUCCEEDED(targetItem->GetCurrentPatternAs(UIA_LegacyIAccessiblePatternId, IID_PPV_ARGS(&legacy2))) && legacy2)
    {
        legacy2->DoDefaultAction();
        return true;
    }
    return false;
}

void ClickPrintDialogCancel(HWND hwndPrint)
{
    CComPtr<IUIAutomation> uia;
    if (FAILED(uia.CoCreateInstance(CLSID_CUIAutomation)))
        return;

    CComPtr<IUIAutomationElement> rootEl;
    if (FAILED(uia->ElementFromHandle(hwndPrint, &rootEl)) || !rootEl)
        return;

    CComPtr<IUIAutomationCondition> nameCond;
    uia->CreatePropertyCondition(UIA_NamePropertyId, CComVariant(L"Cancel"), &nameCond);

    CComPtr<IUIAutomationCondition> typeCond;
    uia->CreatePropertyCondition(UIA_ControlTypePropertyId, CComVariant(UIA_ButtonControlTypeId), &typeCond);

    CComPtr<IUIAutomationCondition> andCond;
    uia->CreateAndCondition(nameCond, typeCond, &andCond);

    CComPtr<IUIAutomationElement> cancelBtn;
    rootEl->FindFirst(TreeScope_Descendants, andCond, &cancelBtn);
    if (!cancelBtn)
        return;

    CComPtr<IUIAutomationInvokePattern> invoke;
    cancelBtn->GetCurrentPatternAs( UIA_InvokePatternId, IID_IUIAutomationInvokePattern, reinterpret_cast<void**>(&invoke));
    if (!invoke)
        return;

    invoke->Invoke();
}