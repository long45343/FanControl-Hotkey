#ifndef APP_CONTEXT_H
#define APP_CONTEXT_H

#include <windows.h>

#define APP_VERSION L"1.1"
#define MAX_MODES 4
#define MAX_CONFIG_PATH 260
#define MAX_HOTKEY_LEN 32
#define MAX_PROCESS_NAMES 16
#define MAX_PROCESS_LEN 64
#define MAX_PROCESS_LIST_LEN (MAX_PROCESS_NAMES * (MAX_PROCESS_LEN + 2))
#define MAX_RUNNING_PROCS 1024
#define DEFAULT_MODE_INDEX 1
#define POLL_INTERVAL_MS 2000

#define FANCONTROL_EXE_DEFAULT L"C:\\Program Files (x86)\\FanControl\\FanControl.exe"
#define MUTEX_NAME L"FanControl_Hotkey_Mutex_3F7A2E"
#define APP_REG_KEY L"Software\\Microsoft\\Windows\\CurrentVersion\\Run"
#define APP_REG_VAL L"FanControlHotkey"
#define AUTORUN_FLAG L"--autostart"

/* 主窗口控件 ID（1-99） */
#define IDC_BTN_BASE 1
#define IDC_SETTINGS 100
#define IDC_EXIT 101

/* 通用复选框 ID（3000-3099） */
#define IDC_CHK_STARTUP 3000
#define IDC_CHK_TRAY 3001
#define IDC_CHK_AUTOSWITCH 3002
#define IDC_CHK_FC_AUTODETECT 3003

/* 设置窗口：FanControl 路径区（3100-3199） */
#define IDC_FC_PATH_EDT 3100
#define IDC_FC_PATH_BRW 3101

/* 设置窗口：模式配置区（1000-1399） */
#define IDC_CHK_BASE 1000
#define IDC_BROWSE_BASE 1100
#define IDC_CAPTURE_BASE 1300

/* 设置窗口：进程名单区（1400-1899） */
#define IDC_PROC_LIST_BASE 1400
#define IDC_PROC_EDT_BASE 1500
#define IDC_PROC_ADD_BASE 1600
#define IDC_PROC_DEL_BASE 1700
#define IDC_PROC_BRW_BASE 1800

/* 设置窗口：按钮（2000-2099） */
#define IDC_OK 2000
#define IDC_CANCEL 2001

/* 自定义窗口消息 */
#define WM_TRAYICON (WM_APP + 1)
#define WM_REFRESH_BROWSE (WM_APP + 2)
#define WM_AUTO_SWITCH_MODE (WM_APP + 3)

/* 托盘菜单命令（201-299） */
#define IDM_TRAY_SHOW 201
#define IDM_TRAY_EXIT 202

typedef struct {
    wchar_t name[32];
    wchar_t config[MAX_CONFIG_PATH];
    wchar_t hotkey[MAX_HOTKEY_LEN];
    wchar_t processNames[MAX_PROCESS_NAMES][MAX_PROCESS_LEN];
    int processCount;
    int enabled;
    int hotkeyId;
} ModeConfig;

typedef struct {
    ModeConfig modes[MAX_MODES];
    int autostart;
    int showTray;
    int autoSwitch;
    int fcPathUserSet;
    wchar_t fanControlExe[MAX_PATH];
} AppConfig;

typedef struct {
    HINSTANCE hInstance;
    HWND hwndMain;
    HWND hwndSettings;
    HWND hwndCapture;
    HFONT hFont;
    int currentDpi;
    wchar_t iniPath[MAX_PATH];
    AppConfig config;
    int lastAutoMode;
    int trayAdded;
    HANDLE hMutex;
    HANDLE hMonitorThread;
    HANDLE hMonitorStopEvent;
} AppContext;

#endif
