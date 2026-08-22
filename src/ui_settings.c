#include <windows.h>
#include <commdlg.h>
#include <stdio.h>
#include "ui_settings.h"
#include "config.h"
#include "hotkey.h"
#include "dpi_utils.h"
#include "strings.h"
#include "runner.h"

/* 设置窗口的临时草稿状态（Draft 事务隔离，消除脏读） */
typedef struct {
    AppContext *ctx;
    AppConfig draft;
    int captureTarget;
    HWND hChkStartup;
    HWND hChkTray;
    HWND hChkAutoSwitch;
    HWND hChkFcAutoDetect;
    HWND hFcPathEdt;
    HWND hFcPathBrw;
    HWND hChkMode[MAX_MODES];
    HWND hEdtCfg[MAX_MODES];
    HWND hBtnBrw[MAX_MODES];
    HWND hEdtHk[MAX_MODES];
    HWND hBtnCap[MAX_MODES];
    HWND hProcList[MAX_MODES];
    HWND hProcEdt[MAX_MODES];
    HWND hProcAdd[MAX_MODES];
    HWND hProcDel[MAX_MODES];
    HWND hProcBrw[MAX_MODES];
} SettingsState;

static SettingsState *g_ss = NULL;

static BOOL CALLBACK SetChildFontProc(HWND h, LPARAM l) {
    SendMessage(h, WM_SETFONT, l, TRUE);
    return TRUE;
}

static void RefreshProcessList(int idx) {
    if (!g_ss || !g_ss->hProcList[idx]) return;
    SendMessage(g_ss->hProcList[idx], LB_RESETCONTENT, 0, 0);
    for (int i = 0; i < g_ss->draft.modes[idx].processCount; i++) {
        SendMessage(g_ss->hProcList[idx], LB_ADDSTRING, 0, (LPARAM)g_ss->draft.modes[idx].processNames[i]);
    }
}

static void UpdateProcessControlsState(void) {
    if (!g_ss) return;
    BOOL master = (SendMessage(g_ss->hChkAutoSwitch, BM_GETCHECK, 0, 0) == BST_CHECKED);
    for (int i = 0; i < MAX_MODES; i++) {
        EnableWindow(g_ss->hProcList[i], master);
        EnableWindow(g_ss->hProcEdt[i], master);
        EnableWindow(g_ss->hProcAdd[i], master);
        EnableWindow(g_ss->hProcDel[i], master);
        EnableWindow(g_ss->hProcBrw[i], master);
    }
}

static void UpdateBrowseState(void) {
    if (!g_ss) return;
    for (int i = 0; i < MAX_MODES; i++) {
        BOOL on = (SendMessage(g_ss->hChkMode[i], BM_GETCHECK, 0, 0) == BST_CHECKED);
        EnableWindow(g_ss->hBtnBrw[i], on);
        EnableWindow(g_ss->hEdtCfg[i], on);
        EnableWindow(g_ss->hEdtHk[i], on);
        EnableWindow(g_ss->hBtnCap[i], on);
    }
}

static int IsProcessDuplicate(int idx, const wchar_t *name) {
    if (!g_ss || !name) return 0;
    for (int i = 0; i < g_ss->draft.modes[idx].processCount; i++) {
        if (_wcsicmp(g_ss->draft.modes[idx].processNames[i], name) == 0) return 1;
    }
    return 0;
}

static void OpenBrowseDialog(HWND hwnd, int idx) {
    if (!g_ss) return;
    const AppStrings *s = Strings_Get();
    wchar_t file[MAX_PATH] = L"";
    GetWindowTextW(g_ss->hEdtCfg[idx], file, MAX_PATH);

    wchar_t filter[256];
    wchar_t *p = filter;
    wcscpy(p, s->json_filter); p += wcslen(p) + 1;
    wcscpy(p, L"*.json"); p += wcslen(p) + 1;
    wcscpy(p, s->all_filter); p += wcslen(p) + 1;
    wcscpy(p, L"*.*"); p += wcslen(p) + 1;
    *p = 0;

    OPENFILENAMEW ofn;
    memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = file;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

    if (GetOpenFileNameW(&ofn)) {
        SetWindowTextW(g_ss->hEdtCfg[idx], file);
        wcsncpy(g_ss->draft.modes[idx].config, file, MAX_CONFIG_PATH - 1);
    }
}

