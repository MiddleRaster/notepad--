#pragma once

#include "WindowFinder.h"
#include "..\src\Resource.h"
#include "UIA.h"

namespace WindowFinder::Has
{
    struct EitherChildClass
    {
        const wchar_t* className1;
        const wchar_t* className2;
        bool operator()(HWND hwnd) const
        {
            return (nullptr != WindowFinder::FindDesiredChildWindow(hwnd, WindowFinder::Has::ClassName{className1})) ||
                   (nullptr != WindowFinder::FindDesiredChildWindow(hwnd, WindowFinder::Has::ClassName{className2})) ;
        }
    };
}

namespace
{
    using namespace TDD20;

    class ProcessGuard
    {
        ProcessGuard           (                    ) = delete;
        ProcessGuard           (const ProcessGuard& ) = delete;
        ProcessGuard& operator=(const ProcessGuard& ) = delete;
        ProcessGuard           (      ProcessGuard&&) = delete;
        ProcessGuard& operator=(      ProcessGuard&&) = delete;

        PROCESS_INFORMATION pi{};
    public:
        const HANDLE& hProcess;
        const HANDLE& hThread;
        const bool created;

        template <typename LaunchFn> explicit ProcessGuard(LaunchFn&& launch) : hProcess(pi.hProcess), hThread(pi.hThread), created(launch(pi)) {}
       ~ProcessGuard()
        {
            if (created)
                if (WaitForSingleObject(hProcess, 1000) == WAIT_TIMEOUT)
                    TerminateProcess(hProcess, 1);
            if (hThread)
                CloseHandle(hThread);
            if (hProcess)
                CloseHandle(hProcess);
        }
    };
}

namespace FileUtils
{
    using namespace std::chrono_literals;

    inline std::filesystem::path GetTempFilename(const wchar_t* fileName)
    {
        wchar_t tempPath[MAX_PATH]{};
        Assert::IsTrue(GetTempPathW(MAX_PATH, tempPath) > 0, "Failed to get temp path");
        std::filesystem::path filePath = std::filesystem::path(tempPath) / fileName;
        DeleteFileW(filePath.c_str());
        return filePath;
    }
    inline std::filesystem::path CreateTempUtf8File(const wchar_t* fileName, const wchar_t* text)
    {
        wchar_t tempPath[MAX_PATH]{};
        Assert::IsTrue(GetTempPathW(MAX_PATH, tempPath) > 0, "Failed to get temp path");
        std::filesystem::path filePath = std::filesystem::path(tempPath) / fileName;
        DeleteFileW(filePath.c_str());

        const int utf8Size = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
        Assert::IsTrue(utf8Size > 0, "Failed to size UTF-8 buffer");
        std::vector<char> utf8(static_cast<size_t>(utf8Size - 1) + 3);
        utf8[0] = static_cast<char>(0xEF);
        utf8[1] = static_cast<char>(0xBB);
        utf8[2] = static_cast<char>(0xBF);
        WideCharToMultiByte(CP_UTF8, 0, text, -1, utf8.data() + 3, utf8Size - 1, nullptr, nullptr);

        HANDLE file = CreateFileW(filePath.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        Assert::IsTrue(file != INVALID_HANDLE_VALUE, "Failed to create temp file");
        DWORD written = 0;
        WriteFile(file, utf8.data(), static_cast<DWORD>(utf8.size()), &written, nullptr);
        CloseHandle(file);
        return filePath;
    }

    inline std::wstring ReadFileUtf8(const std::filesystem::path& filePath)
    {
        std::ifstream in;
        for (int i=0; i<20; ++i)
        {
            in.open(filePath, std::ios::binary);
            if (!in.is_open())
                std::this_thread::sleep_for(50ms);
            else
                break;
        }
        Assert::IsTrue(in.is_open(), "Failed to open saved file");
        std::string bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        in.close();
        if (bytes.size() >= 3 && static_cast<unsigned char>(bytes[0]) == 0xEF && static_cast<unsigned char>(bytes[1]) == 0xBB && static_cast<unsigned char>(bytes[2]) == 0xBF)
            bytes.erase(0, 3);

        const int wideSize = MultiByteToWideChar(CP_UTF8, 0, bytes.data(), static_cast<int>(bytes.size()), nullptr, 0);
        Assert::IsTrue(wideSize >= 0, "Failed to convert saved file to UTF-16");
        std::wstring wide(static_cast<size_t>(wideSize), L'\0');
        MultiByteToWideChar(CP_UTF8, 0, bytes.data(), static_cast<int>(bytes.size()), wide.data(), wideSize);
        return wide;
    }
    inline void DeleteFileWithRetry(const std::filesystem::path& filePath)
    {
        for (int i=0; i<20; ++i)
        {
            if (DeleteFileW(filePath.c_str()) != 0)
                return;
            std::this_thread::sleep_for(50ms);
        }
        Assert::IsTrue(false, "Failed to delete test file");
    }
}
namespace WindowUtils
{
    inline HWND WaitForWindow(std::chrono::milliseconds timeout, auto&& pred)
    {
        const auto deadline = std::chrono::steady_clock::now() + timeout;
        while (std::chrono::steady_clock::now() < deadline)
        {
            if (HWND hwnd = pred())
                return hwnd;
            std::this_thread::sleep_for(50ms);
        }
        return nullptr;
    }
}
namespace Poll
{
    void Until(std::chrono::milliseconds timeout, std::chrono::milliseconds step, auto&& pred)
    {
        const auto deadline = std::chrono::steady_clock::now() + timeout;
        while (std::chrono::steady_clock::now() < deadline)
        {
            if (pred())
                return;
            std::this_thread::sleep_for(step);
        }
    }
    void While(std::chrono::milliseconds timeout, std::chrono::milliseconds step, auto&& pred)
    {
        const auto deadline = std::chrono::steady_clock::now() + timeout;
        while (std::chrono::steady_clock::now() < deadline)
        {
            if (!pred())
                return;
            std::this_thread::sleep_for(step);
        }
    }
}

namespace TestAutomation
{
    using namespace std::chrono_literals;

