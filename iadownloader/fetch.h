#pragma once
#include <string>
#include <vector>
#include "globals.h"

// ---------------------------------------------------------------------------
// Hits the Internet Archive Metadata API and parses the response.
// Called on a background thread -- communicates back via PostMessage.
// ---------------------------------------------------------------------------
void fetchThread(std::string identifier);


// Performs a blocking HTTP GET and returns the full response body.
// Sets cancel=true externally to abort mid-transfer.
std::string httpGet(const std::string& url, bool& cancel);

// Pulls the string value of a key from a flat JSON object string.
// e.g. jsonGetStr("{\"name\":\"foo\"}", "name") -> "foo"
std::string jsonGetStr(const std::string& json, const std::string& key);

// Walks the "files" array in an IA metadata JSON blob and builds FileEntry list.
std::vector<FileEntry> parseIAMetadata(const std::string& json,
    const std::string& identifier);
