# FanControl Hotkey

一个轻量级的 [FanControl](https://getfancontrol.com/) 风扇模式热键切换工具。纯 Win32 API 实现，零运行时依赖。

## 功能

- **四种风扇模式切换**：静音模式 / 日常模式 / 野兽模式 / 涡轮模式
- **自定义快捷键**：为每个模式配置热键（如 `Ctrl+Alt+1`）
- **GUI 设置窗口**：浏览选择配置文件、自定义热键、启用/禁用各模式
- **开机自启**：可选，通过注册表实现（无需管理员权限）
- **系统托盘图标**：可选，关闭窗口时隐藏到托盘
- **单实例检测**：重复启动时将已有窗口恢复到前台
- **INI 配置持久化**：所有设置保存到 exe 同目录的 `fan_hotkey.ini`
- **零依赖**：仅链接 Windows 系统 DLL（`KERNEL32`、`USER32`、`SHELL32`、`msvcrt`）

## 工作原理

当触发热键或点击按钮时，程序执行：

```
FanControl.exe -c <配置文件.json>
```

FanControl 检测到已有实例运行后，会热切换配置而无需重启。这利用了 FanControl 原生的 `-c` 命令行参数。

## 编译

### 环境要求

- [MinGW-w64](https://www.mingw-w64.org/) 工具链（GCC）

### 编译命令

```bash
gcc -mwindows -municode -Os -s -o fan_hotkey.exe fan_hotkey.c -lcomdlg32
```

- `-mwindows`：GUI 子系统，无控制台窗口
- `-municode`：使用 `wWinMain` 入口点，支持 Unicode
- `-Os`：体积优先优化
- `-s`：去除符号表
- `-lcomdlg32`：链接通用对话框库（文件选择对话框）

### 编译参数说明

| 参数 | 作用 |
|---|---|
| `-mwindows` | 无控制台，GUI 程序 |
| `-municode` | Unicode `wWinMain` 入口 |
| `-Os` | 体积优化 |
| `-s` | 去除符号表 |

## 使用方法

1. 在 FanControl 界面中配置四套风扇策略，分别导出为 JSON 文件。
2. 运行 `fan_hotkey.exe`。
3. 点击 **设置** 按钮配置每个模式：
   - 勾选 **启用** 需要使用的模式
   - 点击 **选择路径...** 选择对应的 JSON 配置文件
   - 填写快捷键（格式：`Ctrl+Alt+1`、`Shift+F5` 等）
4. 点击 **确定** 保存。
5. 通过热键或主窗口按钮切换模式。

### 快捷键格式

```
Ctrl+Alt+1
Ctrl+Shift+F5
Win+Alt+S
```

支持的修饰键：`Ctrl`/`Control`、`Alt`、`Shift`、`Win`
支持的按键：单个字符（A-Z, 0-9）或 F1-F12

## 配置文件

设置保存在 exe 同目录的 `fan_hotkey.ini`：

```ini
[General]
AutoStart=0
ShowTray=1

[Mode0]
Enabled=1
Config=C:\path\to\silent.json
Hotkey=Ctrl+Alt+1

[Mode1]
Enabled=1
Config=C:\path\to\normal.json
Hotkey=Ctrl+Alt+2
```

## 性能指标

| 指标 | 数值 |
|---|---|
| 内存占用 | ~1.6 MB |
| 磁盘体积 | ~54 KB |
| CPU（空闲） | 0% |
| CPU（触发时） | <0.1% 瞬时 |
| 热键响应延迟 | <1ms |
| 依赖 | 无（仅 4 个 Windows 系统 DLL） |

## 许可证

MIT
