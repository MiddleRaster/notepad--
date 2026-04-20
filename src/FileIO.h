#ifndef FILEIO_H
#define FILEIO_H

#pragma once
#include <windows.h>
#undef min
#include <shobjidl.h>
#include <atlbase.h>

#include <string>
#include <algorithm>
#include <cstdint>
#include <vector>

struct FileIO
{
    enum Encoding
    {
        eANSI      = 0,
        eUNICODE   = 1,
        eUNICODEbe = 2,
        eUTF8      = 3,
        eUTF8noBOM = 4,
    };

private:
    class EncodedReaders
    {
        static bool IsValidUTF8(const uint8_t* s, size_t n)
        {
            size_t i = 0;

            while (i < n)
            {
                uint8_t c = s[i];

                // ASCII fast path
                if (c <= 0x7F) {
                    i++;
                    continue;
                }

                // Determine sequence length
                size_t len = 0;
                uint32_t cp = 0;

                if ((c & 0xE0) == 0xC0) {          // 110xxxxx → 2‑byte
                    len = 2;
                    cp = c & 0x1F;
                    if (cp == 0) return false;     // overlong: U+0000 encoded in 2 bytes
                }
                else if ((c & 0xF0) == 0xE0) {     // 1110xxxx → 3‑byte
                    len = 3;
                    cp = c & 0x0F;
                }
                else if ((c & 0xF8) == 0xF0) {     // 11110xxx → 4‑byte
                    len = 4;
                    cp = c & 0x07;
                }
                else
                    return false;                  // invalid leading byte

                if (i + len > n)
                    return false;                  // truncated sequence

                // Validate continuation bytes
                for (size_t k = 1; k < len; k++) {
                    uint8_t cc = s[i + k];
                    if ((cc & 0xC0) != 0x80)
                        return false;
                    cp = (cp << 6) | (cc & 0x3F);
                }

                // Reject overlong encodings
                if (len == 2 && cp < 0x80)        return false;
                if (len == 3 && cp < 0x800)       return false;
                if (len == 4 && cp < 0x10000)     return false;

                // Reject UTF‑16 surrogate range
                if (cp >= 0xD800 && cp <= 0xDFFF)
                    return false;

                // Reject code points beyond Unicode max
                if (cp > 0x10FFFF)
                    return false;

                i += len;
            }
            return true;
        }
    public:
        static std::pair<Encoding, bool> DetectEncoding(const std::vector<uint8_t>& b)
        {
            // first try BOMs, if any
            if (b.size() >= 3) {
                if (b[0] == 0xEF)
                if (b[1] == 0xBB)
                if (b[2] == 0xBF)
                    return {Encoding::eUTF8, true};
            }
            if (b.size() >= 2) {
                if (b[0] == 0xFF)
                if (b[1] == 0xFE)
                    return {Encoding::eUNICODE, true};

                if (b[0] == 0xFE)
                if (b[1] == 0xFF)
                    return {Encoding::eUNICODEbe, true};
            }
            // no BOM:  heuristics from here on

            const size_t n = b.size();

            // 1. UTF-16LE heuristic
            if (n % 2 == 0) {
                size_t zeros = 0;
                for (size_t i = 1; i < n; i += 2)
                    if (b[i] == 0) zeros++;

                if (zeros > n / 4)
                    return {Encoding::eUNICODE, false};
            }

            // 2. UTF-16BE heuristic
            if (n % 2 == 0) {
                size_t zeros = 0;
                for (size_t i = 0; i < n; i += 2)
                    if (b[i] == 0) zeros++;

                if (zeros > n / 4)
                    return {Encoding::eUNICODEbe, false};
            }

            // 3. UTF-8 strict validation
            if (IsValidUTF8(b.data(), b.size()))
                return {Encoding::eUTF8noBOM, false};

            // 4. Fallback
            return {Encoding::eANSI, false};
        }

