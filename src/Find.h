#ifndef FIND_H
#define FIND_H

#pragma once
#include <windows.h>
#include <commdlg.h>
#include <cwctype>

class Find
{
    FINDREPLACEW fr  = {};
    WCHAR findBuf[260]{}; // TODO:  REVIEW:  BUGBUG:  alloc as wstring?
    HWND edit{};
    HWND* pDlg{};

public:
    Find()
    { // initialize with things that don't change
        fr.lStructSize      = sizeof(fr);
        fr.lpstrFindWhat    = findBuf;
        fr.wFindWhatLen     = ARRAYSIZE(findBuf);
     // fr.lpstrReplaceWith = szReplaceBuf; // only for ReplaceText
     // fr.wReplaceWithLen  = ARRAYSIZE(szReplaceBuf);
    }
    void Display(HWND hWnd, HWND hEdit, HWND* pDialog, const std::wstring& text)
    {
        pDlg         = pDialog; // hang onto pointer, so we can zero it out when we terminate
        edit         = hEdit;
        fr.hwndOwner = hWnd;
        text.copy(fr.lpstrFindWhat, 260, 0);
        fr.Flags     = FR_DOWN; // (re)set flags, otherwise we'll hit FR_DIALOGTERM again

        *pDlg = FindText(&fr);
    }
    bool HandleMessage()
    {
        if (fr.Flags & FR_DIALOGTERM)
            *pDlg = nullptr;
        if (fr.Flags & FR_FINDNEXT)   { OnFindNext  (); }
        if (fr.Flags & FR_REPLACE)    { OnReplace   (); }
        if (fr.Flags & FR_REPLACEALL) { OnReplaceAll(); }
        return true;
    }
private:
    void OnFindNext()
    {
        DWORD s=0, e=0;
        if (FindNextOne(fr.lpstrFindWhat, !!(fr.Flags&FR_MATCHCASE), !!(fr.Flags&FR_WHOLEWORD), s, e))
        {
            SendMessage(edit, EM_SETSEL,      s, e);
            SendMessage(edit, EM_SCROLLCARET, 0, 0);
            SetFocus   (edit);
        } else {
            EnableWindow(*pDlg, FALSE);
            MessageBox  (*pDlg, std::format(L"Cannot find '{}'", fr.lpstrFindWhat).c_str(), L"Find", MB_OK | MB_ICONINFORMATION | MB_SYSTEMMODAL);
            EnableWindow(*pDlg, TRUE);
        }
    }
    void OnReplace() {}
    void OnReplaceAll() {}

    std::wstring GetEditText()
    {
        int len = GetWindowTextLengthW(edit);
        if (len <= 0)
            return {};

        std::wstring text(len + 1, L'\0');
        GetWindowTextW(edit, text.data(), len + 1);
        text.resize(len);
        return text;
    }
    bool FindNextOne(const std::wstring& findWhat, bool matchCase, bool wholeWord, DWORD& outStart, DWORD& outEnd)
    {

        DWORD selStart=0, selEnd=0;
        SendMessage(edit, EM_GETSEL, reinterpret_cast<WPARAM>(&selStart), reinterpret_cast<LPARAM>(&selEnd));

        std::wstring haystack = GetEditText();
        std::wstring needle   = findWhat;
        if (!matchCase) {
            std::transform(haystack.begin(), haystack.end(), haystack.begin(), [](wchar_t c) { return std::towlower(c); });
            std::transform(  needle.begin(),   needle.end(),   needle.begin(), [](wchar_t c) { return std::towlower(c); });
        }

        size_t searchFrom = selStart+1;
        size_t pos = haystack.find(needle, searchFrom);

        if (pos == std::wstring::npos)
            return false;

        if (wholeWord)
        {
            while (pos != std::wstring::npos)
            {
                bool leftOk = (pos == 0) || !iswalnum(haystack[pos - 1]);
                bool rightOk = (pos+needle.size() >= haystack.size()) || !iswalnum(haystack[pos+needle.size()]);
                if (leftOk && rightOk)
                    break;
                pos = haystack.find(needle, pos+1);
            }
            if (pos == std::wstring::npos)
                return false;
        }

        outStart = static_cast<DWORD>(pos);
        outEnd   = static_cast<DWORD>(pos + needle.size());
        return true;
    }
};

#endif
