#include <windows.h>
#include <shlobj.h>
#include <stdio.h>
#include <string.h>
#include "config.h"
#include "strings.h"

static int IsDirectoryWritable(const wchar_t *dirPath) {
    if (!dirPath || !dirPath[0]) return 0;
    wchar_t testFile[MAX_PATH];
    _snwprintf(testFile, MAX_PATH, L"%s\\_fc_perm_test.tmp", dirPath);
    HANDLE hFile = CreateFileW(testFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, NULL);
    if (hFile != INVALID_HANDLE_VALUE) {
        CloseHandle(hFile);
        DeleteFileW(testFile);
        return 1;
    }
    return 0;
}

void Config_GetIniPath(wchar_t *outPath, int maxLen) {
    if (!outPath || maxLen <= 0) return;

    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);

    /* 1. 尝试便携式同级目录 INI */
    wchar_t localIni[MAX_PATH];
    wcsncpy(localIni, exePath, MAX_PATH - 1);
    localIni[MAX_PATH - 1] = 0;
    wchar_t *dot = wcsrchr(localIni, L'.');
    if (dot) wcscpy(dot, L".ini");
    else wcsncat(localIni, L".ini", MAX_PATH - wcslen(localIni) - 1);

    /* 获取 exe 所在目录 */
    wchar_t exeDir[MAX_PATH];
    wcsncpy(exeDir, exePath, MAX_PATH - 1);
    exeDir[MAX_PATH - 1] = 0;
    wchar_t *slash = wcsrchr(exeDir, L'\\');
    if (slash) *slash = 0;

    /* 如果同级 INI 存在，直接优先使用 */
    if (GetFileAttributesW(localIni) != INVALID_FILE_ATTRIBUTES) {
        wcsncpy(outPath, localIni, maxLen - 1);
        outPath[maxLen - 1] = 0;
        return;
    }

    /* 如果同级目录可写（如便携运行于用户目录），使用同级 INI */
    if (IsDirectoryWritable(exeDir)) {
        wcsncpy(outPath, localIni, maxLen - 1);
        outPath[maxLen - 1] = 0;
        return;
    }

    /* 2. 否则安全回退至 %APPDATA%\FanControlHotkey\config.ini */
    wchar_t appData[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, appData))) {
        wchar_t appDir[MAX_PATH];
        _snwprintf(appDir, MAX_PATH, L"%s\\FanControlHotkey", appData);
        CreateDirectoryW(appDir, NULL);
        _snwprintf(outPath, maxLen, L"%s\\config.ini", appDir);
        return;
    }

    /* 兜底 */
    wcsncpy(outPath, localIni, maxLen - 1);
    outPath[maxLen - 1] = 0;
}

void Config_ParseProcessList(ModeConfig *mode, const wchar_t *text) {
    if (!mode) return;
    mode->processCount = 0;
    if (!text || !text[0]) return;

    wchar_t buf[MAX_PROCESS_LIST_LEN];
    wcsncpy(buf, text, MAX_PROCESS_LIST_LEN - 1);
    buf[MAX_PROCESS_LIST_LEN - 1] = 0;

    wchar_t *ctx = NULL;
    wchar_t *tok = wcstok_s(buf, L",", &ctx);
    while (tok && mode->processCount < MAX_PROCESS_NAMES) {
        while (*tok == L' ' || *tok == L'\t') tok++;
        int len = (int)wcslen(tok);
        while (len > 0 && (tok[len - 1] == L' ' || tok[len - 1] == L'\t'))
            tok[--len] = 0;
        if (len > 0) {
            wcsncpy(mode->processNames[mode->processCount], tok, MAX_PROCESS_LEN - 1);
            mode->processNames[mode->processCount][MAX_PROCESS_LEN - 1] = 0;
            mode->processCount++;
        }
        tok = wcstok_s(NULL, L",", &ctx);
    }
}

void Config_BuildProcessList(const ModeConfig *mode, wchar_t *out, int outLen) {
    if (!out || outLen <= 0) return;
    out[0] = 0;
    if (!mode) return;

    int pos = 0;
    for (int i = 0; i < mode->processCount; i++) {
        int len = (int)wcslen(mode->processNames[i]);
        if (pos + len + 2 >= outLen) break;
        if (i > 0) out[pos++] = L',';
        wcscpy(out + pos, mode->processNames[i]);
        pos += len;
    }
    out[pos] = 0;
}

void Config_InitDefaults(AppConfig *config) {
    if (!config) return;
    memset(config, 0, sizeof(AppConfig));
    const AppStrings *s = Strings_Get();

    for (int i = 0; i < MAX_MODES; i++) {
        wcsncpy(config->modes[i].name, s->mode_names[i], 31);
        config->modes[i].config[0] = 0;
        _snwprintf(config->modes[i].hotkey, MAX_HOTKEY_LEN, L"Ctrl+Alt+%d", i + 1);
        config->modes[i].enabled = 1;
        config->modes[i].processCount = 0;
        config->modes[i].hotkeyId = 0;
    }

    config->autostart = 0;
    config->showTray = 1;
    config->autoSwitch = 1;
    config->fcPathUserSet = 0;
    wcsncpy(config->fanControlExe, FANCONTROL_EXE_DEFAULT, MAX_PATH - 1);
}

