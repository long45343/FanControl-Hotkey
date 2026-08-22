# FanControl Hotkey

超轻量的采用快捷键实现切换Fancontrol配置工具

## 功能

- **四种风扇模式切换**：静音模式 / 日常模式 / 野兽模式 / 涡轮模式
- **自定义快捷键**：为每个模式配置热键（如 `Ctrl+Alt+1`）
- **GUI 设置窗口**：浏览选择配置文件、自定义热键、启用/禁用各模式
- **零依赖**：下载单exe即可运行

## 工作原理

通过如下方式实现配置文件切换：

```
FanControl.exe -c <配置文件.json>
```

FanControl 检测到已有实例运行后，会热切换配置而无需重启。

## 编译

### 环境要求

- [MinGW-w64](https://www.mingw-w64.org/) 工具链（GCC）(w64devkit)

### 编译命令

```bash
windres resource.rc -O coff -o resource.o
gcc -mwindows -municode -Os -s -D_UNICODE -DUNICODE -o FanControlHotkey.exe fan_hotkey.c resource.o -luser32 -lshell32 -lcomdlg32 -ladvapi32 -lgdi32
```

- `windres resource.rc -O coff -o resource.o`：将图标资源编译为对象文件
- `-mwindows`：GUI 子系统，无控制台窗口
- `-municode`：使用 `wWinMain` 入口点，支持 Unicode
- `-Os`：体积优先优化
- `-s`：去除符号表
- `-D_UNICODE -DUNICODE`：启用 Win32 API 的 Unicode 宏
- 链接库说明：
  - `-luser32`：窗口消息、热键、托盘图标
  - `-lshell32`：执行外部程序、托盘通知
  - `-lcomdlg32`：通用文件对话框
  - `-ladvapi32`：注册表读写（开机自启）
  - `-lgdi32`：字体与 GDI 资源

### 编译参数说明

| 参数 | 作用 |
|---|---|
| `-mwindows` | 无控制台，GUI 程序 |
| `-municode` | Unicode `wWinMain` 入口 |
| `-Os` | 体积优化 |
| `-s` | 去除符号表 |
| `-D_UNICODE -DUNICODE` | 启用 Unicode 宏 |

## 使用方法

1. 在 FanControl 界面中配置四套风扇策略，分别导出为 JSON 文件，导出的文件默认在fancontrol的安装位置，如C:\Program Files (x86)\FanControl\Configurations。
2. 运行 `FanControlHotkey.exe`。
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

## 许可证

MIT
