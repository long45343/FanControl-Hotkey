#ifndef CONFIG_H
#define CONFIG_H

#include "app_context.h"

void Config_GetIniPath(wchar_t *outPath, int maxLen);
void Config_ParseProcessList(ModeConfig *mode, const wchar_t *text);
void Config_BuildProcessList(const ModeConfig *mode, wchar_t *out, int outLen);

void Config_InitDefaults(AppConfig *config);
void Config_Clone(const AppConfig *src, AppConfig *dst);
void Config_Load(AppContext *ctx);
void Config_Save(const AppContext *ctx);

int Config_DetectFanControlPath(wchar_t *out, int outLen);
int Config_IsAutostartEnabled(void);
void Config_SetAutostart(int on);

#endif
