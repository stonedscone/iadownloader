/*
 * Internet Archive Downloader
 * Portable Win32 C++ desktop application
 *
 * Lets you browse any archive.org collection, select files,
 * and batch download them with progress tracking.
 *
 * Build (MSVC Developer Command Prompt):
 *   cl /std:c++17 /O2 /EHsc main.cpp ui.cpp fetch.cpp download.cpp
 *      /link /SUBSYSTEM:WINDOWS
 *      user32.lib gdi32.lib wininet.lib comctl32.lib shell32.lib ole32.lib
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <objbase.h>
#include <commctrl.h>
#include <sstream>
#include <iomanip>

#include "globals.h"
#include "ui.h"

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

 // ---------------------------------------------------------------------------
 // Global variable definitions (declared extern in globals.h)
 // ---------------------------------------------------------------------------
HWND g_hwnd = nullptr;
HWND g_listView = nullptr;
HWND g_destEdit = nullptr;
HWND g_statusLabel = nullptr;
HWND g_progressBar = nullptr;
HWND g_fileProgress = nullptr;
HWND g_totalSizeLabel = nullptr;
HWND g_etaLabel = nullptr;
HWND g_downloadBtn = nullptr;
HWND g_cancelBtn = nullptr;
HWND g_fetchBtn = nullptr;
HWND g_identEdit = nullptr;
HWND g_filterEdit = nullptr;

HFONT  g_fontTitle = nullptr;
HFONT  g_fontNormal = nullptr;
HFONT  g_fontMono = nullptr;
HBRUSH g_bgBrush = nullptr;

std::atomic<bool>       g_cancelDownload{ false };
std::atomic<bool>       g_downloading{ false };
std::atomic<bool>       g_fetching{ false };
std::vector<FileEntry>  g_allFiles;
std::mutex              g_mutex;

// ---------------------------------------------------------------------------
// Utility functions (used across fetch.cpp and download.cpp)
// ---------------------------------------------------------------------------
std::string formatSize(long long bytes) {
    if (bytes < 0) return "Unknown";
    std::ostringstream o;
    if (bytes < 1024) {
        o << bytes << " B";
        return o.str();
    }
    double kb = bytes / 1024.0;
    if (kb < 1024) {
        o << std::fixed << std::setprecision(1) << kb << " KB";
        return o.str();
    }
    double mb = kb / 1024.0;
    if (mb < 1024) {
        o << std::fixed << std::setprecision(1) << mb << " MB";
        return o.str();
    }
    double gb = mb / 1024.0;
    o << std::fixed << std::setprecision(2) << gb << " GB";
    return o.str();
}

std::string formatETA(double seconds) {
    if (seconds < 0) return "Calculating...";
    int s = static_cast<int>(seconds);
    std::ostringstream o;
    if (s < 60) { o << s << "s";                          return o.str(); }
    if (s < 3600) { o << s / 60 << "m " << s % 60 << "s";      return o.str(); }
    o << s / 3600 << "h " << (s % 3600) / 60 << "m";
    return o.str();
}

// Posts a heap-allocated string to the UI thread.
// The window proc picks it up in WM_UPDATE_STATUS and deletes it.
void setStatus(const std::string& msg, COLORREF color) {
    PostMessage(g_hwnd, WM_UPDATE_STATUS,
        static_cast<WPARAM>(color),
        reinterpret_cast<LPARAM>(new std::string(msg)));
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------
int WINAPI WinMain(_In_     HINSTANCE hInst,
    _In_opt_ HINSTANCE,
    _In_     LPSTR,
    _In_     int) {

    if (FAILED(CoInitialize(nullptr))) return 1;
    InitCommonControls();

    WNDCLASSEXA wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(CLR_BG);
    wc.lpszClassName = "IADownloader";
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    RegisterClassExA(&wc);

    const int W = 664, H = 700;
    int sx = GetSystemMetrics(SM_CXSCREEN);
    int sy = GetSystemMetrics(SM_CYSCREEN);

    g_hwnd = CreateWindowExA(0, "IADownloader",
        "Internet Archive Downloader",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        (sx - W) / 2, (sy - H) / 2, W, H,
        nullptr, nullptr, hInst, nullptr);

    ShowWindow(g_hwnd, SW_SHOW);
    UpdateWindow(g_hwnd);

    MSG msg;
    while (GetMessageA(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    CoUninitialize();
    return 0;
}
