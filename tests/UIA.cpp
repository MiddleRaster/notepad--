
#include <atlbase.h>
#include <UIAutomation.h>

#pragma comment(lib, "UIAutomationCore.lib")

bool ClickMenuItemViaUIA(HWND hwndNotepad, const wchar_t* topMenuName, const wchar_t* itemName)
{
    bool ok = false;

    if (hwndNotepad && topMenuName && itemName)
    {
        CComPtr<IUIAutomation> uia;
        if (SUCCEEDED(uia.CoCreateInstance(CLSID_CUIAutomation)))
        {
            CComPtr<IUIAutomationElement> window;
            if (SUCCEEDED(uia->ElementFromHandle(hwndNotepad, &window)) && window)
            {
                CComPtr<IUIAutomationCondition> topCond;
                if (SUCCEEDED(uia->CreatePropertyCondition(UIA_NamePropertyId, CComVariant(topMenuName), &topCond)))
                {
                    CComPtr<IUIAutomationElement> topMenu;
                    if (SUCCEEDED(window->FindFirst(TreeScope_Descendants, topCond, &topMenu)) && topMenu)
                    {
                        // --- Open the top-level menu ---
                        {
                            bool opened = false;

                            // Try InvokePattern
                            CComPtr<IUIAutomationInvokePattern> invoke;
                            if (SUCCEEDED(topMenu->GetCurrentPatternAs(UIA_InvokePatternId, IID_PPV_ARGS(&invoke))) && invoke)
                            {
                                invoke->Invoke();
                                opened = true;
                            }
                            else
                            {
                                // Try ExpandCollapsePattern
                                CComPtr<IUIAutomationExpandCollapsePattern> expand;
                                if (SUCCEEDED(topMenu->GetCurrentPatternAs(UIA_ExpandCollapsePatternId, IID_PPV_ARGS(&expand))) && expand)
                                {
                                    expand->Expand();
                                    opened = true;
                                }
                                else
                                {
                                    // Try LegacyIAccessiblePattern (Inspect.exe uses this)
                                    CComPtr<IUIAutomationLegacyIAccessiblePattern> legacy;
                                    if (SUCCEEDED(topMenu->GetCurrentPatternAs(UIA_LegacyIAccessiblePatternId, IID_PPV_ARGS(&legacy))) && legacy)
                                    {
                                        legacy->DoDefaultAction();
                                        opened = true;
                                    }
                                }
                            }

                            if (opened)
                            {
                                // --- Wait for the popup menu and find the target item ---
                                CComPtr<IUIAutomationCondition> itemCond;
                                if (SUCCEEDED(uia->CreatePropertyCondition(UIA_NamePropertyId, CComVariant(itemName), &itemCond)))
                                {
                                    CComPtr<IUIAutomationElement> targetItem;

                                    // Search scoped to the #32768 popup owned by hwndNotepad
                                    CComPtr<IUIAutomationCondition> popupCond;
                                    if (SUCCEEDED(uia->CreatePropertyCondition(UIA_ClassNamePropertyId, CComVariant(L"#32768"), &popupCond)))
                                    {
                                        CComPtr<IUIAutomationElement> popup;
                                        for (int i=0; i<40 && !popup; ++i)
                                        {
                                            CComPtr<IUIAutomationElement> desktop;
                                            if (SUCCEEDED(uia->GetRootElement(&desktop)) && desktop)
                                                desktop->FindFirst(TreeScope_Descendants, popupCond, &popup);

                                            if (popup)
                                            {   // Verify it belongs to our process, not some other app
                                                int pid{};
                                                popup->get_CurrentProcessId(&pid);
                                                DWORD ourPid{};
                                                GetWindowThreadProcessId(hwndNotepad, &ourPid);
                                                if (static_cast<DWORD>(pid) != ourPid)
                                                    popup.Release(); // wrong process, keep looking
                                            }

                                            if (!popup)
                                                Sleep(25);
                                        }
                                        if (popup)
                                            popup->FindFirst(TreeScope_Descendants, itemCond, &targetItem);
                                    }

                                    if (targetItem)
                                    {
                                        // --- Invoke the menu item ---
                                        bool invoked = false;

                                        // Try InvokePattern
                                        CComPtr<IUIAutomationInvokePattern> invoke2;
                                        if (SUCCEEDED(targetItem->GetCurrentPatternAs(UIA_InvokePatternId, IID_PPV_ARGS(&invoke2))) && invoke2)
                                        {
                                            invoke2->Invoke();
                                            invoked = true;
                                        }
                                        else
                                        {
                                            // Try LegacyIAccessiblePattern
                                            CComPtr<IUIAutomationLegacyIAccessiblePattern> legacy2;
                                            if (SUCCEEDED(targetItem->GetCurrentPatternAs(UIA_LegacyIAccessiblePatternId, IID_PPV_ARGS(&legacy2))) && legacy2)
                                            {
                                                legacy2->DoDefaultAction();
                                                invoked = true;
                                            }
                                        }

                                        if (invoked)
                                        {
                                            ok = true;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return ok;
}
