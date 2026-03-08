# XDown

基于 C++17、Qt 6.5、QML 构建的现代化、高性能本地下载工具。本项目现已接入 **SmartRoute V3 自动化流水线框架**，支持前后端分离的“纯动口”全自动 AI 驱动开发模式。

## ✨ 功能特性

- 多线程下载与断点续传
- 下载速度限制与任务队列管理
- SQLite 本地状态存储
- 现代化与流畅的 QML 界面
- **SmartRoute V3 AI 驱动开发**：自带闭环的多智能体（Multi-Agent）开发流水线，实现代码编写、系统编译、单元测试、报错修复全链路自动化。

## 💻 技术栈

| 技术 | 说明 |
|-----|------|
| C++ | C++ 17 |
| Qt | Qt 6.5.3 (Core, Gui, Quick, Network, Sql 等) |
| QML | Qt Quick, 用于仅负责展示与动画的前端 UI |
| 数据库 | SQLite |
| 构建系统 | CMake 3.20+ |
| 自动化流水线| Python 3 (SmartRoute V3.0) |

## 🤖 SmartRoute V3.0 AI 流水线使用指南

本项目推荐使用 **SmartRoute V3 后台全自动流水线** 进行开发和维护。作为开发者，你只需在前台对话框设计架构，把“写代码与跑编译”等脏活累活丢给后台智能体。

### 触发开发的 2 种主要方式：
当你和 AI 在系统前台（如 Claude 对话框）讨论完需求或 Bug 修复方案后，可通过以下方式拉起后台：
1. **指令触发（推荐）**：直接在对话中输入 `/project:test-loop`
2. **自然语言触发**：告诉 AI “方案敲定，请写入 `.smartroute/task.md` 工单并启动基于 `test_loop.py` 的后台流水线。”

### 工作流原理：
引擎会拉起 `Planner`, `Coder`, `Test Coder`, `Fixer` 等多角色，在后台闭门执行**“写代码 ➔ 编译 ➔ 测试 ➔ 修复”**的循环，直到任务达到要求且终端输出 `[SUCCESS]` 才结束交接。

## 🛠️ 手动构建与测试说明

在使用自动化流水线之外，如果需要手动构建与调试，请参考以下步骤。

### 1. 同步框架与环境配置
```bash
# 同步并初始化配置（如从 smartroute.config.json 加载密钥等）
python .pipeline/setup.py
```

### 2. CMake 编译 (需 MinGW 64-bit)
```bash
# 配置 CMake
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug

# 编译项目 (自动执行风铃等关联工具与依赖)
cmake --build build --config Debug -j 8

# 运行主程序
./build/appXDown.exe
```
*(注：由于项目中已配置 `windeployqt` 的 CMake POST_BUILD 挂载，只要编译成功，相关的 Qt DLL 会自动拷贝到执行目录，无需手动搬运 DLL。)*

### 3. 本地测试执行
本项目配套了完善的 Python 和 PowerShell 测试脚本，能够一键拉起 mock server 和执行程序：
```bash
# 运行单元测试
python run_tests.py

# 运行系统集成测试 (自动拉起 Mock HTTP Server)
powershell -ExecutionPolicy Bypass -File .\Run-Test.ps1 -Module net -Param "http://127.0.0.1:28080/file1.zip"
```

## 📁 核心项目结构

```text
XDown/
├── src/                    # C++ 源代码目录
│   ├── core/              # 核心业务逻辑 (DBManager, HttpDownloader, DownloadEngine 等)
│   └── gui/               # GUI ViewModel 与数据绑定
├── resources/             # 资源文件 (qml.qrc)
│   └── qml/              # QML 界面源码
├── tests/                  # 测试代码层
│   ├── src/               # Qt Test 测试源码
│   └── CMakeLists.txt     # 测试组件编译配置
├── .pipeline/              # SmartRoute V3 自动化流水线脚本
├── .smartroute/            # AI 任务工单与执行计划目录
├── docs/                   # 项目设计与测试文档
└── CMakeLists.txt         # 主构建配置文件
```

## 📄 许可证

MIT License