    struct Clipboard
    {
        static std::wstring GetClipboardText()
        {
            std::wstring result;
            if (OpenClipboard(nullptr))
            {
                if (HANDLE hData = GetClipboardData(CF_UNICODETEXT))
                {
                    if (auto* p = static_cast<wchar_t*>(GlobalLock(hData)))
                    {
                        result = p;
                        GlobalUnlock(hData);
                    }
                }
                CloseClipboard();
            }
            return result;
        }
        static void SetClipboardText(const std::wstring& text)
        {
            if (OpenClipboard(GetDesktopWindow()))
            {
                EmptyClipboard();
                if (HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, (text.size() + 1) * sizeof(wchar_t)))
                {
                    if (auto* p = static_cast<wchar_t*>(GlobalLock(hMem)))
                    {
                        std::wmemcpy(p, text.data(), text.size());
                        p[text.size()] = L'\0';
                        GlobalUnlock(hMem);

                        if (!SetClipboardData(CF_UNICODETEXT, hMem))
                            GlobalFree(hMem); // they don't own the memory, because SetClipboardData failed. So we must free it.
                    }
                    else
                        GlobalFree(hMem);
                }
                CloseClipboard();
            }
        }
        static void EmptyClipboard()
        {
            if (OpenClipboard(nullptr))
            {
                ::EmptyClipboard();   // If it fails, there's nothing else to do
                CloseClipboard();
            }
        }
    };

    struct MessageBoxUtils
    {
        static std::wstring GetStaticText(HWND hwnd)
        {
            HWND staticText = WindowFinder::FindDesiredChildWindow(hwnd, WindowFinder::Has::ClassName(L"Static"), WindowFinder::Has::NotStyle{SS_ICON});
            Assert::AreNotEqual(nullptr, staticText, "static text field not found");

            std::wstring text;
            int len = GetWindowTextLengthW(staticText);
            if (len > 0) {
                text.resize(len+1);
                GetWindowTextW(staticText, text.data(), len+1);
                text.resize(len);
            }
            return text;
        }
        static void PushOkButton(HWND hwnd)
        {
            HWND okButton = GetDlgItem(hwnd, IDOK);
            if (okButton != nullptr)
                SendMessageW(okButton, BM_CLICK, 0, 0);
            else
                SendMessageW(hwnd, WM_CLOSE, 0, 0);
        }
    };

    class FileDirtyMessageBox
    {
        void ClickButton(int id)
        {
            HWND control = GetDlgItem(messageBox, id);
            Assert::AreNotEqual(nullptr, control, "Button not found");
            SendMessage(control, BM_CLICK, 0, 0);
        }

        HWND messageBox;
    public:
        FileDirtyMessageBox(DWORD pid)
        {
            messageBox = WindowUtils::WaitForWindow(5s, [&pid]() { return WindowFinder::FindDesiredChildWindow(nullptr, WindowFinder::Has::Pid{pid}, WindowFinder::Has::ClassName{L"#32770"}); });
            Assert::AreNotEqual(nullptr, messageBox, "Exit dialog not found");
        }
        std::wstring GetStaticMessage() const { return MessageBoxUtils::GetStaticText(messageBox); }

        void PressDontSave() { ClickButton(IDNO); }
        void PressSave    () { ClickButton(IDYES); }
        void PressCancel  () { ClickButton(IDCANCEL); }
    };