        static std::wstring UTF8ToWideChar(const std::vector<uint8_t>& bytes, bool hasBom)
        {
            int offset = hasBom ? 3 : 0;
            int wideSize = MultiByteToWideChar(CP_UTF8, 0, (LPCCH)(bytes.data() + offset), static_cast<int>(bytes.size() - offset), nullptr, 0);
            if (wideSize != 0) {
                std::wstring wide(static_cast<size_t>(wideSize), L'\0');
                MultiByteToWideChar(CP_UTF8, 0, (LPCCH)(bytes.data() + offset), static_cast<int>(bytes.size() - offset), wide.data(), wideSize);
                return wide;
            }
            return AnsiToWideChar(bytes, false);
        }
        static std::wstring AnsiToWideChar(const std::vector<uint8_t>& bytes, bool /*hasBom*/)
        {
            std::wstring wide;
            wide.reserve(bytes.size());
            std::transform(bytes.begin(), bytes.end(), std::back_inserter(wide), [](uint8_t b) { return static_cast<wchar_t>(b); });
            return wide;
        }
        static std::wstring UnicodeToWideChar(const std::vector<uint8_t>& bytes, bool hasBom)
        {
            const uint8_t* p = bytes.data() + (hasBom ? 2 : 0);
            size_t size      = bytes.size() - (hasBom ? 2 : 0);
            std::wstring wide(size/2, L'\0');
            std::memcpy(wide.data(), p, size);
            return wide;
        }
        static std::wstring UnicodeBEToWideChar(const std::vector<uint8_t>& bytes, bool hasBom)
        {
            auto it = bytes.begin();
            if (hasBom)
                it += 2;
            std::wstring wide;
            wide.reserve(std::distance(it, bytes.end())/2);
            for (; it != bytes.end(); ) {
                uint8_t lo = *it++;
                uint8_t hi = *it++;
                wide.push_back(static_cast<wchar_t>((uint16_t(lo) << 8) | hi));
            }
            return wide;
        }
    };

public:
    static bool LoadFileToEdit(HWND hWnd, HWND edit, const wchar_t* path, Encoding& encoding)
    {
        bool ret = false;
        HANDLE file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file != INVALID_HANDLE_VALUE)
        {
            LARGE_INTEGER size{};
            if (GetFileSizeEx(file, &size) != 0 && size.QuadPart >= 0 && size.QuadPart <= INT_MAX)
            {
                std::vector<uint8_t> bytes(static_cast<size_t>(size.QuadPart));
                DWORD read = 0;
                if (ReadFile(file, bytes.data(), static_cast<DWORD>(bytes.size()), &read, nullptr) != 0)
                {
                    bytes.resize(read);

                    std::wstring wide;
                    auto [encoded, hasBom] = EncodedReaders::DetectEncoding(bytes);
                    switch (encoding = encoded)
                    {
                        default:
                        case Encoding::eUTF8:
                        case Encoding::eUTF8noBOM:  wide = EncodedReaders::     UTF8ToWideChar(bytes, hasBom);   break;
                        case Encoding::eANSI:       wide = EncodedReaders::     AnsiToWideChar(bytes, hasBom);   break;
                        case Encoding::eUNICODE:    wide = EncodedReaders::  UnicodeToWideChar(bytes, hasBom);   break;
                        case Encoding::eUNICODEbe:  wide = EncodedReaders::UnicodeBEToWideChar(bytes, hasBom);   break;
                    }

                    if (SendMessageW(edit, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(wide.c_str())) != 0)
                    {
                        SendMessageW(edit, EM_SETMODIFY, 0, 0);

                        std::wstring title = std::filesystem::path{path}.filename().wstring();
                        if (!title.empty())
                            SetWindowTextW(hWnd, title.c_str());
                        ret = true;
                    }
                }
            }
            CloseHandle(file);
        }
        return ret;
    }

