#pragma once

#include <filesystem>
#include <string>
#include <vector>
#include <windows.h>
#include <commdlg.h>
#include <shellapi.h>

#include "Resource.h"

class MessageHandler
{
    HWND edit{};

    bool LoadFileToEdit(HWND hWnd, const wchar_t* path)
    {
        HANDLE file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file != INVALID_HANDLE_VALUE)
        {
            LARGE_INTEGER size{};
            if (GetFileSizeEx(file, &size) != 0 && size.QuadPart >= 0 && size.QuadPart <= INT_MAX)
            {
                std::vector<char> bytes(static_cast<size_t>(size.QuadPart));
                DWORD read = 0;
                if (ReadFile(file, bytes.data(), static_cast<DWORD>(bytes.size()), &read, nullptr) != 0)
                {
                    bytes.resize(read);
                    size_t offset = 0;
                    if (bytes.size() >= 3 && static_cast<unsigned char>(bytes[0]) == 0xEF && static_cast<unsigned char>(bytes[1]) == 0xBB && static_cast<unsigned char>(bytes[2]) == 0xBF)
                        offset = 3;

                    const int wideSize = MultiByteToWideChar(CP_UTF8, 0, bytes.data() + offset, static_cast<int>(bytes.size() - offset), nullptr, 0);
                    if (wideSize != 0)
                    {
                        std::wstring wide(static_cast<size_t>(wideSize), L'\0');
                        MultiByteToWideChar(CP_UTF8, 0, bytes.data() + offset, static_cast<int>(bytes.size() - offset), wide.data(), wideSize);
                        if (SendMessageW(edit, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(wide.c_str())) != 0)
                        {
                            SendMessageW(edit, EM_SETMODIFY, 0, 0);

                            std::wstring title = std::filesystem::path{ path }.filename().wstring();
                            if (!title.empty())
                                SetWindowTextW(hWnd, title.c_str());
                            CloseHandle(file);
                            return true;
                        }
                    }
                }
            }
            CloseHandle(file);
        }
        return false;
    }

    bool SaveEditTextToFile(HWND hWnd, const wchar_t* path)
    {
        const LRESULT length = SendMessageW(edit, WM_GETTEXTLENGTH, 0, 0);
        std::wstring text(static_cast<size_t>(length) + 1, L'\0');
        SendMessageW(edit, WM_GETTEXT, static_cast<WPARAM>(text.size()), reinterpret_cast<LPARAM>(text.data()));
        text.resize(static_cast<size_t>(length));
        const int utf8Size = WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
        if (utf8Size == 0)
            return false;

        std::vector<char> utf8(static_cast<size_t>(utf8Size) + 3);
        utf8[0] = static_cast<char>(0xEF);
        utf8[1] = static_cast<char>(0xBB);
        utf8[2] = static_cast<char>(0xBF);
        WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), utf8.data() + 3, utf8Size, nullptr, nullptr);

        bool ret = false;
        HANDLE file = CreateFileW(path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file != INVALID_HANDLE_VALUE)
        {
            DWORD written = 0;
            if (WriteFile(file, utf8.data(), static_cast<DWORD>(utf8.size()), &written, nullptr) != 0)
                if (written == utf8.size())
                    ret = true;
            CloseHandle(file);
        }
        return ret;
    }

    int Handle_WM_CREATE(HWND hWnd)
    {
        edit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL, 0, 0, 0, 0, hWnd, nullptr, GetModuleHandle(nullptr), nullptr);
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
        if (!LoadFileToEdit(hWnd, args.argv[1]))
            MessageBoxW(hWnd, L"Failed to open file.", L"Notepad--", MB_OK | MB_ICONERROR);
    }

public:
    static MessageHandler& GetHandler(HWND hWnd) { return *reinterpret_cast<MessageHandler*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA)); }
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
        {
            if (SaveEditTextToFile(hWnd, filePath))
            {
                std::wstring title = std::filesystem::path{ filePath }.filename().wstring();
                if (!title.empty())
                    SetWindowTextW(hWnd, title.c_str());
            }
            else
                MessageBoxW(hWnd, L"Failed to save file.", L"Notepad--", MB_OK | MB_ICONERROR);
        }
    }
    void Handle_Exit(HWND hWnd)
    {
        if (SendMessageW(edit, EM_GETMODIFY, 0, 0) == 0)
        {
            DestroyWindow(hWnd);
            return;
        }

        const INT_PTR choice = DialogBox(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDD_EXITCONFIRM), hWnd, [](HWND hDlg, UINT message, WPARAM wParam, LPARAM /*lParam*/) -> INT_PTR
            {
                if (message == WM_INITDIALOG) return (INT_PTR)TRUE;
                if (message == WM_COMMAND)
                {
                    switch (LOWORD(wParam))
                    {
                    case IDC_SAVE:
                    case IDC_DONTSAVE:
                    case IDCANCEL:
                        EndDialog(hDlg, LOWORD(wParam));
                        return (INT_PTR)TRUE;
                    }
                }
                return (INT_PTR)FALSE;
            });

        switch (choice)
        {
        case IDC_SAVE:
            Handle_FileSaveAs(hWnd);
            DestroyWindow(hWnd);
            break;
        case IDC_DONTSAVE:
            DestroyWindow(hWnd);
            break;
        default:
            break;
        }
    }

    void Handle_FileOpen(HWND hWnd)
    {
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
            if (!LoadFileToEdit(hWnd, filePath))
                MessageBoxW(hWnd, L"Failed to open file.", L"Notepad--", MB_OK | MB_ICONERROR);
        }
    }
    void Handle_About(HWND hWnd)
    {
        DialogBox(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, [](HWND hDlg, UINT message, WPARAM wParam, LPARAM /*lParam*/) -> INT_PTR
            {
                if (message == WM_INITDIALOG) return (INT_PTR)TRUE;
                if (message == WM_COMMAND && LOWORD(wParam) == IDOK) return (INT_PTR)!!EndDialog(hDlg, LOWORD(wParam));
                return (INT_PTR)FALSE;
            });
    }
};
