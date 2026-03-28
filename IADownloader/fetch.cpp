#include "fetch.h"
#include "globals.h"
#include <wininet.h>
#include <sstream>
#include <iomanip>

#pragma comment(lib, "wininet.lib")

// ---------------------------------------------------------------------------
// Formats bytes into human readable string
// ---------------------------------------------------------------------------
std::string FormatSize(long long bytes) {
    if (bytes < 0) return "Unknown";
    std::ostringstream o;
    if (bytes < 1024) {
        o << bytes << " B";
        return o.str();
    }
    double kb = bytes / 1024.0;
    if (kb < 1024) {
        o << std::fixed << std::setprecision(1) << kb << " KB";
        return o.str();
    }
    double mb = kb / 1024.0;
    if (mb < 1024) {
        o << std::fixed << std::setprecision(1) << mb << " MB";
        return o.str();
    }
    double gb = mb / 1024.0;
    o << std::fixed << std::setprecision(2) << gb << " GB";
    return o.str();
}

// ---------------------------------------------------------------------------
// Performs a blocking HTTP GET and returns the response body
// ---------------------------------------------------------------------------
static std::string HttpGet(const std::string& url) {
    HINTERNET hInet = InternetOpenA("IADownloader/1.0",
        INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);
    if (!hInet) return "";

    HINTERNET hUrl = InternetOpenUrlA(hInet, url.c_str(), nullptr, 0,
        INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_RELOAD |
        INTERNET_FLAG_SECURE |
        INTERNET_FLAG_IGNORE_CERT_CN_INVALID |
        INTERNET_FLAG_IGNORE_CERT_DATE_INVALID, 0);
    if (!hUrl) {
        InternetCloseHandle(hInet);
        return "";
    }

    std::string result;
    char buf[8192];
    DWORD read = 0;
    while (InternetReadFile(hUrl, buf, sizeof(buf), &read) && read > 0)
        result.append(buf, read);

    InternetCloseHandle(hUrl);
    InternetCloseHandle(hInet);
    return result;
}

// ---------------------------------------------------------------------------
// Pulls a string value out of a flat JSON object
// e.g. jsonGetStr("{\"name\":\"foo\"}", "name") -> "foo"
// ---------------------------------------------------------------------------
static std::string JsonGetStr(const std::string& json,
    const std::string& key) {
    std::string search = "\"" + key + "\":\"";
    auto pos = json.find(search);
    if (pos == std::string::npos) return "";
    pos += search.size();
    auto end = json.find('"', pos);
    if (end == std::string::npos) return "";
    return json.substr(pos, end - pos);
}

// ---------------------------------------------------------------------------
// Parses the IA metadata JSON and builds the file list
// ---------------------------------------------------------------------------
static void ParseMetadata(const std::string& json,
    const std::string& identifier) {
    g_files.clear();

    auto filesPos = json.find("\"files\"");
    if (filesPos == std::string::npos) return;

    size_t pos = filesPos;
    while (true) {
        auto objStart = json.find('{', pos);
        if (objStart == std::string::npos) break;
        auto objEnd = json.find('}', objStart);
        if (objEnd == std::string::npos) break;

        std::string obj = json.substr(objStart, objEnd - objStart + 1);
        std::string name = JsonGetStr(obj, "name");
        std::string sizeS = JsonGetStr(obj, "size");

        if (!name.empty()) {
            FileEntry fe;
            fe.name = name;
            fe.url = "https://archive.org/download/" +
                identifier + "/" + name;
            fe.size = sizeS.empty() ? -1 : std::stoll(sizeS);
            fe.sizeStr = FormatSize(fe.size);
            g_files.push_back(fe);
        }
        pos = objEnd + 1;
    }
}

// ---------------------------------------------------------------------------
// Fetch thread -- called from WM_COMMAND when Fetch Files is clicked
// ---------------------------------------------------------------------------
void FetchThread(std::string identifier) {
    try {
        g_fetching = true;

        // Update status label
        PostMessage(g_hwnd, WM_UPDATE_STATUS,
            (WPARAM)"Fetching file list from Internet Archive...", 0);

        // Trim whitespace from identifier
        while (!identifier.empty() &&
            (identifier.front() == ' ' || identifier.front() == '\t' ||
                identifier.front() == '\r' || identifier.front() == '\n'))
            identifier.erase(identifier.begin());
        while (!identifier.empty() &&
            (identifier.back() == ' ' || identifier.back() == '\t' ||
                identifier.back() == '\r' || identifier.back() == '\n'))
            identifier.pop_back();

        // Hit the IA metadata API
        std::string json = HttpGet(
            "https://archive.org/metadata/" + identifier);

        if (json.empty()) {
            PostMessage(g_hwnd, WM_UPDATE_STATUS,
                (WPARAM)"Error: Could not reach archive.org.", 0);
            g_fetching = false;
            PostMessage(g_hwnd, WM_FETCH_DONE, 0, 0);
            return;
        }

        ParseMetadata(json, identifier);

        if (g_files.empty()) {
            PostMessage(g_hwnd, WM_UPDATE_STATUS,
                (WPARAM)"No files found. Check your identifier.", 0);
            g_fetching = false;
            PostMessage(g_hwnd, WM_FETCH_DONE, 0, 0);
            return;
        }

        g_fetching = false;
        PostMessage(g_hwnd, WM_FETCH_DONE,
            (WPARAM)g_files.size(), 0);

    }
    catch (...) {
        PostMessage(g_hwnd, WM_UPDATE_STATUS,
            (WPARAM)"Unknown error during fetch.", 0);
        g_fetching = false;
        PostMessage(g_hwnd, WM_FETCH_DONE, 0, 0);
    }
}