    struct CommonFileDialogUtils
    {
        static HWND FindSaveAsEdit(HWND dlg)
        {
            if (HWND duiview  = WindowFinder::FindDesiredChildWindow(dlg,      WindowFinder::Has::ClassName{L"DUIViewWndClassName"}))
            if (HWND directUI = WindowFinder::FindDesiredChildWindow(duiview,  WindowFinder::Has::ClassName{L"DirectUIHWND"       }))
            if (HWND sink     = WindowFinder::FindDesiredChildWindow(directUI, WindowFinder::Has::ClassName{L"FloatNotifySink"    }))
            if (HWND combo    = WindowFinder::FindDesiredChildWindow(sink,     WindowFinder::Has::ClassName{L"ComboBox"           }))
            if (HWND edit     = WindowFinder::FindDesiredChildWindow(combo,    WindowFinder::Has::ClassName{L"Edit"               }))
                return edit;
            return WindowFinder::FindDesiredChildWindow(dlg, WindowFinder::Has::ClassName{L"Edit"});
        }
    };

    class OpenDialog : private CommonFileDialogUtils
    {
        HWND FindOpenFileEdit()
        {
            if (HWND comboEx = WindowFinder::FindDesiredChildWindow(openDlg, WindowFinder::Has::ClassName{L"ComboBoxEx32"}))
            if (HWND combo   = WindowFinder::FindDesiredChildWindow(comboEx, WindowFinder::Has::ClassName{L"ComboBox"}))
            if (HWND edit    = WindowFinder::FindDesiredChildWindow(combo,   WindowFinder::Has::ClassName{L"Edit"}))
                return edit;
            return FindSaveAsEdit(openDlg);
        }

        HWND openDlg;
    public:
        OpenDialog(HWND hwnd) : openDlg(hwnd) {}
        void OpenFile(const std::filesystem::path& filePath)
        {
            HWND dlgEdit = WindowUtils::WaitForWindow(1s, [&]() {return FindOpenFileEdit(); });
            Assert::AreNotEqual(nullptr, dlgEdit, "File Open edit control not found");
            
            auto ret = SendMessageW(dlgEdit, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(filePath.c_str()));
            Assert::AreNotEqual(0, ret, "Failed to set File Open filename");

            HWND okButton = GetDlgItem(openDlg, IDOK);
            Assert::AreNotEqual(nullptr, okButton, "File Open OK button not found");
            SendMessageW(okButton, BM_CLICK, 0, 0);

            Poll::While(1s, 1ms, [this]() { return IsWindow(openDlg); });
        }
        void PressCancel()
        {
            HWND cancel = GetDlgItem(openDlg, IDCANCEL);
            Assert::AreNotEqual(nullptr, cancel, "File Open's Cancel button not found");

            SendMessageW(cancel, BM_CLICK, 0, 0);
            Poll::While(1s, 1ms, [this]() { return IsWindow(openDlg); });
        }
    };
    class SaveAsDialog : private CommonFileDialogUtils
    {
        HWND saveAs;
    public:
        SaveAsDialog(HWND hwnd) : saveAs(hwnd) {}
        void SaveFile(const std::filesystem::path& fileName)
        {
            HWND dlgEdit = WindowUtils::WaitForWindow(1s, [&]() { return FindSaveAsEdit(saveAs); });
            Assert::AreNotEqual(nullptr, dlgEdit, "Save As edit control not found");
            Assert::IsTrue(SendMessageW(dlgEdit, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(fileName.c_str())) != 0, "Failed to set Save As filename");

            HWND okButton = GetDlgItem(saveAs, IDOK);
            Assert::AreNotEqual(nullptr, okButton, "Save As OK button not found");
            
            SendMessageW(okButton, BM_CLICK, 0, 0);
            Poll::While(1s, 1ms, [this]() { return IsWindow(saveAs); });

            Poll::Until(1s, 1ms, [&fileName]() { return std::filesystem::exists(fileName); });
            Assert::IsTrue(std::filesystem::exists(fileName), "Save As did not create the file");
        }
        void Cancel()
        {
            HWND cancelButton = GetDlgItem(saveAs, IDCANCEL);
            Assert::AreNotEqual(nullptr, cancelButton, "Cancel button not found");

            SendMessageW(cancelButton, BM_CLICK, 0, 0);
            Poll::While(1s, 1ms, [this]() { return IsWindow(saveAs); });
        }
    };

