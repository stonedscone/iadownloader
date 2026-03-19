#include "fetch.h"
#include "globals.h"

#include <wininet.h>
#include <sstream>

#pragma comment(lib, "wininet.lib")

// ---------------------------------------------------------------------------
// HTTP GET via WinInet
// ---------------------------------------------------------------------------
std::string httpGet(const std::string& url, bool& cancel) {
    HINTERNET hInet = InternetOpenA("IADownloader/1.0",
        INTERNET_OPEN_TYPE_PRECONFIG,
        nullptr, nullptr, 0);
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

    while (!cancel) {
        if (!InternetReadFile(hUrl, buf, sizeof(buf), &read)) break;
        if (read == 0) break;
        result.append(buf, read);
    }

    InternetCloseHandle(hUrl);
    InternetCloseHandle(hInet);
    return result;
}

// ---------------------------------------------------------------------------
// Minimal JSON field extractor -- no library needed for flat objects
// ---------------------------------------------------------------------------
std::string jsonGetStr(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\":\"";
    auto pos = json.find(search);
    if (pos == std::string::npos) return "";
    pos += search.size();
    auto end = json.find('"', pos);
    if (end == std::string::npos) return "";
    return json.substr(pos, end - pos);
}

// ---------------------------------------------------------------------------
// Parse the IA metadata JSON blob into a list of FileEntry structs
// ---------------------------------------------------------------------------
std::vector<FileEntry> parseIAMetadata(const std::string& json,
    const std::string& identifier) {
    std::vector<FileEntry> files;

    auto filesPos = json.find("\"files\"");
    if (filesPos == std::string::npos) return files;

    size_t pos = filesPos;
    while (true) {
        auto objStart = json.find('{', pos);
        if (objStart == std::string::npos) break;
        auto objEnd = json.find('}', objStart);
        if (objEnd == std::string::npos) break;

        std::string obj = json.substr(objStart, objEnd - objStart + 1);
        std::string name = jsonGetStr(obj, "name");
        std::string sizeS = jsonGetStr(obj, "size");

        if (!name.empty()) {
            FileEntry fe;
            fe.name = name;
            fe.url = "https://archive.org/download/" + identifier + "/" + name;
            fe.size = sizeS.empty() ? -1 : static_cast<long long>(std::stoll(sizeS));
            fe.sizeStr = formatSize(fe.size);
            files.push_back(fe);
        }
        pos = objEnd + 1;
    }
    return files;
}

// ---------------------------------------------------------------------------
// Fetch thread -- runs off the UI thread, posts results back via messages
// ---------------------------------------------------------------------------
void fetchThread(std::string identifier) {
    g_fetching = true;
    setStatus("Fetching file list from Internet Archive...", CLR_ACCENT);

    // Trim whitespace from identifier
    while (!identifier.empty() &&
        (identifier.front() == ' ' || identifier.front() == '\t' ||
            identifier.front() == '\r' || identifier.front() == '\n'))
        identifier.erase(identifier.begin());

    while (!identifier.empty() &&
        (identifier.back() == ' ' || identifier.back() == '\t' ||
            identifier.back() == '\r' || identifier.back() == '\n'))
        identifier.pop_back();

    bool cancel = false;
    std::string json = httpGet(
        "https://archive.org/metadata/" + identifier, cancel);

    if (json.empty()) {
        setStatus(
            "Error: Could not reach archive.org. "
            "Check your connection or identifier.", CLR_ERROR);
        g_fetching = false;
        PostMessage(g_hwnd, WM_FETCH_DONE, 0, 0);
        return;
    }

    auto files = parseIAMetadata(json, identifier);
    if (files.empty()) {
        setStatus("No files found. Check your identifier and try again.",
            CLR_ERROR);
        g_fetching = false;
        PostMessage(g_hwnd, WM_FETCH_DONE, 0, 0);
        return;
    }

    {
        std::lock_guard<std::mutex> lock(g_mutex);
        g_allFiles = files;
    }

    std::ostringstream msg;
    msg << "Found " << files.size()
        << " files. Select what you want to download.";
    setStatus(msg.str(), CLR_SUCCESS);

    g_fetching = false;
    PostMessage(g_hwnd, WM_FETCH_DONE, static_cast<WPARAM>(files.size()), 0);
}
