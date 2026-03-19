#include "ui.h"
#include "fetch.h"
#include "download.h"
#include "globals.h"

#include <commctrl.h>
#include <shlobj.h>
#include <sstream>
#include <thread>
#include <string>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")

// ---------------------------------------------------------------------------
// Window creation helpers
// ---------------------------------------------------------------------------
HWND makeLabel(HWND parent, const char* text, int x, int y, int w, int h) {
    return CreateWindowExA(0, "STATIC", text,
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        x, y, w, h, parent, nullptr,
        GetModuleHandleA(nullptr), nullptr);
}

HWND makeEdit(HWND parent, int id, int x, int y, int w, int h, DWORD extra) {
    return CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL | extra,
        x, y, w, h, parent,
        reinterpret_cast<HMENU>(static_cast<UINT_PTR>(id)),
        GetModuleHandleA(nullptr), nullptr);
}

HWND makeButton(HWND parent, const char* text, int id,
    int x, int y, int w, int h) {
    return CreateWindowExA(0, "BUTTON", text,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        x, y, w, h, parent,
        reinterpret_cast<HMENU>(static_cast<UINT_PTR>(id)),
        GetModuleHandleA(nullptr), nullptr);
}

// ---------------------------------------------------------------------------
// ListView helpers
// ---------------------------------------------------------------------------
void populateListView(const std::vector<FileEntry>& files,
    const std::string& filter) {
    ListView_DeleteAllItems(g_listView);

    std::string filterLow = filter;
    for (auto& c : filterLow)
        c = static_cast<char>(tolower(static_cast<unsigned char>(c)));

    int idx = 0;
    for (size_t fi = 0; fi < files.size(); ++fi) {
        const auto& f = files[fi];

        if (!filterLow.empty()) {
            std::string nameLow = f.name;
            for (auto& c : nameLow)
                c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
            if (nameLow.find(filterLow) == std::string::npos) continue;
        }

        LVITEMA lvi = {};
        lvi.mask = LVIF_TEXT | LVIF_PARAM;
        lvi.iItem = idx++;
        lvi.lParam = static_cast<LPARAM>(fi);
        // pszText is non-const but ListView copies it immediately
        lvi.pszText = const_cast<LPSTR>(f.name.c_str());
        int row = ListView_InsertItem(g_listView, &lvi);
        ListView_SetItemText(g_listView, row, 1,
            const_cast<LPSTR>(f.sizeStr.c_str()));
    }
}

void updateTotalSizeLabel() {
    long long total = 0;
    int       count = 0;
    int       n = ListView_GetItemCount(g_listView);

    for (int i = 0; i < n; i++) {
        if (!ListView_GetCheckState(g_listView, i)) continue;

        LVITEMA lvi = {};
        lvi.mask = LVIF_PARAM;
        lvi.iItem = i;
        ListView_GetItem(g_listView, &lvi);
        int origIdx = static_cast<int>(lvi.lParam);

        std::lock_guard<std::mutex> lock(g_mutex);
        if (origIdx >= 0 && origIdx < static_cast<int>(g_allFiles.size())) {
            if (g_allFiles[origIdx].size > 0)
                total += g_allFiles[origIdx].size;
            ++count;
        }
    }

    std::ostringstream o;
    o << count << " file(s) selected  |  Total: " << formatSize(total);
    SetWindowTextA(g_totalSizeLabel, o.str().c_str());
}

