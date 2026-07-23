#include <windows.h>
#include <shellapi.h>
#include <commdlg.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <wchar.h>

#define MAX_MODES 4
#define MAX_PATH_LEN 260
#define MAX_HOTKEY_LEN 32

#define FANCONTROL_EXE L"C:\\Program Files (x86)\\FanControl\\FanControl.exe"
#define MUTEX_NAME L"FanControl_Hotkey_Mutex_3F7A2E"
#define APP_REG_KEY L"Software\\Microsoft\\Windows\\CurrentVersion\\Run"
#define APP_REG_VAL L"FanControlHotkey"

#define IDC_BTN_BASE    1
#define IDC_SETTINGS    100
#define IDC_CHK_BASE   1000
#define IDC_BROWSE_BASE 1100
#define IDC_OK          2000
#define IDC_CANCEL      2001
#define IDC_CHK_STARTUP 3000
#define IDC_CHK_TRAY    3001
#define IDC_CAPTURE_BASE 1300

#define WM_TRAYICON       (WM_APP + 1)
#define WM_REFRESH_BROWSE (WM_APP + 2)
#define IDM_TRAY_SHOW   201
#define IDM_TRAY_EXIT   202

/* ---------- Bilingual String Table ---------- */

typedef struct {
    const wchar_t *mode_names[MAX_MODES];
    const wchar_t *app_title;
    const wchar_t *settings;
    const wchar_t *general;
    const wchar_t *autostart;
    const wchar_t *show_tray;
    const wchar_t *mode_config;
    const wchar_t *enable_fmt;
    const wchar_t *config_file;
    const wchar_t *browse;
    const wchar_t *hotkey;
    const wchar_t *capture_btn;
    const wchar_t *ok;
    const wchar_t *cancel;
    const wchar_t *tray_show;
    const wchar_t *tray_exit;
    const wchar_t *capture_prompt;
    const wchar_t *capture_title;
    const wchar_t *json_filter;
    const wchar_t *all_filter;
    const wchar_t *hotkey_conflict_msg;
    const wchar_t *hotkey_conflict_title;
    const wchar_t *not_enabled_msg;
    const wchar_t *not_configured_fmt;
    const wchar_t *notice;
    const wchar_t *disabled_suffix;
    const wchar_t *not_configured_suffix;
} Strings;

static const Strings str_zh = {
    { L"静音模式", L"日常模式", L"野兽模式", L"涡轮模式" },
    L"FanControl 模式切换",
    L"设置",
    L"通用选项",
    L"开机自动启动",
    L"显示系统托盘图标",
    L"模式配置",
    L"启用 %s",
    L"配置文件：",
    L"选择路径...",
    L"快捷键：",
    L"直接输入",
    L"确定",
    L"取消",
    L"显示",
    L"退出",
    L"请按下快捷键组合…（Esc 取消）",
    L"捕获快捷键",
    L"JSON 文件 (*.json)",
    L"所有文件 (*.*)",
    L"该快捷键已被其他模式使用，请选择其他组合。",
    L"快捷键冲突",
    L"该模式未启用，请在设置中勾选启用。",
    L"尚未为「%s」设置配置文件路径，请先在设置中选择。",
    L"提示",
    L"（未启用）",
    L"（未配置）",
};

static const Strings str_en = {
    { L"Silent", L"Normal", L"Performance", L"Turbo" },
    L"FanControl Mode",
    L"Settings",
    L"General",
    L"Start with Windows",
    L"Show tray icon",
    L"Mode Configuration",
    L"Enable %s",
    L"Config file:",
    L"Browse...",
    L"Hotkey:",
    L"Capture",
    L"OK",
    L"Cancel",
    L"Show",
    L"Exit",
    L"Press a key combination... (Esc to cancel)",
    L"Capture Hotkey",
    L"JSON Files (*.json)",
    L"All Files (*.*)",
    L"This hotkey is already used by another mode. Please choose a different combination.",
    L"Hotkey Conflict",
    L"This mode is not enabled. Enable it in Settings.",
    L"No config file set for \"%s\". Please select one in Settings.",
    L"Notice",
    L" (Disabled)",
    L" (Not configured)",
};

