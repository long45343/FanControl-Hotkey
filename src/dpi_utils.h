#ifndef DPI_UTILS_H
#define DPI_UTILS_H

#include <windows.h>

int DPI_GetForWindow(HWND hwnd);
int DPI_Scale(int value, int dpi);
HFONT DPI_CreateAppFont(int dpi, const wchar_t *fontName, int pointSize);

#endif
