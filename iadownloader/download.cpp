#include "download.h"
#include "globals.h"

#include <wininet.h>
#include <sstream>
#include <vector>

#pragma comment(lib, "wininet.lib")

// ---------------------------------------------------------------------------
// Download thread
// Loops through each selected file, streams it from archive.org to disk,
// and posts progress messages back to the main window.
// ---------------------------------------------------------------------------
void downloadThread(DownloadTask* task) {
    g_downloading = true;
    g_cancelDownload = false;

    auto& files = task->files;
    auto& destDir = task->destDir;

    // Calculate total expected bytes for overall progress bar
    long long totalBytes = 0;
    for (auto& f : files)
        if (f.size > 0) totalBytes += f.size;

    long long downloadedTotal = 0;
    ULONGLONG startTime = GetTickCount64();

    for (size_t i = 0; i < files.size() && !g_cancelDownload; ++i) {
        auto& f = files[i];

        // Update status label with current file
        std::ostringstream statusMsg;
        statusMsg << "[" << (i + 1) << "/" << files.size()
            << "] Downloading: " << f.name;
        setStatus(statusMsg.str(), CLR_ACCENT);

        // Update overall progress bar
        int overallPct = totalBytes > 0
            ? static_cast<int>((downloadedTotal * 100) / totalBytes)
            : static_cast<int>(i * 100 / files.size());
        PostMessage(g_hwnd, WM_UPDATE_PROGRESS,
            static_cast<WPARAM>(overallPct), 0);

        // Build full destination path, creating subdirs if needed
        std::string destPath = destDir + "\\" + f.name;
        size_t slashPos = f.name.rfind('/');
        if (slashPos == std::string::npos) slashPos = f.name.rfind('\\');
        if (slashPos != std::string::npos) {
            std::string subDir = destDir + "\\" + f.name.substr(0, slashPos);
            CreateDirectoryA(subDir.c_str(), nullptr);
        }

        // Skip if file already exists at the correct size (resume support)
        WIN32_FILE_ATTRIBUTE_DATA fa{};
        if (GetFileAttributesExA(destPath.c_str(), GetFileExInfoStandard, &fa)) {
            LARGE_INTEGER existing{};
            existing.LowPart = fa.nFileSizeLow;
            existing.HighPart = fa.nFileSizeHigh;
            if (f.size > 0 && existing.QuadPart == f.size) {
                downloadedTotal += f.size;
                setStatus(statusMsg.str() + " (already exists, skipped)",
                    CLR_MUTED);
                PostMessage(g_hwnd, WM_FILE_DONE, 1, 0);
                continue;
            }
        }

        // Open HTTP connection to archive.org
        HINTERNET hInet = InternetOpenA("IADownloader/1.0",
            INTERNET_OPEN_TYPE_PRECONFIG,
            nullptr, nullptr, 0);
        if (!hInet) {
            setStatus("Failed to open internet handle.", CLR_ERROR);
            continue;
        }

        HINTERNET hUrl = InternetOpenUrlA(hInet, f.url.c_str(), nullptr, 0,
            INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_RELOAD, 0);
        if (!hUrl) {
            InternetCloseHandle(hInet);
            setStatus("Failed to open URL: " + f.name, CLR_ERROR);
            continue;
        }

        // Create the destination file on disk
        HANDLE hFile = CreateFileA(destPath.c_str(), GENERIC_WRITE, 0,
            nullptr, CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL, nullptr);
        if (hFile == INVALID_HANDLE_VALUE) {
            InternetCloseHandle(hUrl);
            InternetCloseHandle(hInet);
            setStatus("Cannot create file: " + destPath, CLR_ERROR);
            continue;
        }

        // Stream data: read chunk -> write to disk -> update progress
        std::vector<char> buf(65536);
        DWORD     read = 0;
        long long fileDownloaded = 0;
        bool      fileFailed = false;

        while (!g_cancelDownload) {
            if (!InternetReadFile(hUrl, buf.data(),
                static_cast<DWORD>(buf.size()), &read)) {
                fileFailed = true;
                break;
            }
            if (read == 0) break;

            DWORD written = 0;
            WriteFile(hFile, buf.data(), read, &written, nullptr);
            fileDownloaded += read;
            downloadedTotal += read;

            // Per-file progress bar
            int filePct = (f.size > 0)
                ? static_cast<int>((fileDownloaded * 100) / f.size)
                : 50;
            PostMessage(g_hwnd, WM_FILE_DONE,
                static_cast<WPARAM>(filePct), 1);

            // Overall progress bar
            int ovPct = (totalBytes > 0)
                ? static_cast<int>((downloadedTotal * 100) / totalBytes)
                : overallPct;
            PostMessage(g_hwnd, WM_UPDATE_PROGRESS,
                static_cast<WPARAM>(ovPct), 0);

            // Speed and ETA -- only calculate after 500ms to avoid noise
            ULONGLONG elapsed = GetTickCount64() - startTime;
            if (downloadedTotal > 0 && elapsed > 500) {
                double rate = downloadedTotal / (elapsed / 1000.0);
                long long remain = totalBytes - downloadedTotal;
                double eta = (rate > 0 && remain > 0)
                    ? remain / rate : -1.0;

                std::string etaStr = "ETA: " + formatETA(eta) +
                    "  Speed: " +
                    formatSize(static_cast<long long>(rate)) +
                    "/s";
                PostMessage(g_hwnd, WM_UPDATE_STATUS,
                    static_cast<WPARAM>(CLR_MUTED),
                    reinterpret_cast<LPARAM>(new std::string(etaStr)));
            }
        }

        CloseHandle(hFile);
        InternetCloseHandle(hUrl);
        InternetCloseHandle(hInet);

        if (g_cancelDownload) {
            // Clean up incomplete file so it doesn't look like a valid download
            DeleteFileA(destPath.c_str());
        }
        else if (!fileFailed) {
            PostMessage(g_hwnd, WM_FILE_DONE, 100, 1);
        }
    }

    delete task;

    if (g_cancelDownload) {
        setStatus("Download cancelled.", CLR_ACCENT2);
    }
    else {
        setStatus("All downloads complete!", CLR_SUCCESS);
        PostMessage(g_hwnd, WM_UPDATE_PROGRESS, 100, 0);
    }

    g_downloading = false;
    PostMessage(g_hwnd, WM_DOWNLOAD_DONE, 0, 0);
}
