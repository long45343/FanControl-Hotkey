# AGENTS.md instructions for D:\EDCs\code\FanControl-Hotkey

只能使用简体中文回答，锐评、决策、spec均生成在当前项目文件夹的specs目录下。

--- project-doc ---

# Agents

## 构建环境

- 编译器：MinGW-w64 GCC (x64)
- 默认路径：`D:\EDCs\Coding\MinGW\bin`
- 关键工具：`gcc.exe`、`windres.exe`

## 构建与测试命令

### PowerShell 构建脚本（推荐）

```powershell
$env:Path = "D:\EDCs\Coding\MinGW\bin;" + $env:Path
.\build.ps1        # 编译生成 FanControlHotkey.exe
.\build.ps1 -Test  # 编译并运行自动化单元测试
.\build.ps1 -Clean # 清理构建产物
```

### Makefile 命令

```powershell
$env:Path = "D:\EDCs\Coding\MinGW\bin;" + $env:Path
make      # 构建可执行文件
make test # 运行单元测试
make clean # 清理
```

## 架构说明

- `src/`：纯 C 模块化实现，统一原生 `wchar_t` 宽字符，通过 `AppContext` 显式传递状态
- `res/`：图标资源、VERSIONINFO 与 Per-Monitor V2 DPI-Awareness 应用程序清单
- `tests/`：微型纯 C 单元测试套件
- `specs/`：技术规范、决策记录与代码审查归档

