# FanControl Hotkey

A lightweight hotkey-based fan profile switcher for [FanControl](https://getfancontrol.com/). Built with pure Win32 API, zero runtime dependencies.

## Features

- **4 fan mode switching**: Silent / Normal / Performance / Turbo
- **Custom hotkeys**: Configure hotkeys for each mode (e.g. `Ctrl+Alt+1`)
- **GUI settings window**: Browse for config files, customize hotkeys, toggle modes on/off
- **Startup autostart**: Optional, via registry (no admin required)
- **System tray icon**: Optional, hide to tray on close
- **Single instance**: Second launch brings the existing window to foreground
- **INI persistence**: All settings saved to `fan_hotkey.ini` next to the exe
- **Zero dependencies**: Links only to Windows system DLLs (`KERNEL32`, `USER32`, `SHELL32`, `msvcrt`)

## How It Works

When a hotkey or button is triggered, the program executes:

```
FanControl.exe -c <config.json>
```

FanControl detects the running instance and hot-swaps the configuration without restarting. This leverages FanControl's native `-c` command-line argument.

## Build

### Requirements

- [MinGW-w64](https://www.mingw-w64.org/) toolchain (GCC)

### Compile

```bash
gcc -mwindows -municode -Os -s -o fan_hotkey.exe fan_hotkey.c -lcomdlg32
```

- `-mwindows`: GUI subsystem, no console window
- `-municode`: Use `wWinMain` entry point for Unicode support
- `-Os`: Optimize for size
- `-s`: Strip symbol table
- `-lcomdlg32`: Link common dialog (file open dialog)

## Usage

1. In FanControl UI, configure 4 fan profiles and export them as JSON files.
2. Run `fan_hotkey.exe`.
3. Click **Settings** to configure each mode:
   - Check **Enable** for modes you want to use
   - Browse and select the corresponding JSON config file
   - Set a hotkey (format: `Ctrl+Alt+1`, `Shift+F5`, etc.)
4. Click **OK** to save.
5. Switch modes via hotkeys or click the buttons on the main window.

### Hotkey Format

```
Ctrl+Alt+1
Ctrl+Shift+F5
Win+Alt+S
```

Supported modifiers: `Ctrl`/`Control`, `Alt`, `Shift`, `Win`
Supported keys: Single character (A-Z, 0-9) or F1-F12

## Config File

Settings are stored in `fan_hotkey.ini` next to the executable:

```ini
[General]
AutoStart=0
ShowTray=1

[Mode0]
Enabled=1
Config=C:\path\to\silent.json
Hotkey=Ctrl+Alt+1

[Mode1]
Enabled=1
Config=C:\path\to\normal.json
Hotkey=Ctrl+Alt+2
```

## Performance

| Metric | Value |
|---|---|
| Memory | ~1.6 MB |
| Disk | ~54 KB |
| CPU (idle) | 0% |
| CPU (on trigger) | <0.1% instantaneous |
| Hotkey response | <1ms |
| Dependencies | None (4 Windows system DLLs) |

## License

MIT
