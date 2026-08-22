#include "strings.h"

static const AppStrings str_zh = {
    { L"静音模式", L"日常模式", L"野兽模式", L"涡轮模式" },
    L"FanControl 模式切换",
    L"设置",
    L"通用选项",
    L"开机自动启动",
    L"显示系统托盘图标",
    L"启用进程感知自动切换",
    L"模式配置",
    L"进程配置",
    L"启用 %s",
    L"配置文件：",
    L"选择路径...",
    L"快捷键：",
    L"直接输入",
    L"添加",
    L"删除",
    L"确定",
    L"取消",
    L"显示",
    L"退出",
    L"请按下快捷键组合…（Esc 取消）",
    L"捕获快捷键",
    L"JSON 文件 (*.json)",
    L"所有文件 (*.*)",
    L"可执行文件 (*.exe)",
    L"该快捷键已被其他模式使用，请选择其他组合。",
    L"快捷键冲突",
    L"注册快捷键「%s」失败，可能已被其他程序占用。",
    L"该模式未启用，请在设置中勾选启用。",
    L"尚未为「%s」设置配置文件路径，请先在设置中选择。",
    L"无法启动 FanControl（路径：%s），请检查 FanControl 是否已安装。",
    L"提示",
    L"（未启用）",
    L"（未配置）",
    L"退出",
    L"FanControl 路径：",
    L"自动检测",
};

static const AppStrings str_en = {
    { L"Silent", L"Normal", L"Performance", L"Turbo" },
    L"FanControl Mode",
    L"Settings",
    L"General",
    L"Start with Windows",
    L"Show tray icon",
    L"Enable process-aware auto switch",
    L"Mode Configuration",
    L"Process Configuration",
    L"Enable %s",
    L"Config file:",
    L"Browse...",
    L"Hotkey:",
    L"Capture",
    L"Add",
    L"Delete",
    L"OK",
    L"Cancel",
    L"Show",
    L"Exit",
    L"Press a key combination... (Esc to cancel)",
    L"Capture Hotkey",
    L"JSON Files (*.json)",
    L"All Files (*.*)",
    L"Executable Files (*.exe)",
    L"This hotkey is already used by another mode. Please choose a different combination.",
    L"Hotkey Conflict",
    L"Failed to register hotkey \"%s\". It may already be in use by another program.",
    L"This mode is not enabled. Enable it in Settings.",
    L"No config file set for \"%s\". Please select one in Settings.",
    L"Failed to start FanControl (path: %s). Please check if FanControl is installed.",
    L"Notice",
    L" (Disabled)",
    L" (Not configured)",
    L"Exit",
    L"FanControl path:",
    L"Auto detect",
};

static const AppStrings *g_strings = &str_en;

void Strings_Init(void) {
    LANGID lang = GetUserDefaultUILanguage();
    if (PRIMARYLANGID(lang) == LANG_CHINESE) {
        g_strings = &str_zh;
    } else {
        g_strings = &str_en;
    }
}

const AppStrings *Strings_Get(void) {
    return g_strings;
}
