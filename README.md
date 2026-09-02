# FanControl Hotkey

**English** | [简体中文](README-Zh-CN.md)

A lightweight hotkey and process-aware profile switcher for [FanControl](https://getfancontrol.com/). Written in pure native C with Win32 API, zero external runtime dependencies, and full Per-Monitor V2 High-DPI support.

---

## ✨ Features

- 4 Fan Mode Presets: Silent / Normal / Performance / Turbo
- Custom Hotkey Bindings: Bind any key combination (e.g., `Ctrl+Alt+1`, `Shift+Win+F12`)
- Async Process-Aware Auto Switching: Dedicated background worker thread with O(1) FNV-1a hash matching of active processes (e.g., automatically switches to Performance when `Cyberpunk2077.exe` starts, reverts to Normal on exit)
- GUI Settings Window: Browse and configure FanControl executable path, profile JSONs, hotkeys, and process rules
- Transactional Settings Draft: In-memory draft buffer for settings; Cancel has zero side effects, OK applies changes atomically
- Security Hardened & Robust Storage:
  - Auto-start registry path strictly quoted (`\"%s\" %s`) and verified against the running binary path
  - Multi-tier INI config persistence: portable `.ini` next to `.exe` with automatic `%APPDATA%\FanControlHotkey\` fallback for restricted directories
  - External process launching via `CreateProcessW` with argument escaping and file verification
- Native Wide Character & High-DPI: All internal strings use `wchar_t` (UTF-16) with manifest-based Per-Monitor V2 DPI scaling
- System Tray & Single Instance: Runs in system tray; re-launching restores existing window to foreground
- Zero Dependencies: Links only Windows system DLLs (`USER32`, `SHELL32`, `COMDLG32`, `ADVAPI32`, `GDI32`)

---

## 🛠️ Build & Test

### Requirements

- **MinGW-w64** (GCC 8.0+, C99/C11 compliant)

### Local Compilation

#### Option 1: PowerShell Script (Recommended)

```powershell
# Build FanControlHotkey.exe
.\build.ps1

# Run unit tests
.\build.ps1 -Test

# Clean build artifacts
.\build.ps1 -Clean
```

#### Option 2: Make

```bash
# Build binary
make

# Run unit tests
make test

# Clean artifacts
make clean
```

---

## 📂 Project Structure

```text
├── .github/workflows/ci.yml   # GitHub Actions CI & Auto-Release pipeline
├── src/                       # Modular C source code
│   ├── main.c                 # Entry point and mutex single-instance check
│   ├── app_context.h          # Global context and data models
│   ├── strings.h / .c         # Bilingual string tables (EN/ZH)
│   ├── config.h / .c          # INI serialization, AppData fallback & autostart
│   ├── hotkey.h / .c          # Hotkey parser and registration manager
│   ├── process_monitor.h / .c # Background thread snapshot & FNV-1a hash matching engine
│   ├── runner.h / .c          # CreateProcessW secure process launcher
│   ├── dpi_utils.h / .c       # Per-Monitor DPI scaling utilities
│   ├── ui_main.h / .c         # Main window (GWLP_USERDATA bound) and message loop
│   └── ui_settings.h / .c     # Modular settings window and hotkey capture
├── tests/
│   └── test_main.c            # Pure C unit test suite
├── res/                       # Resource files
│   ├── app.manifest           # Per-Monitor V2 DPI-Aware manifest
│   ├── resource.h / .rc       # Icon & VERSIONINFO resources
│   └── icon.ico               # Application icon
├── specs/                     # Local archive of specs & decision records (gitignored)
├── build.ps1                  # Windows PowerShell build script
├── Makefile                   # Windows Makefile (MinGW-w64 toolchain)
└── README.md                  # English documentation
```

---

## 📄 License

This project is licensed under the [MIT License](LICENSE).
