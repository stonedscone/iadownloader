#pragma once
#include "globals.h"

// ---------------------------------------------------------------------------
// Download thread entry point.
// Takes ownership of the DownloadTask pointer and deletes it when done.
// Communicates progress back to the UI thread via PostMessage.
// ---------------------------------------------------------------------------



void downloadThread(DownloadTask* task);
