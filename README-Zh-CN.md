# FanControl Hotkey (v1.0)

[English](README.md) | **简体中文**

一个轻量级的 [FanControl](https://getfancontrol.com/) 风扇模式热键与进程感知自动切换工具。纯 Win32 API 原生 C 实现，零运行时依赖，支持 Per-Monitor V2 高 DPI 缩放。

---

## ✨ 核心特性

- **四种风扇模式切换**：静音模式 / 日常模式 / 野兽模式 / 涡轮模式
- **自定义快捷键绑定**：支持为每个模式配置任意组合键（如 `Ctrl+Alt+1`、`Shift+Win+F12`）
- **进程感知自动切换**：后台 $O(1)$ FNV-1a 哈希匹配活动进程（如检测到 `Cyberpunk2077.exe` 自动切至野兽/涡轮模式，退出后平滑切回日常模式）
- **GUI 设置窗口**：支持浏览选择 FanControl 路径与配置文件、自定义热键、增删进程规则
- **事务性设置草稿（Draft）**：设置界面编辑操作与运行态完全隔离，取消（Cancel）零副作用，确定（OK）原子应用
- **安全加固**：
  - 开机自启使用严格双引号路径包裹（`\"%s\" %s`），杜绝未加引号启动路径（Unquoted Search Path）劫持漏洞
  - 外部 FanControl 启动采用 `CreateProcessW` 安全转义与路径校验，消除 ShellExecute 注入与歧义
- **原生宽字符与高 DPI 适配**：内部全链路采用 `wchar_t` (UTF-16)，结合应用程序清单（Manifest）与动态等比缩放，4K 高分屏原生清晰不模糊
- **系统托盘与单实例保护**：支持隐藏到托盘、重复启动智能置前已有窗口
- **零外部依赖**：仅链接 Windows 系统 DLL（`USER32`、`SHELL32`、`COMDLG32`、`ADVAPI32`、`GDI32`）

---

## 🛠️ 构建与测试

### 环境要求

- **MinGW-w64**（GCC 8.0+，支持 C99/C11）

### 本地编译

#### 方式一：PowerShell 脚本（推荐）

```powershell
# 编译生成 FanControlHotkey.exe
.\build.ps1

# 运行自动化单元测试
.\build.ps1 -Test

# 清理构建产物
.\build.ps1 -Clean
```

#### 方式二：Make 编译

```bash
# 编译应用程序
make

# 运行单元测试
make test

# 清理
make clean
```

---

## 📂 项目结构

```text
├── .github/workflows/ci.yml   # GitHub Actions 持续集成与 Release 自动打包
├── src/                       # 模块化源代码
│   ├── main.c                 # 程序入口与单实例互斥量
│   ├── app_context.h          # 全局上下文与数据模型定义
│   ├── strings.h / .c         # 双语字符串表 (中/英)
│   ├── config.h / .c          # INI 配置序列化、自启管理与草稿克隆
│   ├── hotkey.h / .c          # 快捷键解析器与热键注册管理
│   ├── process_monitor.h / .c # 进程快照采集与 FNV-1a 哈希匹配引擎
│   ├── runner.h / .c          # CreateProcessW 安全进程拉起
│   ├── dpi_utils.h / .c       # Per-Monitor DPI 动态缩放工具
│   ├── ui_main.h / .c         # 主窗口与托盘消息循环
│   └── ui_settings.h / .c     # 设置窗口与热键捕获 (基于 Draft 事务交互)
├── tests/
│   └── test_main.c            # 纯 C 微型自动化单元测试套件
├── res/                       # 资源文件
│   ├── app.manifest           # Per-Monitor V2 DPI-Aware 清单
│   ├── resource.h / .rc       # PE 图标与 VERSIONINFO 资源定义
│   └── icon.ico               # 应用程序图标
├── specs/                     # 架构规范与锐评决策归档
├── build.ps1                  # PowerShell 构建脚本
├── Makefile                   # 标准 Makefile
└── README-Zh-CN.md            # 中文说明文档
```

---

## 📄 开源许可证

本项目基于 [MIT 许可证](LICENSE) 开源。