static void OpenFcPathBrowseDialog(HWND hwnd) {
    if (!g_ss) return;
    const AppStrings *s = Strings_Get();
    wchar_t file[MAX_PATH] = L"";
    wchar_t initDir[MAX_PATH] = L"";

    GetWindowTextW(g_ss->hFcPathEdt, initDir, MAX_PATH);
    if (initDir[0]) {
        wchar_t *slash = wcsrchr(initDir, L'\\');
        if (slash) *slash = 0;
    }

    wchar_t filter[256];
    wchar_t *p = filter;
    wcscpy(p, s->exe_filter); p += wcslen(p) + 1;
    wcscpy(p, L"*.exe"); p += wcslen(p) + 1;
    wcscpy(p, s->all_filter); p += wcslen(p) + 1;
    wcscpy(p, L"*.*"); p += wcslen(p) + 1;
    *p = 0;

    OPENFILENAMEW ofn;
    memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = file;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrInitialDir = initDir[0] ? initDir : NULL;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

    if (GetOpenFileNameW(&ofn)) {
        SetWindowTextW(g_ss->hFcPathEdt, file);
        wcsncpy(g_ss->draft.fanControlExe, file, MAX_PATH - 1);
    }
}

static void OpenExeBrowseDialog(HWND hwnd, int idx) {
    if (!g_ss) return;
    const AppStrings *s = Strings_Get();
    wchar_t file[MAX_PATH] = L"";

    wchar_t filter[256];
    wchar_t *p = filter;
    wcscpy(p, s->exe_filter); p += wcslen(p) + 1;
    wcscpy(p, L"*.exe"); p += wcslen(p) + 1;
    wcscpy(p, s->all_filter); p += wcslen(p) + 1;
    wcscpy(p, L"*.*"); p += wcslen(p) + 1;
    *p = 0;

    OPENFILENAMEW ofn;
    memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = file;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

    if (GetOpenFileNameW(&ofn)) {
        wchar_t *name = wcsrchr(file, L'\\');
        if (!name) name = wcsrchr(file, L'/');
        if (name) name++;
        else name = file;

        if (!IsProcessDuplicate(idx, name) && g_ss->draft.modes[idx].processCount < MAX_PROCESS_NAMES) {
            wcsncpy(g_ss->draft.modes[idx].processNames[g_ss->draft.modes[idx].processCount], name, MAX_PROCESS_LEN - 1);
            g_ss->draft.modes[idx].processCount++;
            RefreshProcessList(idx);
        }
    }
}

static void AddProcessFromEdit(int idx) {
    if (!g_ss || g_ss->draft.modes[idx].processCount >= MAX_PROCESS_NAMES) return;
    wchar_t name[MAX_PROCESS_LEN];
    GetWindowTextW(g_ss->hProcEdt[idx], name, MAX_PROCESS_LEN);

    int len = (int)wcslen(name);
    while (len > 0 && (name[len - 1] == L' ' || name[len - 1] == L'\t')) name[--len] = 0;
    int pos = 0;
    while (name[pos] == L' ' || name[pos] == L'\t') pos++;
    if (len <= pos) return;

    wchar_t *clean = name + pos;
    if (IsProcessDuplicate(idx, clean)) return;

    wcsncpy(g_ss->draft.modes[idx].processNames[g_ss->draft.modes[idx].processCount], clean, MAX_PROCESS_LEN - 1);
    g_ss->draft.modes[idx].processCount++;
    RefreshProcessList(idx);
    SetWindowTextW(g_ss->hProcEdt[idx], L"");
}

static void DeleteSelectedProcess(int idx) {
    if (!g_ss || !g_ss->hProcList[idx]) return;
    int sel = (int)SendMessage(g_ss->hProcList[idx], LB_GETCURSEL, 0, 0);
    if (sel < 0 || sel >= g_ss->draft.modes[idx].processCount) return;

    for (int i = sel; i < g_ss->draft.modes[idx].processCount - 1; i++) {
        memcpy(g_ss->draft.modes[idx].processNames[i],
               g_ss->draft.modes[idx].processNames[i + 1],
               sizeof(wchar_t) * MAX_PROCESS_LEN);
    }
    g_ss->draft.modes[idx].processCount--;
    RefreshProcessList(idx);
}

