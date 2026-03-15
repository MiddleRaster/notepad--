#include <windows.h>
#include <commdlg.h>
#include <filesystem>
#include <string>
#include <vector>
#include "MessageHandler.h"
#include "Resource.h"

#define MAX_LOADSTRING 100

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
    case WM_CREATE:              (new MessageHandler())->Handle_WM_CREATE(hWnd);    break;
    case WM_NCDESTROY:  MessageHandler::GetHandler(hWnd).Handle_WM_NCDESTROY(hWnd); break;
    case WM_SIZE:       MessageHandler::GetHandler(hWnd).Handle_WM_SIZE(LOWORD(lParam), HIWORD(lParam));    break;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {   // Parse the menu selections:
        case IDM_SAVEAS:
            MessageHandler::GetHandler(hWnd).Handle_SaveAs(hWnd);
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
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE /*hPrevInstance*/, _In_ LPWSTR /*lpCmdLine*/, _In_ int nCmdShow)
{
    WCHAR szTitle      [MAX_LOADSTRING]; // The title bar text
    WCHAR szWindowClass[MAX_LOADSTRING]; // the main window class name
    LoadStringW(hInstance, IDS_APP_TITLE_NOTEPAD, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_NOTEPAD,     szWindowClass, MAX_LOADSTRING);

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
    RegisterClassExW(&wcex);

    HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);
    if (!hWnd)
        return -1;
    ShowWindow  (hWnd, nCmdShow);
    UpdateWindow(hWnd);

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
