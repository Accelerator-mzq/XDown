# XDown

基于 C++17、Qt 6.5、QML 构建的现代化、高性能本地下载工具。

> 该项目已完全融合进 **smartroute-kit** 框架体系，利用模块化的 `.claude/commands` 目录实现 AI 辅助开发的深度定制，具备完善的单元及自动化系统测试双轨流程。

## 🌟 功能特性

- 多线程下载
- 断点续传
- 下载速度限制
- 任务队列管理
- SQLite 本地存储
- 现代化 QML 界面
- **[New]** 基于 `smartroute-kit` 规范的高度可测试性和组件化架构

## 🛠️ 技术栈

| 技术 | 版本 |
|-----|------|
| C++ | 17 |
| Qt | 6.5.3 |
| QML | Qt Quick |
| 数据库 | SQLite |
| 构建工具 | CMake |
| 测试支撑 | Python 3.12 (Mock Server) + PowerShell |

## 📁 项目结构

```
XDown (smartroute-kit 框架版)/
├── src/                   # C++ 源代码
│   ├── common/            # 公共模块 (DownloadTask)
│   ├── core/              # 核心业务逻辑 (DBManager, HttpDownloader, DownloadEngine)
│   └── gui/               # GUI 相关 (TaskListModel)
├── resources/             # 资源文件 (QML 界面等)
├── tests/                 # 测试代码与自动化框架
│   ├── src/               # Qt Test 源码与 Mock HTTP 服务器
│   ├── automation/        # 自动化测试脚本
│   └── CMakeLists.txt     # 测试组件构建配置
├── docs/                  # 项目文档库
│   ├── design/            # 概要与详细设计说明书
│   ├── requirements/      # 产品规格 (SRS)
│   ├── review/            # Code Review 报告
│   └── test/              # 测试自动计划与测试用例
├── .claude/               # [智能开发扩展] Skill 细则定义库
│   └── commands/          # 包含测试规范、UI 标准及部署规范 (如 code.md, qt-project.md 等)
├── data/                  # 运行时默认数据存储
├── smartroute.config.json # [核心] 项目文档与编译测试自动化配置中枢
├── Run-Test.ps1           # [集成] 系统级自动化端到端测试入口
├── CLAUDE.md              # 智能体核心约束与入口配置
└── CMakeLists.txt         # 顶层构建配置
```

## 🚀 构建说明

### 环境要求

- Qt 6.5.3 MinGW 64-bit
- CMake 3.20+
- MinGW GCC 11.2.0

### 编译与运行

在项目根目录下执行：

```bash
# 配置 CMake
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH="E:/Qt/6.5.3/mingw_64"

# 编译 (8线程并发)
cmake --build build --config Debug -j 8

# 运行程序
./build/appXDown.exe
```

## 🧪 测试体系

XDown 融合 `smartroute-kit` 实现了基于双层验收的测试流程（C++ 单元测试/组件测试 + 外部自动化交互验证）：

### 1. 运行核心单元/集成测试 (推荐)

使用 `ctest` 进行 100% 覆盖通过的本地运行验证，请预先确保后端的 Python Mock 服务器 (位于 `tests/src/mock_http_server.py`) 在后台启动：

```bash
# 启动 Mock 服务器后台 (端口: 28080)
python tests/src/mock_http_server.py &

# 进入 build 目录并运行 ctest
cd build
ctest -C Debug -V
```

### 2. 运行自动化系统级交互测试

`Run-Test.ps1` 定义了面向最终集成端点的一键式校验流程。

```powershell
# 在根目录下通过 PowerShell 触发集成模块测试 (例如 net 模块)
.\Run-Test.ps1 -Module net -Param "https://www.w3.org/WAI/ER/tests/xhtml/testfiles/resources/pdf/dummy.pdf"
```

## 🤖 AI 辅助开发指南 (smartroute-kit)

- 开发核心范式请参阅根目录下的 [`CLAUDE.md`](CLAUDE.md)
- 更详尽的单一场景开发指南和审查标准（Test Fix、C++ 代码风格及 Qt 部署），均已分离存储于 `.claude/commands/` 下，实现按需精准备加载。

## 📄 许可证

MIT License
