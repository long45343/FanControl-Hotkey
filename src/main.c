#include <windows.h>
#include "app_context.h"
#include "strings.h"
#include "config.h"
#include "dpi_utils.h"
#include "ui_main.h"

static int FindExistingWindow(void) {
    HWND h = FindWindowW(L"fcgui", NULL);
    if (h) {
        ShowWindow(h, IsIconic(h) ? SW_RESTORE : SW_SHOW);
        SetForegroundWindow(h);
        return 1;
    }
    return 0;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nShowCmd) {
    (void)hPrevInstance;

    /* 单实例互斥量保护 */
    HANDLE hMutex = CreateMutexW(NULL, TRUE, MUTEX_NAME);
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        if (hMutex) CloseHandle(hMutex);
        FindExistingWindow();
        return 0;
    }

    Strings_Init();

    AppContext ctx;
    memset(&ctx, 0, sizeof(AppContext));
    ctx.hInstance = hInstance;
    ctx.hMutex = hMutex;
    ctx.lastAutoMode = -1;

    int dpi = DPI_GetForWindow(NULL);
    ctx.currentDpi = dpi;
    ctx.hFont = DPI_CreateAppFont(dpi, L"Microsoft YaHei UI", 10);

    Config_InitDefaults(&ctx.config);
    Config_Load(&ctx);

    int startedByAutorun = (lpCmdLine && wcsstr(lpCmdLine, AUTORUN_FLAG) != NULL);
    int show = SW_SHOW;
    if (startedByAutorun) {
        show = SW_HIDE;
    } else if (nShowCmd != SW_HIDE && nShowCmd != SW_SHOWMINIMIZED) {
        show = SW_SHOW;
    }

    if (!UIMain_Init(&ctx)) {
        UIMain_Cleanup(&ctx);
        return 1;
    }

    int ret = UIMain_RunLoop(&ctx, show);
    UIMain_Cleanup(&ctx);
    return ret;
}
