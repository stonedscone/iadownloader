#pragma once
#include "globals.h"
#include "fetch.h"
#include <string>
#include <vector>

// Custom message posted when all downloads are complete
#define WM_DOWNLOAD_DONE    (WM_USER + 4)
#define WM_FILE_DONE        (WM_USER + 5)

// Bundles everything the download thread needs
struct DownloadTask {
    std::vector<FileEntry> files;
    std::string            destDir;
};

// Formats seconds into human readable ETA e.g. "2m 30s"
std::string FormatETA(double seconds);

// Entry point for the download background thread
void DownloadThread(DownloadTask* task);