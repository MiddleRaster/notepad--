#include <windows.h>
#include <filesystem>
#include <string>
#include <vector>
#include "MessageHandler.h"
#include "Resource.h"

#pragma comment(lib, "comdlg32.lib")

#define MAX_LOADSTRING 100

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    MessageHandler* messageHandler = MessageHandler::GetHandler(hWnd);

    switch (message)
    {
    case WM_CREATE: return MessageHandler::CreateHandlerAndEditWindow(hWnd);
    case WM_NCDESTROY:     MessageHandler::DestroyHandler(hWnd);                              break;
    case WM_SIZE:          messageHandler->Handle_WM_SIZE(LOWORD(lParam), HIWORD(lParam));    break;
    case WM_INITMENUPOPUP: messageHandler->Handle_MenuPopUp(reinterpret_cast<HMENU>(wParam)); break;
    case WM_ACTIVATE:      messageHandler->Handle_Activate(LOWORD(wParam));                   break;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_EDITFIELD:
            if (HIWORD(wParam) == EN_CHANGE) 
                            messageHandler->Handle_EN_CHANGE (hWnd);  break;
        case IDM_NEW:       messageHandler->Handle_FileNew   (hWnd);  break;
        case IDM_OPEN:      messageHandler->Handle_FileOpen  (hWnd);  break;
        case IDM_SAVE:      messageHandler->Handle_FileSave  (hWnd);  break;
        case IDM_SAVEAS:    messageHandler->Handle_FileSaveAs(hWnd);  break;
        case IDM_PRINT:     messageHandler->Handle_Print     (hWnd);  break;
        case IDM_ABOUT:     messageHandler->Handle_About     (hWnd);  break;
        case IDM_EXIT:      messageHandler->Handle_Exit      (hWnd);  break;
        case IDM_UNDO:      messageHandler->Handle_Undo      (hWnd);  break;
        case IDM_COPY:      messageHandler->Handle_Copy      (hWnd);  break;
        case IDM_CUT:       messageHandler->Handle_Cut       (hWnd);  break;
        case IDM_PASTE:     messageHandler->Handle_Paste     (hWnd);  break;
        case IDM_DELETE:    messageHandler->Handle_Delete    (hWnd);  break;
        case IDM_SELECTALL: messageHandler->Handle_SelectAll (hWnd);  break;
        case IDM_FIND:      messageHandler->Handle_Find      (hWnd);  break;
        default: return DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;
    case WM_DESTROY:       PostQuitMessage(0);                        break;
    default:
        if (messageHandler && messageHandler->IsFindMessage(message)) break;
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

    // for modeless Find dialog
    MessageHandler* pMessageHandler = reinterpret_cast<MessageHandler*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    pMessageHandler->SetRegisteredFindMessage(RegisterWindowMessage(FINDMSGSTRING));

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_NOTEPAD));
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (pMessageHandler->DoDialogMessage(&msg))
            continue;

        if (!TranslateAccelerator(hWnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    return (int)msg.wParam;
}