/* 捕获热键窗口过程 */
static LRESULT CALLBACK CaptureWndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    switch (m) {
    case WM_CREATE: {
        const AppStrings *s = Strings_Get();
        HWND hStatic = CreateWindowW(L"STATIC", s->capture_prompt,
            WS_CHILD | WS_VISIBLE | SS_CENTER,
            10, 20, 260, 24, h, NULL, NULL, NULL);
        if (g_ss && g_ss->ctx) SendMessage(hStatic, WM_SETFONT, (WPARAM)g_ss->ctx->hFont, TRUE);
        return 0;
    }
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN: {
        if (w == VK_ESCAPE) {
            DestroyWindow(h);
            return 0;
        }
        if (w == VK_CONTROL || w == VK_MENU || w == VK_SHIFT || w == VK_LWIN || w == VK_RWIN) {
            return 0;
        }

        wchar_t buf[64] = L"";
        int pos = 0;
        if (GetAsyncKeyState(VK_CONTROL) & 0x8000) pos += _snwprintf(buf + pos, 64 - pos, L"Ctrl+");
        if (GetAsyncKeyState(VK_MENU) & 0x8000) pos += _snwprintf(buf + pos, 64 - pos, L"Alt+");
        if (GetAsyncKeyState(VK_SHIFT) & 0x8000) pos += _snwprintf(buf + pos, 64 - pos, L"Shift+");
        if ((GetAsyncKeyState(VK_LWIN) & 0x8000) || (GetAsyncKeyState(VK_RWIN) & 0x8000)) pos += _snwprintf(buf + pos, 64 - pos, L"Win+");

        wchar_t key[8];
        Hotkey_VkToString((UINT)w, key, 8);
        if (key[0] && g_ss && g_ss->captureTarget >= 0) {
            wcsncat(buf, key, 63 - wcslen(buf));
            UINT newMods, newVk;
            if (Hotkey_Parse(buf, &newMods, &newVk)) {
                if (Hotkey_CheckConflict(&g_ss->draft, g_ss->captureTarget, newMods, newVk)) {
                    MessageBoxW(h, Strings_Get()->hotkey_conflict_msg, Strings_Get()->hotkey_conflict_title, MB_OK | MB_ICONWARNING);
                } else {
                    SetWindowTextW(g_ss->hEdtHk[g_ss->captureTarget], buf);
                    wcsncpy(g_ss->draft.modes[g_ss->captureTarget].hotkey, buf, MAX_HOTKEY_LEN - 1);
                    SetFocus(g_ss->hEdtHk[g_ss->captureTarget]);
                }
            }
        }
        DestroyWindow(h);
        return 0;
    }
    case WM_DESTROY:
        if (g_ss && g_ss->ctx) g_ss->ctx->hwndCapture = NULL;
        return 0;
    }
    return DefWindowProcW(h, m, w, l);
}

static void OpenCaptureWindow(HWND parent, int idx) {
    if (!g_ss || !g_ss->ctx) return;
    if (g_ss->ctx->hwndCapture) {
        SetForegroundWindow(g_ss->ctx->hwndCapture);
        return;
    }
    g_ss->captureTarget = idx;

    /* 同步当前编辑框内的热键到草稿 */
    for (int i = 0; i < MAX_MODES; i++) {
        GetWindowTextW(g_ss->hEdtHk[i], g_ss->draft.modes[i].hotkey, MAX_HOTKEY_LEN);
    }

    g_ss->ctx->hwndCapture = CreateWindowW(
        L"fccapture",
        Strings_Get()->capture_title,
        WS_OVERLAPPED | WS_CAPTION,
        CW_USEDEFAULT, CW_USEDEFAULT,
        DPI_Scale(290, g_ss->ctx->currentDpi),
        DPI_Scale(100, g_ss->ctx->currentDpi),
        parent, NULL,
        g_ss->ctx->hInstance, NULL
    );

    ShowWindow(g_ss->ctx->hwndCapture, SW_SHOW);
    UpdateWindow(g_ss->ctx->hwndCapture);
    SetFocus(g_ss->ctx->hwndCapture);
}

