#include <memory>

#include <atlbase.h>
#include <UIAutomation.h>

#include "UIA.h"

#pragma comment(lib, "UIAutomationCore.lib")

class UIAimpl
{
    CComPtr<IUIAutomation> uia;
public:
    UIAimpl() { uia.CoCreateInstance(CLSID_CUIAutomation); }

    HRESULT Click(HWND hwnd)
    {
        HRESULT hr = E_FAIL;
        CComPtr<IUIAutomationElement> el;
        if (SUCCEEDED(hr = uia->ElementFromHandle(hwnd, &el)))
        {
            CComPtr<IUIAutomationInvokePattern> invoke;
            if (SUCCEEDED(hr = el->GetCurrentPatternAs(UIA_InvokePatternId, IID_PPV_ARGS(&invoke))))
                if (SUCCEEDED(hr = invoke->Invoke()))
                    return S_OK;
        }
        return hr;
    }
    HRESULT SetText(HWND hwnd, const wchar_t* text)
    {
        HRESULT hr = E_FAIL;
        CComPtr<IUIAutomationElement> el;
        if (SUCCEEDED(hr = uia->ElementFromHandle(hwnd, &el)))
        {
            el->SetFocus();
            Sleep(50);


#ifdef KEEP
            for (const wchar_t* p=text; *p; ++p)
            {
                INPUT ch[2]{};
                ch[0].type       = INPUT_KEYBOARD;
                ch[0].ki.wScan   = *p;
                ch[0].ki.dwFlags = KEYEVENTF_UNICODE;
                ch[1]            = ch[0];
                ch[1].ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;
                SendInput(2, ch, sizeof(INPUT));
                Sleep(10);
            }
#endif
            CComPtr<IUIAutomationValuePattern> valuePattern;
            if (SUCCEEDED(hr = el->GetCurrentPatternAs(UIA_ValuePatternId, IID_PPV_ARGS(&valuePattern))))
                hr = valuePattern->SetValue(CComBSTR(text));
        }

        SetForegroundWindow(GetAncestor(hwnd /*Edit*/, GA_ROOT));
        SetFocus(hwnd /*Edit*/);

        // Select all via SendInput Ctrl+A to trigger IFileSaveDialog's internal something-or-other
        INPUT inputs[4]{};
        inputs[0].type       = INPUT_KEYBOARD;
        inputs[0].ki.wVk     = VK_CONTROL;
        inputs[1].type       = INPUT_KEYBOARD;
        inputs[1].ki.wVk     = 'A';
        inputs[2]            = inputs[1];
        inputs[2].ki.dwFlags = KEYEVENTF_KEYUP;
        inputs[3]            = inputs[0];
        inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;
        SendInput(4, inputs, sizeof(INPUT));

        return hr;
    }
    HRESULT GetText(HWND hwnd, wchar_t* buffer, size_t size)
    {
        buffer[0] = L'\0';
        HRESULT hr;
        CComPtr<IUIAutomationElement> element;
        if (SUCCEEDED(hr = uia->ElementFromHandle(hwnd, &element))) {
            CComPtr<IUIAutomationValuePattern> value;
            if (SUCCEEDED(hr = element->GetCurrentPatternAs(UIA_ValuePatternId, IID_PPV_ARGS(&value)))) {
                CComBSTR bstr;
                if (SUCCEEDED(hr = value->get_CurrentValue(&bstr))) {
                    size_t len = bstr.Length();
                    if (len >= size)
                        len  = size-1;
                    memcpy(buffer, bstr, len*sizeof(wchar_t));
                    buffer[len] = L'\0';
                }
            }
        }
        return hr;
    }

    HRESULT SelectByName(HWND hComboBox, const wchar_t* itemName)
    {
        CComPtr<IUIAutomationElement> pCombo;
        HRESULT hr = uia->ElementFromHandle(hComboBox, &pCombo);
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
        uia->CreateCacheRequest(&pCache);
        pCache->AddProperty(UIA_NamePropertyId);
        pCache->AddPattern(UIA_SelectionItemPatternId);

        CComVariant                     varItemName(itemName);
        CComPtr<IUIAutomationCondition> pItemCondition;
        uia->CreatePropertyCondition(UIA_NamePropertyId, varItemName, &pItemCondition);

        CComPtr<IUIAutomationElement> pTargetItem;
        hr = pCombo->FindFirstBuildCache(TreeScope_Descendants, pItemCondition, pCache, &pTargetItem);
        if (FAILED(hr) || !pTargetItem) return E_FAIL;

        CComPtr<IUIAutomationSelectionItemPattern> pSelectionPattern;
        hr = pTargetItem->GetCachedPatternAs(UIA_SelectionItemPatternId, IID_PPV_ARGS(&pSelectionPattern));
        if (SUCCEEDED(hr) && pSelectionPattern)
            hr = pSelectionPattern->Select();
        return hr;
    }
};
        UIA::~UIA() = default;
        UIA::UIA          (                                             ) {        impl = std::make_unique<UIAimpl>(); }
HRESULT UIA::Click        (HWND hwnd                                    ) { return impl->Click       (hwnd); }
HRESULT UIA::SetText      (HWND hwnd, const wchar_t* text               ) { return impl->SetText     (hwnd, text); }
HRESULT UIA::GetText      (HWND hwnd,       wchar_t* buffer, size_t size) { return impl->GetText     (hwnd, buffer, size); }
HRESULT UIA::SelectByName (HWND hwnd, const wchar_t* itemName           ) { return impl->SelectByName(hwnd, itemName); }

void GetDirectUIText(HWND hwndDialog, wchar_t* buf, int bufLen)
{
    buf[0] = L'\0';

    CComPtr<IUIAutomation>        uia;
    if (SUCCEEDED(uia.CoCreateInstance(__uuidof(CUIAutomation))))
    {
        CComPtr<IUIAutomationElement> root;
        if (SUCCEEDED(uia->ElementFromHandle(hwndDialog, &root)))
        {
            CComPtr<IUIAutomationCondition> trueCondition;
            if (SUCCEEDED(uia->CreateTrueCondition(&trueCondition)))
            {
                CComPtr<IUIAutomationElementArray> elements;
                if (SUCCEEDED(root->FindAll(TreeScope_Subtree, trueCondition, &elements)))
                {
                    int count = 0;
                    elements->get_Length(&count);
                    for (int i=0; i<count; i++)
                    {
                        CComPtr<IUIAutomationElement> el;
                        if (SUCCEEDED(elements->GetElement(i, &el)))
                        {
                            CComBSTR name;
                            if (SUCCEEDED(el->get_CurrentName(&name)) && name.Length() > 0)
                            {
                                if (buf[0] != L'\0')
                                    wcsncat_s(buf, bufLen, L" | ", _TRUNCATE);
                                wcsncat_s(buf, bufLen, name, _TRUNCATE);
                            }
                        }
                    }
                }
            }
        }
    }
}

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