static const Strings *g_str;

typedef struct {
    wchar_t name[32];
    char    config[MAX_PATH_LEN];
    char    hotkey[MAX_HOTKEY_LEN];
    int enabled;
    int hotkeyId;
} Mode;

static Mode g_modes[MAX_MODES] = {
    {L"", "", "Ctrl+Alt+1", 1, 0},
    {L"", "", "Ctrl+Alt+2", 1, 0},
    {L"", "", "Ctrl+Alt+3", 1, 0},
    {L"", "", "Ctrl+Alt+4", 1, 0},
};

static int g_autostart = 0;
static int g_showTray = 1;
static int g_trayAdded = 0;

static HWND g_hwnd;
static HFONT g_font;
static char g_iniPath[MAX_PATH];
static HWND g_btnMain[MAX_MODES];
static HWND g_swHwnd;
static HWND g_swChk[MAX_MODES];
static HWND g_swEdtCfg[MAX_MODES];
static HWND g_swBtnBrw[MAX_MODES];
static HWND g_swEdtHk[MAX_MODES];
static HWND g_swBtnCap[MAX_MODES];
static HWND g_swChkStartup;
static HWND g_swChkTray;

static BOOL CALLBACK SetChildFontProc(HWND h, LPARAM l) {
    SendMessage(h, WM_SETFONT, l, TRUE);
    return TRUE;
}

/* ---------- INI ---------- */

static void GetIniPath(void) {
    GetModuleFileNameA(NULL, g_iniPath, MAX_PATH);
    char *dot = strrchr(g_iniPath, '.');
    if (dot) strcpy(dot, ".ini");
    else strcat(g_iniPath, ".ini");
}

static void LoadConfig(void) {
    GetIniPath();
    for (int i = 0; i < MAX_MODES; i++) {
        char sec[16];
        sprintf(sec, "Mode%d", i);
        g_modes[i].enabled = GetPrivateProfileIntA(sec, "Enabled", 1, g_iniPath);
        GetPrivateProfileStringA(sec, "Config", "",
            g_modes[i].config, MAX_PATH_LEN, g_iniPath);
        GetPrivateProfileStringA(sec, "Hotkey", g_modes[i].hotkey,
            g_modes[i].hotkey, MAX_HOTKEY_LEN, g_iniPath);
    }
    g_autostart = GetPrivateProfileIntA("General", "AutoStart", 0, g_iniPath);
    g_showTray = GetPrivateProfileIntA("General", "ShowTray", 1, g_iniPath);
}

static void SaveConfig(void) {
    for (int i = 0; i < MAX_MODES; i++) {
        char sec[16], buf[16];
        sprintf(sec, "Mode%d", i);
        sprintf(buf, "%d", g_modes[i].enabled);
        WritePrivateProfileStringA(sec, "Enabled", buf, g_iniPath);
        WritePrivateProfileStringA(sec, "Config", g_modes[i].config, g_iniPath);
        WritePrivateProfileStringA(sec, "Hotkey", g_modes[i].hotkey, g_iniPath);
    }
    char buf[16];
    sprintf(buf, "%d", g_autostart);
    WritePrivateProfileStringA("General", "AutoStart", buf, g_iniPath);
    sprintf(buf, "%d", g_showTray);
    WritePrivateProfileStringA("General", "ShowTray", buf, g_iniPath);
}

/* ---------- Auto Start (Registry) ---------- */

static int IsAutostartEnabled(void) {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, APP_REG_KEY, 0, KEY_READ, &hKey)
        != ERROR_SUCCESS)
        return 0;
    wchar_t path[MAX_PATH];
    DWORD len = sizeof(path);
    DWORD type;
    int found = 0;
    if (RegQueryValueExW(hKey, APP_REG_VAL, NULL, &type,
        (LPBYTE)path, &len) == ERROR_SUCCESS)
        found = 1;
    RegCloseKey(hKey);
    return found;
}

