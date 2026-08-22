#ifndef HOTKEY_H
#define HOTKEY_H

#include "app_context.h"

int Hotkey_Parse(const wchar_t *str, UINT *mods, UINT *vk);
void Hotkey_VkToString(UINT vk, wchar_t *out, int len);
void Hotkey_UnregisterAll(AppContext *ctx);
void Hotkey_RegisterAll(AppContext *ctx);
int Hotkey_CheckConflict(const AppConfig *config, int targetModeIndex, UINT mods, UINT vk);

#endif
