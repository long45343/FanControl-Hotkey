# FanControl-Hotkey 架构重构与改造技术规范 (REFACTOR_SPEC)

## 1. 决策总览与冲突一致性检查

经逐项检查，所有 10 项技术决策（均选 A 方案）构成了一个高度协同、内聚且零冲突的重构方案体系：

| 模块/维度 | 采纳决策 | 协同与一致性保证 |
|---|---|---|
| **字符集模型 (Q5)** | 全局原生 `wchar_t` (UTF-16) | 与 Win32 API 100% 契合，彻底消除了 Q6 进程哈希比对、Q2 命令执行、Q4 配置持久化中的字符转码损耗。 |
| **模块解耦 (Q3)** | 多文件拆分 + 显式 `AppContext` | 彻底移除 30+ 静态全局变量，UI 与业务状态分离，为 Q9 单元测试和 Q4 事务草稿提供清晰边界。 |
| **事务草稿 (Q4)** | `ConfigDraft` 局部副本与原子提交 | 设置对话框在独立的 Draft 上操作，Cancel 零副作用，OK 原子替换 `AppContext.config` 并通知刷新。 |
| **安全调用 (Q1 & Q2)** | 引号加固 + `CreateProcessW` 转义 | 消除未加引号路径漏洞（`\"%s\" %s`），参数以宽字符严格转义并校验文件存在性，杜绝命令注入。 |
| **性能监控 (Q6)** | 进程快照 + FNV-1a 不区分大小写哈希 | 保持轻量定时快照的同时将模式进程比对降至 $O(1)$，纯 C 实现无沉重外部系统依赖。 |
| **UI & DPI (Q7)** | DPI-Aware Manifest + `ScaleDpi()` | 高分屏原生清晰，绝对坐标与控件尺寸按当前 DPI 等比缩放，窗口在多显示器间平滑适配。 |
| **工程体系 (Q8, Q9, Q10)**| `Makefile` + `build.ps1` + 纯 C 测试 + GitHub Actions | 本地开发一键构建与秒级单测验证，GitHub Actions CI/CD 自动矩阵构建与 Release 发版。 |

---

## 2. 目标目录与模块结构设计

```text
FanControl-Hotkey/
├── .github/
│   └── workflows/
│       └── ci.yml                 # GitHub Actions 持续集成与 Release 自动打包
├── src/
│   ├── app_context.h              # 核心数据结构与全局上下文定义
│   ├── strings.h                  # 双语字符串表 (中/英) 定义与接口
│   ├── config.h / config.c        # INI 读写、配置深拷贝、自启注册表管理 (全 wchar_t)
│   ├── hotkey.h / hotkey.c        # 热键解析 (ParseHotkey)、注册、注销与冲突检测
│   ├── process_monitor.h / .c     # 系统进程快照采集、FNV-1a 大小写不敏感哈希匹配
│   ├── runner.h / runner.c        # FanControl.exe 探测与 CreateProcessW 安全调用
│   ├── dpi_utils.h / dpi_utils.c  # DPI 检测与 ScaleDpi 坐标缩放工具
│   ├── ui_main.h / ui_main.c      # 主窗口、系统托盘、消息循环与入口调度
│   ├── ui_settings.h / ui_settings.c # 设置窗口 (基于 ConfigDraft 的事务交互)
│   └── main.c                     # wWinMain 入口、单实例互斥体与模块初始化
├── tests/
│   └── test_main.c                # 纯 C 微型单元测试 (覆盖 hotkey、config、hash 等)
├── res/
│   ├── resource.h
│   ├── resource.rc
│   ├── app.manifest               # 启用 Per-Monitor V2 DPI-Awareness
│   └── icon.ico
├── specs/
│   ├── REPO_ROAST_REPORT.md       # 审查锐评归档
│   ├── DECISIONS.md               # 决策与选项全记录
│   └── REFACTOR_SPEC.md           # 本技术设计规范
├── build.ps1                      # Windows 本地 PowerShell 一键构建/测试脚本
├── Makefile                       # 标准 MinGW / CI 构建 Makefile
├── README.md                      # 英文说明文档
└── README-Zh-CN.md                # 中文说明文档
```

---

## 3. 核心数据结构与接口定义

### 3.1 `src/app_context.h`
```c
#ifndef APP_CONTEXT_H
#define APP_CONTEXT_H

#include <windows.h>

#define MAX_MODES 4
#define MAX_CONFIG_PATH 260
#define MAX_HOTKEY_LEN 32
#define MAX_PROCESS_NAMES 16
#define MAX_PROCESS_LEN 64
#define MAX_PROCESS_LIST_LEN (MAX_PROCESS_NAMES * (MAX_PROCESS_LEN + 2))
#define DEFAULT_MODE_INDEX 1
#define POLL_INTERVAL_MS 2000

typedef struct {
    wchar_t name[32];
    wchar_t config[MAX_CONFIG_PATH];
    wchar_t hotkey[MAX_HOTKEY_LEN];
    wchar_t processNames[MAX_PROCESS_NAMES][MAX_PROCESS_LEN];
    int processCount;
    int enabled;
    int hotkeyId;
} ModeConfig;

typedef struct {
    ModeConfig modes[MAX_MODES];
    int autostart;
    int showTray;
    int autoSwitch;
    int fcPathUserSet;
    wchar_t fanControlExe[MAX_PATH];
} AppConfig;

typedef struct {
    HINSTANCE hInstance;
    HWND hwndMain;
    HWND hwndSettings;
    HFONT hFont;
    int currentDpi;
    AppConfig config;
    int lastAutoMode;
    int trayAdded;
    HANDLE hMutex;
} AppContext;

#endif
```