static void SetAutostart(int on) {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, APP_REG_KEY, 0, KEY_SET_VALUE, &hKey)
        != ERROR_SUCCESS)
        return;
    if (on) {
        wchar_t path[MAX_PATH];
        GetModuleFileNameW(NULL, path, MAX_PATH);
        RegSetValueExW(hKey, APP_REG_VAL, 0, REG_SZ,
            (const BYTE *)path, (lstrlenW(path) + 1) * sizeof(wchar_t));
    } else {
        RegDeleteValueW(hKey, APP_REG_VAL);
    }
    RegCloseKey(hKey);
}

/* ---------- Hotkey ---------- */

static int ParseHotkey(const char *str, UINT *mods, UINT *vk) {
    UINT m = 0;
    const char *p = str;
    char tok[32];
    while (1) {
        const char *plus = strchr(p, '+');
        int len = plus ? (int)(plus - p) : (int)strlen(p);
        if (len <= 0 || len >= 32) return 0;
        memcpy(tok, p, len);
        tok[len] = 0;
        if (!plus) {
            if (strlen(tok) == 1) {
                *vk = toupper((unsigned char)tok[0]);
            } else if (toupper((unsigned char)tok[0]) == 'F'
                       && isdigit((unsigned char)tok[1])) {
                int n = atoi(tok + 1);
                if (n < 1 || n > 12) return 0;
                *vk = VK_F1 + n - 1;
            } else {
                return 0;
            }
            *mods = m;
            return 1;
        }
        if (!_stricmp(tok, "Ctrl") || !_stricmp(tok, "Control")) m |= MOD_CONTROL;
        else if (!_stricmp(tok, "Alt"))    m |= MOD_ALT;
        else if (!_stricmp(tok, "Shift"))  m |= MOD_SHIFT;
        else if (!_stricmp(tok, "Win"))    m |= MOD_WIN;
        else return 0;
        p = plus + 1;
    }
}

static void UnregisterAllHotkeys(void) {
    for (int i = 0; i < MAX_MODES; i++) {
        if (g_modes[i].hotkeyId) {
            UnregisterHotKey(g_hwnd, g_modes[i].hotkeyId);
            g_modes[i].hotkeyId = 0;
        }
    }
}

static void RegisterAllHotkeys(void) {
    UnregisterAllHotkeys();
    for (int i = 0; i < MAX_MODES; i++) {
        if (!g_modes[i].enabled || !g_modes[i].config[0]) continue;
        UINT mods, vk;
        if (ParseHotkey(g_modes[i].hotkey, &mods, &vk)) {
            g_modes[i].hotkeyId = i + 1;
            RegisterHotKey(g_hwnd, g_modes[i].hotkeyId, mods, vk);
        }
    }
}

/* ---------- Tray & Switch ---------- */

static void AddTrayIcon(void) {
    if (!g_showTray || g_trayAdded) return;
    NOTIFYICONDATAW nid = {0};
    nid.cbSize = sizeof(nid);
    nid.hWnd = g_hwnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = LoadIconW(NULL, IDI_APPLICATION);
    wcscpy(nid.szTip, g_str->app_title);
    Shell_NotifyIconW(NIM_ADD, &nid);
    g_trayAdded = 1;
}

static void RemoveTrayIcon(void) {
    if (!g_trayAdded) return;
    NOTIFYICONDATAW nid = {0};
    nid.cbSize = sizeof(nid);
    nid.hWnd = g_hwnd;
    nid.uID = 1;
    Shell_NotifyIconW(NIM_DELETE, &nid);
    g_trayAdded = 0;
}

static void ApplyTraySetting(void) {
    if (g_showTray) AddTrayIcon();
    else RemoveTrayIcon();
}

