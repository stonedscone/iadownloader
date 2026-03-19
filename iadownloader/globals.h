#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include <vector>
#include <atomic>
#include <mutex>

// ---------------------------------------------------------------------------
// Custom window messages (posted from worker threads to the UI thread)
// ---------------------------------------------------------------------------
#define WM_UPDATE_STATUS    (WM_USER + 1)
#define WM_UPDATE_PROGRESS  (WM_USER + 2)
#define WM_FETCH_DONE       (WM_USER + 3)
#define WM_DOWNLOAD_DONE    (WM_USER + 4)
#define WM_FILE_DONE        (WM_USER + 5)

// ---------------------------------------------------------------------------
// Control IDs
// ---------------------------------------------------------------------------
#define IDC_IDENTIFIER_EDIT     102
#define IDC_FETCH_BTN           103
#define IDC_FILE_LIST           104
#define IDC_SELECT_ALL          105
#define IDC_SELECT_NONE         106
#define IDC_DEST_EDIT           108
#define IDC_BROWSE_BTN          109
#define IDC_DOWNLOAD_BTN        110
#define IDC_CANCEL_BTN          111
#define IDC_PROGRESS            112
#define IDC_STATUS_LABEL        113
#define IDC_TOTAL_SIZE_LABEL    114
#define IDC_ETA_LABEL           115
#define IDC_FILE_PROGRESS       116
#define IDC_FILTER_EDIT         119

// ---------------------------------------------------------------------------
// Colors
// ---------------------------------------------------------------------------
const COLORREF CLR_BG = RGB(15, 17, 23);
const COLORREF CLR_PANEL = RGB(22, 26, 36);
const COLORREF CLR_ACCENT = RGB(82, 196, 255);
const COLORREF CLR_ACCENT2 = RGB(255, 160, 60);
const COLORREF CLR_TEXT = RGB(220, 230, 245);
const COLORREF CLR_MUTED = RGB(100, 115, 140);
const COLORREF CLR_SUCCESS = RGB(80, 220, 140);
const COLORREF CLR_ERROR = RGB(255, 90, 90);

// ---------------------------------------------------------------------------
// Represents one file in an Internet Archive collection
// ---------------------------------------------------------------------------
struct FileEntry {
    std::string name;
    std::string url;
    long long   size = -1;   // bytes; -1 = unknown
    std::string sizeStr;        // human-readable e.g. "1.2 GB"
};

// ---------------------------------------------------------------------------
// Passed to the download thread
// ---------------------------------------------------------------------------
struct DownloadTask {
    std::vector<FileEntry> files;
    std::string            destDir;
};

// ---------------------------------------------------------------------------
// Shared state (defined in main.cpp, used across all translation units)
// ---------------------------------------------------------------------------

// Window handles
extern HWND g_hwnd;
extern HWND g_listView;
extern HWND g_destEdit;
extern HWND g_statusLabel;
extern HWND g_progressBar;
extern HWND g_fileProgress;
extern HWND g_totalSizeLabel;
extern HWND g_etaLabel;
extern HWND g_downloadBtn;
extern HWND g_cancelBtn;
extern HWND g_fetchBtn;
extern HWND g_identEdit;
extern HWND g_filterEdit;

// Fonts / brushes
extern HFONT  g_fontTitle;
extern HFONT  g_fontNormal;
extern HFONT  g_fontMono;
extern HBRUSH g_bgBrush;

// Thread-safe state flags
extern std::atomic<bool> g_cancelDownload;
extern std::atomic<bool> g_downloading;
extern std::atomic<bool> g_fetching;

// Shared file list (protected by g_mutex)
extern std::vector<FileEntry> g_allFiles;
extern std::mutex             g_mutex;

// ---------------------------------------------------------------------------
// Utility functions used across multiple files
// ---------------------------------------------------------------------------
std::string formatSize(long long bytes);
std::string formatETA(double seconds);

// Posts a status string to the UI thread (safe to call from any thread)
void setStatus(const std::string& msg, COLORREF color = CLR_TEXT);
