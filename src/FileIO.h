#ifndef FILEIO_H
#define FILEIO_H

#pragma once
#include <windows.h>
#undef min
#include <string>
#include <algorithm>

struct FileIO
{
    static bool LoadFileToEdit(HWND hWnd, HWND edit, const wchar_t* path)
    {
        bool ret = false;
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
                            ret = true;
                        }
                    }
                }
            }
            CloseHandle(file);
        }
        return ret;
    }

    static bool SaveEditTextToFile(HWND edit, const wchar_t* path)
    {
        const LRESULT length = SendMessageW(edit, WM_GETTEXTLENGTH, 0, 0);
        std::wstring text(static_cast<size_t>(length) + 1, L'\0');
        SendMessageW(edit, WM_GETTEXT, static_cast<WPARAM>(text.size()), reinterpret_cast<LPARAM>(text.data()));
        text.resize(static_cast<size_t>(length));
        const int utf8Size = WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
        if (utf8Size == 0)
            if (length > 0) // ok if it's a 0-byte file
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
    
    static void SaveFile(HWND hWnd, HWND edit, const wchar_t* filePath, std::filesystem::path& FilePath)
    {
        if (SaveEditTextToFile(edit, filePath))
        {
            SendMessageW(edit, EM_SETMODIFY, 0, 0); // clear the dirty flag, until the user types again.

            std::wstring title = std::filesystem::path{filePath}.filename().wstring();
            if (!title.empty())
                SetWindowTextW(hWnd, title.c_str());

            FilePath = filePath; // save filePath for "File->Save" (and pre-populating SaveAs dialog)
        }
        else
            MessageBoxW(hWnd, L"Failed to save file.", L"Notepad--", MB_OK | MB_ICONERROR);
    }

    static void FileSaveAs(HWND hWnd, HWND edit, std::filesystem::path& FilePath)
    {
        wchar_t filePath[MAX_PATH];

        auto src = FilePath.native();
        size_t toCopy = std::min(src.size(), static_cast<size_t>(MAX_PATH-1));
        src.copy(filePath, toCopy);
        filePath[toCopy] = L'\0';

        OPENFILENAMEW ofn{};
        ofn.lStructSize  = sizeof(ofn);
        ofn.hwndOwner    = hWnd;
        ofn.lpstrFile    = filePath;
        ofn.nMaxFile     = MAX_PATH;
        ofn.lpstrFilter  = L"Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
        ofn.nFilterIndex = 1;
        ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
        if (GetSaveFileNameW(&ofn))
            FileIO::SaveFile(hWnd, edit, filePath, FilePath);
    }
};

#endif