static void SwitchTo(int idx) {
    if (idx < 0 || idx >= MAX_MODES) return;
    if (!g_modes[idx].enabled) {
        MessageBoxW(g_hwnd, g_str->not_enabled_msg,
            g_str->notice, MB_OK | MB_ICONINFORMATION);
        return;
    }
    if (!g_modes[idx].config[0]) {
        wchar_t msg[128];
        _snwprintf(msg, 128, g_str->not_configured_fmt,
            g_modes[idx].name);
        MessageBoxW(g_hwnd, msg, g_str->notice, MB_OK | MB_ICONINFORMATION);
        return;
    }
    wchar_t args[MAX_PATH * 2];
    wchar_t wcfg[MAX_PATH];
    MultiByteToWideChar(CP_UTF8, 0, g_modes[idx].config, -1, wcfg, MAX_PATH);
    _snwprintf(args, sizeof(args)/sizeof(args[0]), L"-c \"%s\"", wcfg);
    ShellExecuteW(NULL, L"open", FANCONTROL_EXE, args, NULL, SW_HIDE);
}

static void RefreshMainButtons(void) {
    for (int i = 0; i < MAX_MODES; i++) {
        wchar_t label[96];
        if (!g_modes[i].enabled) {
            _snwprintf(label, 96, L"%s%s", g_modes[i].name, g_str->disabled_suffix);
        } else if (!g_modes[i].config[0]) {
            _snwprintf(label, 96, L"%s%s", g_modes[i].name, g_str->not_configured_suffix);
        } else if (g_modes[i].hotkey[0]) {
            wchar_t whk[MAX_HOTKEY_LEN];
            MultiByteToWideChar(CP_UTF8, 0, g_modes[i].hotkey, -1, whk, MAX_HOTKEY_LEN);
            _snwprintf(label, 96, L"%s (%s)", g_modes[i].name, whk);
        } else {
            _snwprintf(label, 96, L"%s", g_modes[i].name);
        }
        SetWindowTextW(g_btnMain[i], label);
    }
}

static void ShowTrayMenu(HWND h) {
    POINT pt;
    GetCursorPos(&pt);
    HMENU menu = CreatePopupMenu();
    AppendMenuW(menu, MF_STRING, IDM_TRAY_SHOW, g_str->tray_show);
    AppendMenuW(menu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(menu, MF_STRING, IDM_TRAY_EXIT, g_str->tray_exit);
    SetForegroundWindow(h);
    TrackPopupMenu(menu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, h, NULL);
    DestroyMenu(menu);
}

/* ---------- Settings Window ---------- */

/* 捕获热键的弹出窗口 */
static HWND g_capHwnd = NULL;
static int   g_capTarget = -1;

static void VkToString(UINT vk, wchar_t *out, int len) {
    if (vk >= 'A' && vk <= 'Z') {
        _snwprintf(out, len, L"%c", vk);
    } else if (vk >= '0' && vk <= '9') {
        _snwprintf(out, len, L"%c", vk);
    } else if (vk >= VK_F1 && vk <= VK_F12) {
        _snwprintf(out, len, L"F%d", vk - VK_F1 + 1);
    } else {
        out[0] = 0;
    }
}

static LRESULT CALLBACK CaptureProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    switch (m) {
    case WM_CREATE: {
        CreateWindowW(L"STATIC", g_str->capture_prompt,
            WS_CHILD | WS_VISIBLE | SS_CENTER,
            10, 20, 260, 24, h, NULL, NULL, NULL);
        SendMessage(GetWindow(h, GW_CHILD), WM_SETFONT, (WPARAM)g_font, TRUE);
        return 0;
    }
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN: {
        if (w == VK_ESCAPE) {
            DestroyWindow(h);
            return 0;
        }
        /* 忽略纯修饰键 */
        if (w == VK_CONTROL || w == VK_MENU || w == VK_SHIFT || w == VK_LWIN
            || w == VK_RWIN)
            return 0;

        wchar_t buf[64] = L"";
        int pos = 0;
        if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
            pos += _snwprintf(buf + pos, 64 - pos, L"Ctrl+");
        if (GetAsyncKeyState(VK_MENU) & 0x8000)
            pos += _snwprintf(buf + pos, 64 - pos, L"Alt+");
        if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
            pos += _snwprintf(buf + pos, 64 - pos, L"Shift+");
        if ((GetAsyncKeyState(VK_LWIN) & 0x8000) || (GetAsyncKeyState(VK_RWIN) & 0x8000))
            pos += _snwprintf(buf + pos, 64 - pos, L"Win+");

        wchar_t key[8];
        VkToString((UINT)w, key, 8);
        if (key[0]) {
            wcscat(buf, key);
            if (g_capTarget >= 0) {
                /* 将捕获结果转为窄字符串并用 ParseHotkey 解析 */
                char buf_a[64];
                WideCharToMultiByte(CP_UTF8, 0, buf, -1, buf_a, 64, NULL, NULL);
                UINT newMods, newVk;
                int ok = ParseHotkey(buf_a, &newMods, &newVk);

                int dup = 0;
                if (ok) {
                    for (int i = 0; i < MAX_MODES; i++) {
                        if (i == g_capTarget) continue;
                        char existing_a[64];
                        GetWindowTextA(g_swEdtHk[i], existing_a, 64);
                        UINT exMods, exVk;
                        if (ParseHotkey(existing_a, &exMods, &exVk)
                            && exMods == newMods && exVk == newVk) {
                            dup = 1;
                            break;
                        }
                    }
                }
                if (dup) {
                    MessageBoxW(h, g_str->hotkey_conflict_msg,
                        g_str->hotkey_conflict_title, MB_OK | MB_ICONWARNING);
                } else {
                    SetWindowTextW(g_swEdtHk[g_capTarget], buf);
                    SetFocus(g_swEdtHk[g_capTarget]);
                }
            }
        }
        DestroyWindow(h);
        return 0;
    }
    case WM_DESTROY:
        g_capHwnd = NULL;
        g_capTarget = -1;
        if (g_swHwnd) SetForegroundWindow(g_swHwnd);
        return 0;
    }
    return DefWindowProcW(h, m, w, l);
}

