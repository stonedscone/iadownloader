#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <string>
#include <vector>
#include <atomic>

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
#define IDC_HELP_BTN            120
#define IDC_PAUSE_BTN           121
#define FILTER_TIMER_ID        1001
#define FILTER_TIMER_DELAY      300

// ---------------------------------------------------------------------------
// Colors
// ---------------------------------------------------------------------------
#define CLR_BG      RGB(15,  17,  23)
#define CLR_PANEL   RGB(22,  26,  36)
#define CLR_ACCENT  RGB(82,  196, 255)
#define CLR_ACCENT2 RGB(255, 160, 60)
#define CLR_TEXT    RGB(220, 230, 245)
#define CLR_MUTED   RGB(100, 115, 140)
#define CLR_SUCCESS RGB(80,  220, 140)
#define CLR_ERROR   RGB(255, 90,  90)

// ---------------------------------------------------------------------------
// Shared window handles
// ---------------------------------------------------------------------------
extern HWND g_hwnd;
extern HWND g_hListView;
extern HWND g_hStatus;
extern HWND g_hEta;
extern HWND g_hProgress;
extern HWND g_hFileProgress;
extern HWND g_hTotalLabel;
extern HWND g_hDestEdit;
extern HWND g_hIdentEdit;
extern HWND g_hFilterEdit;
extern HWND g_hDownloadBtn;
extern HWND g_hCancelBtn;
extern HWND g_hFetchBtn;
extern HWND g_hPauseBtn;


// ---------------------------------------------------------------------------
// Shared fonts
// ---------------------------------------------------------------------------
extern HFONT g_fontTitle;
extern HFONT g_fontNormal;
extern HFONT g_fontMono;
extern HBRUSH g_bgBrush;



// ---------------------------------------------------------------------------
// Thread state flags
// ---------------------------------------------------------------------------
extern std::atomic<bool> g_fetching;
extern std::atomic<bool> g_downloading;
extern std::atomic<bool> g_cancelDownload;
extern std::atomic<bool> g_pauseDownload;