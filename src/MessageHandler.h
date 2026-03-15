#pragma once

#include <filesystem>
#include <string>
#include <vector>
#include <windows.h>

class MessageHandler
{
    HWND edit{};

    void SaveEditTextToFile(HWND hWnd, const wchar_t* path)
    {
        const LRESULT length = SendMessageW(edit, WM_GETTEXTLENGTH, 0, 0);
        std::wstring text(static_cast<size_t>(length) + 1, L'\0');
        SendMessageW(edit, WM_GETTEXT, static_cast<WPARAM>(text.size()), reinterpret_cast<LPARAM>(text.data()));
        text.resize(static_cast<size_t>(length));

        const int utf8Size = WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
        if (utf8Size == 0)
            return; // TODO: BUGBUG: report error to user?

        std::vector<char> utf8(static_cast<size_t>(utf8Size) + 3);
        utf8[0] = static_cast<char>(0xEF);
        utf8[1] = static_cast<char>(0xBB);
        utf8[2] = static_cast<char>(0xBF);
        WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), utf8.data() + 3, utf8Size, nullptr, nullptr);

        HANDLE file = CreateFileW(path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE)
            return; // TODO: BUGBUG: report error to user

        DWORD written = 0;
        WriteFile(file, utf8.data(), static_cast<DWORD>(utf8.size()), &written, nullptr);
        CloseHandle(file);
    }

    int Handle_WM_CREATE(HWND hWnd)
    {
        edit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL, 0, 0, 0, 0, hWnd, nullptr, GetModuleHandle(nullptr), nullptr);
        if (edit == nullptr)
        {
            MessageBoxW(hWnd, L"Failed to create edit control. The application will now exit.", L"Notepad--", MB_OK | MB_ICONERROR);
            return -1;
        }
        SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
        return S_OK;
    }
    void Handle_WM_NCDESTROY(HWND hWnd)
    {
        SetWindowLongPtrW(hWnd, GWLP_USERDATA, 0);
        delete this;
    }

public:
    static MessageHandler& GetHandler(HWND hWnd) { return *reinterpret_cast<MessageHandler*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA)); }
    static int CreateHandlerAndWindow(HWND hWnd)
    {
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

    void Handle_SaveAs(HWND hWnd)
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
            SaveEditTextToFile(hWnd, filePath);
            std::wstring title = std::filesystem::path{ filePath }.filename().wstring();
            if (!title.empty())
                SetWindowTextW(hWnd, title.c_str());
        }
    }
};