static void OpenCaptureWindow(int idx) {
    if (g_capHwnd) {
        SetForegroundWindow(g_capHwnd);
        return;
    }
    g_capTarget = idx;
    g_capHwnd = CreateWindowW(L"fccapture", g_str->capture_title,
        WS_OVERLAPPED | WS_CAPTION,
        CW_USEDEFAULT, CW_USEDEFAULT, 290, 90, g_swHwnd, NULL,
        (HINSTANCE)GetWindowLongPtrW(g_hwnd, GWLP_HINSTANCE), NULL);
    ShowWindow(g_capHwnd, SW_SHOW);
    UpdateWindow(g_capHwnd);
    SetFocus(g_capHwnd);
}

static void OpenBrowseDialog(int idx) {
    wchar_t file[MAX_PATH] = L"";
    GetWindowTextW(g_swEdtCfg[idx], file, MAX_PATH);
    wchar_t filter[256];
    wchar_t *p = filter;
    wcscpy(p, g_str->json_filter); p += wcslen(p) + 1;
    wcscpy(p, L"*.json");          p += wcslen(p) + 1;
    wcscpy(p, g_str->all_filter);  p += wcslen(p) + 1;
    wcscpy(p, L"*.*");             p += wcslen(p) + 1;
    *p = 0;
    OPENFILENAMEW ofn = {0};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g_swHwnd;
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = file;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    if (GetOpenFileNameW(&ofn)) {
        char cfg[MAX_PATH];
        WideCharToMultiByte(CP_UTF8, 0, file, -1, cfg, MAX_PATH, NULL, NULL);
        SetWindowTextA(g_swEdtCfg[idx], cfg);
    }
}

static void UpdateBrowseState(void) {
    for (int i = 0; i < MAX_MODES; i++) {
        BOOL on = (SendMessage(g_swChk[i], BM_GETCHECK, 0, 0) == BST_CHECKED);
        EnableWindow(g_swBtnBrw[i], on);
        EnableWindow(g_swEdtCfg[i], on);
        EnableWindow(g_swEdtHk[i], on);
        EnableWindow(g_swBtnCap[i], on);
    }
}