void Config_Clone(const AppConfig *src, AppConfig *dst) {
    if (!src || !dst) return;
    memcpy(dst, src, sizeof(AppConfig));
}

int Config_DetectFanControlPath(wchar_t *out, int outLen) {
    if (!out || outLen <= 0) return 0;
    const wchar_t *roots[] = {
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
        L"SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
        NULL
    };

    for (int r = 0; roots[r]; r++) {
        HKEY hUninstall;
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, roots[r], 0, KEY_READ, &hUninstall) != ERROR_SUCCESS)
            continue;

        int index = 0;
        while (1) {
            wchar_t subName[256];
            DWORD subLen = 256;
            LONG rc = RegEnumKeyExW(hUninstall, index, subName, &subLen, NULL, NULL, NULL, NULL);
            if (rc != ERROR_SUCCESS) break;
            index++;

            HKEY hSub;
            if (RegOpenKeyExW(hUninstall, subName, 0, KEY_READ, &hSub) != ERROR_SUCCESS)
                continue;

            wchar_t displayName[256];
            DWORD nameLen = sizeof(displayName);
            DWORD type;
            if (RegQueryValueExW(hSub, L"DisplayName", NULL, &type, (LPBYTE)displayName, &nameLen) == ERROR_SUCCESS
                && type == REG_SZ && wcsstr(displayName, L"FanControl") != NULL) {

                wchar_t loc[MAX_PATH];
                DWORD locLen = sizeof(loc);
                if (RegQueryValueExW(hSub, L"InstallLocation", NULL, &type, (LPBYTE)loc, &locLen) == ERROR_SUCCESS
                    && type == REG_SZ && loc[0]) {
                    int locLen2 = (int)wcslen(loc);
                    if (locLen2 > 0 && loc[locLen2 - 1] == L'\\')
                        _snwprintf(out, outLen, L"%sFanControl.exe", loc);
                    else
                        _snwprintf(out, outLen, L"%s\\FanControl.exe", loc);
                    RegCloseKey(hSub);
                    RegCloseKey(hUninstall);
                    return 1;
                }

                wchar_t uns[MAX_PATH * 2];
                DWORD unsLen = sizeof(uns);
                if (RegQueryValueExW(hSub, L"UninstallString", NULL, &type, (LPBYTE)uns, &unsLen) == ERROR_SUCCESS
                    && type == REG_SZ && uns[0]) {
                    wchar_t *p = uns;
                    if (*p == L'"') p++;
                    wchar_t *slash = wcsrchr(p, L'\\');
                    if (slash) {
                        *slash = 0;
                        _snwprintf(out, outLen, L"%s\\FanControl.exe", p);
                        RegCloseKey(hSub);
                        RegCloseKey(hUninstall);
                        return 1;
                    }
                }
            }
            RegCloseKey(hSub);
        }
        RegCloseKey(hUninstall);
    }
    return 0;
}

void Config_Load(AppContext *ctx) {
    if (!ctx) return;
    Config_GetIniPath(ctx->iniPath, MAX_PATH);
    const AppStrings *s = Strings_Get();

    for (int i = 0; i < MAX_MODES; i++) {
        wchar_t sec[16];
        _snwprintf(sec, 16, L"Mode%d", i);
        wcsncpy(ctx->config.modes[i].name, s->mode_names[i], 31);
        ctx->config.modes[i].enabled = GetPrivateProfileIntW(sec, L"Enabled", 1, ctx->iniPath);

        GetPrivateProfileStringW(sec, L"Config", L"",
                                 ctx->config.modes[i].config, MAX_CONFIG_PATH, ctx->iniPath);

        wchar_t defHk[MAX_HOTKEY_LEN];
        _snwprintf(defHk, MAX_HOTKEY_LEN, L"Ctrl+Alt+%d", i + 1);
        GetPrivateProfileStringW(sec, L"Hotkey", defHk,
                                 ctx->config.modes[i].hotkey, MAX_HOTKEY_LEN, ctx->iniPath);

        wchar_t wprocs[MAX_PROCESS_LIST_LEN];
        GetPrivateProfileStringW(sec, L"Processes", L"",
                                 wprocs, MAX_PROCESS_LIST_LEN, ctx->iniPath);
        Config_ParseProcessList(&ctx->config.modes[i], wprocs);
    }

    ctx->config.autostart = GetPrivateProfileIntW(L"General", L"AutoStart", 0, ctx->iniPath);
    ctx->config.showTray = GetPrivateProfileIntW(L"General", L"ShowTray", 1, ctx->iniPath);
    ctx->config.autoSwitch = GetPrivateProfileIntW(L"General", L"AutoSwitch", 1, ctx->iniPath);
    ctx->config.fcPathUserSet = GetPrivateProfileIntW(L"General", L"FcPathUserSet", 0, ctx->iniPath);

    GetPrivateProfileStringW(L"General", L"FanControlExe", FANCONTROL_EXE_DEFAULT,
                             ctx->config.fanControlExe, MAX_PATH, ctx->iniPath);

    if (!ctx->config.fcPathUserSet) {
        wchar_t detected[MAX_PATH];
        if (Config_DetectFanControlPath(detected, MAX_PATH)) {
            wcsncpy(ctx->config.fanControlExe, detected, MAX_PATH - 1);
        }
    }
}