/* 原子提交草稿 (Q4-A) */
static void ApplySettings(HWND hwnd) {
    if (!g_ss || !g_ss->ctx) return;
    AppContext *ctx = g_ss->ctx;

    for (int i = 0; i < MAX_MODES; i++) {
        g_ss->draft.modes[i].enabled = (SendMessage(g_ss->hChkMode[i], BM_GETCHECK, 0, 0) == BST_CHECKED);
        GetWindowTextW(g_ss->hEdtCfg[i], g_ss->draft.modes[i].config, MAX_CONFIG_PATH);
        GetWindowTextW(g_ss->hEdtHk[i], g_ss->draft.modes[i].hotkey, MAX_HOTKEY_LEN);
    }

    g_ss->draft.autostart = (SendMessage(g_ss->hChkStartup, BM_GETCHECK, 0, 0) == BST_CHECKED);
    g_ss->draft.showTray = (SendMessage(g_ss->hChkTray, BM_GETCHECK, 0, 0) == BST_CHECKED);
    g_ss->draft.autoSwitch = (SendMessage(g_ss->hChkAutoSwitch, BM_GETCHECK, 0, 0) == BST_CHECKED);
    GetWindowTextW(g_ss->hFcPathEdt, g_ss->draft.fanControlExe, MAX_PATH);
    g_ss->draft.fcPathUserSet = (SendMessage(g_ss->hChkFcAutoDetect, BM_GETCHECK, 0, 0) != BST_CHECKED);

    /* 原子应用到运行态 */
    Config_Clone(&g_ss->draft, &ctx->config);
    Config_SetAutostart(ctx->config.autostart);
    Config_Save(ctx);

    ctx->lastAutoMode = -1;
    Hotkey_RegisterAll(ctx);

    /* 通知主窗口刷新按钮和托盘 */
    PostMessage(ctx->hwndMain, WM_COMMAND, MAKEWPARAM(IDC_SETTINGS, 0), 0);

    DestroyWindow(hwnd);
}

