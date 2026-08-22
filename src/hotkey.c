#include <windows.h>
#include <stdio.h>
#include <ctype.h>
#include <wchar.h>
#include "hotkey.h"
#include "strings.h"

int Hotkey_Parse(const wchar_t *str, UINT *mods, UINT *vk) {
    if (!str || !mods || !vk) return 0;
    UINT m = 0;
    const wchar_t *p = str;
    wchar_t tok[32];

    while (1) {
        const wchar_t *plus = wcschr(p, L'+');
        int len = plus ? (int)(plus - p) : (int)wcslen(p);
        if (len <= 0 || len >= 32) return 0;
        wcsncpy(tok, p, len);
        tok[len] = 0;

        if (!plus) {
            if (wcslen(tok) == 1) {
                *vk = towupper(tok[0]);
            } else if (towupper(tok[0]) == L'F' && iswdigit(tok[1])) {
                int n = _wtoi(tok + 1);
                if (n < 1 || n > 12) return 0;
                *vk = VK_F1 + n - 1;
            } else {
                return 0;
            }
            *mods = m;
            return 1;
        }

        if (!_wcsicmp(tok, L"Ctrl") || !_wcsicmp(tok, L"Control")) m |= MOD_CONTROL;
        else if (!_wcsicmp(tok, L"Alt")) m |= MOD_ALT;
        else if (!_wcsicmp(tok, L"Shift")) m |= MOD_SHIFT;
        else if (!_wcsicmp(tok, L"Win")) m |= MOD_WIN;
        else return 0;

        p = plus + 1;
    }
}

void Hotkey_VkToString(UINT vk, wchar_t *out, int len) {
    if (!out || len <= 0) return;
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

void Hotkey_UnregisterAll(AppContext *ctx) {
    if (!ctx || !ctx->hwndMain) return;
    for (int i = 0; i < MAX_MODES; i++) {
        if (ctx->config.modes[i].hotkeyId) {
            UnregisterHotKey(ctx->hwndMain, ctx->config.modes[i].hotkeyId);
            ctx->config.modes[i].hotkeyId = 0;
        }
    }
}

void Hotkey_RegisterAll(AppContext *ctx) {
    if (!ctx || !ctx->hwndMain) return;
    Hotkey_UnregisterAll(ctx);
    const AppStrings *s = Strings_Get();

    for (int i = 0; i < MAX_MODES; i++) {
        ModeConfig *m = &ctx->config.modes[i];
        if (!m->enabled || !m->config[0]) continue;

        UINT mods, vk;
        if (Hotkey_Parse(m->hotkey, &mods, &vk)) {
            m->hotkeyId = i + 1;
            if (!RegisterHotKey(ctx->hwndMain, m->hotkeyId, mods, vk)) {
                m->hotkeyId = 0;
                wchar_t msg[128];
                _snwprintf(msg, 128, s->hotkey_register_failed_fmt, m->hotkey);
                MessageBoxW(ctx->hwndMain, msg, s->notice, MB_OK | MB_ICONWARNING);
            }
        }
    }
}

int Hotkey_CheckConflict(const AppConfig *config, int targetModeIndex, UINT mods, UINT vk) {
    if (!config) return 0;
    for (int i = 0; i < MAX_MODES; i++) {
        if (i == targetModeIndex) continue;
        UINT exMods, exVk;
        if (Hotkey_Parse(config->modes[i].hotkey, &exMods, &exVk)) {
            if (exMods == mods && exVk == vk) return 1;
        }
    }
    return 0;
}
