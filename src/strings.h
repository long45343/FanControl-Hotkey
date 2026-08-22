#ifndef STRINGS_H
#define STRINGS_H

#include <windows.h>
#include "app_context.h"

typedef struct {
    const wchar_t *mode_names[MAX_MODES];
    const wchar_t *app_title;
    const wchar_t *settings;
    const wchar_t *general;
    const wchar_t *autostart;
    const wchar_t *show_tray;
    const wchar_t *auto_switch;
    const wchar_t *mode_config;
    const wchar_t *process_config;
    const wchar_t *enable_fmt;
    const wchar_t *config_file;
    const wchar_t *browse;
    const wchar_t *hotkey;
    const wchar_t *capture_btn;
    const wchar_t *add;
    const wchar_t *del;
    const wchar_t *ok;
    const wchar_t *cancel;
    const wchar_t *tray_show;
    const wchar_t *tray_exit;
    const wchar_t *capture_prompt;
    const wchar_t *capture_title;
    const wchar_t *json_filter;
    const wchar_t *all_filter;
    const wchar_t *exe_filter;
    const wchar_t *hotkey_conflict_msg;
    const wchar_t *hotkey_conflict_title;
    const wchar_t *hotkey_register_failed_fmt;
    const wchar_t *not_enabled_msg;
    const wchar_t *not_configured_fmt;
    const wchar_t *fancontrol_not_found_fmt;
    const wchar_t *notice;
    const wchar_t *disabled_suffix;
    const wchar_t *not_configured_suffix;
    const wchar_t *exit_btn;
    const wchar_t *fancontrol_path;
    const wchar_t *auto_detect;
} AppStrings;

void Strings_Init(void);
const AppStrings *Strings_Get(void);

#endif