static LRESULT CALLBACK SettingsWndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    switch (m) {
    case WM_CREATE: {
        CREATESTRUCTW *cs = (CREATESTRUCTW *)l;
        AppContext *ctx = (AppContext *)cs->lpCreateParams;
        const AppStrings *s = Strings_Get();
        int dpi = ctx ? ctx->currentDpi : 96;

        g_ss = (SettingsState *)malloc(sizeof(SettingsState));
        memset(g_ss, 0, sizeof(SettingsState));
        g_ss->ctx = ctx;
        Config_Clone(&ctx->config, &g_ss->draft);

        int y = DPI_Scale(12, dpi);

        /* 通用选项 */
        CreateWindowW(L"STATIC", s->general, WS_CHILD | WS_VISIBLE | SS_LEFT,
                      DPI_Scale(12, dpi), y, DPI_Scale(200, dpi), DPI_Scale(22, dpi), h, NULL, cs->hInstance, NULL);
        y += DPI_Scale(26, dpi);

        g_ss->hChkStartup = CreateWindowW(L"BUTTON", s->autostart, WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                                          DPI_Scale(22, dpi), y, DPI_Scale(200, dpi), DPI_Scale(26, dpi), h,
                                          (HMENU)IDC_CHK_STARTUP, cs->hInstance, NULL);
        SendMessage(g_ss->hChkStartup, BM_SETCHECK, Config_IsAutostartEnabled() ? BST_CHECKED : BST_UNCHECKED, 0);
        y += DPI_Scale(30, dpi);

        g_ss->hChkTray = CreateWindowW(L"BUTTON", s->show_tray, WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                                       DPI_Scale(22, dpi), y, DPI_Scale(200, dpi), DPI_Scale(26, dpi), h,
                                       (HMENU)IDC_CHK_TRAY, cs->hInstance, NULL);
        SendMessage(g_ss->hChkTray, BM_SETCHECK, g_ss->draft.showTray ? BST_CHECKED : BST_UNCHECKED, 0);
        y += DPI_Scale(30, dpi);

        g_ss->hChkAutoSwitch = CreateWindowW(L"BUTTON", s->auto_switch, WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                                             DPI_Scale(22, dpi), y, DPI_Scale(260, dpi), DPI_Scale(26, dpi), h,
                                             (HMENU)IDC_CHK_AUTOSWITCH, cs->hInstance, NULL);
        SendMessage(g_ss->hChkAutoSwitch, BM_SETCHECK, g_ss->draft.autoSwitch ? BST_CHECKED : BST_UNCHECKED, 0);
        y += DPI_Scale(32, dpi);

        CreateWindowW(L"STATIC", s->fancontrol_path, WS_CHILD | WS_VISIBLE | SS_LEFT,
                      DPI_Scale(12, dpi), y, DPI_Scale(200, dpi), DPI_Scale(20, dpi), h, NULL, cs->hInstance, NULL);
        y += DPI_Scale(24, dpi);

        g_ss->hFcPathEdt = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                                           DPI_Scale(22, dpi), y, DPI_Scale(350, dpi), DPI_Scale(26, dpi), h,
                                           (HMENU)IDC_FC_PATH_EDT, cs->hInstance, NULL);
        SetWindowTextW(g_ss->hFcPathEdt, g_ss->draft.fanControlExe);

        g_ss->hFcPathBrw = CreateWindowW(L"BUTTON", s->browse, WS_CHILD | WS_VISIBLE,
                                         DPI_Scale(378, dpi), y, DPI_Scale(95, dpi), DPI_Scale(26, dpi), h,
                                         (HMENU)IDC_FC_PATH_BRW, cs->hInstance, NULL);
        y += DPI_Scale(28, dpi);

        g_ss->hChkFcAutoDetect = CreateWindowW(L"BUTTON", s->auto_detect, WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                                               DPI_Scale(22, dpi), y, DPI_Scale(150, dpi), DPI_Scale(24, dpi), h,
                                               (HMENU)IDC_CHK_FC_AUTODETECT, cs->hInstance, NULL);
        SendMessage(g_ss->hChkFcAutoDetect, BM_SETCHECK, g_ss->draft.fcPathUserSet ? BST_UNCHECKED : BST_CHECKED, 0);
        y += DPI_Scale(34, dpi);

        /* 模式配置 */
        CreateWindowW(L"STATIC", s->mode_config, WS_CHILD | WS_VISIBLE | SS_LEFT,
                      DPI_Scale(12, dpi), y, DPI_Scale(200, dpi), DPI_Scale(22, dpi), h, NULL, cs->hInstance, NULL);
        y += DPI_Scale(26, dpi);

        for (int i = 0; i < MAX_MODES; i++) {
            wchar_t chkLabel[64];
            _snwprintf(chkLabel, 64, s->enable_fmt, g_ss->draft.modes[i].name);
            g_ss->hChkMode[i] = CreateWindowW(L"BUTTON", chkLabel, WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                                              DPI_Scale(22, dpi), y, DPI_Scale(200, dpi), DPI_Scale(24, dpi), h,
                                              (HMENU)(INT_PTR)(IDC_CHK_BASE + i), cs->hInstance, NULL);
            SendMessage(g_ss->hChkMode[i], BM_SETCHECK, g_ss->draft.modes[i].enabled ? BST_CHECKED : BST_UNCHECKED, 0);
            y += DPI_Scale(26, dpi);

            CreateWindowW(L"STATIC", s->config_file, WS_CHILD | WS_VISIBLE | SS_LEFT,
                          DPI_Scale(32, dpi), y + DPI_Scale(2, dpi), DPI_Scale(85, dpi), DPI_Scale(20, dpi), h, NULL, cs->hInstance, NULL);

            g_ss->hEdtCfg[i] = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                                               DPI_Scale(120, dpi), y, DPI_Scale(255, dpi), DPI_Scale(26, dpi), h, NULL, cs->hInstance, NULL);
            SetWindowTextW(g_ss->hEdtCfg[i], g_ss->draft.modes[i].config);

            g_ss->hBtnBrw[i] = CreateWindowW(L"BUTTON", s->browse, WS_CHILD | WS_VISIBLE,
                                             DPI_Scale(385, dpi), y, DPI_Scale(95, dpi), DPI_Scale(28, dpi), h,
                                             (HMENU)(INT_PTR)(IDC_BROWSE_BASE + i), cs->hInstance, NULL);
            y += DPI_Scale(30, dpi);

            CreateWindowW(L"STATIC", s->hotkey, WS_CHILD | WS_VISIBLE | SS_LEFT,
                          DPI_Scale(32, dpi), y + DPI_Scale(2, dpi), DPI_Scale(85, dpi), DPI_Scale(20, dpi), h, NULL, cs->hInstance, NULL);

            g_ss->hEdtHk[i] = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                                              DPI_Scale(120, dpi), y, DPI_Scale(110, dpi), DPI_Scale(26, dpi), h, NULL, cs->hInstance, NULL);
            SetWindowTextW(g_ss->hEdtHk[i], g_ss->draft.modes[i].hotkey);

            g_ss->hBtnCap[i] = CreateWindowW(L"BUTTON", s->capture_btn, WS_CHILD | WS_VISIBLE,
                                             DPI_Scale(235, dpi), y, DPI_Scale(75, dpi), DPI_Scale(26, dpi), h,
                                             (HMENU)(INT_PTR)(IDC_CAPTURE_BASE + i), cs->hInstance, NULL);
            y += DPI_Scale(34, dpi);
        }

        /* 进程感知配置 (右侧面板) */
        int ry = DPI_Scale(12, dpi);
        CreateWindowW(L"STATIC", s->process_config, WS_CHILD | WS_VISIBLE | SS_LEFT,
                      DPI_Scale(510, dpi), ry, DPI_Scale(200, dpi), DPI_Scale(22, dpi), h, NULL, cs->hInstance, NULL);
        ry += DPI_Scale(26, dpi);

        for (int i = 0; i < MAX_MODES; i++) {
            wchar_t title[64];
            _snwprintf(title, 64, L"%s %s", g_ss->draft.modes[i].name, s->process_config);
            CreateWindowW(L"STATIC", title, WS_CHILD | WS_VISIBLE | SS_LEFT,
                          DPI_Scale(510, dpi), ry, DPI_Scale(310, dpi), DPI_Scale(20, dpi), h, NULL, cs->hInstance, NULL);
            ry += DPI_Scale(22, dpi);

            g_ss->hProcList[i] = CreateWindowW(L"LISTBOX", L"",
                                               WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY | LBS_STANDARD,
                                               DPI_Scale(510, dpi), ry, DPI_Scale(190, dpi), DPI_Scale(70, dpi), h,
                                               (HMENU)(INT_PTR)(IDC_PROC_LIST_BASE + i), cs->hInstance, NULL);
            RefreshProcessList(i);

            g_ss->hProcAdd[i] = CreateWindowW(L"BUTTON", s->add, WS_CHILD | WS_VISIBLE,
                                              DPI_Scale(710, dpi), ry, DPI_Scale(80, dpi), DPI_Scale(26, dpi), h,
                                              (HMENU)(INT_PTR)(IDC_PROC_ADD_BASE + i), cs->hInstance, NULL);

            g_ss->hProcDel[i] = CreateWindowW(L"BUTTON", s->del, WS_CHILD | WS_VISIBLE,
                                              DPI_Scale(710, dpi), ry + DPI_Scale(30, dpi), DPI_Scale(80, dpi), DPI_Scale(26, dpi), h,
                                              (HMENU)(INT_PTR)(IDC_PROC_DEL_BASE + i), cs->hInstance, NULL);

            ry += DPI_Scale(74, dpi);

            g_ss->hProcEdt[i] = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                                                DPI_Scale(510, dpi), ry, DPI_Scale(190, dpi), DPI_Scale(24, dpi), h,
                                                (HMENU)(INT_PTR)(IDC_PROC_EDT_BASE + i), cs->hInstance, NULL);

            g_ss->hProcBrw[i] = CreateWindowW(L"BUTTON", s->browse, WS_CHILD | WS_VISIBLE,
                                              DPI_Scale(710, dpi), ry, DPI_Scale(80, dpi), DPI_Scale(24, dpi), h,
                                              (HMENU)(INT_PTR)(IDC_PROC_BRW_BASE + i), cs->hInstance, NULL);
            ry += DPI_Scale(32, dpi);
        }

        /* 底部 OK / Cancel 按钮 */
        int by = (y > ry ? y : ry) + DPI_Scale(8, dpi);
        CreateWindowW(L"BUTTON", s->ok, WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                      DPI_Scale(300, dpi), by, DPI_Scale(95, dpi), DPI_Scale(32, dpi), h, (HMENU)IDC_OK, cs->hInstance, NULL);

        CreateWindowW(L"BUTTON", s->cancel, WS_CHILD | WS_VISIBLE,
                      DPI_Scale(410, dpi), by, DPI_Scale(95, dpi), DPI_Scale(32, dpi), h, (HMENU)IDC_CANCEL, cs->hInstance, NULL);

        if (ctx) EnumChildWindows(h, (WNDENUMPROC)SetChildFontProc, (LPARAM)ctx->hFont);
        UpdateBrowseState();
        UpdateProcessControlsState();
        return 0;
    }
    case WM_COMMAND: {
        int id = LOWORD(w);
        if (HIWORD(w) == BN_CLICKED) {
            if (id == IDC_OK) {
                ApplySettings(h);
            } else if (id == IDC_CANCEL) {
                /* Cancel 直接销毁草稿退出，零副作用 (Q4-A) */
                DestroyWindow(h);
            } else if (id == IDC_FC_PATH_BRW) {
                OpenFcPathBrowseDialog(h);
            } else if (id == IDC_CHK_AUTOSWITCH) {
                UpdateProcessControlsState();
            } else {
                for (int i = 0; i < MAX_MODES; i++) {
                    if (id == IDC_CHK_BASE + i) {
                        UpdateBrowseState();
                        break;
                    } else if (id == IDC_BROWSE_BASE + i) {
                        OpenBrowseDialog(h, i);
                        break;
                    } else if (id == IDC_CAPTURE_BASE + i) {
                        OpenCaptureWindow(h, i);
                        break;
                    } else if (id == IDC_PROC_ADD_BASE + i) {
                        AddProcessFromEdit(i);
                        break;
                    } else if (id == IDC_PROC_DEL_BASE + i) {
                        DeleteSelectedProcess(i);
                        break;
                    } else if (id == IDC_PROC_BRW_BASE + i) {
                        OpenExeBrowseDialog(h, i);
                        break;
                    }
                }
            }
        }
        return 0;
    }
    case WM_DESTROY:
        if (g_ss) {
            if (g_ss->ctx) g_ss->ctx->hwndSettings = NULL;
            free(g_ss);
            g_ss = NULL;
        }
        return 0;
    }
    return DefWindowProcW(h, m, w, l);
}