    class EditField
    {
        HWND edit;
    public:
        EditField(HWND hwnd) : edit(hwnd) {}
        void AppendText(const std::wstring& text)
        {
            auto existing = GetText();
            DWORD dw = (DWORD)existing.length();
            SetCursorPosition(dw, dw);

            for (auto c : text)
                SendMessageW(edit, WM_CHAR, c, 1);
        }
        void SetText(const std::wstring& text)
        {
            Assert::IsTrue(SendMessageW(edit, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(L"")) != 0, "Failed to clear edit field text");
            AppendText(text);
        }
        std::wstring GetText() const
        {
            const LRESULT length = SendMessageW(edit, WM_GETTEXTLENGTH, 0, 0);
            std::wstring buffer(static_cast<size_t>(length) + 1, L'\0');
            SendMessageW(edit, WM_GETTEXT, static_cast<WPARAM>(buffer.size()), reinterpret_cast<LPARAM>(buffer.data()));
            buffer.resize(static_cast<size_t>(length));
            return buffer;
        }
        void MarkAsDirty   () { SendMessageW(edit, EM_SETMODIFY, TRUE,  0); }
        void ClearDirtyFlag() { SendMessageW(edit, EM_SETMODIFY, FALSE,  0); }
        bool IsDirty () const { return !!SendMessageW(edit, EM_GETMODIFY, 0, 0); }
        bool HasFocus() const
        {
            HWND main = GetParent(edit);
            SetForegroundWindow(main);
            Poll::Until(1s, 1ms, [&main]() { return GetForegroundWindow() == main; });
            DWORD tid = GetWindowThreadProcessId(GetForegroundWindow(), nullptr);
            GUITHREADINFO gti{ sizeof(GUITHREADINFO) };
            GetGUIThreadInfo(tid, &gti);
            return gti.hwndFocus == edit;
        }
        void GetCursorPosition(DWORD& start, DWORD& end)
        {
            DWORD s=0, e=0;
            SendMessage(edit, EM_GETSEL, (WPARAM)&s, (LPARAM)&e);
            start = s;
            end   = e;
        }
        void SetCursorPosition(DWORD start, DWORD end)
        {
            SendMessage(edit, EM_SETSEL, (WPARAM)start, (LPARAM)end);
        }
    };

    class ModalMessageBox
    {
        HWND msgBox;
        std::function<void()> onDismissed;
    public:
        ModalMessageBox(HWND hwnd, std::function<void()> onDismissed) : msgBox(hwnd), onDismissed(std::move(onDismissed)) {}
        void PressOk()
        {
            MessageBoxUtils::PushOkButton(msgBox);
            onDismissed(); // let MainWindow peer know that the modal messagebox is gone, so it can look for the main window
        }
    };
    struct PrintDialog
    {
        HWND printDlg;
        void Cancel()
        {
         // PostMessage(printDlg, WM_CLOSE, 0, 0); // now using UIA, because print dialog is no longer the classic win32 dialogbox            
            Poll::While(90s, 50ms, [this]()
                {
                    ClickPrintDialogCancel(printDlg);
                    return IsWindowVisible(printDlg);
                });
            Assert::IsFalse(IsWindowVisible(printDlg), "print dialog should have been dismissed");
        }
    };

    struct FindReplaceCommonalities
    {
        static void Cancel(HWND dlg, const std::string& assertMessage)
        {
            HWND cancel = WindowFinder::FindDesiredChildWindow(dlg, WindowFinder::Has::ClassName(L"Button"), WindowFinder::Has::Caption(L"Cancel"));
            PostMessage(cancel, BM_CLICK, 0, 0);
            Poll::While(1s, 1ms, [&]() { return IsWindowVisible(dlg); });
            Assert::IsFalse(IsWindowVisible(dlg), assertMessage);
        }
    };

    struct FindDialog
    {
        HWND findDlg;
        void Cancel() { FindReplaceCommonalities::Cancel(findDlg, "Find dialog should have been dismissed by now"); }
        std::wstring GetEditFieldText() const
        {
            HWND edit = WindowFinder::FindDesiredChildWindow(findDlg, WindowFinder::Has::ClassName(L"Edit"));

            int len = static_cast<int>(SendMessage(edit, WM_GETTEXTLENGTH, 0, 0));
            if (len <= 0)
                return {};
            
            std::wstring s(len+1, L'\0');
            SendMessage(edit, WM_GETTEXT, len + 1, reinterpret_cast<LPARAM>(s.data()));
            s.resize(len);
            return s;
        }
        void SetEditFieldText(const std::wstring& criterion)
        {
            HWND edit = WindowFinder::FindDesiredChildWindow(findDlg, WindowFinder::Has::ClassName(L"Edit"));
            SendMessage(edit, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(criterion.c_str()));
        }
        void SelectWholeWordCheckbox(bool b)
        {
            HWND checkBox = WindowFinder::FindDesiredChildWindow(findDlg, WindowFinder::Has::ClassName(L"Button"), WindowFinder::Has::Caption(L"Match &whole word only"));
            SendMessageW(checkBox, BM_SETCHECK, b ? BST_CHECKED : BST_UNCHECKED, 0);
        }

