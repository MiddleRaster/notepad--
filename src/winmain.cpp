#include <windows.h>
#include <filesystem>
#include <string>
#include <vector>
#include "MessageHandler.h"
#include "Resource.h"

#define MAX_LOADSTRING 100

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE: return MessageHandler::CreateHandlerAndEditWindow(hWnd);
    case WM_NCDESTROY:     MessageHandler::DestroyHandler(hWnd);                                            break;
    case WM_SIZE:          MessageHandler::GetHandler(hWnd).Handle_WM_SIZE(LOWORD(lParam), HIWORD(lParam)); break;
    case WM_INITMENUPOPUP: MessageHandler::GetHandler(hWnd).Handle_MenuPopUp(reinterpret_cast<HMENU>(wParam)); break;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {   // Parse the menu selections:
        case IDM_NEW:      MessageHandler::GetHandler(hWnd).Handle_FileNew(hWnd);   break;
        case IDM_OPEN:     MessageHandler::GetHandler(hWnd).Handle_FileOpen(hWnd);  break;
        case IDM_SAVE  :   MessageHandler::GetHandler(hWnd).Handle_FileSave(hWnd);  break;
        case IDM_SAVEAS:   MessageHandler::GetHandler(hWnd).Handle_FileSaveAs(hWnd);break;
        case IDM_PRINT:    MessageHandler::GetHandler(hWnd).Handle_Print(hWnd);     break;
        case IDM_ABOUT:    MessageHandler::GetHandler(hWnd).Handle_About(hWnd);     break;
        case IDM_EXIT:     MessageHandler::GetHandler(hWnd).Handle_Exit(hWnd);      break;
        default:           return DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;
    case WM_DESTROY:       PostQuitMessage(0);                                      break;
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
    wcex.hIconSm       = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_NOTEPAD));
    wcex.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName  = MAKEINTRESOURCEW(IDC_NOTEPAD);
    wcex.lpszClassName = szWindowClass;
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