void UISettings_Open(AppContext *ctx) {
    if (!ctx) return;
    if (ctx->hwndSettings) {
        ShowWindow(ctx->hwndSettings, SW_SHOW);
        SetForegroundWindow(ctx->hwndSettings);
        return;
    }

    WNDCLASSW wcSet;
    memset(&wcSet, 0, sizeof(wcSet));
    wcSet.lpfnWndProc = SettingsWndProc;
    wcSet.hInstance = ctx->hInstance;
    wcSet.hIcon = LoadIconW(ctx->hInstance, MAKEINTRESOURCEW(101));
    wcSet.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcSet.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wcSet.lpszClassName = L"fcsettings";
    RegisterClassW(&wcSet);

    WNDCLASSW wcCap;
    memset(&wcCap, 0, sizeof(wcCap));
    wcCap.lpfnWndProc = CaptureWndProc;
    wcCap.hInstance = ctx->hInstance;
    wcCap.hIcon = LoadIconW(ctx->hInstance, MAKEINTRESOURCEW(101));
    wcCap.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcCap.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wcCap.lpszClassName = L"fccapture";
    RegisterClassW(&wcCap);

    int dpi = ctx->currentDpi;
    ctx->hwndSettings = CreateWindowW(
        L"fcsettings",
        Strings_Get()->settings,
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT,
        DPI_Scale(830, dpi),
        DPI_Scale(680, dpi),
        ctx->hwndMain, NULL,
        ctx->hInstance,
        ctx
    );

    ShowWindow(ctx->hwndSettings, SW_SHOW);
    UpdateWindow(ctx->hwndSettings);
}
