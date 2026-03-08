# XDown v2.0 自动化测试方案文档

**版本号**：v2.0
**文档状态**：Draft
**编写日期**：2026-03-08
**基于文档**：xdown-detailed-design-v2.0.md, xdown-system-test-cases-v1.0.md, xdown-unit-test-cases-v1.0.md

---

## 目录

1. [自动化测试目标和范围](#1-自动化测试目标和范围)
2. [自动化测试框架选型](#2-自动化测试框架选型)
3. [单元测试自动化方案](#3-单元测试自动化方案)
4. [集成测试自动化方案](#4-集成测试自动化方案)
5. [系统测试自动化方案](#5-系统测试自动化方案)
6. [CI/CD 集成方案](#6-cicd-集成方案)
7. [测试数据管理方案](#7-测试数据管理方案)
8. [测试报告生成方案](#8-测试报告生成方案)
9. [自动化测试执行流程](#9-自动化测试执行流程)
10. [维护和扩展策略](#10-维护和扩展策略)

---

## 1. 自动化测试目标和范围

### 1.1 测试目标

本自动化测试方案旨在实现以下目标：

1. **提高测试效率**：通过自动化测试减少手工测试时间 70%，提高测试覆盖率至 85%+
2. **保证代码质量**：在每次代码提交前自动运行测试，及早发现问题
3. **支持持续集成**：与 CI/CD 流水线集成，实现自动化构建和测试
4. **回归测试保障**：确保新功能不会破坏现有功能
5. **性能监控**：自动化监控内存使用、下载速度等性能指标

### 1.2 测试范围

#### 1.2.1 单元测试范围（7 个核心模块）

| 模块 | 测试类 | 测试用例数 | 覆盖率目标 | 优先级 |
|------|--------|-----------|-----------|--------|
| TaskManager | TaskManagerTest | 15 | 90% | P0 |
| HTTPDownloadEngine | HTTPDownloadEngineTest | 20 | 85% | P0 |
| ChunkDownloader | ChunkDownloaderTest | 10 | 85% | P0 |
| AdaptiveThreadController | AdaptiveThreadControllerTest | 8 | 80% | P1 |
| BTDownloadEngine | BTDownloadEngineTest | 15 | 80% | P1 |
| FileManager | FileManagerTest | 12 | 90% | P0 |
| DatabaseManager | DatabaseManagerTest | 10 | 90% | P0 |

#### 1.2.2 集成测试范围（8 个集成场景）

| 场景编号 | 测试场景 | 测试类 | 优先级 |
|---------|---------|--------|--------|
| IT-1 | HTTP/HTTPS 下载端到端流程 | DownloadWorkflowTest | P0 |
| IT-2 | 断点续传功能 | ResumeWorkflowTest | P0 |
| IT-3 | 多任务并发控制 | ConcurrentWorkflowTest | P0 |
| IT-4 | 数据库持久化与恢复 | PersistenceWorkflowTest | P0 |
| IT-5 | 错误处理与重试机制 | ErrorHandlingWorkflowTest | P1 |
| IT-6 | 文件管理与完整性校验 | FileIntegrityWorkflowTest | P1 |
| IT-7 | UI 与后端数据绑定 | DataBindingWorkflowTest | P1 |
| IT-8 | BT 下载完整流程 | BTWorkflowTest | P2 |

#### 1.2.3 系统测试范围（通过 Run-Test.ps1）

- **真实网络测试**：使用真实 HTTP 服务器测试下载功能
- **Mock 服务器测试**：使用 mock_http_server.py 模拟各种场景
- **边界条件测试**：0 字节文件、超大文件、特殊字符文件名
- **错误场景测试**：404、500、403、超时、重定向
- **性能测试**：内存占用、下载速度、UI 响应性

### 1.3 不在自动化范围内的测试

以下测试需要手工执行：

- UI 布局和视觉效果测试
- 用户交互体验测试（拖拽、右键菜单等）
- 跨平台兼容性测试（需要多个操作系统环境）
- 安全性渗透测试
- 长时间稳定性测试（7×24 小时运行）

---

## 2. 自动化测试框架选型

### 2.1 单元测试框架：Qt Test

**选型理由**：

1. **原生支持**：Qt Test 是 Qt 官方测试框架，与项目技术栈完美匹配
2. **信号槽测试**：内置对 Qt 信号槽机制的测试支持（QSignalSpy）
3. **异步测试**：支持异步操作测试，适合网络下载场景
4. **集成便捷**：与 CMake 和 CTest 无缝集成
5. **数据驱动测试**：支持 `_data()` 函数实现数据驱动测试

**核心特性**：

```cpp
// Qt Test 测试用例结构
class TestDownloadTask : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();      // 测试套件初始化（执行一次）
    void init();              // 每个测试用例前执行
    void testFormatSize();    // 测试用例
    void testFormatSize_data(); // 数据驱动测试
    void cleanup();           // 每个测试用例后执行
    void cleanupTestCase();   // 测试套件清理（执行一次）
};
```

### 2.2 构建和测试管理：CMake + CTest

**选型理由**：

1. **统一构建**：CMake 是项目的构建系统，CTest 是其内置测试工具
2. **跨平台支持**：支持 Windows、Linux、macOS
3. **并行执行**：支持并行运行多个测试用例（`ctest -j8`）
4. **灵活配置**：支持测试分组、超时设置、环境变量配置
5. **结果输出**：支持 XML、JSON 等多种格式的测试报告

**核心配置**：

```cmake
# tests/CMakeLists.txt
enable_testing()

# 添加单元测试
add_test(NAME DownloadTaskTest COMMAND test_downloadtask)
set_tests_properties(DownloadTaskTest PROPERTIES TIMEOUT 60)

# 添加系统测试
add_test(NAME SystemTest COMMAND test_systemtest)
set_tests_properties(SystemTest PROPERTIES TIMEOUT 300)

# 设置测试环境变量
set_tests_properties(SystemTest PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)
```

### 2.3 Mock 服务器：Python HTTP Server

**选型理由**：

1. **轻量级**：基于 Python 标准库 `http.server`，无需额外依赖
2. **灵活配置**：可以模拟各种 HTTP 响应（状态码、延迟、重定向等）
3. **易于扩展**：可以快速添加新的测试端点
4. **跨平台**：Python 在所有平台上都可用
5. **端口可配置**：默认使用 28080 端口，避免与其他服务冲突

**核心功能**：

| 端点 | HTTP 状态码 | 用途 | 实现状态 |
|-----|-----------|------|---------|
| `/404` | 404 | 测试 404 错误处理 | ✅ |
| `/500` | 500 | 测试服务器错误处理 | ✅ |
| `/403` | 403 | 测试禁止访问处理 | ✅ |
| `/timeout` | 200 (延迟 30s) | 测试超时处理 | ✅ |
| `/empty` | 200 (0 字节) | 测试空文件处理 | ✅ |
| `/file1.zip` ~ `/file5.zip` | 200 (1MB) | 测试并发下载 | ✅ |
| `/redirect` | 302 | 测试重定向 | ✅ |
| `/redirect-chain` | 302 (3 跳) | 测试重定向链 | ✅ |
| `/redirect-loop` | 302 (无限循环) | 测试重定向循环 | ✅ |
| `/resumable` | 206 | 测试断点续传 | ✅ |
| `/no-resume` | 200 | 测试不支持续传 | ✅ |
| `/named/report.pdf` | 200 | 测试文件名解析 | ✅ |
| `/noname` | 200 | 测试无文件名 URL | ✅ |
| `/special` | 200 | 测试特殊字符文件名 | ✅ |
| `/health` | 200 | 服务器健康检查 | ✅ |

### 2.4 系统测试脚本：PowerShell + Python

**PowerShell 脚本**（`Run-Test.ps1`）：

- **用途**：Windows 平台系统测试自动化
- **功能**：
  - 自动设置 Qt 环境变量（PATH、QT_PLUGIN_PATH）
  - 启动 Mock HTTP 服务器
  - 执行测试可执行文件
  - 解析测试结果并输出
  - 清理测试环境

**Python 脚本**（`run_tests.py`）：

- **用途**：跨平台测试执行脚本
- **功能**：
  - 启动 Mock HTTP 服务器
  - 执行 `ctest` 命令
  - 设置环境变量（PATH、PYTHONIOENCODING）
  - 捕获测试输出并返回退出码

---

## 3. 单元测试自动化方案

### 3.1 测试用例组织结构

```
tests/
├── unit/                          # 单元测试目录
│   ├── test_task_manager.cpp      # TaskManager 测试
│   ├── test_http_engine.cpp       # HTTPDownloadEngine 测试
│   ├── test_chunk_downloader.cpp  # ChunkDownloader 测试
│   ├── test_adaptive_controller.cpp # AdaptiveThreadController 测试
│   ├── test_bt_engine.cpp         # BTDownloadEngine 测试
│   ├── test_file_manager.cpp      # FileManager 测试
│   └── test_database_manager.cpp  # DatabaseManager 测试
├── integration/                   # 集成测试目录
│   ├── test_download_workflow.cpp # 下载流程集成测试
│   ├── test_resume_workflow.cpp   # 断点续传集成测试
│   └── test_concurrent_workflow.cpp # 并发控制集成测试
├── system/                        # 系统测试目录
│   └── tst_systemtest.cpp         # 系统测试主文件
└── src/                           # 测试辅助代码
    ├── mock_http_server.py        # Mock HTTP 服务器
    ├── test_helpers.h             # 测试辅助函数
    └── test_fixtures.h            # 测试夹具（Fixtures）
```

### 3.2 核心模块测试方案

#### 3.2.1 TaskManager 测试

**测试文件**：`tests/unit/test_task_manager.cpp`

**测试用例清单**：

| 用例编号 | 测试方法 | 测试场景 | 断言 |
|---------|---------|---------|------|
| TM-1 | testCreateTask | 创建 HTTP 任务 | 返回有效任务 ID |
| TM-2 | testCreateTaskInvalidUrl | 创建无效 URL 任务 | 返回空字符串 |
| TM-3 | testStartTask | 启动任务 | 任务状态变为 Downloading |
| TM-4 | testPauseTask | 暂停任务 | 任务状态变为 Paused |
| TM-5 | testResumeTask | 恢复任务 | 任务状态变为 Downloading |
| TM-6 | testDeleteTask | 删除任务 | 任务从列表中移除 |
| TM-7 | testDeleteTaskWithFile | 删除任务并删除文件 | 任务和文件都被删除 |
| TM-8 | testConcurrencyLimit | 并发限制 | 活跃任务数 ≤ 最大并发数 |
| TM-9 | testWaitingQueue | 等待队列 | 超限任务进入等待队列 |
| TM-10 | testAutoSchedule | 自动调度 | 任务完成后自动启动等待任务 |
| TM-11 | testDuplicateCheck | 重复性校验 | 重复 URL 被拒绝 |
| TM-12 | testStartAll | 启动所有任务 | 所有任务状态变为 Downloading |
| TM-13 | testPauseAll | 暂停所有任务 | 所有任务状态变为 Paused |
| TM-14 | testGetTaskList | 获取任务列表 | 返回正确的任务列表 |
| TM-15 | testFilterTaskList | 筛选任务列表 | 按状态筛选正确 |



#### 3.2.2 HTTPDownloadEngine 测试

**测试文件**：`tests/unit/test_http_engine.cpp`

**关键测试用例**：

- 检测 Range 支持
- 计算分块策略
- 启动/暂停/恢复下载
- 合并分块文件
- 完整性校验
- 进度和速度计算
- 错误处理和重试
- 重定向处理
- 自适应线程数调整

#### 3.2.3 DatabaseManager 测试

**测试文件**：`tests/unit/test_database_manager.cpp`

**关键测试用例**：

- 插入/更新/删除任务
- 加载任务列表
- 重复性检查
- 事务处理
- 进度节流
- 分块持久化
- 数据库恢复

### 3.3 测试辅助工具

#### 3.3.1 测试夹具（Test Fixtures）

```cpp
// tests/src/test_fixtures.h
class TestFixture
{
public:
    TestFixture()
    {
        m_testDir = QDir::temp().filePath("xdown_test_" + QUuid::createUuid().toString());
        QDir().mkpath(m_testDir);
        m_dbPath = m_testDir + "/test.db";
    }

    ~TestFixture()
    {
        QDir(m_testDir).removeRecursively();
    }

    QString testDir() const { return m_testDir; }
    QString dbPath() const { return m_dbPath; }

private:
    QString m_testDir;
    QString m_dbPath;
};
```

---

## 4. 集成测试自动化方案

### 4.1 集成测试架构

```
┌─────────────────────────────────────────────────────────────┐
│                    集成测试架构                              │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│   ┌──────────────┐     ┌──────────────┐     ┌──────────┐  │
│   │  TaskManager │────►│DownloadEngine│────►│HttpDown- │  │
│   │              │     │              │     │loader    │  │
│   └──────────────┘     └──────────────┘     └──────────┘  │
│          │                     │                   │       │
│          ▼                     ▼                   ▼       │
│   ┌──────────────┐     ┌──────────────┐     ┌──────────┐  │
│   │ DatabaseMgr  │     │ FileManager  │     │ Network  │  │
│   └──────────────┘     └──────────────┘     └──────────┘  │
│                            │                               │
│                            ▼                               │
│                   ┌──────────────┐                         │
│                   │ Mock Server  │                         │
│                   └──────────────┘                         │
└─────────────────────────────────────────────────────────────┘
```

### 4.2 集成测试用例

#### 4.2.1 IT-1: HTTP/HTTPS 下载端到端流程

**测试场景**：

1. 创建下载任务
2. 启动下载
3. 监控进度更新
4. 等待下载完成
5. 验证文件完整性
6. 验证数据库记录

#### 4.2.2 IT-2: 断点续传功能

**测试场景**：

1. 创建下载任务并启动
2. 下载到 50% 时暂停
3. 验证进度保存到数据库
4. 恢复下载
5. 验证从断点继续
6. 验证文件完整性

#### 4.2.3 IT-3: 多任务并发控制

**测试场景**：

1. 创建 5 个下载任务
2. 同时启动所有任务
3. 验证只有 3 个任务在下载（并发限制）
4. 验证 2 个任务在等待队列
5. 等待一个任务完成
6. 验证等待任务自动启动

---

## 5. 系统测试自动化方案

### 5.1 Run-Test.ps1 脚本设计

**文件路径**：`Run-Test.ps1`

**功能**：

1. 设置 Qt 环境变量（PATH、QT_PLUGIN_PATH）
2. 启动 Mock HTTP 服务器
3. 执行测试可执行文件
4. 解析测试结果
5. 清理测试环境

**核心流程**：

```powershell
# 设置环境变量
$qt_bin = "E:\Qt\6.5.3\mingw_64\bin"
$env:PATH = "$env:PATH;$qt_bin"

# 启动 Mock 服务器
$serverJob = Start-Job -ScriptBlock { python mock_http_server.py }
Start-Sleep -Seconds 2

# 执行测试
& appXDown.exe --test-net $Param

# 清理
Stop-Job $serverJob
```

### 5.2 run_tests.py 脚本设计

**文件路径**：`run_tests.py`

**功能**：

1. 跨平台测试执行
2. 启动 Mock HTTP 服务器
3. 执行 ctest 命令
4. 设置环境变量
5. 捕获测试输出

### 5.3 系统测试用例映射

| 用例编号 | 测试场景 | Mock 端点 | 自动化状态 |
|---------|---------|----------|-----------|
| ST-1.1 | 端到端下载 | `/file1.zip` | ✅ |
| ST-1.2 | 断点续传 | `/resumable` | ✅ |
| ST-1.3 | UI 响应性 | `/file1.zip` | ✅ |
| ST-2.2 | 内存红线测试 | `/file1.zip` | ✅ |
| ST-3.2 | 多文件并发 | `/file1-5.zip` | ✅ |
| ST-4.2 | 删除任务（含文件） | `/file1.zip` | ✅ |
| ST-5.1 | 404 错误 | `/404` | ✅ |
| ST-5.5 | 5xx 错误 | `/500` | ✅ |
| ST-9.1 | HTTP 重定向 | `/redirect` | ✅ |
| ST-9.2 | 403 错误 | `/403` | ✅ |
| ST-11 | 文件名解析 | `/named/report.pdf` | ✅ |
| ST-12 | 重定向超限 | `/redirect-loop` | ✅ |
| ST-13 | 不支持续传 | `/no-resume` | ✅ |

---

## 6. CI/CD 集成方案

### 6.1 GitHub Actions 配置

**文件路径**：`.github/workflows/test.yml`

```yaml
name: XDown Automated Tests

on:
  push:
    branches: [ main, develop ]
  pull_request:
    branches: [ main, develop ]

jobs:
  test:
    runs-on: windows-latest

    steps:
    - name: Checkout code
      uses: actions/checkout@v3

    - name: Setup Qt
      uses: jurplel/install-qt-action@v3
      with:
        version: '6.5.3'
        host: 'windows'
        target: 'desktop'
        arch: 'win64_mingw'

    - name: Setup Python
      uses: actions/setup-python@v4
      with:
        python-version: '3.10'

    - name: Configure CMake
      run: cmake -B build -G "MinGW Makefiles"

    - name: Build
      run: cmake --build build --config Debug -j 8

    - name: Run Tests
      run: python run_tests.py

    - name: Upload Test Results
      if: always()
      uses: actions/upload-artifact@v3
      with:
        name: test-results
        path: build/Testing/Temporary/LastTest.log
```

### 6.2 CI/CD 流程

```
1. 开发者提交代码到 GitHub
2. GitHub Actions 自动触发
3. 构建项目
4. 运行所有测试
5. 生成测试报告
6. 通知测试结果
```

---

## 7. 测试数据管理方案

### 7.1 测试数据目录结构

```
tests/
├── data/                          # 测试数据目录
│   ├── fixtures/                  # 测试夹具数据
│   │   ├── sample_1mb.zip         # 1MB 测试文件
│   │   ├── sample_10mb.zip        # 10MB 测试文件
│   │   └── sample_empty.zip       # 0 字节测试文件
│   ├── expected/                  # 预期结果数据
│   │   ├── file1_md5.txt          # 文件 MD5 校验值
│   │   └── file2_sha256.txt       # 文件 SHA256 校验值
│   └── mock/                      # Mock 数据
│       ├── http_responses.json    # HTTP 响应配置
│       └── database_seeds.sql     # 数据库种子数据
```

### 7.2 测试数据生成

```cpp
// 生成测试文件
bool generateTestFile(const QString &filePath, qint64 size)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    QByteArray data(1024, 'X');
    qint64 written = 0;
    while (written < size) {
        qint64 toWrite = qMin(data.size(), size - written);
        file.write(data.data(), toWrite);
        written += toWrite;
    }

    file.close();
    return true;
}
```

---

## 8. 测试报告生成方案

### 8.1 覆盖率报告（使用 gcov/lcov）

```bash
# 生成覆盖率报告
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="--coverage"
cmake --build build
ctest --test-dir build
lcov --capture --directory build --output-file coverage.info
genhtml coverage.info --output-directory coverage_report
```

### 8.2 测试结果报告（CTest XML）

```bash
# 生成 XML 格式测试报告
ctest --test-dir build --output-junit test_results.xml
```

### 8.3 测试报告内容

测试报告应包含以下内容：

1. **测试摘要**：总测试数、通过数、失败数、跳过数
2. **覆盖率统计**：行覆盖率、分支覆盖率、函数覆盖率
3. **失败详情**：失败测试的详细信息和堆栈跟踪
4. **性能指标**：测试执行时间、内存使用情况
5. **趋势分析**：与历史测试结果的对比

---

## 9. 自动化测试执行流程

### 9.1 本地开发流程

```
1. 编写代码
2. 编写单元测试
3. 运行单元测试：python run_tests.py
4. 修复失败的测试
5. 提交代码
```

### 9.2 CI/CD 流程

```
1. 开发者提交代码到 GitHub
2. GitHub Actions 自动触发
3. 构建项目
4. 运行所有测试
5. 生成测试报告
6. 通知测试结果
```

### 9.3 测试执行命令

```bash
# 运行所有测试
python run_tests.py

# 运行单元测试
ctest --test-dir build -R ".*Test$"

# 运行系统测试
powershell -ExecutionPolicy Bypass -File .\Run-Test.ps1 -Module net -Param "http://example.com/file.zip"

# 运行特定测试
ctest --test-dir build -R "DownloadTaskTest"

# 并行运行测试
ctest --test-dir build -j 8

# 详细输出
ctest --test-dir build -V
```

---

## 10. 维护和扩展策略

### 10.1 测试维护原则

1. **测试与代码同步更新**：修改代码时同步更新测试
2. **保持测试独立性**：每个测试用例应独立运行，不依赖其他测试
3. **定期审查测试**：每月审查测试用例，删除过时测试
4. **测试命名规范**：使用清晰的测试方法名（test + 功能描述）
5. **避免测试重复**：相同功能不要重复测试

### 10.2 扩展策略

1. **添加新模块测试**：为新模块编写单元测试和集成测试
2. **增加测试覆盖率**：逐步提高代码覆盖率至 90%+
3. **性能测试自动化**：引入性能测试框架（如 Google Benchmark）
4. **UI 自动化测试**：引入 Qt Test QML 测试框架
5. **压力测试**：添加长时间运行和高并发测试

### 10.3 测试代码质量标准

1. **可读性**：测试代码应清晰易懂，注释充分
2. **可维护性**：测试代码应易于修改和扩展
3. **可靠性**：测试结果应稳定可靠，避免随机失败
4. **性能**：测试执行时间应控制在合理范围内
5. **覆盖率**：核心模块覆盖率应达到 85%+

### 10.4 测试失败处理流程

```
1. 测试失败 → 2. 分析失败原因
              ↓
3. 是代码问题？
   ├─ 是 → 修复代码 → 重新运行测试
   └─ 否 → 是测试问题？
           ├─ 是 → 修复测试 → 重新运行测试
           └─ 否 → 是环境问题？
                   ├─ 是 → 修复环境 → 重新运行测试
                   └─ 否 → 升级问题，寻求帮助
```

---

## 11. 附录

### 11.1 测试工具清单

| 工具 | 用途 | 版本要求 |
|------|------|---------|
| Qt Test | 单元测试框架 | Qt 6.5+ |
| CMake | 构建系统 | 3.20+ |
| CTest | 测试管理 | 3.20+ |
| Python | Mock 服务器 | 3.8+ |
| PowerShell | 系统测试脚本 | 5.1+ |
| gcov/lcov | 覆盖率分析 | 可选 |

### 11.2 参考文档

- [Qt Test 官方文档](https://doc.qt.io/qt-6/qtest-overview.html)
- [CMake Testing 文档](https://cmake.org/cmake/help/latest/manual/ctest.1.html)
- [XDown 详细设计文档](xdown-detailed-design-v2.0.md)
- [XDown 系统测试用例](xdown-system-test-cases-v1.0.md)
- [XDown 单元测试用例](xdown-unit-test-cases-v1.0.md)

### 11.3 常见问题

**Q1: 测试运行失败，提示找不到 Qt DLL？**

A: 确保 Qt bin 目录已添加到 PATH 环境变量，或运行 `windeployqt` 部署 Qt 依赖。

**Q2: Mock 服务器启动失败？**

A: 检查端口 28080 是否被占用，可以修改 `mock_http_server.py` 中的 PORT 变量。

**Q3: 测试超时？**

A: 增加测试超时时间，在 CMakeLists.txt 中设置 `set_tests_properties(TestName PROPERTIES TIMEOUT 600)`。

**Q4: 如何调试失败的测试？**

A: 使用 `ctest -V` 查看详细输出，或在 IDE 中直接运行测试可执行文件并设置断点。

---

**文档结束**
