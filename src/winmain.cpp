#include <windows.h>
#include <commdlg.h>
#include <filesystem>
#include <string>
#include <vector>
#include "Resource.h"

#define MAX_LOADSTRING 100

static void SaveEditTextToFile(HWND hWnd, const wchar_t* path)
{
    HWND edit = reinterpret_cast<HWND>(GetWindowLongPtrW(hWnd, GWLP_USERDATA)); // GetEditHandle(hWnd);
    if (edit == nullptr)
        return;

    const LRESULT length = SendMessageW(edit, WM_GETTEXTLENGTH, 0, 0);
    std::wstring text(static_cast<size_t>(length) + 1, L'\0');
    SendMessageW(edit, WM_GETTEXT, static_cast<WPARAM>(text.size()), reinterpret_cast<LPARAM>(text.data()));
    text.resize(static_cast<size_t>(length));

    const int utf8Size = WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
    if (utf8Size < 0)
        return;

    std::vector<char> utf8(static_cast<size_t>(utf8Size) + 3);
    utf8[0] = static_cast<char>(0xEF);
    utf8[1] = static_cast<char>(0xBB);
    utf8[2] = static_cast<char>(0xBF);
    WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), utf8.data() + 3, utf8Size, nullptr, nullptr);

    HANDLE file = CreateFileW(path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
        return;

    DWORD written = 0;
    WriteFile(file, utf8.data(), static_cast<DWORD>(utf8.size()), &written, nullptr);
    CloseHandle(file);
}

static void DoSaveAs(HWND hWnd)
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
        std::wstring title = std::filesystem::path{filePath}.filename().wstring();
        if (!title.empty())
            SetWindowTextW(hWnd, title.c_str());
    }
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow, const WCHAR* szWindowClass, const WCHAR* szTitle)
{
    HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);
    if (!hWnd)
        return FALSE;

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);
    return TRUE;
}

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        {
            HWND edit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",WS_CHILD|WS_VISIBLE|WS_VSCROLL|WS_HSCROLL|ES_LEFT|ES_MULTILINE|ES_AUTOVSCROLL|ES_AUTOHSCROLL, 0,0,0,0, hWnd, nullptr, GetModuleHandle(nullptr), nullptr);
            SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(edit));
        }
        break;
    case WM_SIZE:
        {
            HWND edit = reinterpret_cast<HWND>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
            if (edit != nullptr)
                MoveWindow(edit, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
        }
        break;
    case WM_COMMAND:
        // Parse the menu selections:
        switch (LOWORD(wParam))
        {
        case IDM_SAVEAS:
            DoSaveAs(hWnd);
            break;
        case IDM_ABOUT:
            DialogBox(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

ATOM MyRegisterClass(HINSTANCE hInstance, const WCHAR* szWindowClass)
{
    WNDCLASSEXW wcex;
    wcex.cbSize        = sizeof(WNDCLASSEX);
    wcex.style         = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc   = WndProc;
    wcex.cbClsExtra    = 0;
    wcex.cbWndExtra    = 0;
    wcex.hInstance     = hInstance;
    wcex.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_NOTEPAD));
    wcex.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName  = MAKEINTRESOURCEW(IDC_NOTEPAD);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm       = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL_NOTEPAD));
    return RegisterClassExW(&wcex);
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    WCHAR szTitle      [MAX_LOADSTRING]; // The title bar text
    WCHAR szWindowClass[MAX_LOADSTRING]; // the main window class name

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE_NOTEPAD, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_NOTEPAD,     szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance, szWindowClass);
    if (!InitInstance (hInstance, nCmdShow, szWindowClass, szTitle))
        return FALSE;

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_NOTEPAD));
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    return (int)msg.wParam;
}
