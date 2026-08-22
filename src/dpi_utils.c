#include "dpi_utils.h"

typedef UINT (WINAPI *GetDpiForWindowFunc)(HWND);

int DPI_GetForWindow(HWND hwnd) {
    HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
    if (hUser32) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
        GetDpiForWindowFunc pGetDpiForWindow = (GetDpiForWindowFunc)(void *)GetProcAddress(hUser32, "GetDpiForWindow");
#pragma GCC diagnostic pop
        if (pGetDpiForWindow && hwnd) {
            UINT dpi = pGetDpiForWindow(hwnd);
            if (dpi != 0) return (int)dpi;
        }
    }
    HDC hdc = GetDC(hwnd);
    int dpi = 96;
    if (hdc) {
        dpi = GetDeviceCaps(hdc, LOGPIXELSY);
        ReleaseDC(hwnd, hdc);
    }
    return (dpi > 0) ? dpi : 96;
}

int DPI_Scale(int value, int dpi) {
    if (dpi <= 0) dpi = 96;
    return MulDiv(value, dpi, 96);
}

HFONT DPI_CreateAppFont(int dpi, const wchar_t *fontName, int pointSize) {
    int height = -MulDiv(pointSize, dpi, 72);
    return CreateFontW(
        height, 0, 0, 0,
        FW_NORMAL,
        FALSE, FALSE, FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        fontName ? fontName : L"Microsoft YaHei UI"
    );
}
