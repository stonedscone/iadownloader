# Internet Archive Downloader

A simple desktop tool for batch downloading files from [archive.org](https://archive.org).

I built this because I wanted to download a full anime series off Internet Archive and the built-in "zip on the fly" feature couldn't handle the file size. So I had to either download 291 files one by one, or build something to do it for me.

---

## What it does

- Paste in any archive.org collection identifier and it pulls the full file list automatically
- Check the files you want, filter by name, see the total download size before you start
- Tracks overall progress, per-file progress, download speed, and estimated time remaining
- Skips files you already downloaded so you can safely re-run it without re-downloading everything
- Cancel at any time — partial files get cleaned up automatically

No installation needed. Single `.exe`, just run it.

---

## How to use it

1. Go to archive.org and find the collection you want
2. Copy the identifier from the URL — it's the part after `/details/`
   - Example: `https://archive.org/details/dragon-ball-z-episodes` → identifier is `dragon-ball-z-episodes`
3. Paste it into the top field and click **Fetch Files**
4. Check the files you want to download
5. Pick a destination folder and click **Start Download**

---

## Building from source

You'll need **Visual Studio** (Community edition is free) with the **Desktop development with C++** workload installed.

Open a **Developer Command Prompt for VS** (search for it in the Start Menu — do not use a regular terminal) and run:

```
cl /std:c++17 /O2 /EHsc main.cpp ui.cpp fetch.cpp download.cpp /link /SUBSYSTEM:WINDOWS user32.lib gdi32.lib wininet.lib comctl32.lib shell32.lib ole32.lib
```

That's it. You'll get `main.exe` in the same folder.

### Project structure

```
main.cpp        Entry point, shared utilities
globals.h       Structs, constants, shared state
ui.h / ui.cpp   Window, controls, all UI logic
fetch.h / fetch.cpp       Archive.org API + JSON parsing
download.h / download.cpp Download thread + progress tracking
```

---

## Things that could be improved

- Downloading multiple files at the same time
- Retry logic if a file fails mid-download
- Pause and resume support
- Better JSON parsing (currently just scans the string manually)
- Remember the last used destination folder

Pull requests are welcome if you want to take a crack at any of these.
