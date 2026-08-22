#include <stdio.h>
#include <string.h>
#include <windows.h>
#include "../src/app_context.h"
#include "../src/strings.h"
#include "../src/hotkey.h"
#include "../src/config.h"
#include "../src/process_monitor.h"
#include "../src/dpi_utils.h"

static int g_tests_run = 0;
static int g_tests_failed = 0;

#define TEST_ASSERT(expr, msg) do { \
    g_tests_run++; \
    if (!(expr)) { \
        printf("  [FAIL] %s:%d: %s\n", __FILE__, __LINE__, msg); \
        g_tests_failed++; \
    } else { \
        printf("  [PASS] %s\n", msg); \
    } \
} while (0)

static void Test_Hotkey_Parse(void) {
    printf("[RUN] Test_Hotkey_Parse...\n");
    UINT mods, vk;

    TEST_ASSERT(Hotkey_Parse(L"Ctrl+Alt+1", &mods, &vk) == 1, "Parse Ctrl+Alt+1");
    TEST_ASSERT(mods == (MOD_CONTROL | MOD_ALT), "Modifiers Ctrl+Alt match");
    TEST_ASSERT(vk == '1', "Virtual Key '1' matches");

    TEST_ASSERT(Hotkey_Parse(L"Shift+Win+F12", &mods, &vk) == 1, "Parse Shift+Win+F12");
    TEST_ASSERT(mods == (MOD_SHIFT | MOD_WIN), "Modifiers Shift+Win match");
    TEST_ASSERT(vk == VK_F12, "Virtual Key VK_F12 matches");

    TEST_ASSERT(Hotkey_Parse(L"Ctrl+Alt+Shift+Win+A", &mods, &vk) == 1, "Parse 4-modifier Hotkey");
    TEST_ASSERT(mods == (MOD_CONTROL | MOD_ALT | MOD_SHIFT | MOD_WIN), "All 4 modifiers match");
    TEST_ASSERT(vk == 'A', "Virtual key 'A' matches");

    TEST_ASSERT(Hotkey_Parse(L"InvalidKey", &mods, &vk) == 0, "Reject invalid key string");
    TEST_ASSERT(Hotkey_Parse(L"Ctrl+Alt+", &mods, &vk) == 0, "Reject trailing plus");
    TEST_ASSERT(Hotkey_Parse(L"", &mods, &vk) == 0, "Reject empty hotkey");
}

static void Test_Process_List_Parse_And_Build(void) {
    printf("[RUN] Test_Process_List_Parse_And_Build...\n");
    ModeConfig mode;
    memset(&mode, 0, sizeof(mode));

    Config_ParseProcessList(&mode, L"game.exe, test.exe ,  obs64.exe  ");
    TEST_ASSERT(mode.processCount == 3, "Parse process list count is 3");
    TEST_ASSERT(wcscmp(mode.processNames[0], L"game.exe") == 0, "First process is game.exe");
    TEST_ASSERT(wcscmp(mode.processNames[1], L"test.exe") == 0, "Second process is test.exe");
    TEST_ASSERT(wcscmp(mode.processNames[2], L"obs64.exe") == 0, "Third process is obs64.exe");

    wchar_t built[MAX_PROCESS_LIST_LEN];
    Config_BuildProcessList(&mode, built, MAX_PROCESS_LIST_LEN);
    TEST_ASSERT(wcscmp(built, L"game.exe,test.exe,obs64.exe") == 0, "Build comma-separated string matches");
}

