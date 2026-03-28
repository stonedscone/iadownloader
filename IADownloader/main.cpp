#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <string>
#include "globals.h"

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

// Global definitions
HWND g_hwnd = nullptr;
HWND g_hListView = nullptr;
HWND g_hStatus = nullptr;
HWND g_hEta = nullptr;
HWND g_hProgress = nullptr;
HWND g_hFileProgress = nullptr;
HWND g_hTotalLabel = nullptr;
HWND g_hDestEdit = nullptr;
HWND g_hIdentEdit = nullptr;
HWND g_hFilterEdit = nullptr;
HWND g_hDownloadBtn = nullptr;
HWND g_hCancelBtn = nullptr;
HWND g_hFetchBtn = nullptr;

HFONT  g_fontTitle = nullptr;
HFONT  g_fontNormal = nullptr;
HFONT  g_fontMono = nullptr;
HBRUSH g_bgBrush = nullptr;

// Declared in ui.cpp
void CreateControls(HWND hwnd, HINSTANCE hInstance);

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
        CreateControls(hwnd, (HINSTANCE)GetModuleHandleA(nullptr));
        return 0;

    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wParam;
        SetBkColor(hdc, CLR_BG);
        SetTextColor(hdc, CLR_TEXT);
        return (LRESULT)g_bgBrush;
    }

    case WM_CTLCOLOREDIT: {
        HDC hdc = (HDC)wParam;
        SetBkColor(hdc, CLR_PANEL);
        SetTextColor(hdc, CLR_TEXT);
        static HBRUSH editBg = CreateSolidBrush(CLR_PANEL);
        return (LRESULT)editBg;
    }

    case WM_CTLCOLORBTN: {
        HDC hdc = (HDC)wParam;
        SetBkColor(hdc, CLR_PANEL);
        SetTextColor(hdc, CLR_ACCENT);
        static HBRUSH btnBg = CreateSolidBrush(CLR_PANEL);
        return (LRESULT)btnBg;
    }

    case WM_ERASEBKGND: {
        RECT rc;
        GetClientRect(hwnd, &rc);
        FillRect((HDC)wParam, &rc, g_bgBrush);
        return 1;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(_In_     HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_     LPSTR     lpCmdLine,
    _In_     int       nCmdShow)
{
    INITCOMMONCONTROLSEX icx = {};
    icx.dwSize = sizeof(icx);
    icx.dwICC = ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icx);

    WNDCLASSEXA wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "IADownloader";
    RegisterClassExA(&wc);

    g_hwnd = CreateWindowExA(
        0,
        "IADownloader",
        "Internet Archive Downloader",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        664, 740,
        nullptr, nullptr, hInstance, nullptr);

    ShowWindow(g_hwnd, nCmdShow);
    UpdateWindow(g_hwnd);

    MSG msg;
    while (GetMessageA(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    return 0;
}