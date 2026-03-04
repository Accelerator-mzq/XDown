# XDown

基于 C++17、Qt 6.5、QML 构建的现代化、高性能本地下载工具。

## 功能特性

- 多线程下载
- 断点续传
- 下载速度限制
- 任务队列管理
- SQLite 本地存储
- 现代化 QML 界面

## 技术栈

| 技术 | 版本 |
|-----|------|
| C++ | 17 |
| Qt | 6.5.3 |
| QML | Qt Quick |
| 数据库 | SQLite |
| 构建工具 | CMake |

## 项目结构

```
XDown/
├── src/                    # C++ 源代码
│   ├── common/            # 公共模块 (DownloadTask)
│   ├── core/              # 核心业务逻辑 (DBManager, HttpDownloader, DownloadEngine)
│   └── gui/               # GUI 相关 (TaskListModel)
├── resources/             # 资源文件
│   └── qml/              # QML 界面
├── tests/                  # 测试代码
│   ├── tst_*.cpp          # Qt Test 测试源码
│   ├── mock_http_server.py # Mock HTTP 服务器
│   ├── Run-SystemTest.ps1  # PowerShell 测试脚本
│   └── automation/        # 自动化测试脚本
├── docs/                   # 项目文档
└── CMakeLists.txt         # 构建配置
```

## 构建说明

### 环境要求

- Qt 6.5.3 MinGW 64-bit
- CMake 3.20+
- MinGW GCC 11.2.0

### 编译步骤

```bash
# 配置 CMake
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH="E:/Qt/6.5.3/mingw_64"

# 编译
cmake --build build --config Debug -j 8

# 运行程序
./build/appXDown.exe
```

### 运行测试

```bash
cd build
ctest -C Debug -V
```

## 开发指南

详见 [CLAUDE.md](CLAUDE.md)

## 测试用例

### 单元测试 (Qt Test 框架)

| 测试文件 | 描述 | 状态 |
|---------|------|------|
| `tests/tst_downloadtask.cpp` | DownloadTask 数据结构测试 | ✅ Pass |
| `tests/tst_systemtest.cpp` | HttpDownloader 系统测试 | ✅ Pass |
| `tests/tst_httptest.cpp` | 简化 HTTP 下载测试 | ✅ Pass |

### 测试脚本与工具

| 文件 | 描述 |
|-----|------|
| `tests/mock_http_server.py` | Mock HTTP 服务器 (支持 404/500/超时等) |
| `tests/Run-SystemTest.ps1` | PowerShell 自动化测试脚本 |
| `tests/automation/mock_server.py` | Python Mock 服务器 |
| `tests/automation/run_automation_tests.py` | Python 自动化测试脚本 |

### 运行测试

```bash
# 方式1: 使用 ctest (推荐)
cd build
ctest -C Debug --output-on-failure

# 方式2: 运行单个测试
cd build/tests
./test_systemtest.exe

# 方式3: 使用 PowerShell 脚本 (自动启动 Mock 服务器)
cd tests
./Run-SystemTest.ps1
```

### Mock HTTP 服务器端点

| 端点 | 状态码 | 用途 |
|-----|--------|------|
| `/404` | 404 | 404 错误测试 |
| `/500` | 500 | 500 错误测试 |
| `/timeout` | 200 (延迟) | 超时测试 |
| `/empty` | 200 (空内容) | 0字节文件测试 |
| `/file1.zip` ~ `/file5.zip` | 200 | 并发下载测试 |
| `/health` | 200 | 健康检查 |

## 许可证

MIT License