        void SelectDirection(bool down)
        {
            HWND radio = WindowFinder::FindDesiredChildWindow(findDlg, WindowFinder::Has::ClassName(L"Button"), down ? WindowFinder::Has::Caption(L"&Down") : WindowFinder::Has::Caption(L"&Up"));
            PostMessage(radio, BM_CLICK, 0, 0);
        }
        void SelectUpRadioButton  () { SelectDirection(false); }
        void SelectDownRadioButton() { SelectDirection(true ); }

        void FindNext()
        {
            HWND findNext = WindowFinder::FindDesiredChildWindow(findDlg, WindowFinder::Has::ClassName(L"Button"), WindowFinder::Has::Caption(L"&Find Next"));
            if (!(GetWindowLong(findNext, GWL_STYLE) & BS_DEFPUSHBUTTON))
            {   // @#%@# don't know why this is necessary, but it works
                PostMessage(findNext, BM_CLICK, 0, 0);
                Poll::Until(1s, 1ms, [&]() { return (GetWindowLong(findNext, GWL_STYLE) & BS_DEFPUSHBUTTON) != 0; });
            }
            PostMessage(findNext, BM_CLICK, 0, 0);
        }
    };
    struct ReplaceDialog
    {
        HWND replaceDlg;
        void Cancel() { FindReplaceCommonalities::Cancel(replaceDlg, "Replace dialog should have been dismissed by now"); }
    };

    class UnfoundMessageBox
    {
        HWND unfound;
    public:
        UnfoundMessageBox(HWND unfound) : unfound(unfound) {}
        std::wstring GetText() const { return MessageBoxUtils::GetStaticText(unfound); }
        void    PushOkButton()       {        MessageBoxUtils::PushOkButton(unfound); }
    };

    class SubMenu
    {
        static void SendKey(WORD vk, bool keyUp = false)
        {
            INPUT in{};
            in.type       = INPUT_KEYBOARD;
            in.ki.wVk     = vk;
            in.ki.dwFlags = keyUp ? KEYEVENTF_KEYUP : 0;
            SendInput(1, &in, sizeof(INPUT));
        }
        void SelectMenuItemViaKeyboard(WORD openKey,    // e.g., VK_F10 or 'E' for Edit
                                       WORD itemKey)    // e.g., 'C' for Copy
        {
            SetForegroundWindow(hwnd);
            Poll::Until(2s, 1ms, [this]() { return GetForegroundWindow() == hwnd; });

            // Alt tap (not Alt+E)
            SendKey(VK_MENU);        // Alt down
            SendKey(VK_MENU, true);  // Alt up

            // Now send mnemonic for the menu
            SendKey(openKey);        // 'E'
            SendKey(openKey, true);

            // Now send mnemonic for the item
            SendKey(itemKey);        // 'C'
            SendKey(itemKey, true);
        }

        HWND hwnd;
        HMENU subMenu;
    public:
        SubMenu(HWND hwnd, HMENU menu) : hwnd(hwnd), subMenu(menu) {}

        bool IsMenuItemEnabled(UINT id)
        {
            SendMessage(hwnd, WM_INITMENUPOPUP, (WPARAM)subMenu, 0);

            UINT state = GetMenuState(subMenu, id, MF_BYCOMMAND);
            Assert::AreNotEqual((UINT)(-1), state, "menu item not found");
            return !(state & MF_GRAYED) && !(state & MF_DISABLED);
        }
        void SelectMenuItem(UINT id)
        {
            switch (id)
            {
            case IDM_COPY     : SelectMenuItemViaKeyboard('E', 'C'); break;
            case IDM_CUT      : SelectMenuItemViaKeyboard('E', 'T'); break;
            case IDM_PASTE    : SelectMenuItemViaKeyboard('E', 'P'); break;
            case IDM_DELETE   : SelectMenuItemViaKeyboard('E', 'D'); break;
            case IDM_SELECTALL: SelectMenuItemViaKeyboard('E', 'A'); break;
            case IDM_FIND     : SelectMenuItemViaKeyboard('E', 'F'); break;
            case IDM_FINDNEXT : SelectMenuItemViaKeyboard('E', 'N'); break;
            default:
                Assert::Fail("mapping from menu id to keyboard selection is not implemented");
                break;
            }
        }
        void ClickMenuItem(const std::wstring& toplevel, const std::wstring& itemName)
        {
            ClickMenuItemViaUIA(hwnd, toplevel.c_str(), itemName.c_str());
        }
    };

    class Menu
    {
        SubMenu GetMenu(const std::string& name) const
        {
            int count = GetMenuItemCount(menu);
            for (int i=0; i<count; ++i)
            {
                char buf[64]{};
                GetMenuStringA(menu, i, buf, ARRAYSIZE(buf), MF_BYPOSITION);
                if (buf == name)
                    return {hwnd, GetSubMenu(menu, i)};
            }
            Assert::Fail(name + " menu item not found");
            return {nullptr, nullptr};
        }

