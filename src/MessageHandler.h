#pragma once

#include <filesystem>
#include <string>
#include <vector>
#include <windows.h>
#undef min
#undef max
#include <commdlg.h>
#include <shellapi.h>

#include "Resource.h"
#include "FileIO.h"
#include "PrintEngine.h"
#include "Undo.h"
#include "Find.h"



class MessageHandler
{
    void GetEditFieldSelection(DWORD& start, DWORD& end) const { SendMessage(edit, EM_GETSEL, (WPARAM)&start, (LPARAM)&end); }
    std::wstring GetEditFieldText() const
    {
        const LRESULT length = SendMessageW(edit, WM_GETTEXTLENGTH, 0, 0);
        std::wstring text(static_cast<size_t>(length) + 1, L'\0');
        SendMessageW(edit, WM_GETTEXT, static_cast<WPARAM>(text.size()), reinterpret_cast<LPARAM>(text.data()));
        text.resize(static_cast<size_t>(length));
        return text;
    }
    std::wstring GetSelectedText(DWORD start, DWORD end) const
    {
        if (start == end)
            return {};

        std::wstring selection = GetEditFieldText();
        return selection.substr(start, end - start);
    }

    HWND edit{};
    std::filesystem::path FilePath{};
    Undo undo;

    HWND hDlgFind = nullptr;
    UINT uFindMsg{};
    Find find;

    bool wordWrap = false; // my default is 'no word wrap' which is the opposite of new Notepad.

    int Handle_WM_CREATE(HWND hWnd)
    {
        edit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD|WS_VISIBLE|WS_VSCROLL|WS_HSCROLL|ES_LEFT|ES_MULTILINE|ES_AUTOVSCROLL|ES_AUTOHSCROLL, 0, 0, 0, 0, hWnd, reinterpret_cast<HMENU>(IDC_EDITFIELD), GetModuleHandle(nullptr), nullptr);
        if (edit == nullptr)
        {
            MessageBoxW(hWnd, L"Failed to create edit control. The application will now exit.", L"Notepad--", MB_OK | MB_ICONERROR);
            delete this;
            SetWindowLongPtrW(hWnd, GWLP_USERDATA, 0);
            return -1;
        }
        SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

        // if there's a filename on the command-line, load it up
        LoadCommandlineArgIfAny(hWnd);

        // set up undo
        undo.UpdateUndo(edit);

        return 0;
    }
    void Handle_WM_NCDESTROY(HWND hWnd)
    {
        SetWindowLongPtrW(hWnd, GWLP_USERDATA, 0);
        delete this;
    }
    void LoadCommandlineArgIfAny(HWND hWnd)
    {
        int argc = 0;
        struct ArgvGuard {
            LPWSTR* argv;
            explicit ArgvGuard(int* argc) : argv(CommandLineToArgvW(GetCommandLineW(), argc)) {}
            ~ArgvGuard() { if (argv) LocalFree(argv); }
        } args(&argc);
        if ((args.argv == nullptr) || (argc <= 1))
            return;
        if (!FileIO::LoadFileToEdit(hWnd, edit, args.argv[1]))
            MessageBoxW(hWnd, L"Failed to open file.", L"Notepad--", MB_OK | MB_ICONERROR);
        FilePath = args.argv[1];
    }

    auto GetWantToSaveChangedMessageBoxChoice(HWND hWnd)
    {
        std::wstring filename = FilePath.filename();
        if (filename == L"")
            filename = L"Untitled";
        return MessageBoxW(hWnd, std::format(L"Do you want to save changes to {}?", filename).c_str(), L"Notepad--", MB_YESNOCANCEL | MB_ICONEXCLAMATION);
    }
    bool IsDirty() const
    {
        return SendMessageW(edit, EM_GETMODIFY, 0, 0) != 0;
    }