static LRESULT CALLBACK SettingsWndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    switch (m) {
    case WM_CREATE: {
        CREATESTRUCTW *cs = (CREATESTRUCTW *)l;
        int y = 12;

        /* 通用选项区 */
        CreateWindowW(L"STATIC", g_str->general,
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            12, y, 200, 22, h, NULL, cs->hInstance, NULL);
        y += 26;
        g_swChkStartup = CreateWindowW(L"BUTTON", g_str->autostart,
            WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
            22, y, 200, 26, h,
            (HMENU)IDC_CHK_STARTUP, cs->hInstance, NULL);
        SendMessage(g_swChkStartup, BM_SETCHECK,
            IsAutostartEnabled() ? BST_CHECKED : BST_UNCHECKED, 0);
        y += 30;
        g_swChkTray = CreateWindowW(L"BUTTON", g_str->show_tray,
            WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
            22, y, 200, 26, h,
            (HMENU)IDC_CHK_TRAY, cs->hInstance, NULL);
        SendMessage(g_swChkTray, BM_SETCHECK,
            g_showTray ? BST_CHECKED : BST_UNCHECKED, 0);
        y += 38;

        /* 分隔线 */
        CreateWindowW(L"STATIC", g_str->mode_config,
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            12, y, 200, 22, h, NULL, cs->hInstance, NULL);
        y += 26;

        for (int i = 0; i < MAX_MODES; i++) {
            wchar_t lbl[64];
            _snwprintf(lbl, 64, g_str->enable_fmt, g_modes[i].name);
            g_swChk[i] = CreateWindowW(L"BUTTON", lbl,
                WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                12, y, 260, 26, h,
                (HMENU)(INT_PTR)(IDC_CHK_BASE + i), cs->hInstance, NULL);
            SendMessage(g_swChk[i], BM_SETCHECK,
                g_modes[i].enabled ? BST_CHECKED : BST_UNCHECKED, 0);

            y += 30;
            CreateWindowW(L"STATIC", g_str->config_file, WS_CHILD | WS_VISIBLE,
                22, y + 3, 80, 22, h, NULL, cs->hInstance, NULL);
            g_swEdtCfg[i] = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT",
                L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                108, y, 285, 28, h, NULL, cs->hInstance, NULL);
            wchar_t cfgw[MAX_PATH];
            MultiByteToWideChar(CP_UTF8, 0, g_modes[i].config, -1, cfgw, MAX_PATH);
            SetWindowTextW(g_swEdtCfg[i], cfgw);
            g_swBtnBrw[i] = CreateWindowW(L"BUTTON", g_str->browse,
                WS_CHILD | WS_VISIBLE, 400, y, 80, 28, h,
                (HMENU)(INT_PTR)(IDC_BROWSE_BASE + i), cs->hInstance, NULL);

            y += 34;
            CreateWindowW(L"STATIC", g_str->hotkey, WS_CHILD | WS_VISIBLE,
                22, y + 3, 80, 22, h, NULL, cs->hInstance, NULL);
            g_swEdtHk[i] = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT",
                L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                108, y, 120, 28, h, NULL, cs->hInstance, NULL);
            wchar_t hkw[MAX_HOTKEY_LEN];
            MultiByteToWideChar(CP_UTF8, 0, g_modes[i].hotkey, -1, hkw, MAX_HOTKEY_LEN);
            SetWindowTextW(g_swEdtHk[i], hkw);
            g_swBtnCap[i] = CreateWindowW(L"BUTTON", g_str->capture_btn,
                WS_CHILD | WS_VISIBLE, 235, y, 80, 28, h,
                (HMENU)(INT_PTR)(IDC_CAPTURE_BASE + i), cs->hInstance, NULL);

            y += 42;
        }
        CreateWindowW(L"BUTTON", g_str->ok, WS_CHILD | WS_VISIBLE,
            275, y, 95, 32, h, (HMENU)IDC_OK, cs->hInstance, NULL);
        CreateWindowW(L"BUTTON", g_str->cancel, WS_CHILD | WS_VISIBLE,
            385, y, 95, 32, h, (HMENU)IDC_CANCEL, cs->hInstance, NULL);

        EnumChildWindows(h, (WNDENUMPROC)SetChildFontProc, (LPARAM)g_font);
        PostMessage(h, WM_REFRESH_BROWSE, 0, 0);
        return 0;
    }
    case WM_REFRESH_BROWSE:
        UpdateBrowseState();
        return 0;
    case WM_COMMAND:
        if (HIWORD(w) == BN_CLICKED) {
            int id = LOWORD(w);
            if (id == IDC_OK) {
                for (int i = 0; i < MAX_MODES; i++) {
                    g_modes[i].enabled = (SendMessage(
                        g_swChk[i], BM_GETCHECK, 0, 0) == BST_CHECKED);
                    GetWindowTextA(g_swEdtCfg[i],
                        g_modes[i].config, MAX_PATH_LEN);
                    GetWindowTextA(g_swEdtHk[i],
                        g_modes[i].hotkey, MAX_HOTKEY_LEN);
                }
                g_autostart = (SendMessage(
                    g_swChkStartup, BM_GETCHECK, 0, 0) == BST_CHECKED);
                g_showTray = (SendMessage(
                    g_swChkTray, BM_GETCHECK, 0, 0) == BST_CHECKED);
                SetAutostart(g_autostart);
                SaveConfig();
                RegisterAllHotkeys();
                RefreshMainButtons();
                ApplyTraySetting();
                DestroyWindow(h);
                g_swHwnd = NULL;
            } else if (id == IDC_CANCEL) {
                DestroyWindow(h);
                g_swHwnd = NULL;
                RegisterAllHotkeys();
            } else if (id >= IDC_CHK_BASE && id < IDC_CHK_BASE + MAX_MODES) {
                UpdateBrowseState();
            } else {
                for (int i = 0; i < MAX_MODES; i++) {
                    if (id == IDC_BROWSE_BASE + i) {
                        OpenBrowseDialog(i);
                        break;
                    }
                    if (id == IDC_CAPTURE_BASE + i) {
                        OpenCaptureWindow(i);
                        break;
                    }
                }
            }
        }
        return 0;
    case WM_CLOSE:
        DestroyWindow(h);
        g_swHwnd = NULL;
        RegisterAllHotkeys();
        return 0;
    }
    return DefWindowProcW(h, m, w, l);
}

