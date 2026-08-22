#include <windows.h>
#include <shellapi.h>
#include <stdio.h>
#include "ui_main.h"
#include "ui_settings.h"
#include "hotkey.h"
#include "process_monitor.h"
#include "runner.h"
#include "dpi_utils.h"
#include "strings.h"

static HWND g_btnMain[MAX_MODES];
static AppContext *g_mainCtx = NULL;

static BOOL CALLBACK SetChildFontProc(HWND h, LPARAM l) {
    SendMessage(h, WM_SETFONT, l, TRUE);
    return TRUE;
}

void UIMain_RefreshButtons(AppContext *ctx) {
    if (!ctx || !ctx->hwndMain) return;
    const AppStrings *s = Strings_Get();

    for (int i = 0; i < MAX_MODES; i++) {
        const ModeConfig *m = &ctx->config.modes[i];
        wchar_t label[96];
        if (!m->enabled) {
            _snwprintf(label, 96, L"%s%s", m->name, s->disabled_suffix);
        } else if (!m->config[0]) {
            _snwprintf(label, 96, L"%s%s", m->name, s->not_configured_suffix);
        } else if (m->hotkey[0]) {
            _snwprintf(label, 96, L"%s (%s)", m->name, m->hotkey);
        } else {
            _snwprintf(label, 96, L"%s", m->name);
        }
        SetWindowTextW(g_btnMain[i], label);
    }
}

static void AddTrayIcon(AppContext *ctx) {
    if (!ctx || !ctx->config.showTray || ctx->trayAdded) return;
    NOTIFYICONDATAW nid;
    memset(&nid, 0, sizeof(nid));
    nid.cbSize = sizeof(nid);
    nid.hWnd = ctx->hwndMain;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = LoadIconW(ctx->hInstance, MAKEINTRESOURCEW(101));
    wcsncpy(nid.szTip, Strings_Get()->app_title, 127);
    Shell_NotifyIconW(NIM_ADD, &nid);
    ctx->trayAdded = 1;
}

static void RemoveTrayIcon(AppContext *ctx) {
    if (!ctx || !ctx->trayAdded) return;
    NOTIFYICONDATAW nid;
    memset(&nid, 0, sizeof(nid));
    nid.cbSize = sizeof(nid);
    nid.hWnd = ctx->hwndMain;
    nid.uID = 1;
    Shell_NotifyIconW(NIM_DELETE, &nid);
    ctx->trayAdded = 0;
}

void UIMain_ApplyTraySetting(AppContext *ctx) {
    if (!ctx) return;
    if (ctx->config.showTray) AddTrayIcon(ctx);
    else RemoveTrayIcon(ctx);
}