// ---------------------------------------------------------------------------
// WM_CREATE -- build all controls
// ---------------------------------------------------------------------------
static void onCreate(HWND hwnd) {
    g_bgBrush = CreateSolidBrush(CLR_BG);

    g_fontTitle = CreateFontA(-22, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH, "Segoe UI");
    g_fontNormal = CreateFontA(-13, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH, "Segoe UI");
    g_fontMono = CreateFontA(-12, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        FIXED_PITCH, "Consolas");

    // Title
    HWND hTitle = makeLabel(hwnd, "Internet Archive Downloader", 20, 18, 500, 30);
    SendMessage(hTitle, WM_SETFONT, reinterpret_cast<WPARAM>(g_fontTitle), TRUE);

    HWND hSub = makeLabel(hwnd,
        "Download files from any archive.org collection", 22, 48, 420, 18);
    SendMessage(hSub, WM_SETFONT, reinterpret_cast<WPARAM>(g_fontNormal), TRUE);

    // Identifier input
    HWND hIdLabel = makeLabel(hwnd, "Archive.org Identifier:", 20, 82, 160, 18);
    SendMessage(hIdLabel, WM_SETFONT, reinterpret_cast<WPARAM>(g_fontNormal), TRUE);

    g_identEdit = makeEdit(hwnd, IDC_IDENTIFIER_EDIT, 20, 100, 480, 26);
    SendMessage(g_identEdit, WM_SETFONT, reinterpret_cast<WPARAM>(g_fontMono), TRUE);
    SendMessageW(g_identEdit, EM_SETCUEBANNER, FALSE,
        reinterpret_cast<LPARAM>(L"e.g.  dragon-ball-z-episodes  or  YourCollectionIdentifier"));

    g_fetchBtn = makeButton(hwnd, "Fetch Files", IDC_FETCH_BTN, 510, 99, 110, 28);
    SendMessage(g_fetchBtn, WM_SETFONT, reinterpret_cast<WPARAM>(g_fontNormal), TRUE);

    // Filter row
    HWND hFiltLabel = makeLabel(hwnd, "Filter:", 20, 136, 40, 18);
    SendMessage(hFiltLabel, WM_SETFONT, reinterpret_cast<WPARAM>(g_fontNormal), TRUE);

    g_filterEdit = makeEdit(hwnd, IDC_FILTER_EDIT, 64, 134, 290, 22);
    SendMessage(g_filterEdit, WM_SETFONT, reinterpret_cast<WPARAM>(g_fontNormal), TRUE);
    SendMessageW(g_filterEdit, EM_SETCUEBANNER, FALSE,
        reinterpret_cast<LPARAM>(L"Type to filter files..."));

    HWND hSelAll = makeButton(hwnd, "Select All", IDC_SELECT_ALL, 370, 133, 95, 24);
    HWND hSelNone = makeButton(hwnd, "Select None", IDC_SELECT_NONE, 472, 133, 95, 24);
    SendMessage(hSelAll, WM_SETFONT, reinterpret_cast<WPARAM>(g_fontNormal), TRUE);
    SendMessage(hSelNone, WM_SETFONT, reinterpret_cast<WPARAM>(g_fontNormal), TRUE);

    // File list (ListView with checkboxes)
    g_listView = CreateWindowExA(WS_EX_CLIENTEDGE, WC_LISTVIEWA, "",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_REPORT | LVS_SHOWSELALWAYS,
        20, 164, 620, 280, hwnd,
        reinterpret_cast<HMENU>(IDC_FILE_LIST),
        GetModuleHandleA(nullptr), nullptr);
    SendMessage(g_listView, WM_SETFONT, reinterpret_cast<WPARAM>(g_fontNormal), TRUE);
    ListView_SetExtendedListViewStyle(g_listView,
        LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_CHECKBOXES);

    char colName[] = "File Name";
    char colSize[] = "Size";
    LVCOLUMNA col = {};
    col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
    col.fmt = LVCFMT_LEFT;
    col.cx = 450;
    col.pszText = colName;
    ListView_InsertColumn(g_listView, 0, &col);
    col.cx = 160;
    col.pszText = colSize;
    ListView_InsertColumn(g_listView, 1, &col);

    // Total size label
    g_totalSizeLabel = makeLabel(hwnd, "No files selected", 20, 452, 500, 18);
    SendMessage(g_totalSizeLabel, WM_SETFONT,
        reinterpret_cast<WPARAM>(g_fontNormal), TRUE);

    // Destination folder
    HWND hDestLabel = makeLabel(hwnd, "Download To:", 20, 482, 100, 18);
    SendMessage(hDestLabel, WM_SETFONT, reinterpret_cast<WPARAM>(g_fontNormal), TRUE);

    g_destEdit = makeEdit(hwnd, IDC_DEST_EDIT, 20, 500, 480, 26);
    SendMessage(g_destEdit, WM_SETFONT, reinterpret_cast<WPARAM>(g_fontMono), TRUE);

    char defaultDest[MAX_PATH] = {};
    SHGetFolderPathA(nullptr, CSIDL_PERSONAL, nullptr, 0, defaultDest);
    SetWindowTextA(g_destEdit,
        (std::string(defaultDest) + "\\IA Downloads").c_str());

    HWND hBrowse = makeButton(hwnd, "Browse...", IDC_BROWSE_BTN, 510, 499, 110, 28);
    SendMessage(hBrowse, WM_SETFONT, reinterpret_cast<WPARAM>(g_fontNormal), TRUE);

    // Progress bars
    HWND hOvLabel = makeLabel(hwnd, "Overall:", 20, 540, 55, 16);
    SendMessage(hOvLabel, WM_SETFONT, reinterpret_cast<WPARAM>(g_fontNormal), TRUE);

    g_progressBar = CreateWindowExA(0, PROGRESS_CLASSA, "",
        WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
        80, 538, 560, 18, hwnd,
        reinterpret_cast<HMENU>(IDC_PROGRESS),
        GetModuleHandleA(nullptr), nullptr);
    SendMessage(g_progressBar, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
    SendMessage(g_progressBar, PBM_SETBKCOLOR, 0, static_cast<LPARAM>(CLR_PANEL));
    SendMessage(g_progressBar, PBM_SETBARCOLOR, 0, static_cast<LPARAM>(CLR_ACCENT));

    HWND hFLabel = makeLabel(hwnd, "File:", 20, 565, 55, 16);
    SendMessage(hFLabel, WM_SETFONT, reinterpret_cast<WPARAM>(g_fontNormal), TRUE);

    g_fileProgress = CreateWindowExA(0, PROGRESS_CLASSA, "",
        WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
        80, 563, 560, 18, hwnd,
        reinterpret_cast<HMENU>(IDC_FILE_PROGRESS),
        GetModuleHandleA(nullptr), nullptr);
    SendMessage(g_fileProgress, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
    SendMessage(g_fileProgress, PBM_SETBKCOLOR, 0, static_cast<LPARAM>(CLR_PANEL));
    SendMessage(g_fileProgress, PBM_SETBARCOLOR, 0, static_cast<LPARAM>(CLR_ACCENT2));

    // Status labels
    g_statusLabel = makeLabel(hwnd,
        "Enter an identifier above and click Fetch Files.",
        20, 590, 560, 18);
    SendMessage(g_statusLabel, WM_SETFONT,
        reinterpret_cast<WPARAM>(g_fontNormal), TRUE);

    g_etaLabel = makeLabel(hwnd, "", 20, 610, 620, 16);
    SendMessage(g_etaLabel, WM_SETFONT,
        reinterpret_cast<WPARAM>(g_fontNormal), TRUE);

    // Action buttons
    g_downloadBtn = makeButton(hwnd, "> Start Download",
        IDC_DOWNLOAD_BTN, 20, 635, 180, 34);
    g_cancelBtn = makeButton(hwnd, "[ Cancel ]",
        IDC_CANCEL_BTN, 210, 635, 120, 34);
    SendMessage(g_downloadBtn, WM_SETFONT,
        reinterpret_cast<WPARAM>(g_fontNormal), TRUE);
    SendMessage(g_cancelBtn, WM_SETFONT,
        reinterpret_cast<WPARAM>(g_fontNormal), TRUE);
    EnableWindow(g_cancelBtn, FALSE);
}

// ---------------------------------------------------------------------------
// WM_COMMAND -- button clicks and edit control notifications
// ---------------------------------------------------------------------------
static LRESULT onCommand(HWND hwnd, WPARAM wParam) {
    int id = LOWORD(wParam);
    int code = HIWORD(wParam);

    if (id == IDC_FILTER_EDIT && code == EN_CHANGE) {
        char buf[256] = {};
        GetWindowTextA(g_filterEdit, buf, 256);
        std::lock_guard<std::mutex> lock(g_mutex);
        populateListView(g_allFiles, buf);
        int n = ListView_GetItemCount(g_listView);
        for (int i = 0; i < n; i++) ListView_SetCheckState(g_listView, i, TRUE);
        updateTotalSizeLabel();
        return 0;
    }

    if (id == IDC_FETCH_BTN) {
        if (g_fetching || g_downloading) return 0;
        char buf[512] = {};
        GetWindowTextA(g_identEdit, buf, 512);
        std::string ident(buf);
        if (ident.empty()) {
            SetWindowTextA(g_statusLabel,
                "Please enter an archive.org identifier first.");
            return 0;
        }
        EnableWindow(g_fetchBtn, FALSE);
        ListView_DeleteAllItems(g_listView);
        SetWindowTextA(g_totalSizeLabel, "");
        std::thread(fetchThread, ident).detach();
        return 0;
    }

    if (id == IDC_SELECT_ALL) {
        int n = ListView_GetItemCount(g_listView);
        for (int i = 0; i < n; i++) ListView_SetCheckState(g_listView, i, TRUE);
        updateTotalSizeLabel();
        return 0;
    }

    if (id == IDC_SELECT_NONE) {
        int n = ListView_GetItemCount(g_listView);
        for (int i = 0; i < n; i++) ListView_SetCheckState(g_listView, i, FALSE);
        updateTotalSizeLabel();
        return 0;
    }

    if (id == IDC_BROWSE_BTN) {
        BROWSEINFOA bi = {};
        bi.hwndOwner = hwnd;
        bi.lpszTitle = "Select Download Folder";
        bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
        PIDLIST_ABSOLUTE pidl = SHBrowseForFolderA(&bi);
        if (pidl) {
            char path[MAX_PATH] = {};
            SHGetPathFromIDListA(pidl, path);
            SetWindowTextA(g_destEdit, path);
            CoTaskMemFree(pidl);
        }
        return 0;
    }

    if (id == IDC_CANCEL_BTN) {
        g_cancelDownload = true;
        SetWindowTextA(g_cancelBtn, "Cancelling...");
        EnableWindow(g_cancelBtn, FALSE);
        return 0;
    }

    if (id == IDC_DOWNLOAD_BTN) {
        if (g_downloading || g_fetching) return 0;

        // Collect checked files into a DownloadTask
        auto* task = new DownloadTask();
        int n = ListView_GetItemCount(g_listView);
        for (int i = 0; i < n; i++) {
            if (!ListView_GetCheckState(g_listView, i)) continue;
            LVITEMA lvi = {};
            lvi.mask = LVIF_PARAM;
            lvi.iItem = i;
            ListView_GetItem(g_listView, &lvi);
            int origIdx = static_cast<int>(lvi.lParam);
            std::lock_guard<std::mutex> lock(g_mutex);
            if (origIdx >= 0 && origIdx < static_cast<int>(g_allFiles.size()))
                task->files.push_back(g_allFiles[origIdx]);
        }

        if (task->files.empty()) {
            SetWindowTextA(g_statusLabel,
                "No files selected. Check some boxes first.");
            delete task;
            return 0;
        }

        char destBuf[MAX_PATH] = {};
        GetWindowTextA(g_destEdit, destBuf, MAX_PATH);
        task->destDir = destBuf;
        if (task->destDir.empty()) {
            SetWindowTextA(g_statusLabel,
                "Please select a download destination.");
            delete task;
            return 0;
        }
        CreateDirectoryA(task->destDir.c_str(), nullptr);

        EnableWindow(g_downloadBtn, FALSE);
        EnableWindow(g_cancelBtn, TRUE);
        EnableWindow(g_fetchBtn, FALSE);
        SetWindowTextA(g_cancelBtn, "[ Cancel ]");
        SendMessage(g_progressBar, PBM_SETPOS, 0, 0);
        SendMessage(g_fileProgress, PBM_SETPOS, 0, 0);

        std::thread(downloadThread, task).detach();
        return 0;
    }

    return 0;
}

// ---------------------------------------------------------------------------
// Main window procedure
// ---------------------------------------------------------------------------
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {

    case WM_CREATE:
        onCreate(hwnd);
        return 0;

        // Dark theme: color static labels, edits, and buttons
    case WM_CTLCOLORSTATIC: {
        HDC hdc = reinterpret_cast<HDC>(wParam);
        SetBkColor(hdc, CLR_BG);
        SetTextColor(hdc, CLR_TEXT);
        return reinterpret_cast<LRESULT>(g_bgBrush);
    }

    case WM_CTLCOLOREDIT: {
        HDC hdc = reinterpret_cast<HDC>(wParam);
        SetBkColor(hdc, CLR_PANEL);
        SetTextColor(hdc, CLR_TEXT);
        static HBRUSH panelBrush = CreateSolidBrush(CLR_PANEL);
        return reinterpret_cast<LRESULT>(panelBrush);
    }

    case WM_CTLCOLORBTN: {
        HDC hdc = reinterpret_cast<HDC>(wParam);
        SetBkColor(hdc, CLR_PANEL);
        SetTextColor(hdc, CLR_ACCENT);
        static HBRUSH panelBrush = CreateSolidBrush(CLR_PANEL);
        return reinterpret_cast<LRESULT>(panelBrush);
    }

    case WM_ERASEBKGND: {
        RECT rc;
        GetClientRect(hwnd, &rc);
        FillRect(reinterpret_cast<HDC>(wParam), &rc, g_bgBrush);
        return 1;
    }

                      // Posted from worker threads -- update status label safely on UI thread
    case WM_UPDATE_STATUS: {
        auto* msg_str = reinterpret_cast<std::string*>(lParam);
        HWND target = g_statusLabel;
        if (msg_str && msg_str->find("ETA:") == 0) target = g_etaLabel;
        if (msg_str) {
            SetWindowTextA(target, msg_str->c_str());
            delete msg_str;
        }
        InvalidateRect(target, nullptr, TRUE);
        return 0;
    }

    case WM_UPDATE_PROGRESS:
        SendMessage(g_progressBar, PBM_SETPOS, static_cast<int>(wParam), 0);
        return 0;

    case WM_FILE_DONE:
        if (lParam == 1)
            SendMessage(g_fileProgress, PBM_SETPOS, static_cast<int>(wParam), 0);
        return 0;

    case WM_FETCH_DONE:
        EnableWindow(g_fetchBtn, TRUE);
        if (wParam > 0) {
            std::lock_guard<std::mutex> lock(g_mutex);
            populateListView(g_allFiles);
            int n = ListView_GetItemCount(g_listView);
            for (int i = 0; i < n; i++)
                ListView_SetCheckState(g_listView, i, TRUE);
            updateTotalSizeLabel();
        }
        return 0;

    case WM_DOWNLOAD_DONE:
        EnableWindow(g_downloadBtn, TRUE);
        EnableWindow(g_cancelBtn, FALSE);
        EnableWindow(g_fetchBtn, TRUE);
        return 0;

    case WM_NOTIFY: {
        auto* pnm = reinterpret_cast<LPNMHDR>(lParam);
        if (pnm->idFrom == IDC_FILE_LIST && pnm->code == LVN_ITEMCHANGED)
            updateTotalSizeLabel();
        return 0;
    }

    case WM_COMMAND:
        return onCommand(hwnd, wParam);

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcA(hwnd, msg, wParam, lParam);
}