### 3.2 事务草稿（`Draft`）机制设计
- 打开设置窗口：`Config_Clone(&ctx->config, &draftConfig)`。
- 设置界面所有列表增删、文本框编辑均仅作用于 `draftConfig`。
- 用户点击 `Cancel` 或关闭窗口：直接销毁 `draftConfig`，不产生任何副作用。
- 用户点击 `OK`：
  1. 调用 `Config_Apply(ctx, &draftConfig)` 覆盖当前运行态；
  2. 调用 `Config_Save(ctx)` 持久化到 `fan_hotkey.ini`；
  3. 调用 `SetAutostart(ctx->config.autostart)` 更新注册表双引号启动项；
  4. 重新调用 `Hotkey_RegisterAll(ctx)` 与 `UIMain_RefreshButtons(ctx)`。

---

## 4. 关键安全与算法实现规范

### 4.1 安全开机启动项写入（消灭未加引号路径漏洞）
```c
void SetAutostart(int on) {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, APP_REG_KEY, 0, KEY_SET_VALUE, &hKey) != ERROR_SUCCESS)
        return;
    if (on) {
        wchar_t exePath[MAX_PATH];
        GetModuleFileNameW(NULL, exePath, MAX_PATH);
        wchar_t cmdLine[MAX_PATH * 2 + 32];
        /* 严格使用双引号包裹可执行文件绝对路径 */
        _snwprintf(cmdLine, sizeof(cmdLine)/sizeof(cmdLine[0]), L"\"%s\" %s", exePath, AUTORUN_FLAG);
        RegSetValueExW(hKey, APP_REG_VAL, 0, REG_SZ,
                       (const BYTE *)cmdLine, (lstrlenW(cmdLine) + 1) * sizeof(wchar_t));
    } else {
        RegDeleteValueW(hKey, APP_REG_VAL);
    }
    RegCloseKey(hKey);
}
```

### 4.2 外部进程安全调用（`CreateProcessW`）
```c
BOOL Runner_SwitchTo(AppContext *ctx, int modeIndex, BOOL silent) {
    if (modeIndex < 0 || modeIndex >= MAX_MODES) return FALSE;
    ModeConfig *m = &ctx->config.modes[modeIndex];
    if (!m->enabled) {
        if (!silent) MessageBoxW(ctx->hwndMain, Strings_Get()->not_enabled_msg, Strings_Get()->notice, MB_OK | MB_ICONINFORMATION);
        return FALSE;
    }
    if (!m->config[0] || GetFileAttributesW(m->config) == INVALID_FILE_ATTRIBUTES) {
        if (!silent) {
            wchar_t msg[256];
            _snwprintf(msg, 256, Strings_Get()->not_configured_fmt, m->name);
            MessageBoxW(ctx->hwndMain, msg, Strings_Get()->notice, MB_OK | MB_ICONINFORMATION);
        }
        return FALSE;
    }
    
    wchar_t cmdLine[MAX_PATH * 2 + 64];
    _snwprintf(cmdLine, sizeof(cmdLine)/sizeof(cmdLine[0]), L"\"%s\" -c \"%s\"", ctx->config.fanControlExe, m->config);
    
    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi = { 0 };
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    
    BOOL success = CreateProcessW(NULL, cmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    if (success) {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    } else if (!silent) {
        wchar_t msg[MAX_PATH + 128];
        _snwprintf(msg, sizeof(msg)/sizeof(msg[0]), Strings_Get()->fancontrol_not_found_fmt, ctx->config.fanControlExe);
        MessageBoxW(ctx->hwndMain, msg, Strings_Get()->notice, MB_OK | MB_ICONERROR);
    }
    return success;
}
```

### 4.3 进程感知 $O(1)$ 哈希匹配算法
```c
/* 大小写不敏感 FNV-1a 宽字符哈希算法 */
static inline UINT32 HashProcessNameW(const wchar_t *str) {
    UINT32 hash = 2166136261u;
    while (*str) {
        wchar_t c = towlower(*str);
        hash ^= (UINT32)c;
        hash *= 16777619u;
        str++;
    }
    return hash;
}
```
每次收集系统活动进程时，将进程哈希存入小哈希表（哈希集合），模式规则匹配时直接通过哈希命中，比对耗时从毫秒级降至微秒级。

---

## 5. 验收标准与交付物

1. **安全与正确性**：
   - 包含空格的目录路径下，自启注册表键能正确拉起带有 `--autostart` 的程序。
   - 带有空格或中文路径的 FanControl 配置可正常切换，无参数注入隐患。
2. **架构与事务**：
   - 设置界面取消时，全局状态与磁盘文件零修改；确定时，状态原子刷新生效。
3. **单元测试**：
   - 运行 `make test` 或 `.\build.ps1 -Test`，所有测试用例 100% 通过。
4. **编译与高 DPI 渲染**：
   - 在 100%、150%、200% 屏幕缩放比例下，窗口与文字清晰无模糊、无重叠。
   - MinGW 编译零警告（`-Wall -Wextra`）。