static void Test_Process_Hash_And_Matching(void) {
    printf("[RUN] Test_Process_Hash_And_Matching...\n");
    unsigned int h1 = ProcessMonitor_HashName(L"Cyberpunk2077.exe");
    unsigned int h2 = ProcessMonitor_HashName(L"CYBERPUNK2077.EXE");
    unsigned int h3 = ProcessMonitor_HashName(L"chrome.exe");

    TEST_ASSERT(h1 != 0, "Hash is non-zero");
    TEST_ASSERT(h1 == h2, "Hash is case-insensitive (FNV-1a)");
    TEST_ASSERT(h1 != h3, "Distinct process names yield distinct hashes");

    AppConfig config;
    Config_InitDefaults(&config);

    /* 配置模式 3 (涡轮) 匹配 cyberpunk2077.exe */
    wcsncpy(config.modes[3].processNames[0], L"cyberpunk2077.exe", MAX_PROCESS_LEN - 1);
    config.modes[3].processCount = 1;
    config.modes[3].enabled = 1;

    /* 配置模式 2 (野兽) 匹配 blender.exe */
    wcsncpy(config.modes[2].processNames[0], L"blender.exe", MAX_PROCESS_LEN - 1);
    config.modes[2].processCount = 1;
    config.modes[2].enabled = 1;

    wchar_t running[4][MAX_PROCESS_LEN] = {
        L"explorer.exe",
        L"BLENDER.EXE",
        L"Cyberpunk2077.exe",
        L"svchost.exe"
    };

    /* 模式 3 优先级高于模式 2 */
    int target = ProcessMonitor_EvaluateTargetMode(&config, running, 4);
    TEST_ASSERT(target == 3, "High-priority Mode 3 (Turbo) matched successfully over Mode 2");

    /* 仅运行 blender 时匹配模式 2 */
    wchar_t running2[2][MAX_PROCESS_LEN] = {
        L"explorer.exe",
        L"blender.exe"
    };
    target = ProcessMonitor_EvaluateTargetMode(&config, running2, 2);
    TEST_ASSERT(target == 2, "Mode 2 matched when only blender is running");

    /* 无匹配项时回退到默认模式 1 (Normal) */
    wchar_t running3[2][MAX_PROCESS_LEN] = {
        L"explorer.exe",
        L"notepad.exe"
    };
    target = ProcessMonitor_EvaluateTargetMode(&config, running3, 2);
    TEST_ASSERT(target == DEFAULT_MODE_INDEX, "Fallback to default Mode 1 (Normal) when no processes match");
}

static void Test_Config_Clone(void) {
    printf("[RUN] Test_Config_Clone...\n");
    AppConfig src, dst;
    Config_InitDefaults(&src);
    src.autostart = 1;
    src.autoSwitch = 0;
    wcsncpy(src.modes[0].config, L"C:\\my_silent.json", MAX_CONFIG_PATH - 1);

    Config_Clone(&src, &dst);
    TEST_ASSERT(dst.autostart == 1, "Clone autostart field matches");
    TEST_ASSERT(dst.autoSwitch == 0, "Clone autoSwitch field matches");
    TEST_ASSERT(wcscmp(dst.modes[0].config, L"C:\\my_silent.json") == 0, "Clone config string matches");

    /* 确认修改 dst 不影响 src */
    dst.autostart = 0;
    TEST_ASSERT(src.autostart == 1, "Draft isolation: modifying clone does not mutate original");
}

static void Test_Dpi_Scale(void) {
    printf("[RUN] Test_Dpi_Scale...\n");
    TEST_ASSERT(DPI_Scale(100, 96) == 100, "100% DPI (96) scaling is 100");
    TEST_ASSERT(DPI_Scale(100, 120) == 125, "125% DPI (120) scaling is 125");
    TEST_ASSERT(DPI_Scale(100, 144) == 150, "150% DPI (144) scaling is 150");
    TEST_ASSERT(DPI_Scale(100, 192) == 200, "200% DPI (192) scaling is 200");
}

int wmain(void) {
    printf("==========================================\n");
    printf("   FanControl-Hotkey Unit Test Suite\n");
    printf("==========================================\n");
    Strings_Init();

    Test_Hotkey_Parse();
    Test_Process_List_Parse_And_Build();
    Test_Process_Hash_And_Matching();
    Test_Config_Clone();
    Test_Dpi_Scale();

    printf("==========================================\n");
    printf("Tests Run: %d, Failed: %d\n", g_tests_run, g_tests_failed);
    if (g_tests_failed == 0) {
        printf("ALL TESTS PASSED!\n");
        return 0;
    } else {
        printf("SOME TESTS FAILED!\n");
        return 1;
    }
}