static void ShowTrayMenu(AppContext *ctx) {
    if (!ctx) return;
    POINT pt;
    GetCursorPos(&pt);
    HMENU menu = CreatePopupMenu();
    const AppStrings *s = Strings_Get();
    AppendMenuW(menu, MF_STRING, IDM_TRAY_SHOW, s->tray_show);
    AppendMenuW(menu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(menu, MF_STRING, IDM_TRAY_EXIT, s->tray_exit);

    SetForegroundWindow(ctx->hwndMain);
    TrackPopupMenu(menu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, ctx->hwndMain, NULL);
    DestroyMenu(menu);
}

static LRESULT CALLBACK MainWndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    AppContext *ctx = g_mainCtx;
    switch (m) {
    case WM_HOTKEY: {
        int id = (int)w;
        for (int i = 0; i < MAX_MODES; i++) {
            if (ctx && ctx->config.modes[i].hotkeyId == id) {
                Runner_SwitchTo(ctx, i, 0);
                break;
            }
        }
        return 0;
    }
    case WM_COMMAND: {
        int id = LOWORD(w);
        if (id == IDC_SETTINGS) {
            if (HIWORD(w) == 0 && l == 0) {
                /* 由 Settings Apply 触发的刷新通知 */
                UIMain_RefreshButtons(ctx);
                UIMain_ApplyTraySetting(ctx);
            } else {
                UISettings_Open(ctx);
            }
        } else if (id == IDC_EXIT || id == IDM_TRAY_EXIT) {
            DestroyWindow(h);
        } else if (id == IDM_TRAY_SHOW) {
            ShowWindow(h, SW_SHOW);
            SetForegroundWindow(h);
        } else {
            for (int i = 0; i < MAX_MODES; i++) {
                if (id == IDC_BTN_BASE + i) {
                    Runner_SwitchTo(ctx, i, 0);
                    break;
                }
            }
        }
        return 0;
    }
    case WM_TRAYICON:
        if (l == WM_LBUTTONDBLCLK) {
            ShowWindow(h, SW_SHOW);
            SetForegroundWindow(h);
        } else if (l == WM_RBUTTONUP) {
            ShowTrayMenu(ctx);
        }
        return 0;
    case WM_CLOSE:
        ShowWindow(h, SW_HIDE);
        return 0;
    case WM_TIMER:
        if (w == TIMER_POLL_ID) {
            ProcessMonitor_CheckAndTrigger(ctx);
        }
        return 0;
    case WM_DESTROY:
        KillTimer(h, TIMER_POLL_ID);
        RemoveTrayIcon(ctx);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(h, m, w, l);
}

int UIMain_Init(AppContext *ctx) {
    if (!ctx) return 0;
    g_mainCtx = ctx;

    WNDCLASSW wcMain;
    memset(&wcMain, 0, sizeof(wcMain));
    wcMain.lpfnWndProc = MainWndProc;
    wcMain.hInstance = ctx->hInstance;
    wcMain.hIcon = LoadIconW(ctx->hInstance, MAKEINTRESOURCEW(101));
    wcMain.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcMain.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wcMain.lpszClassName = L"fcgui";
    RegisterClassW(&wcMain);

    int dpi = ctx->currentDpi;
    ctx->hwndMain = CreateWindowW(
        L"fcgui",
        Strings_Get()->app_title,
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT,
        DPI_Scale(290, dpi),
        DPI_Scale(310, dpi),
        NULL, NULL,
        ctx->hInstance, NULL
    );

    if (!ctx->hwndMain) return 0;

    int y = DPI_Scale(12, dpi);
    for (int i = 0; i < MAX_MODES; i++) {
        g_btnMain[i] = CreateWindowW(
            L"BUTTON", L"",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            DPI_Scale(20, dpi), y,
            DPI_Scale(240, dpi), DPI_Scale(45, dpi),
            ctx->hwndMain,
            (HMENU)(INT_PTR)(IDC_BTN_BASE + i),
            ctx->hInstance, NULL
        );
        y += DPI_Scale(52, dpi);
    }

    CreateWindowW(
        L"BUTTON", Strings_Get()->settings,
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        DPI_Scale(20, dpi), y + DPI_Scale(4, dpi),
        DPI_Scale(115, dpi), DPI_Scale(36, dpi),
        ctx->hwndMain, (HMENU)IDC_SETTINGS,
        ctx->hInstance, NULL
    );

    CreateWindowW(
        L"BUTTON", Strings_Get()->exit_btn,
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        DPI_Scale(145, dpi), y + DPI_Scale(4, dpi),
        DPI_Scale(115, dpi), DPI_Scale(36, dpi),
        ctx->hwndMain, (HMENU)IDC_EXIT,
        ctx->hInstance, NULL
    );

    EnumChildWindows(ctx->hwndMain, (WNDENUMPROC)SetChildFontProc, (LPARAM)ctx->hFont);

    UIMain_RefreshButtons(ctx);
    UIMain_ApplyTraySetting(ctx);
    Hotkey_RegisterAll(ctx);
    SetTimer(ctx->hwndMain, TIMER_POLL_ID, POLL_INTERVAL_MS, NULL);
    ProcessMonitor_CheckAndTrigger(ctx);

    return 1;
}

int UIMain_RunLoop(AppContext *ctx, int initialShow) {
    if (!ctx || !ctx->hwndMain) return 0;

    ShowWindow(ctx->hwndMain, initialShow);
    UpdateWindow(ctx->hwndMain);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return (int)msg.wParam;
}

void UIMain_Cleanup(AppContext *ctx) {
    if (!ctx) return;
    Hotkey_UnregisterAll(ctx);
    RemoveTrayIcon(ctx);
    if (ctx->hFont) {
        DeleteObject(ctx->hFont);
        ctx->hFont = NULL;
    }
    if (ctx->hMutex) {
        CloseHandle(ctx->hMutex);
        ctx->hMutex = NULL;
    }
}