        HWND hwnd;
        HMENU menu;
    public:
        Menu(HWND hwnd, HMENU hmenu) : hwnd(hwnd), menu(hmenu) {}
        SubMenu GetFileMenu() const { return GetMenu("&File"); }
        SubMenu GetEditMenu() const { return GetMenu("&Edit"); }
    };

    struct COM
    {
        COM()
        {
            CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        }
       ~COM()
        {
            CoUninitialize();
        }
    };
    class MainWindow : private COM
    {
        static std::wstring GetModuleDirectory()
        {
            std::wstring path(MAX_PATH, L'\0');
            DWORD len = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
            while (len == path.size())
            {
                path.resize(path.size() * 2, L'\0');
                len = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
            }
            path.resize(len);
            std::filesystem::path p(path);
            return p.parent_path().wstring();
        }
        static std::wstring GetNotepadExePath()
        {   // tests.exe lives alongside Notepad--.exe under <repo>\x64\<config>
            std::filesystem::path p(GetModuleDirectory());
            p /= L"Notepad--.exe";
            return p.wstring();
        }
        static bool LaunchNotepadAndWait(PROCESS_INFORMATION& pi, const wchar_t* args=nullptr)
        {
            const std::wstring exePath = GetNotepadExePath();
            Assert::IsTrue(std::filesystem::exists(exePath), "Notepad--.exe not found");

            STARTUPINFOW si{};
            si.cb = sizeof(si);
            std::wstring cmdLine = L"\"" + exePath + L"\"";
            if (args != nullptr && args[0] != L'\0')
            {
                cmdLine += L" ";
                cmdLine += L"\"";
                cmdLine += args;
                cmdLine += L"\"";
            }
            Assert::IsTrue(FALSE != CreateProcessW(exePath.c_str(), cmdLine.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi), "Failed to launch Notepad--.exe");
            WaitForInputIdle(pi.hProcess, 5000);
            return true;
        }
        void ValidateProcAndAssignHWND()
        {
            Assert::IsTrue(proc.created, "Failed to launch Notepad--.exe");
            hwnd = WindowUtils::WaitForWindow(1s, [pid = GetProcessId(proc.hProcess)]() { return WindowFinder::FindDesiredChildWindow(nullptr, WindowFinder::Has::Pid{pid}, WindowFinder::Is::Visible, WindowFinder::Is::UnOwned); });
            Assert::AreNotEqual(nullptr, hwnd, "Main window did not appear");
        }

        ProcessGuard proc;
        HWND hwnd{};
    public:
        MainWindow() : proc([&](PROCESS_INFORMATION& pi) { return LaunchNotepadAndWait(pi); })
        {
            ValidateProcAndAssignHWND();
        }
        MainWindow(const std::filesystem::path& filePath) : proc([&](PROCESS_INFORMATION& pi) { return LaunchNotepadAndWait(pi, filePath.c_str()); })
        {
            if (std::filesystem::exists(filePath))
                ValidateProcAndAssignHWND();
        }
        ModalMessageBox GetModalMessageBox()
        {
            HWND msg = WindowUtils::WaitForWindow(1s, [pid = GetProcessId(proc.hProcess)]() { return WindowFinder::FindDesiredChildWindow(nullptr, WindowFinder::Has::Pid{pid}, WindowFinder::Has::ClassName{L"#32770"}); });
            Assert::AreNotEqual(nullptr, msg, "Error message box not shown for bad file path");

            return ModalMessageBox(msg, [this]() { this->ValidateProcAndAssignHWND(); });
        }
        ~MainWindow()
        {   // we need to shut down cleanly no matter what.

            // can't throw from a dtor, ever.
            auto doNotThrow = [](auto fn) {
                try { return fn(); }
                catch (...) { }
            };
                
            if (TRUE != IsWindow(hwnd))
                return;

            doNotThrow([&]() { ExitViaMenu(); });
            if (TRUE != IsWindow(hwnd))
                return;

            doNotThrow([&]() { ExitViaMenuPopUp(); });
            if (TRUE != IsWindow(hwnd))
                return;

            DWORD exitCode = 0;
            if (GetExitCodeProcess(proc.hProcess, &exitCode)) {
                if (exitCode == STILL_ACTIVE)
                    TerminateProcess(proc.hProcess, 0); // process is running: terminate it
                else
                    return;                             // process has exited
            }
            else
                ; // GetExitCodeProcess failed. 

            if (WAIT_OBJECT_0 == WaitForSingleObject(proc.hProcess, 1000))
                TerminateProcess(proc.hProcess, 0);
        }