void Config_Save(const AppContext *ctx) {
    if (!ctx) return;
    for (int i = 0; i < MAX_MODES; i++) {
        wchar_t sec[16], buf[16];
        _snwprintf(sec, 16, L"Mode%d", i);
        _snwprintf(buf, 16, L"%d", ctx->config.modes[i].enabled);
        WritePrivateProfileStringW(sec, L"Enabled", buf, ctx->iniPath);
        WritePrivateProfileStringW(sec, L"Config", ctx->config.modes[i].config, ctx->iniPath);
        WritePrivateProfileStringW(sec, L"Hotkey", ctx->config.modes[i].hotkey, ctx->iniPath);

        wchar_t wprocs[MAX_PROCESS_LIST_LEN];
        Config_BuildProcessList(&ctx->config.modes[i], wprocs, MAX_PROCESS_LIST_LEN);
        WritePrivateProfileStringW(sec, L"Processes", wprocs, ctx->iniPath);
    }

    wchar_t buf[16];
    _snwprintf(buf, 16, L"%d", ctx->config.autostart);
    WritePrivateProfileStringW(L"General", L"AutoStart", buf, ctx->iniPath);
    _snwprintf(buf, 16, L"%d", ctx->config.showTray);
    WritePrivateProfileStringW(L"General", L"ShowTray", buf, ctx->iniPath);
    _snwprintf(buf, 16, L"%d", ctx->config.autoSwitch);
    WritePrivateProfileStringW(L"General", L"AutoSwitch", buf, ctx->iniPath);
    WritePrivateProfileStringW(L"General", L"FanControlExe", ctx->config.fanControlExe, ctx->iniPath);
    _snwprintf(buf, 16, L"%d", ctx->config.fcPathUserSet);
    WritePrivateProfileStringW(L"General", L"FcPathUserSet", buf, ctx->iniPath);
}

int Config_IsAutostartEnabled(void) {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, APP_REG_KEY, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
        return 0;

    wchar_t regVal[MAX_PATH * 2 + 64];
    DWORD len = sizeof(regVal);
    DWORD type;
    int valid = 0;
    if (RegQueryValueExW(hKey, APP_REG_VAL, NULL, &type, (LPBYTE)regVal, &len) == ERROR_SUCCESS && type == REG_SZ) {
        wchar_t currentExe[MAX_PATH];
        GetModuleFileNameW(NULL, currentExe, MAX_PATH);

        wchar_t regExe[MAX_PATH];
        regExe[0] = 0;
        const wchar_t *p = regVal;
        if (*p == L'"') {
            p++;
            const wchar_t *endQuote = wcschr(p, L'"');
            if (endQuote) {
                int exeLen = (int)(endQuote - p);
                if (exeLen < MAX_PATH) {
                    wcsncpy(regExe, p, exeLen);
                    regExe[exeLen] = 0;
                }
            }
        } else {
            const wchar_t *space = wcschr(p, L' ');
            if (space) {
                int exeLen = (int)(space - p);
                if (exeLen < MAX_PATH) {
                    wcsncpy(regExe, p, exeLen);
                    regExe[exeLen] = 0;
                }
            } else {
                wcsncpy(regExe, p, MAX_PATH - 1);
                regExe[MAX_PATH - 1] = 0;
            }
        }

        if (regExe[0] && _wcsicmp(regExe, currentExe) == 0) {
            valid = 1;
        }
    }
    RegCloseKey(hKey);
    return valid;
}

void Config_SetAutostart(int on) {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, APP_REG_KEY, 0, KEY_SET_VALUE, &hKey) != ERROR_SUCCESS)
        return;

    if (on) {
        wchar_t exePath[MAX_PATH];
        GetModuleFileNameW(NULL, exePath, MAX_PATH);
        wchar_t cmdLine[MAX_PATH * 2 + 32];
        /* 严格使用双引号包裹路径，彻底消除未加引号路径漏洞 (Q1-A) */
        _snwprintf(cmdLine, sizeof(cmdLine) / sizeof(cmdLine[0]), L"\"%s\" %s", exePath, AUTORUN_FLAG);
        RegSetValueExW(hKey, APP_REG_VAL, 0, REG_SZ,
                       (const BYTE *)cmdLine, (lstrlenW(cmdLine) + 1) * sizeof(wchar_t));
    } else {
        RegDeleteValueW(hKey, APP_REG_VAL);
    }
    RegCloseKey(hKey);
}
