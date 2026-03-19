#pragma once
#include "globals.h"

// ---------------------------------------------------------------------------
// Main window procedure -- handles all Win32 messages
// ---------------------------------------------------------------------------
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);


// Clears and repopulates the file list. Pass an empty filter to show all.
void populateListView(const std::vector<FileEntry>& files,
    const std::string& filter = "");

// Recalculates total selected size and updates the label.
void updateTotalSizeLabel();

// ---------------------------------------------------------------------------
// Window creation helpers -- thin wrappers around CreateWindowExA
// ---------------------------------------------------------------------------
HWND makeLabel(HWND parent, const char* text,
    int x, int y, int w, int h);

HWND makeEdit(HWND parent, int id,
    int x, int y, int w, int h, DWORD extra = 0);

HWND makeButton(HWND parent, const char* text, int id,
    int x, int y, int w, int h);