        bool IsRunning() const { return IsWindow(hwnd); }
        std::wstring GetTitle() const
        {
            wchar_t title[256]{};
            Assert::IsTrue(GetWindowTextW(hwnd, title, static_cast<int>(std::size(title))) > 0, "Failed to read window title");
            return title;
        }
        void FileNew() { PostMessageW(hwnd, WM_COMMAND, IDM_NEW, 0); }
        void TryExit() { PostMessageW(hwnd, WM_COMMAND, IDM_EXIT, 0); }
        void ExitViaMenu()
        {
            HMENU menu = ::GetMenu(hwnd);
            Assert::IsTrue(menu != nullptr, "Menu handle was null");
            SendMessageW(hwnd, WM_INITMENU, reinterpret_cast<WPARAM>(menu), 0);
            SendMessageW(hwnd, WM_COMMAND, IDM_EXIT, 0);
        }
        void ExitViaMenuPopUp()
        {
            HMENU menu = ::GetMenu(hwnd);
            Assert::IsTrue(menu != nullptr, "Menu handle was null");
            HMENU fileMenu = GetSubMenu(menu, 0);
            Assert::IsTrue(fileMenu != nullptr, "File menu handle was null");
            RECT itemRect{};
            Assert::IsTrue(GetMenuItemRect(hwnd, menu, 0, &itemRect) != 0, "Failed to get menu item rect");

            std::thread invokeThread([&]()  {
                                                std::this_thread::sleep_for(std::chrono::milliseconds{ 100 });
                                                PostMessageW(hwnd, WM_COMMAND, IDM_EXIT, 0);
                                            });
            TrackPopupMenu(fileMenu, TPM_LEFTALIGN | TPM_TOPALIGN, itemRect.left, itemRect.bottom, 0, hwnd, nullptr);
            invokeThread.join();
        }

        void Save()
        {
            PostMessageW(hwnd, WM_COMMAND, IDM_SAVE, 0);
        }

        FileDirtyMessageBox GetFileDirtyMessageBox() { return {GetProcessId(proc.hProcess)}; }
        EditField GetEditField()
        {
            HWND edit = FindWindowExW(hwnd, nullptr, L"Edit", nullptr);
            Assert::AreNotEqual(nullptr, edit, "Edit control not found");
            return {edit};
        }
        SaveAsDialog FindExistingFileSaveAsDialogBox()
        {
            HWND saveAs = WindowUtils::WaitForWindow(5s, [pid = GetProcessId(proc.hProcess)]() { return WindowFinder::FindDesiredChildWindow(nullptr, WindowFinder::Has::Pid{pid}, WindowFinder::Has::ClassName{L"#32770"}, WindowFinder::Has::EitherChildClass{ L"DUIViewWndClassName", L"DirectUIHWND" }); });
            Assert::AreNotEqual(nullptr, saveAs, "Save As dialog not found");
            return {saveAs};
        }
        SaveAsDialog SaveAs()
        {
            PostMessageW(hwnd, WM_COMMAND, IDM_SAVEAS, 0);
            return FindExistingFileSaveAsDialogBox();
        }
        OpenDialog FindExistingOpenDialog()
        {
            HWND openDlg = WindowUtils::WaitForWindow(1s, [pid = GetProcessId(proc.hProcess)]() { return WindowFinder::FindDesiredChildWindow(nullptr, WindowFinder::Has::Pid{pid}, WindowFinder::Has::ClassName{L"#32770"}, WindowFinder::Has::EitherChildClass{ L"DUIViewWndClassName", L"DirectUIHWND" }); });
            Assert::AreNotEqual(nullptr, openDlg, "File Open dialog not found");
            return {openDlg};
        }
        void FileOpen() { PostMessageW(hwnd, WM_COMMAND, IDM_OPEN, 0); }

