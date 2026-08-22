#ifndef PROCESS_MONITOR_H
#define PROCESS_MONITOR_H

#include "app_context.h"

unsigned int ProcessMonitor_HashName(const wchar_t *str);
int ProcessMonitor_CollectRunningProcesses(wchar_t (*outNames)[MAX_PROCESS_LEN], int maxCount);
int ProcessMonitor_EvaluateTargetMode(const AppConfig *config, const wchar_t (*runningNames)[MAX_PROCESS_LEN], int runningCount);
void ProcessMonitor_CheckAndTrigger(AppContext *ctx);

#endif