public:
    static MessageHandler* GetHandler(HWND hWnd) { return reinterpret_cast<MessageHandler*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA)); }
    static int CreateHandlerAndEditWindow(HWND hWnd)
    {
        if (GetWindowLongPtrW(hWnd, GWLP_USERDATA) != LONG_PTR(0))
            return 0;
        auto* This = new MessageHandler();
        return This->Handle_WM_CREATE(hWnd);
    }
    static void DestroyHandler(HWND hWnd)
    {
        auto* This = reinterpret_cast<MessageHandler*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
        if (This != nullptr)
            This->Handle_WM_NCDESTROY(hWnd);
    }

    void Handle_WM_SIZE(int width, int height) { MoveWindow(edit, 0, 0, width, height, TRUE); }
    void Handle_Activate(int activateState) { if (activateState != WA_INACTIVE) SetFocus(edit); }

    void Handle_FileSave(HWND hWnd)
    {
        bool titled = FilePath.has_filename();
        bool clean  = !IsDirty();

        // 4 cases:
        // !titled &  clean => Handle_FileSaveAs(hWnd);
        //  titled &  clean => return; // noop
        // !titled & !clean => Handle_FileSaveAs(hWnd);
        //  titled & !clean => SaveFilePrivate(hWnd, FilePath.c_str());

        if (titled && clean)
            return;
        if (!titled)
            Handle_FileSaveAs(hWnd);
        else
            FileIO::SaveFile(hWnd, edit, FilePath.c_str(), FilePath);
    }

    void Handle_FileSaveAs(HWND hWnd)
    {
        wchar_t filePath[MAX_PATH] = L"";
        OPENFILENAMEW ofn{};
        ofn.lStructSize  = sizeof(ofn);
        ofn.hwndOwner    = hWnd;
        ofn.lpstrFile    = filePath;
        ofn.nMaxFile     = MAX_PATH;
        ofn.lpstrFilter  = L"Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
        ofn.nFilterIndex = 1;
        ofn.Flags        = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
        if (GetSaveFileNameW(&ofn))
            FileIO::SaveFile(hWnd, edit, filePath, FilePath);
    }

    void Handle_Print(HWND hWnd)
    {
        PRINTDLGW pd{};
        pd.lStructSize = sizeof(pd);
        pd.hwndOwner   = hWnd;
        pd.Flags       = PD_RETURNDC | PD_NOPAGENUMS | PD_NOSELECTION;
        if (!PrintDlgW(&pd))
            return;

        DOCINFOW di{};
        di.cbSize      = sizeof(di);
        di.lpszDocName = L"Notepad--";
        StartDocW(pd.hDC, &di);
        PrintEngine::PrintToHdc(pd.hDC, GetEditFieldText());
        EndDoc(pd.hDC);
        DeleteDC(pd.hDC);
    }

    void Handle_Exit(HWND hWnd)
    {
        if (!IsDirty())
        {
            DestroyWindow(hWnd);
            return;
        }

        switch (GetWantToSaveChangedMessageBoxChoice(hWnd))
        {
        case IDYES:
            Handle_FileSaveAs(hWnd);
            DestroyWindow(hWnd);
            break;
        case IDNO:
            DestroyWindow(hWnd);
            break;
        case IDCANCEL:
        default:
            break;
        }
    }

    void Handle_FileOpen(HWND hWnd)
    {
        if (IsDirty())
        {
            switch (GetWantToSaveChangedMessageBoxChoice(hWnd))
            {
            case IDYES:
                Handle_FileSaveAs(hWnd);
                break;
            case IDCANCEL:
                return;
            case IDNO:
            default:
                break;
            }
        }

        SetWindowTextW(hWnd, L"Untitled");
        SendMessageW(edit, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(L""));
        SendMessageW(edit, EM_SETMODIFY, 0, 0);

        wchar_t filePath[MAX_PATH] = L"";
        OPENFILENAMEW ofn{};
        ofn.lStructSize  = sizeof(ofn);
        ofn.hwndOwner    = hWnd;
        ofn.lpstrFile    = filePath;
        ofn.nMaxFile     = MAX_PATH;
        ofn.lpstrFilter  = L"Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
        ofn.nFilterIndex = 1;
        ofn.Flags        = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
        if (GetOpenFileNameW(&ofn))
        {
            if (!FileIO::LoadFileToEdit(hWnd, edit, filePath))
                MessageBoxW(hWnd, L"Failed to open file.", L"Notepad--", MB_OK | MB_ICONERROR);
        }
    }

    void Handle_MenuPopUp(HMENU hPopup)
    {
        EnableMenuItem(hPopup, IDM_UNDO,     MF_BYCOMMAND | (undo.CanUndo() ? MF_ENABLED : MF_GRAYED));

        bool hasText = GetEditFieldText().empty();
        EnableMenuItem(hPopup, IDM_FIND,     MF_BYCOMMAND | (!hasText ? MF_ENABLED : MF_GRAYED));
        EnableMenuItem(hPopup, IDM_FINDNEXT, MF_BYCOMMAND | (!hasText ? MF_ENABLED : MF_GRAYED));
        EnableMenuItem(hPopup, IDM_REPLACE,  MF_BYCOMMAND | (!hasText ? MF_ENABLED : MF_GRAYED));

        DWORD start, end;
        GetEditFieldSelection(start, end);
        EnableMenuItem(hPopup, IDM_COPY,     MF_BYCOMMAND | (start != end ? MF_ENABLED : MF_GRAYED));
        EnableMenuItem(hPopup, IDM_CUT,      MF_BYCOMMAND | (start != end ? MF_ENABLED : MF_GRAYED));
        EnableMenuItem(hPopup, IDM_DELETE,   MF_BYCOMMAND | (start != end ? MF_ENABLED : MF_GRAYED));
        EnableMenuItem(hPopup, IDM_PASTE,    MF_BYCOMMAND | (IsClipboardFormatAvailable(CF_UNICODETEXT) || IsClipboardFormatAvailable(CF_TEXT) ? MF_ENABLED : MF_GRAYED));

        CheckMenuItem (hPopup, IDM_WORDWRAP, MF_BYCOMMAND | ( wordWrap    ? MF_CHECKED : MF_UNCHECKED));
        EnableMenuItem(hPopup, IDM_GOTO,     MF_BYCOMMAND | (!wordWrap    ? MF_ENABLED : MF_GRAYED));
    }
    void Handle_EN_CHANGE(HWND) { undo.UpdateUndo(edit); }
    void Handle_Undo     (HWND) { undo.Apply(edit); }
    void Handle_Copy     (HWND) { SendMessage(edit, WM_COPY,   0,  0); } // let edit field's implementation do the work
    void Handle_Cut      (HWND) { SendMessage(edit, WM_CUT,    0,  0); } // let edit field's implementation do the work
    void Handle_Paste    (HWND) { SendMessage(edit, WM_PASTE,  0,  0); } // let edit field's implementation do the work
    void Handle_Delete   (HWND) { SendMessage(edit, WM_CLEAR,  0,  0); } // let edit field's implementation do the work
    void Handle_SelectAll(HWND) { SendMessage(edit, EM_SETSEL, 0, -1); } // let edit field's implementation do the work

    void Handle_FileNew(HWND hWnd)
    {
        if (IsDirty())
        {
            switch (GetWantToSaveChangedMessageBoxChoice(hWnd))
            {
            case IDYES:
                Handle_FileSaveAs(hWnd);
                break;
            case IDNO:
                break;
            case IDCANCEL:
                return; // user changed his mind. Don't do File->New
            default:
                break;
            }
        }
        FilePath.clear();
        SetWindowTextW(hWnd, L"Untitled");
        SendMessageW(edit, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(L""));
        SendMessageW(edit, EM_SETMODIFY, 0, 0);
    }
    void Handle_About(HWND hWnd)
    {
        DialogBox(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, [](HWND hDlg, UINT message, WPARAM wParam, LPARAM) -> INT_PTR
                {
                    switch (message) {
                    case WM_INITDIALOG: return TRUE;
                    case WM_COMMAND: if (LOWORD(wParam) == IDOK) { EndDialog(hDlg, IDOK    ); return TRUE; } break;
                    case WM_CLOSE:                                 EndDialog(hDlg, IDCANCEL); return TRUE;
                    }
                    return FALSE;
                });
    }
    void SetRegisteredFindMessage(UINT findMsg) { uFindMsg = findMsg; } // so that we can respond to the FindDialog's messages

    bool IsFindMessage(UINT message)
    {
        if (message != uFindMsg)
            return false;

        // supposedly lParam is a pointer to the exact same struct I passed to FindText, so there's no need to check
        return find.HandleMessage();
    }
    void Handle_Find(HWND hWnd)
    {
        DWORD start, end;
        GetEditFieldSelection(start, end);
        find.DisplayFindDialog(hWnd, edit, &hDlgFind, GetSelectedText(start, end));
    }
    void Handle_FindNext(HWND hWnd)
    {
        if (false == find.FindNext())
            Handle_Find(hWnd);
    }
    void Handle_Replace(HWND hWnd)
    {
        DWORD start, end;
        GetEditFieldSelection(start, end);
        find.DisplayReplaceDialog(hWnd, edit, &hDlgFind, GetSelectedText(start, end));
    }
    bool DoDialogMessage(MSG* msg) { return hDlgFind && IsDialogMessage(hDlgFind, msg); }

    void Handle_GoTo(HWND hWnd)
    {
        struct GoToData
        {
            int lineNumber = 0;
            BOOL InitDialog(HWND hDlg, LPARAM lParam)
            {
                SetWindowLongPtr(hDlg, GWLP_USERDATA, lParam);
                return TRUE;
            }
            BOOL DoCommand(HWND hDlg, WPARAM wParam)
            {
                switch (LOWORD(wParam))
                {
                case IDOK:
                {
                    HWND hEdit = GetDlgItem(hDlg, IDC_GOTO_EDIT);
                    int len = GetWindowTextLengthW(hEdit);
                    std::wstring text(len, L'\0');
                    GetWindowTextW(hEdit, text.data(), len+1);
                    if (text.empty())
                    { // if edit field is empty, don't accept IDOK, but rather beep
                        MessageBeep(MB_ICONWARNING);
                        SendMessage(hEdit, EM_SETSEL, 0, -1);
                        SetFocus(hEdit);
                    } else {
                        try { lineNumber = std::stoi(text); }
                        catch (...) { lineNumber = std::numeric_limits<int>::max(); }
                        EndDialog(hDlg, IDOK);
                    }
                    return TRUE;
                }
                case IDCANCEL:
                    EndDialog(hDlg, IDCANCEL);
                    return TRUE;
                }
                return FALSE;
            }
            static INT_PTR CALLBACK DialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
            {
                GoToData* pThis = reinterpret_cast<GoToData*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
                switch (msg)
                {
                case WM_INITDIALOG: return pThis->InitDialog(hDlg, lParam);
                case WM_COMMAND:    return pThis->DoCommand (hDlg, wParam);
                }
                return FALSE;
            }
        } goToData;
        if (IDOK == DialogBoxParam(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDD_GOTOBOX), hWnd, GoToData::DialogProc, reinterpret_cast<LPARAM>(&goToData)))
        {
            int charIndex = (int)SendMessageW(edit, EM_LINEINDEX, goToData.lineNumber, 0);
            if (charIndex < 0) // past the end:  EM_LINEINDEX returns -1 if the line doesn't exist
                charIndex = GetWindowTextLengthW(edit);
            SendMessageW(edit, EM_SETSEL, charIndex, charIndex);
            SendMessageW(edit, EM_SCROLLCARET, 0, 0);
        }
    }

    void Handle_WordWrap(HWND hWnd)
    {
        wordWrap = !wordWrap; // toggle the state

        // word wrap is controlled by the style: (WS_HSCROLL | ES_AUTOHSCROLL).
        // the edit field does not read the new style, but rather the whole edit control must be recreated.

        DWORD style = wordWrap == true ? 0 : (WS_HSCROLL | ES_AUTOHSCROLL);
        auto text = GetEditFieldText();
        RECT rc;
        GetClientRect(hWnd, &rc);
        DestroyWindow(edit);
        edit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_VSCROLL |  ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | style, 0, 0, 0, 0, hWnd, reinterpret_cast<HMENU>(IDC_EDITFIELD), GetModuleHandle(nullptr), nullptr);
        MoveWindow(edit, 0, 0, rc.right-rc.left, rc.bottom-rc.top, TRUE);
        SetWindowText(edit, text.c_str());
    }
};
