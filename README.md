# FanControl Hotkey

[中文文档](README-Zh-CN.md)

A Ultra-lightweight Fancontrol configuration Switcher via hotkey
 
## Features

- **4 fan mode switching**: Silent / Normal / Performance / Turbo
- **Custom hotkeys**: Configure hotkeys for each mode (e.g. `Ctrl+Alt+1`)
- **GUI settings window**: Browse for config files, customize hotkeys, toggle modes on/off
- **Zero dependencies**: Links only to Windows system DLLs (`KERNEL32`, `USER32`, `SHELL32`, `msvcrt`)

## How It Works

switch configuration file via execute fancontrol.exe -c <example.json>

FanControl detects the running instance and hot-swaps the configuration without restarting. This leverages FanControl's native `-c` command-line argument.

## Build

### Requirements

- [MinGW-w64](https://www.mingw-w64.org/) toolchain (GCC)

### Compile

```bash
windres resource.rc -O coff -o resource.o
gcc -mwindows -municode -Os -s -D_UNICODE -DUNICODE -o FanControlHotkey.exe fan_hotkey.c resource.o -luser32 -lshell32 -lcomdlg32 -ladvapi32 -lgdi32
```

- `windres resource.rc -O coff -o resource.o`: Compile the icon resource into an object file
- `-mwindows`: GUI subsystem, no console window
- `-municode`: Use `wWinMain` entry point for Unicode support
- `-Os`: Optimize for size
- `-s`: Strip symbol table
- `-D_UNICODE -DUNICODE`: Enable Unicode macros for Win32 API
- Linked libraries:
  - `-luser32`: Window messages, hotkeys, tray icon
  - `-lshell32`: Shell execute, tray notifications
  - `-lcomdlg32`: File open dialog
  - `-ladvapi32`: Registry access (autostart)
  - `-lgdi32`: Fonts and GDI resources

## Usage

1. In FanControl UI, configure 4 fan profiles and export them as JSON files, Usually these files located in C:\Program Files (x86)\FanControl\Configurations.
2. Run `FanControlHotkey.exe`.
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

## License

MIT