static void OpenSettings(HINSTANCE hi) {
    if (g_swHwnd) {
        SetForegroundWindow(g_swHwnd);
        return;
    }
    UnregisterAllHotkeys();
    g_swHwnd = CreateWindowW(L"fcsettings", g_str->settings,
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT, 500, 660, g_hwnd, NULL, hi, NULL);
    ShowWindow(g_swHwnd, SW_SHOW);
    UpdateWindow(g_swHwnd);
}

/* ---------- Main Window ---------- */

static LRESULT CALLBACK MainWndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    switch (m) {
    case WM_HOTKEY:
        for (int i = 0; i < MAX_MODES; i++) {
            if (g_modes[i].hotkeyId == (int)w) {
                SwitchTo(i);
                break;
            }
        }
        return 0;
    case WM_COMMAND:
        if (HIWORD(w) == BN_CLICKED) {
            int id = LOWORD(w);
            if (id == IDC_SETTINGS)
                OpenSettings((HINSTANCE)GetWindowLongPtr(h, GWLP_HINSTANCE));
            else if (id == IDM_TRAY_SHOW) {
                ShowWindow(h, SW_SHOW);
                SetForegroundWindow(h);
            } else if (id == IDM_TRAY_EXIT)
                DestroyWindow(h);
            else {
                for (int i = 0; i < MAX_MODES; i++) {
                    if (id == IDC_BTN_BASE + i) {
                        SwitchTo(i);
                        break;
                    }
                }
            }
        }
        return 0;
    case WM_TRAYICON:
        if (l == WM_LBUTTONDBLCLK) {
            ShowWindow(h, SW_SHOW);
            SetForegroundWindow(h);
        } else if (l == WM_RBUTTONUP)
            ShowTrayMenu(h);
        return 0;
    case WM_CLOSE:
        ShowWindow(h, SW_HIDE);
        return 0;
    case WM_DESTROY:
        RemoveTrayIcon();
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(h, m, w, l);
}

