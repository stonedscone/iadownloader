#pragma once
#include "globals.h"
#include <string>
#include <vector>

// Custom messages posted back to the main window from the fetch thread
#define WM_FETCH_DONE      (WM_USER + 1)
#define WM_UPDATE_STATUS   (WM_USER + 2)
#define WM_UPDATE_PROGRESS (WM_USER + 3)


// Represents one file in an archive.org collection
struct FileEntry {
    std::string name;
    std::string url;
    long long   size = -1;
    std::string sizeStr;
};

// The shared file list -- populated by fetch thread, read by download thread
extern std::vector<FileEntry> g_files;

// Formats a byte count into a human readable string e.g. "1.2 GB"
std::string FormatSize(long long bytes);

// Entry point for the fetch background thread
void FetchThread(std::string identifier);