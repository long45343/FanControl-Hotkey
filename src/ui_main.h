#ifndef UI_MAIN_H
#define UI_MAIN_H

#include "app_context.h"

int UIMain_Init(AppContext *ctx);
void UIMain_RefreshButtons(AppContext *ctx);
void UIMain_ApplyTraySetting(AppContext *ctx);
int UIMain_RunLoop(AppContext *ctx, int initialShow);
void UIMain_Cleanup(AppContext *ctx);

#endif