        PrintDialog FindExistingPrintDialog()
        {
            HWND printDlg = nullptr;
            Poll::While(2s, 10ms, [&printDlg]()
                {
                    struct Finder
                    {
                        HWND hwnd{};
                    } finder;
                    EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL
                                {
                                    auto* finder = reinterpret_cast<Finder*>(lParam);

                                    wchar_t cls[256]{};
                                    GetClassNameW(hwnd, cls, static_cast<int>(std::size(cls)));
                                    if (std::wcscmp(cls, L"ApplicationFrameWindow") == 0)
                                    {
                                        wchar_t cap[512]{};
                                        GetWindowTextW(hwnd, cap, static_cast<int>(std::size(cap))); // better than SendMessage WM_GETTEXT because the latter can hang
                                        if (std::wcscmp(cap, L"Printing from Win32 application - Print") == 0)
                                        {
                                            finder->hwnd = hwnd;
                                            return FALSE; // found it!
                                        }
                                    }
                                    return TRUE; // continue enumeration
                                }, reinterpret_cast<LPARAM>(&finder));

                    printDlg = finder.hwnd;
                    return printDlg == nullptr;
                });
            Assert::AreNotEqual(nullptr, printDlg, "Print dialog not found");
            return {printDlg};
        }
        PrintDialog Print()
        {
            PostMessageW(hwnd, WM_COMMAND, IDM_PRINT, 0);
            return FindExistingPrintDialog();
        }
        FindDialog FindExistingFindDialog()
        {
            HWND findDlg = WindowUtils::WaitForWindow(2s, [pid = GetProcessId(proc.hProcess)]() { return WindowFinder::FindDesiredChildWindow(nullptr, WindowFinder::Has::Pid{pid}, WindowFinder::Has::ClassName{L"#32770"}, WindowFinder::Is::Visible); });
            Assert::AreNotEqual(nullptr, findDlg, "Find dialog not found");
            return {findDlg};
        }
        ReplaceDialog FindExistingReplaceDialog()
        {
            HWND replaceDlg = WindowUtils::WaitForWindow(2s, [pid = GetProcessId(proc.hProcess)]() { return WindowFinder::FindDesiredChildWindow(nullptr, WindowFinder::Has::Pid{pid}, WindowFinder::Has::ClassName{L"#32770"}, WindowFinder::Is::Visible); });
            Assert::AreNotEqual(nullptr, replaceDlg, "Replace dialog not found");
            return {replaceDlg};
        }
        UnfoundMessageBox FindExistingUnfoundMessageBox()
        {
            HWND unfound = WindowUtils::WaitForWindow(1s, [pid = GetProcessId(proc.hProcess)]() { return WindowFinder::FindDesiredChildWindow(nullptr, WindowFinder::Has::Pid{pid}, WindowFinder::Has::ClassName{L"#32770"}, WindowFinder::Is::Enabled, WindowFinder::Contains::ChildThatIsStaticIcon); });
            Assert::AreNotEqual(nullptr, unfound, "Find Error MessageBox not found");
            return {unfound};
        }

        Menu GetMenu() { return {hwnd, ::GetMenu(hwnd)}; }
        void Undo() { PostMessageW(hwnd, WM_COMMAND, IDM_UNDO, 0); }

        void EnsureInForeground()
        {
            Poll::Until(1s, 1ms, [this]()
                { // Attach our thread's input queue to the target's, so SetForegroundWindow is not blocked by UIPI focus rules.
                    DWORD targetTid  = GetWindowThreadProcessId(hwnd, nullptr);
                    DWORD currentTid = GetCurrentThreadId();
                    bool  attached   = (targetTid != currentTid) && AttachThreadInput(currentTid, targetTid, TRUE);

                    SetForegroundWindow(hwnd);
                    BringWindowToTop(hwnd);
                    ShowWindow(hwnd, SW_RESTORE);   // un-minimize if needed

                    if (attached)
                        AttachThreadInput(currentTid, targetTid, FALSE);

                    return GetForegroundWindow() == hwnd;
                });
            Assert::AreEqual(hwnd, GetForegroundWindow(), "failed to set Notepad-- as the foreground window");
        }

        enum KeyStates
        {
            None    = 0,
            Control = 1,
            Shift   = 2,
            BothControlAndShift = 3,
        };
        void SendKey(WORD vk, KeyStates keyStates)
        {
            INPUT inputs[6] = {};
            UINT  count     = 0;
            if (keyStates & Control) { inputs[count].type = INPUT_KEYBOARD; inputs[count].ki.wVk = VK_CONTROL; inputs[count].ki.dwFlags = 0;               count++; }
            if (keyStates & Shift)   { inputs[count].type = INPUT_KEYBOARD; inputs[count].ki.wVk = VK_SHIFT;   inputs[count].ki.dwFlags = 0;               count++; }
                                       inputs[count].type = INPUT_KEYBOARD; inputs[count].ki.wVk = vk;         inputs[count].ki.dwFlags = 0;               count++;
                                       inputs[count].type = INPUT_KEYBOARD; inputs[count].ki.wVk = vk;         inputs[count].ki.dwFlags = KEYEVENTF_KEYUP; count++;
            if (keyStates & Shift)   { inputs[count].type = INPUT_KEYBOARD; inputs[count].ki.wVk = VK_SHIFT;   inputs[count].ki.dwFlags = KEYEVENTF_KEYUP; count++; }
            if (keyStates & Control) { inputs[count].type = INPUT_KEYBOARD; inputs[count].ki.wVk = VK_CONTROL; inputs[count].ki.dwFlags = KEYEVENTF_KEYUP; count++; }
            SendInput(count, inputs, sizeof(INPUT));
        }
        void SendKey(char c, KeyStates keyStates)
        {
            SendKey(static_cast<WORD>(VkKeyScanA(c) & 0xFF), keyStates);
        }
    };
}
