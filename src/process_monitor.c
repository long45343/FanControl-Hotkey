#include <windows.h>
#include <tlhelp32.h>
#include <wctype.h>
#include "process_monitor.h"
#include "runner.h"

#define HASH_TABLE_SIZE 2048

typedef struct {
    unsigned int hashes[HASH_TABLE_SIZE];
    int count;
} ProcessHashSet;

unsigned int ProcessMonitor_HashName(const wchar_t *str) {
    if (!str) return 0;
    unsigned int hash = 2166136261u;
    while (*str) {
        wchar_t c = (wchar_t)towlower(*str);
        hash ^= (unsigned int)c;
        hash *= 16777619u;
        str++;
    }
    return hash;
}

static void BuildHashSet(ProcessHashSet *set, const wchar_t (*runningNames)[MAX_PROCESS_LEN], int count) {
    memset(set, 0, sizeof(ProcessHashSet));
    for (int i = 0; i < count; i++) {
        unsigned int h = ProcessMonitor_HashName(runningNames[i]);
        if (h == 0) continue;
        unsigned int idx = h % HASH_TABLE_SIZE;
        while (set->hashes[idx] != 0 && set->hashes[idx] != h) {
            idx = (idx + 1) % HASH_TABLE_SIZE;
        }
        if (set->hashes[idx] == 0) {
            set->hashes[idx] = h;
            set->count++;
        }
    }
}

static int HashSetContains(const ProcessHashSet *set, unsigned int hash) {
    if (!set || hash == 0 || set->count == 0) return 0;
    unsigned int idx = hash % HASH_TABLE_SIZE;
    unsigned int start = idx;
    while (set->hashes[idx] != 0) {
        if (set->hashes[idx] == hash) return 1;
        idx = (idx + 1) % HASH_TABLE_SIZE;
        if (idx == start) break;
    }
    return 0;
}

int ProcessMonitor_CollectRunningProcesses(wchar_t (*outNames)[MAX_PROCESS_LEN], int maxCount) {
    if (!outNames || maxCount <= 0) return 0;

    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;

    PROCESSENTRY32W pe;
    memset(&pe, 0, sizeof(pe));
    pe.dwSize = sizeof(pe);

    int count = 0;
    if (Process32FirstW(snap, &pe)) {
        do {
            if (count >= maxCount) break;
            wcsncpy(outNames[count], pe.szExeFile, MAX_PROCESS_LEN - 1);
            outNames[count][MAX_PROCESS_LEN - 1] = 0;
            count++;
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return count;
}

int ProcessMonitor_EvaluateTargetMode(const AppConfig *config, const wchar_t (*runningNames)[MAX_PROCESS_LEN], int runningCount) {
    if (!config || !runningNames || runningCount <= 0) return DEFAULT_MODE_INDEX;

    ProcessHashSet set;
    BuildHashSet(&set, runningNames, runningCount);

    /* 优先级从高到低匹配：模式 3 (涡轮) -> 模式 2 (野兽) -> 模式 1 (日常) -> 模式 0 (静音) */
    for (int i = MAX_MODES - 1; i >= 0; i--) {
        const ModeConfig *m = &config->modes[i];
        if (!m->enabled || m->processCount <= 0) continue;

        for (int j = 0; j < m->processCount; j++) {
            unsigned int procHash = ProcessMonitor_HashName(m->processNames[j]);
            if (HashSetContains(&set, procHash)) {
                return i;
            }
        }
    }
    return DEFAULT_MODE_INDEX;
}

static void CheckAndDispatch(AppContext *ctx) {
    if (!ctx || !ctx->config.autoSwitch) return;

    static wchar_t runningNames[MAX_RUNNING_PROCS][MAX_PROCESS_LEN];
    int runningCount = ProcessMonitor_CollectRunningProcesses(runningNames, MAX_RUNNING_PROCS);
    if (runningCount <= 0) return;

    int target = ProcessMonitor_EvaluateTargetMode(&ctx->config, runningNames, runningCount);
    if (target != ctx->lastAutoMode) {
        ctx->lastAutoMode = target;
        if (ctx->hwndMain) {
            PostMessageW(ctx->hwndMain, WM_AUTO_SWITCH_MODE, (WPARAM)target, 0);
        }
    }
}

static DWORD WINAPI MonitorThreadProc(LPVOID lpParam) {
    AppContext *ctx = (AppContext *)lpParam;
    if (!ctx) return 0;

    while (1) {
        DWORD waitRes = WaitForSingleObject(ctx->hMonitorStopEvent, POLL_INTERVAL_MS);
        if (waitRes != WAIT_TIMEOUT) {
            break;
        }
        CheckAndDispatch(ctx);
    }
    return 0;
}

void ProcessMonitor_Start(AppContext *ctx) {
    if (!ctx || ctx->hMonitorThread) return;
    ctx->hMonitorStopEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (!ctx->hMonitorStopEvent) return;
    ctx->hMonitorThread = CreateThread(NULL, 0, MonitorThreadProc, ctx, 0, NULL);
}

void ProcessMonitor_Stop(AppContext *ctx) {
    if (!ctx) return;
    if (ctx->hMonitorStopEvent) {
        SetEvent(ctx->hMonitorStopEvent);
    }
    if (ctx->hMonitorThread) {
        WaitForSingleObject(ctx->hMonitorThread, 3000);
        CloseHandle(ctx->hMonitorThread);
        ctx->hMonitorThread = NULL;
    }
    if (ctx->hMonitorStopEvent) {
        CloseHandle(ctx->hMonitorStopEvent);
        ctx->hMonitorStopEvent = NULL;
    }
}

void ProcessMonitor_TriggerOnce(AppContext *ctx) {
    CheckAndDispatch(ctx);
}
