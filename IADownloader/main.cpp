#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <string>
#include <shlobj.h>
#include <thread>
#include <sstream>

#include "globals.h"
#include "fetch.h"
#include "download.h"

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
HWND g_hPauseBtn = nullptr;

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


//Helper Func. For Filter
void ApplyFilter() {
    char buf[256] = {};
    GetWindowTextA(g_hFilterEdit, buf, 256);
    std::string filter(buf);
    for (auto& c : filter)
        c = (char)tolower((unsigned char)c);

    // Disable redraw while updating
    SendMessage(g_hListView, WM_SETREDRAW, FALSE, 0);
    ListView_DeleteAllItems(g_hListView);

    for (size_t i = 0; i < g_files.size(); i++) {
        std::string nameLow = g_files[i].name;
        for (auto& c : nameLow)
            c = (char)tolower((unsigned char)c);

        if (filter.empty() ||
            nameLow.find(filter) != std::string::npos) {
            LVITEMA lvi = {};
            lvi.mask = LVIF_TEXT | LVIF_PARAM;
            lvi.iItem = ListView_GetItemCount(g_hListView);
            lvi.lParam = (LPARAM)i;
            lvi.pszText = const_cast<LPSTR>(g_files[i].name.c_str());
            int row = ListView_InsertItem(g_hListView, &lvi);
            ListView_SetItemText(g_hListView, row, 1,
                const_cast<LPSTR>(g_files[i].sizeStr.c_str()));
            ListView_SetCheckState(g_hListView, row, FALSE);
        }
    }

    // Re-enable redraw and repaint
    SendMessage(g_hListView, WM_SETREDRAW, TRUE, 0);
    RedrawWindow(g_hListView, nullptr, nullptr,
        RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN);
}


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

    case WM_UPDATE_PROGRESS:
        SendMessage(g_hProgress, PBM_SETPOS, (WPARAM)wParam, 0);
        return 0;

    case WM_FILE_DONE:
        SendMessage(g_hFileProgress, PBM_SETPOS, (WPARAM)wParam, 0);
        return 0;

    case WM_UPDATE_ETA:
        SetWindowTextA(g_hEta, (const char*)wParam);
        return 0;

    case WM_DOWNLOAD_DONE:
        EnableWindow(g_hDownloadBtn, TRUE);
        EnableWindow(g_hFetchBtn, TRUE);
        EnableWindow(g_hCancelBtn, FALSE);
        EnableWindow(g_hPauseBtn, FALSE);
        SetWindowTextA(g_hPauseBtn, "Pause");
        SendMessage(g_hProgress, PBM_SETPOS, 100, 0);
        return 0;
    
    case WM_TIMER: {
        if (wParam == FILTER_TIMER_ID) {
            KillTimer(hwnd, FILTER_TIMER_ID);
            ApplyFilter();
        }
        return 0;
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

        //----------------------------------------------
        // Filter Box Function
        //----------------------------------------------
        if (id == IDC_FILTER_EDIT && HIWORD(wParam) == EN_CHANGE) {
            SetTimer(g_hwnd, FILTER_TIMER_ID, FILTER_TIMER_DELAY, nullptr);
            return 0;
        }


        //----------------------------------------------
        // Download Button Handler
        //----------------------------------------------
        if (id == IDC_DOWNLOAD_BTN) {
            if (g_downloading || g_fetching) return 0;

            // Collect checked files
            auto* task = new DownloadTask();
            int n = ListView_GetItemCount(g_hListView);
            for (int i = 0; i < n; i++) {
                if (!ListView_GetCheckState(g_hListView, i)) continue;
                LVITEMA lvi = {};
                lvi.mask = LVIF_PARAM;
                lvi.iItem = i;
                ListView_GetItem(g_hListView, &lvi);
                int idx = (int)lvi.lParam;
                if (idx >= 0 && idx < (int)g_files.size())
                    task->files.push_back(g_files[idx]);
            }

            if (task->files.empty()) {
                SetWindowTextA(g_hStatus,
                    "No files selected. Check some boxes first.");
                delete task;
                return 0;
            }

            char destBuf[MAX_PATH] = {};
            GetWindowTextA(g_hDestEdit, destBuf, MAX_PATH);
            task->destDir = destBuf;
            if (task->destDir.empty()) {
                SetWindowTextA(g_hStatus,
                    "Please select a destination folder.");
                delete task;
                return 0;
            }
            CreateDirectoryA(task->destDir.c_str(), nullptr);

            EnableWindow(g_hDownloadBtn, FALSE);
            EnableWindow(g_hCancelBtn, TRUE);
            EnableWindow(g_hPauseBtn, TRUE);
            EnableWindow(g_hFetchBtn, FALSE);
            SendMessage(g_hProgress, PBM_SETPOS, 0, 0);
            SendMessage(g_hFileProgress, PBM_SETPOS, 0, 0);
            SetWindowTextA(g_hEta, "");

            std::thread(DownloadThread, task).detach();
            return 0;
        }
        //----------------------------------------------


        //Cancel Button
        if (id == IDC_CANCEL_BTN) {
            g_cancelDownload = true;
            EnableWindow(g_hCancelBtn, FALSE);
            SetWindowTextA(g_hCancelBtn, "Cancelling...");
            return 0;
        }

        //Pause Button
        if (id == IDC_PAUSE_BTN) {
            if (!g_pauseDownload) {
                g_pauseDownload = true;
                SetWindowTextA(g_hPauseBtn, "Resume");
            }
            else {
                g_pauseDownload = false;
                SetWindowTextA(g_hPauseBtn, "Pause");
            }
            return 0;
        }

        return 0;
    }

    // Make Control+A Work
    case WM_KEYDOWN: {
        if (GetKeyState(VK_CONTROL) & 0x8000) {
            if (wParam == 'A') {
                HWND focused = GetFocus();
                if (focused == g_hIdentEdit ||
                    focused == g_hDestEdit ||
                    focused == g_hFilterEdit) {
                    SendMessage(focused, EM_SETSEL, 0, -1);
                    return 0;
                }
            }
        }
        break;
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
        // Make Ctrl+A Freaking Work
        if (msg.message == WM_KEYDOWN &&
            (GetKeyState(VK_CONTROL) & 0x8000) &&
            msg.wParam == 'A') {
            HWND focused = GetFocus();
            if (focused == g_hIdentEdit ||
                focused == g_hDestEdit ||
                focused == g_hFilterEdit) {
                SendMessage(focused, EM_SETSEL, 0, -1);
                continue;
            }
        }
        if (!IsDialogMessageA(g_hwnd, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
    }

    return 0;
}