/* ---------- Single Instance ---------- */

static int FindExistingWindow(void) {
    HWND h = FindWindowW(L"fcgui", NULL);
    if (h) {
        /* 主窗口在 WM_CLOSE 中被 SW_HIDE 隐藏，IsIconic 不会返回 TRUE，
           因此需要显式 SW_SHOW 才能把窗口从隐藏状态恢复显示。 */
        ShowWindow(h, IsIconic(h) ? SW_RESTORE : SW_SHOW);
        SetForegroundWindow(h);
        return 1;
    }
    return 0;
}

int WINAPI wWinMain(HINSTANCE hi, HINSTANCE pi, LPWSTR cmd, int show) {
    /* 单实例检测：用命名互斥量 */
    HANDLE hMutex = CreateMutexW(NULL, TRUE, MUTEX_NAME);
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(hMutex);
        FindExistingWindow();
        return 0;
    }

    /* 语言检测 */
    LANGID lang = GetUserDefaultUILanguage();
    if (PRIMARYLANGID(lang) == LANG_CHINESE)
        g_str = &str_zh;
    else
        g_str = &str_en;

    g_font = CreateFontW(20, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
        DEFAULT_PITCH | FF_SWISS, L"Microsoft YaHei UI");

    WNDCLASSW wcMain = {0};
    wcMain.lpfnWndProc = MainWndProc;
    wcMain.hInstance = hi;
    wcMain.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcMain.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wcMain.lpszClassName = L"fcgui";
    RegisterClassW(&wcMain);

    WNDCLASSW wcSet = {0};
    wcSet.lpfnWndProc = SettingsWndProc;
    wcSet.hInstance = hi;
    wcSet.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcSet.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wcSet.lpszClassName = L"fcsettings";
    RegisterClassW(&wcSet);

    WNDCLASSW wcCap = {0};
    wcCap.lpfnWndProc = CaptureProc;
    wcCap.hInstance = hi;
    wcCap.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcCap.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wcCap.lpszClassName = L"fccapture";
    RegisterClassW(&wcCap);

    LoadConfig();

    /* 设置模式名称（根据语言） */
    for (int i = 0; i < MAX_MODES; i++)
        wcscpy(g_modes[i].name, g_str->mode_names[i]);

    g_hwnd = CreateWindowW(L"fcgui", g_str->app_title,
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 290, 330, NULL, NULL, hi, NULL);

    int y = 12;
    for (int i = 0; i < MAX_MODES; i++) {
        g_btnMain[i] = CreateWindowW(L"BUTTON", L"",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            20, y, 240, 45, g_hwnd,
            (HMENU)(INT_PTR)(IDC_BTN_BASE + i), hi, NULL);
        y += 52;
    }
    CreateWindowW(L"BUTTON", g_str->settings,
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        20, y + 8, 240, 36, g_hwnd, (HMENU)IDC_SETTINGS, hi, NULL);

    EnumChildWindows(g_hwnd, (WNDENUMPROC)SetChildFontProc, (LPARAM)g_font);

    RefreshMainButtons();
    ApplyTraySetting();
    RegisterAllHotkeys();
    ShowWindow(g_hwnd, show);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnregisterAllHotkeys();
    RemoveTrayIcon();
    DeleteObject(g_font);
    CloseHandle(hMutex);
    return 0;
}
