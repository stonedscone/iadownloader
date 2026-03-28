#include "globals.h"
#include <shlobj.h>
#include <string>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")

void CreateControls(HWND hwnd, HINSTANCE hInstance)
{
    g_bgBrush = CreateSolidBrush(CLR_BG);

    // Fonts
    g_fontTitle = CreateFontA(-22, 0, 0, 0, FW_BOLD, 0, 0, 0,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, "Segoe UI");
    g_fontNormal = CreateFontA(-13, 0, 0, 0, FW_NORMAL, 0, 0, 0,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, "Segoe UI");
    g_fontMono = CreateFontA(-13, 0, 0, 0, FW_NORMAL, 0, 0, 0,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, FIXED_PITCH, "Consolas");

    // Title
    HWND hTitle = CreateWindowExA(0, "STATIC",
        "Internet Archive Downloader",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        20, 18, 500, 30,
        hwnd, nullptr, hInstance, nullptr);
    SendMessage(hTitle, WM_SETFONT, (WPARAM)g_fontTitle, TRUE);

    // Subtitle
    HWND hSub = CreateWindowExA(0, "STATIC",
        "Download files from any archive.org collection",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        22, 48, 420, 18,
        hwnd, nullptr, hInstance, nullptr);
    SendMessage(hSub, WM_SETFONT, (WPARAM)g_fontNormal, TRUE);

    // Identifier label
    HWND hIdLabel = CreateWindowExA(0, "STATIC", "Archive.org Identifier:",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        20, 82, 160, 18,
        hwnd, nullptr, hInstance, nullptr);
    SendMessage(hIdLabel, WM_SETFONT, (WPARAM)g_fontNormal, TRUE);

    // Identifier input
    g_hIdentEdit = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
        20, 100, 480, 26,
        hwnd, (HMENU)IDC_IDENTIFIER_EDIT, hInstance, nullptr);
    SendMessage(g_hIdentEdit, WM_SETFONT, (WPARAM)g_fontMono, TRUE);
    SendMessageW(g_hIdentEdit, EM_SETCUEBANNER, FALSE,
        (LPARAM)L"e.g.  your-collection-identifier");

    // Fetch button
    g_hFetchBtn = CreateWindowExA(0, "BUTTON", "Fetch Files",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        510, 99, 110, 28,
        hwnd, (HMENU)IDC_FETCH_BTN, hInstance, nullptr);
    SendMessage(g_hFetchBtn, WM_SETFONT, (WPARAM)g_fontNormal, TRUE);

    // Filter label
    HWND hFilterLabel = CreateWindowExA(0, "STATIC", "Filter:",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        20, 136, 40, 18,
        hwnd, nullptr, hInstance, nullptr);
    SendMessage(hFilterLabel, WM_SETFONT, (WPARAM)g_fontNormal, TRUE);

    // Filter input
    g_hFilterEdit = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
        64, 134, 290, 22,
        hwnd, (HMENU)IDC_FILTER_EDIT, hInstance, nullptr);
    SendMessage(g_hFilterEdit, WM_SETFONT, (WPARAM)g_fontNormal, TRUE);
    SendMessageW(g_hFilterEdit, EM_SETCUEBANNER, FALSE,
        (LPARAM)L"Type to filter files...");

    // Select All / None
    HWND hSelAll = CreateWindowExA(0, "BUTTON", "Select All",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        370, 133, 95, 24,
        hwnd, (HMENU)IDC_SELECT_ALL, hInstance, nullptr);
    SendMessage(hSelAll, WM_SETFONT, (WPARAM)g_fontNormal, TRUE);

    HWND hSelNone = CreateWindowExA(0, "BUTTON", "Select None",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        472, 133, 95, 24,
        hwnd, (HMENU)IDC_SELECT_NONE, hInstance, nullptr);
    SendMessage(hSelNone, WM_SETFONT, (WPARAM)g_fontNormal, TRUE);

    // File list
    g_hListView = CreateWindowExA(WS_EX_CLIENTEDGE, WC_LISTVIEWA, "",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_REPORT | LVS_SHOWSELALWAYS,
        20, 164, 620, 280,
        hwnd, (HMENU)IDC_FILE_LIST, hInstance, nullptr);
    SendMessage(g_hListView, WM_SETFONT, (WPARAM)g_fontNormal, TRUE);
    ListView_SetExtendedListViewStyle(g_hListView,
        LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_CHECKBOXES);

    char colName[] = "File Name";
    char colSize[] = "Size";
    LVCOLUMNA col = {};
    col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
    col.fmt = LVCFMT_LEFT;
    col.cx = 450;
    col.pszText = colName;
    ListView_InsertColumn(g_hListView, 0, &col);
    col.cx = 160;
    col.pszText = colSize;
    ListView_InsertColumn(g_hListView, 1, &col);

    // Total size label
    g_hTotalLabel = CreateWindowExA(0, "STATIC", "No files selected",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        20, 452, 500, 18,
        hwnd, (HMENU)IDC_TOTAL_SIZE_LABEL, hInstance, nullptr);
    SendMessage(g_hTotalLabel, WM_SETFONT, (WPARAM)g_fontNormal, TRUE);

    // Destination label
    HWND hDestLabel = CreateWindowExA(0, "STATIC", "Download To:",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        20, 482, 100, 18,
        hwnd, nullptr, hInstance, nullptr);
    SendMessage(hDestLabel, WM_SETFONT, (WPARAM)g_fontNormal, TRUE);

    // Destination input
    g_hDestEdit = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
        20, 500, 480, 26,
        hwnd, (HMENU)IDC_DEST_EDIT, hInstance, nullptr);
    SendMessage(g_hDestEdit, WM_SETFONT, (WPARAM)g_fontMono, TRUE);

    char defaultDest[MAX_PATH] = {};
    SHGetFolderPathA(nullptr, CSIDL_PERSONAL, nullptr, 0, defaultDest);
    SetWindowTextA(g_hDestEdit,
        (std::string(defaultDest) + "\\IA Downloads").c_str());

    // Browse button
    HWND hBrowse = CreateWindowExA(0, "BUTTON", "Browse...",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        510, 499, 110, 28,
        hwnd, (HMENU)IDC_BROWSE_BTN, hInstance, nullptr);
    SendMessage(hBrowse, WM_SETFONT, (WPARAM)g_fontNormal, TRUE);

    // Overall progress
    HWND hOvLabel = CreateWindowExA(0, "STATIC", "Overall:",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        20, 540, 55, 16,
        hwnd, nullptr, hInstance, nullptr);
    SendMessage(hOvLabel, WM_SETFONT, (WPARAM)g_fontNormal, TRUE);

    g_hProgress = CreateWindowExA(0, PROGRESS_CLASSA, "",
        WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
        80, 538, 560, 18,
        hwnd, (HMENU)IDC_PROGRESS, hInstance, nullptr);
    SendMessage(g_hProgress, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
    SendMessage(g_hProgress, PBM_SETBKCOLOR, 0, (LPARAM)CLR_PANEL);
    SendMessage(g_hProgress, PBM_SETBARCOLOR, 0, (LPARAM)CLR_ACCENT);

    // File progress
    HWND hFileLabel = CreateWindowExA(0, "STATIC", "File:",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        20, 565, 55, 16,
        hwnd, nullptr, hInstance, nullptr);
    SendMessage(hFileLabel, WM_SETFONT, (WPARAM)g_fontNormal, TRUE);

    g_hFileProgress = CreateWindowExA(0, PROGRESS_CLASSA, "",
        WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
        80, 563, 560, 18,
        hwnd, (HMENU)IDC_FILE_PROGRESS, hInstance, nullptr);
    SendMessage(g_hFileProgress, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
    SendMessage(g_hFileProgress, PBM_SETBKCOLOR, 0, (LPARAM)CLR_PANEL);
    SendMessage(g_hFileProgress, PBM_SETBARCOLOR, 0, (LPARAM)CLR_ACCENT2);

    // Status label
    g_hStatus = CreateWindowExA(0, "STATIC",
        "Enter an identifier above and click Fetch Files.",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        20, 590, 560, 18,
        hwnd, (HMENU)IDC_STATUS_LABEL, hInstance, nullptr);
    SendMessage(g_hStatus, WM_SETFONT, (WPARAM)g_fontNormal, TRUE);

    // ETA label
    g_hEta = CreateWindowExA(0, "STATIC", "",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        20, 610, 620, 16,
        hwnd, (HMENU)IDC_ETA_LABEL, hInstance, nullptr);
    SendMessage(g_hEta, WM_SETFONT, (WPARAM)g_fontNormal, TRUE);

    // Start Download button
    g_hDownloadBtn = CreateWindowExA(0, "BUTTON", "> Start Download",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        20, 655, 180, 34,
        hwnd, (HMENU)IDC_DOWNLOAD_BTN, hInstance, nullptr);
    SendMessage(g_hDownloadBtn, WM_SETFONT, (WPARAM)g_fontNormal, TRUE);

    // Cancel button
    g_hCancelBtn = CreateWindowExA(0, "BUTTON", "[ Cancel ]",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        210, 655, 120, 34,
        hwnd, (HMENU)IDC_CANCEL_BTN, hInstance, nullptr);
    SendMessage(g_hCancelBtn, WM_SETFONT, (WPARAM)g_fontNormal, TRUE);
    EnableWindow(g_hCancelBtn, FALSE);

    // Pause button
    g_hPauseBtn = CreateWindowExA(0, "BUTTON", "Pause",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        340, 655, 100, 34,
        hwnd, (HMENU)IDC_PAUSE_BTN, hInstance, nullptr);
    SendMessage(g_hPauseBtn, WM_SETFONT, (WPARAM)g_fontNormal, TRUE);
    EnableWindow(g_hPauseBtn, FALSE);

    // Help button
    HWND hHelpBtn = CreateWindowExA(0, "BUTTON", "?",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        624, 665, 24, 24,
        hwnd, (HMENU)IDC_HELP_BTN, hInstance, nullptr);
    SendMessage(hHelpBtn, WM_SETFONT, (WPARAM)g_fontNormal, TRUE);
}