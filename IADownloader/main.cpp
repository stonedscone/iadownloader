#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <string>
#include <shlobj.h>
#include <thread>
#include <sstream>

#include "globals.h"
#include "fetch.h"
#include "fetch.h"

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")
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


// Fetch
std::vector<FileEntry>  g_files;

// Atomic Logic -- Used so multiple threads can read and write at same time w/o corrupt
std::atomic<bool>       g_fetching{ false };
std::atomic<bool>       g_downloading{ false };
std::atomic<bool>       g_cancelDownload{ false };
std::atomic<bool>       g_pauseDownload{ false };


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
    
    case WM_COMMAND: {
        int id = LOWORD(wParam);

        // Help Button Logic + Prompt
        if (id == IDC_HELP_BTN) {
            MessageBoxA(g_hwnd,
                "How to use Internet Archive Downloader:\n\n"
                "1. Go to archive.org and find a collection\n"
                "2. Copy the identifier from the URL after /details/\n"
                "3. Paste it into the identifier box\n"
                "4. Should look like: /collection/\n"
                "5. Click Fetch Files\n"
                "6. Check the files you want to download or select all\n"
                "7. Pick a destination folder or use default\n"
                "8. Click Start Download\n"
                "9. Enjoy your downloaded content!",
                "Help", MB_OK);
            return 0;
        }

        // Browse File Locale Handler
        if (id == IDC_BROWSE_BTN) {
            BROWSEINFOA bi = {};
            bi.hwndOwner = g_hwnd;
            bi.lpszTitle = "Select Download Folder";
            bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
            PIDLIST_ABSOLUTE pidl = SHBrowseForFolderA(&bi);
            if (pidl) {
                char path[MAX_PATH] = {};
                SHGetPathFromIDListA(pidl, path);
                SetWindowTextA(g_hDestEdit, path);
                CoTaskMemFree(pidl);
            }
            return 0;
        }

        // Fetch Files Button Handler
        if (id == IDC_FETCH_BTN) {
            if (g_fetching || g_downloading) return 0;
            char buf[512] = {};
            GetWindowTextA(g_hIdentEdit, buf, 512);
            std::string ident(buf);
            if (ident.empty()) {
                SetWindowTextA(g_hStatus,
                    "Please enter an identifier first.");
                return 0;
            }
            EnableWindow(g_hFetchBtn, FALSE);
            ListView_DeleteAllItems(g_hListView);
            std::thread(FetchThread, ident).detach();
            return 0;
        }

        // Select All Files Handler
        if (id == IDC_SELECT_ALL) {
            int n = ListView_GetItemCount(g_hListView);
            for (int i = 0; i < n; i++)
                ListView_SetCheckState(g_hListView, i, TRUE);
            return 0;
        }

        // De-Select All Files Handler
        if (id == IDC_SELECT_NONE) {
            int n = ListView_GetItemCount(g_hListView);
            for (int i = 0; i < n; i++)
                ListView_SetCheckState(g_hListView, i, FALSE);
            return 0;
        }

        return 0;
    }

    case WM_UPDATE_STATUS:
        SetWindowTextA(g_hStatus, (const char*)wParam);
        return 0;

    case WM_FETCH_DONE: {
        EnableWindow(g_hFetchBtn, TRUE);
        if (wParam == 0) return 0;

        // Populate the ListView with fetched files
        ListView_DeleteAllItems(g_hListView);
        for (size_t i = 0; i < g_files.size(); i++) {
            LVITEMA lvi = {};
            lvi.mask = LVIF_TEXT | LVIF_PARAM;
            lvi.iItem = (int)i;
            lvi.lParam = (LPARAM)i;
            lvi.pszText = const_cast<LPSTR>(g_files[i].name.c_str());
            int row = ListView_InsertItem(g_hListView, &lvi);
            ListView_SetItemText(g_hListView, row, 1,
                const_cast<LPSTR>(g_files[i].sizeStr.c_str()));
            // Start unchecked
            ListView_SetCheckState(g_hListView, row, FALSE);
        }

        // Update status
        std::ostringstream msg;
        msg << "Found " << g_files.size()
            << " files. Select what you want to download.";
        SetWindowTextA(g_hStatus, msg.str().c_str());
        return 0;
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