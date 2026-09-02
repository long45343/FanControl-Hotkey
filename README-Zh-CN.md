# FanControl Hotkey

[English](README.md) | **简体中文**

一款专为 [FanControl](https://getfancontrol.com/) 打造的轻量级快捷键与进程感知风扇模式自动切换工具。基于纯原生 C 语言与 Win32 API 编写，零外部运行时依赖，原生支持 Per-Monitor V2 高分屏 DPI 缩放。

---

## ✨ 核心特性

- 4 种风扇模式预设：静音 / 日常 / 野兽 / 涡轮
- 自定义快捷键绑定：支持任意组合键捕获与绑定（如 `Ctrl+Alt+1`、`Shift+Win+F12`）
- 异步后台进程感知自动切换：专属后台工作线程与 O(1) FNV-1a 哈希匹配（如启动 `Cyberpunk2077.exe` 自动切为野兽/涡轮，退出时自动回退为日常）
- 可视化图形设置窗口：图形化浏览 FanControl 路径、模式配置文件、快捷键捕获与关联进程管理
- 事务级草稿隔离机制 (Draft Isolation)：设置窗口采用内存临时草稿，点击“取消”零副作用，点击“确定”原子提交应用
- 安全加固与健壮持久化：
  - 开机自启注册表写入严格双引号包裹路径 (`\"%s\" %s`)，并校验运行路径一致性
  - 多级配置存储策略：优先可执行文件同级便携式 `.ini`，受保护目录自动回退至 `%APPDATA%\FanControlHotkey\`
  - 外部进程通过 `CreateProcessW` 安全拉起，配合参数转义与文件存在性校验
- 原生宽字符与 Per-Monitor V2 高分屏：全链路采用 `wchar_t` (UTF-16)，清单声明 DPI-Aware 并动态缩放 GDI 字体
- 系统托盘与单实例防护：常驻系统托盘，重复启动自动唤醒并激活已有实例窗口
- 零外部依赖：仅链接 Windows 原生系统动态库（`USER32`、`SHELL32`、`COMDLG32`、`ADVAPI32`、`GDI32`）

---

## 🛠️ 构建与测试

### 环境要求

- **MinGW-w64**（GCC 8.0+，支持 C99/C11 标准）

### 本地编译

#### 方式一：PowerShell 构建脚本（推荐）

```powershell
# 编译生成 FanControlHotkey.exe
.\build.ps1

# 编译并运行自动化单元测试
.\build.ps1 -Test

# 清理构建产物
.\build.ps1 -Clean
```

#### 方式二：Make 命令

```bash
# 构建可执行文件
make

# 运行单元测试
make test

# 清理构建产物
make clean
```

---

## 📂 项目结构

```text
├── .github/workflows/ci.yml   # GitHub Actions CI 与自动 Release 打包流水线
├── src/                       # 模块化纯 C 源码
│   ├── main.c                 # 程序入口与互斥量单实例保护
│   ├── app_context.h          # 全局数据模型与上下文定义
│   ├── strings.h / .c         # 双语本地化字符表（中 / 英）
│   ├── config.h / .c          # INI 配置读写、AppData 回退与自启动管理
│   ├── hotkey.h / .c          # 快捷键解析、冲突检测与热键注册
│   ├── process_monitor.h / .c # 后台监控工作线程与 FNV-1a 哈希匹配引擎
│   ├── runner.h / .c          # CreateProcessW 安全进程启动器
│   ├── dpi_utils.h / .c       # Per-Monitor V2 DPI 动态适配与字体创建
│   ├── ui_main.h / .c         # 主窗口（GWLP_USERDATA 绑定）与托盘消息循环
│   └── ui_settings.h / .c     # 模块化设置窗口与按键捕获
├── tests/
│   └── test_main.c            # 纯 C 微型单元测试套件
├── res/                       # 资源文件
│   ├── app.manifest           # Per-Monitor V2 DPI 清单与系统兼容性声明
│   ├── resource.h / .rc       # 应用图标与 VERSIONINFO 资源
│   └── icon.ico               # 应用程序图标
├── specs/                     # 本地归档：架构规范、决策记录与代码审查（不入库）
├── build.ps1                  # Windows PowerShell 构建脚本
├── Makefile                   # Windows Makefile（MinGW-w64 工具链）
└── README.md                  # 英文说明文档
```

---

## 📄 开源许可

本项目遵循 [MIT 开源许可证](LICENSE)。
