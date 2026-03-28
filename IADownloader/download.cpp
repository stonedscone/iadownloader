#include "download.h"
#include "globals.h"
#include "fetch.h"

#include <wininet.h>
#include <sstream>
#include <iomanip>
#include <vector>

#pragma comment(lib, "wininet.lib")

// ---------------------------------------------------------------------------
// Formats seconds into human readable ETA
// ---------------------------------------------------------------------------
std::string FormatETA(double seconds) {
    if (seconds < 0) return "Calculating...";
    int s = (int)seconds;
    std::ostringstream o;
    if (s < 60) { o << s << "s"; return o.str(); }
    if (s < 3600) { o << s / 60 << "m " << s % 60 << "s"; return o.str(); }
    o << s / 3600 << "h " << (s % 3600) / 60 << "m";
    return o.str();
}

// ---------------------------------------------------------------------------
// Download thread
// ---------------------------------------------------------------------------
void DownloadThread(DownloadTask* task) {
    try {
        g_downloading = true;
        g_cancelDownload = false;

        auto& files = task->files;
        auto& destDir = task->destDir;

        // Calculate total bytes for overall progress
        long long totalBytes = 0;
        for (auto& f : files)
            if (f.size > 0) totalBytes += f.size;

        long long downloadedTotal = 0;
        ULONGLONG startTime = GetTickCount64();

        for (size_t i = 0; i < files.size(); i++) {

            // Check cancel
            if (g_cancelDownload) break;

            auto& f = files[i];

            // Update status
            std::ostringstream statusMsg;
            statusMsg << "[" << (i + 1) << "/" << files.size()
                << "] Downloading: " << f.name;
            PostMessage(g_hwnd, WM_UPDATE_STATUS,
                (WPARAM)statusMsg.str().c_str(), 0);

            // Build destination path
            std::string destPath = destDir + "\\" + f.name;

            // Handle subdirectories in filename
            size_t slashPos = f.name.rfind('/');
            if (slashPos == std::string::npos)
                slashPos = f.name.rfind('\\');
            if (slashPos != std::string::npos) {
                std::string subDir = destDir + "\\" +
                    f.name.substr(0, slashPos);
                CreateDirectoryA(subDir.c_str(), nullptr);
            }

            // Skip if already exists at correct size
            WIN32_FILE_ATTRIBUTE_DATA fa{};
            if (GetFileAttributesExA(destPath.c_str(),
                GetFileExInfoStandard, &fa)) {
                LARGE_INTEGER existing{};
                existing.LowPart = fa.nFileSizeLow;
                existing.HighPart = fa.nFileSizeHigh;
                if (f.size > 0 && existing.QuadPart == f.size) {
                    downloadedTotal += f.size;
                    continue;
                }
            }

            // Open HTTP connection
            HINTERNET hInet = InternetOpenA("IADownloader/1.0",
                INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);
            if (!hInet) continue;

            HINTERNET hUrl = InternetOpenUrlA(hInet, f.url.c_str(),
                nullptr, 0,
                INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_RELOAD, 0);
            if (!hUrl) {
                InternetCloseHandle(hInet);
                continue;
            }

            // Create destination file
            HANDLE hFile = CreateFileA(destPath.c_str(), GENERIC_WRITE,
                0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (hFile == INVALID_HANDLE_VALUE) {
                InternetCloseHandle(hUrl);
                InternetCloseHandle(hInet);
                continue;
            }

            // Stream chunks to disk
            std::vector<char> buf(65536);
            DWORD     read = 0;
            long long fileDownloaded = 0;

            while (true) {

                // Handle pause -- just wait until unpaused
                while (g_pauseDownload && !g_cancelDownload)
                    Sleep(100);

                // Handle cancel
                if (g_cancelDownload) break;

                if (!InternetReadFile(hUrl, buf.data(),
                    (DWORD)buf.size(), &read)) break;
                if (read == 0) break;

                DWORD written = 0;
                WriteFile(hFile, buf.data(), read, &written, nullptr);
                fileDownloaded += read;
                downloadedTotal += read;

                // Per file progress
                int filePct = (f.size > 0)
                    ? (int)((fileDownloaded * 100) / f.size)
                    : 50;
                PostMessage(g_hwnd, WM_FILE_DONE,
                    (WPARAM)filePct, 0);

                // Overall progress
                int ovPct = (totalBytes > 0)
                    ? (int)((downloadedTotal * 100) / totalBytes)
                    : 0;
                PostMessage(g_hwnd, WM_UPDATE_PROGRESS,
                    (WPARAM)ovPct, 0);

                // Speed and ETA
                ULONGLONG elapsed = GetTickCount64() - startTime;
                if (elapsed > 500 && downloadedTotal > 0) {
                    double rate = downloadedTotal / (elapsed / 1000.0);
                    long long remain = totalBytes - downloadedTotal;
                    double eta = (rate > 0 && remain > 0)
                        ? remain / rate : -1.0;

                    std::ostringstream etaStr;
                    etaStr << "Speed: " << FormatSize((long long)rate)
                        << "/s   ETA: " << FormatETA(eta);
                    PostMessage(g_hwnd, WM_UPDATE_ETA,
                        (WPARAM)etaStr.str().c_str(), 0);
                }
            }

            CloseHandle(hFile);
            InternetCloseHandle(hUrl);
            InternetCloseHandle(hInet);

            // Delete partial file if cancelled
            if (g_cancelDownload)
                DeleteFileA(destPath.c_str());
        }

        delete task;
        g_downloading = false;

        if (g_cancelDownload)
            PostMessage(g_hwnd, WM_UPDATE_STATUS,
                (WPARAM)"Download cancelled.", 0);
        else
            PostMessage(g_hwnd, WM_UPDATE_STATUS,
                (WPARAM)"All downloads complete!", 0);

        PostMessage(g_hwnd, WM_DOWNLOAD_DONE, 0, 0);

    }
    catch (...) {
        g_downloading = false;
        PostMessage(g_hwnd, WM_UPDATE_STATUS,
            (WPARAM)"Unknown error during download.", 0);
        PostMessage(g_hwnd, WM_DOWNLOAD_DONE, 0, 0);
        delete task;
    }
}