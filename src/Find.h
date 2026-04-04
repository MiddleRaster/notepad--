#ifndef FIND_H
#define FIND_H

#pragma once
#include <windows.h>
#include <commdlg.h>
#include <cwctype>

class Find
{
    void FillInStruct(HWND hWnd, HWND hEdit, HWND* pDialog, const std::wstring& text)
    {
        pDlg         = pDialog; // hang onto pointer, so we can zero it out when we terminate
        edit         = hEdit;
        fr.hwndOwner = hWnd;
        fr.Flags    &= ~FR_DIALOGTERM;  // unset FR_DIALOGTERM, otherwise we'll just terminate again
        text.copy(fr.lpstrFindWhat, 260, 0);
    }

    FINDREPLACEW fr  = {};
    WCHAR    findBuf[260]{}; // TODO:  REVIEW:  BUGBUG:  alloc as wstring?
    WCHAR replaceBuf[260]{}; // TODO:  REVIEW:  BUGBUG:  alloc as wstring?
    HWND  edit{};
    HWND* pDlg{};

public:
    Find()
    { // initialize with things that don't change
        fr.lStructSize      = sizeof(fr);
        fr.lpstrFindWhat    = findBuf;
        fr.wFindWhatLen     = ARRAYSIZE(findBuf);
        fr.lpstrReplaceWith = replaceBuf;
        fr.wReplaceWithLen  = ARRAYSIZE(replaceBuf);
        fr.Flags            = FR_DOWN;
    }
    void DisplayFindDialog(HWND hWnd, HWND hEdit, HWND* pDialog, const std::wstring& text)
    {
        FillInStruct(hWnd, hEdit, pDialog, text);
        *pDlg = FindText(&fr);
    }
    void DisplayReplaceDialog(HWND hWnd, HWND hEdit, HWND* pDialog, const std::wstring& text)
    {
        FillInStruct(hWnd, hEdit, pDialog, text);
        *pDlg = ReplaceText(&fr);
    }

    bool FindNext()
    {
        if (fr.lpstrFindWhat[0] == '\0')
            return false; // no string to search for yet. Return false to let MessageHandler's Handle_Find handle it
        OnFindNext();
        return true;
    }
    bool HandleMessage()
    {
        if (fr.Flags & FR_DIALOGTERM) { *pDlg = nullptr; }
        if (fr.Flags & FR_FINDNEXT)   { OnFindNext  ();  }
        if (fr.Flags & FR_REPLACE)    { OnReplace   ();  }
        if (fr.Flags & FR_REPLACEALL) { OnReplaceAll();  }
        return true;
    }
private:
    void OnFindOrReplace(auto onFoundLambda)
    {
        DWORD s=0, e=0;
        if (fr.Flags & FR_DOWN ? FindNextOne    (fr.lpstrFindWhat, !!(fr.Flags & FR_MATCHCASE), !!(fr.Flags & FR_WHOLEWORD), s, e)
                               : FindPreviousOne(fr.lpstrFindWhat, !!(fr.Flags & FR_MATCHCASE), !!(fr.Flags & FR_WHOLEWORD), s, e))
        {
            onFoundLambda(s, e);
            SendMessage(edit, EM_SETSEL,      s, e);
            SendMessage(edit, EM_SCROLLCARET, 0, 0);
            SetFocus   (edit);
        } else {
            EnableWindow(*pDlg, FALSE);
            MessageBox  (*pDlg, std::format(L"Cannot find '{}'", fr.lpstrFindWhat).c_str(), L"Find", MB_OK | MB_ICONINFORMATION | MB_SYSTEMMODAL);
            EnableWindow(*pDlg, TRUE);
        }
    }
    void OnFindNext() { OnFindOrReplace([](DWORD, DWORD) {}); }
    void OnReplace()
    {
        OnFindOrReplace([&](DWORD s, DWORD& e)  {   // s & e are filled out:  cut the string there.
                                                    std::wstring text  = GetEditText();
                                                    std::wstring end   = text.substr(e);
                                                    std::wstring start = text.substr(0, s);

                                                    start += fr.lpstrReplaceWith;
                                                    e      = static_cast<DWORD>(start.length());
                                                    start += end;

                                                    SetWindowTextW(edit, start.c_str());
                                                    SendMessageW  (edit, EM_SETMODIFY, TRUE, 0); // a successful replacement dirties the edit field
                                                });
    }
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
        return FindInText(GetEditText(), selStart != selEnd ? selStart+1 : selStart, findWhat, matchCase, wholeWord, outStart, outEnd);
    }
    bool FindPreviousOne(const std::wstring& findWhat, bool matchCase, bool wholeWord, DWORD& outStart, DWORD& outEnd)
    { // re-use FindInText, but reverse the strings first. Clever, eh?
        DWORD selStart=0, selEnd=0;
        SendMessage(edit, EM_GETSEL, reinterpret_cast<WPARAM>(&selStart), reinterpret_cast<LPARAM>(&selEnd));
        std::wstring haystack = GetEditText();
        DWORD totalLen = static_cast<DWORD>(haystack.size());

        std::wstring revHaystack(haystack.rbegin(), haystack.rend());
        std::wstring revNeedle  (findWhat.rbegin(), findWhat.rend());
        DWORD searchFrom = totalLen - selStart;  // mirror selStart

        DWORD revStart=0, revEnd=0;
        if (!FindInText(revHaystack, searchFrom, revNeedle, matchCase, wholeWord, revStart, revEnd))
            return false;

        outStart = totalLen - revEnd;
        outEnd = totalLen - revStart;
        return true;
    }
    bool FindInText(const std::wstring& editFieldText, size_t searchFrom, const std::wstring& findWhat, bool matchCase, bool wholeWord, DWORD& outStart, DWORD& outEnd)
    {
        std::wstring haystack = editFieldText;
        std::wstring needle   = findWhat;
        if (!matchCase) {
            std::transform(haystack.begin(), haystack.end(), haystack.begin(), [](wchar_t c) { return std::towlower(c); });
            std::transform(  needle.begin(),   needle.end(),   needle.begin(), [](wchar_t c) { return std::towlower(c); });
        }
        size_t pos = haystack.find(needle, searchFrom);
        if (pos == std::wstring::npos)
            return false;

        if (wholeWord)
        {
            while (pos != std::wstring::npos)
            {
                bool  leftOk = (pos == 0) || !iswalnum(haystack[pos - 1]);
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
