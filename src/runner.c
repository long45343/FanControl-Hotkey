#include <windows.h>
#include <stdio.h>
#include "runner.h"
#include "strings.h"

int Runner_SwitchTo(AppContext *ctx, int modeIndex, int silent) {
    if (!ctx || modeIndex < 0 || modeIndex >= MAX_MODES) return 0;
    const AppStrings *s = Strings_Get();
    ModeConfig *m = &ctx->config.modes[modeIndex];

    if (!m->enabled) {
        if (!silent && ctx->hwndMain) {
            MessageBoxW(ctx->hwndMain, s->not_enabled_msg, s->notice, MB_OK | MB_ICONINFORMATION);
        }
        return 0;
    }

    if (!m->config[0]) {
        if (!silent && ctx->hwndMain) {
            wchar_t msg[128];
            _snwprintf(msg, 128, s->not_configured_fmt, m->name);
            MessageBoxW(ctx->hwndMain, msg, s->notice, MB_OK | MB_ICONINFORMATION);
        }
        return 0;
    }

    /* 检查 FanControl.exe 是否存在 */
    DWORD attr = GetFileAttributesW(ctx->config.fanControlExe);
    if (attr == INVALID_FILE_ATTRIBUTES || (attr & FILE_ATTRIBUTE_DIRECTORY)) {
        if (!silent && ctx->hwndMain) {
            wchar_t msg[MAX_PATH + 128];
            _snwprintf(msg, sizeof(msg) / sizeof(msg[0]), s->fancontrol_not_found_fmt, ctx->config.fanControlExe);
            MessageBoxW(ctx->hwndMain, msg, s->notice, MB_OK | MB_ICONERROR);
        }
        return 0;
    }

    /* 构造命令行: "C:\path\to\FanControl.exe" -c "C:\path\to\config.json" */
    wchar_t cmdLine[MAX_PATH * 2 + 64];
    _snwprintf(cmdLine, sizeof(cmdLine) / sizeof(cmdLine[0]),
               L"\"%s\" -c \"%s\"", ctx->config.fanControlExe, m->config);

    STARTUPINFOW si;
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi;
    memset(&pi, 0, sizeof(pi));

    /* 使用 CreateProcessW 直接安全启动子进程，彻底消除 ShellExecute 注入与歧义 (Q2-A) */
    BOOL success = CreateProcessW(
        NULL,
        cmdLine,
        NULL,
        NULL,
        FALSE,
        CREATE_NO_WINDOW,
        NULL,
        NULL,
        &si,
        &pi
    );

    if (success) {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        return 1;
    }

    if (!silent && ctx->hwndMain) {
        wchar_t msg[MAX_PATH + 128];
        _snwprintf(msg, sizeof(msg) / sizeof(msg[0]), s->fancontrol_not_found_fmt, ctx->config.fanControlExe);
        MessageBoxW(ctx->hwndMain, msg, s->notice, MB_OK | MB_ICONERROR);
    }
    return 0;
}