private:
    class EncodedWriters
    {
        static bool WriteBytes(HANDLE hFile, const void* data, DWORD size)
        {
            DWORD written;
            return WriteFile(hFile, data, size, &written, nullptr) && written == size;
        }
        static std::string WideToMB(const std::wstring& text, UINT codepage)
        {
            if (text.empty()) return {};
            int sz = WideCharToMultiByte(codepage, 0, text.data(), (int)text.size(), nullptr, 0, nullptr, nullptr);
            std::string out(sz, '\0');
            WideCharToMultiByte(codepage, 0, text.data(), (int)text.size(), out.data(), sz, nullptr, nullptr);
            return out;
        }
    public:
        static bool WriteUTF8(const std::filesystem::path& filePath, const std::wstring& text, bool wantBOM)
        {
            static const BYTE bom[] = { 0xEF, 0xBB, 0xBF };
            std::string utf8 = WideToMB(text, CP_UTF8);

            bool ok = false;
            HANDLE hFile = CreateFileW(filePath.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (hFile != INVALID_HANDLE_VALUE)
            {
                if (wantBOM)
                     WriteBytes(hFile, bom, sizeof(bom));
                ok = WriteBytes(hFile, utf8.data(), (DWORD)utf8.size());
                CloseHandle(hFile);
            }
            return ok;
        }
        static bool WriteANSI(const std::filesystem::path& filePath, const std::wstring& text)
        {
            std::string narrow = WideToMB(text, CP_ACP);

            bool ok = false;
            HANDLE hFile = CreateFileW(filePath.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (hFile != INVALID_HANDLE_VALUE)
            {
                ok = WriteBytes(hFile, narrow.data(), (DWORD)narrow.size());
                CloseHandle(hFile);
            }
            return ok;
        }
        static bool WriteUTF16LE(const std::filesystem::path& filePath, const std::wstring& text)
        {
            static const BYTE bom[] = { 0xFF, 0xFE };

            bool ok = false;
            HANDLE hFile = CreateFileW(filePath.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (hFile != INVALID_HANDLE_VALUE)
            {
                     WriteBytes(hFile, bom, sizeof(bom));
                ok = WriteBytes(hFile, text.data(), (DWORD)(text.size()*sizeof(wchar_t)));
                CloseHandle(hFile);
            }
            return ok;
        }
        static bool WriteUTF16BE(const std::filesystem::path& filePath, const std::wstring& text)
        {
            static const BYTE bom[] = { 0xFE, 0xFF };

            // Copy LE bytes then swap each pair in-place
            std::string be(text.size()*sizeof(wchar_t), '\0');
            memcpy(be.data(), text.data(), be.size());
            for (size_t i = 0; i+1 < be.size(); i += 2)
                std::swap(be[i], be[i+1]);

            bool ok = false;
            HANDLE hFile = CreateFileW(filePath.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (hFile != INVALID_HANDLE_VALUE)
            {
                     WriteBytes(hFile, bom, sizeof(bom));
                ok = WriteBytes(hFile, be.data(), (DWORD)be.size());
                CloseHandle(hFile);
            }
            return ok;
        }
    };
public:
    static bool SaveEditTextToFile(HWND edit, const wchar_t* filePath, Encoding encoding)
    {
        const LRESULT length = SendMessageW(edit, WM_GETTEXTLENGTH, 0, 0);
        std::wstring text(static_cast<size_t>(length) + 1, L'\0');
        SendMessageW(edit, WM_GETTEXT, static_cast<WPARAM>(text.size()), reinterpret_cast<LPARAM>(text.data()));
        text.resize(static_cast<size_t>(length));

        std::filesystem::path path{filePath};
        switch (encoding)
        {
        default:
        case Encoding::eANSI:      return EncodedWriters::WriteANSI   (filePath, text);
        case Encoding::eUNICODE:   return EncodedWriters::WriteUTF16LE(filePath, text);
        case Encoding::eUNICODEbe: return EncodedWriters::WriteUTF16BE(filePath, text);
        case Encoding::eUTF8:      return EncodedWriters::WriteUTF8   (filePath, text, true);
        case Encoding::eUTF8noBOM: return EncodedWriters::WriteUTF8   (filePath, text, false);
        }
    }
    
    static void SaveFile(HWND hWnd, HWND edit, const wchar_t* filePath, std::filesystem::path& FilePath, Encoding encoding)
    {
        if (SaveEditTextToFile(edit, filePath, encoding))
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

    static void FileSaveAs(HWND hWnd, HWND edit, std::filesystem::path& FilePath, Encoding& encoding)
    {
        struct COM
        {
            COM() { CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED); }
           ~COM() { CoUninitialize(); }
        } com;

        CComPtr<IFileSaveDialog> pDlg;

        if (FAILED(pDlg.CoCreateInstance(CLSID_FileSaveDialog)))
            return;

        CComPtr<IFileDialogCustomize> pCustomize;
        if (SUCCEEDED(pDlg.QueryInterface(&pCustomize)))
        {
            pCustomize->StartVisualGroup(IDC_ENCODING_GROUP, L"Encoding:");
            pCustomize->AddComboBox     (IDC_ENCODING_COMBO);
            pCustomize->AddControlItem  (IDC_ENCODING_COMBO, Encoding::eANSI,      L"ANSI");
            pCustomize->AddControlItem  (IDC_ENCODING_COMBO, Encoding::eUNICODE,   L"Unicode");
            pCustomize->AddControlItem  (IDC_ENCODING_COMBO, Encoding::eUNICODEbe, L"Unicode big endian");
            pCustomize->AddControlItem  (IDC_ENCODING_COMBO, Encoding::eUTF8,      L"UTF-8 with BOM");
            pCustomize->AddControlItem  (IDC_ENCODING_COMBO, Encoding::eUTF8noBOM, L"UTF-8 no BOM");
            pCustomize->SetSelectedControlItem(IDC_ENCODING_COMBO, encoding);
            pCustomize->EndVisualGroup();
        }

        pDlg->SetFileName(FilePath.filename().c_str());

        COMDLG_FILTERSPEC filters[] =
        {
            { L"Text Files (*.txt)", L"*.txt" },
            { L"All Files (*.*)",    L"*.*"   },
        };
        pDlg->SetFileTypes(ARRAYSIZE(filters), filters);
        pDlg->SetFileTypeIndex(1);
        pDlg->SetDefaultExtension(L"txt");

        HRESULT hr = pDlg->Show(hWnd);
        if (SUCCEEDED(hr))
        {
            DWORD selection  = 0;
            if (SUCCEEDED(hr = pCustomize->GetSelectedControlItem(IDC_ENCODING_COMBO, &selection)))
                encoding = static_cast<Encoding>(selection);

            CComPtr<IShellItem> pItem;
            if (SUCCEEDED(hr = pDlg->GetResult(&pItem)))
            {
                CComHeapPtr<WCHAR> pszPath;
                if (SUCCEEDED(hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszPath))) {
                    FileIO::SaveFile(hWnd, edit, pszPath, FilePath, encoding);
                    return;
                }
            }
        }
        if (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED))
            return;

        ::MessageBoxW(hWnd, std::format(L"Unable to save. IFileSaveDialog COM objects reported error: 0x{:08X}", static_cast<unsigned long>(hr)).c_str(), L"Error from FileSaveDialog", MB_ICONERROR | MB_OK);
    }
};

#endif
