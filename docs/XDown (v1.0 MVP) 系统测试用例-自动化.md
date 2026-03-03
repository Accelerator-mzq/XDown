# XDown (v1.0 MVP) 系统测试用例 - 自动化方案

## 一、适合自动化的测试用例

以下测试用例可以通过本地 HTTP 服务器或脚本实现自动化：

| 用例编号 | 测试场景 | 自动化方案 | 优先级 |
|---------|---------|-----------|--------|
| ST-5.1 | 404 错误 | 本地 HTTP 服务器返回 404 | 高 |
| ST-5.5 | 5xx 错误 | 本地 HTTP 服务器返回 500 | 高 |
| ST-5.2 | 网络超时 | 本地服务器延迟响应 | 高 |
| ST-8.1 | 0字节文件 | 本地服务器返回空内容 | 高 |
| ST-3.2 | 多文件并发下载 | 本地 HTTP 服务器并发响应 | 中 |
| ST-6.3 | 特殊字符文件名 | 本地服务器返回特殊名称文件 | 中 |
| ST-6.2 | 重名处理 | 预先创建文件测试覆盖逻辑 | 中 |
| ST-5.4 | 写入权限 | 创建只读目录 | 中 |
| ST-6.1 | 自定义目录 | 创建目录并验证文件位置 | 低 |
| ST-8.2 | 重复URL检测 | 测试数据库查重逻辑 | 低 |

---

## 二、半自动化测试架构

```
┌─────────────────────────────────────────────────────────────────┐
│                      自动化测试框架架构                           │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│   ┌──────────────┐     ┌──────────────┐     ┌──────────────┐   │
│   │   Python     │     │   Qt Test    │     │   Shell      │   │
│   │  HTTP Server │     │   (C++)      │     │  Scripts     │   │
│   │  (Mock)      │◄────┤  启动 app    │────►│  验证结果    │   │
│   └──────────────┘     │              │     └──────────────┘   │
│        ▲              └──────────────┘            ▲            │
│        │                   │                     │            │
│        │            ┌──────┴──────┐              │            │
│        │            │             │              │            │
│        │            ▼             ▼              │            │
│   ┌────────┐   ┌────────┐   ┌────────┐    ┌────────┐        │
│   │ HTTP   │   │ stdout │   │ 文件   │    │ MD5    │        │
│   │ 请求   │   │ 日志   │   │ 系统   │    │ 校验   │        │
│   └────────┘   └────────┘   └────────┘    └────────┘        │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### 核心组件说明

1. **Python HTTP Mock Server** (`tests/automation/mock_server.py`)
   - 模拟各种 HTTP 响应 (404, 500, 超时, 空内容, 延迟等)
   - 支持多线程并发响应
   - 支持自定义响应头 (Content-Length, Range 等)

2. **Qt Test 自动化测试** (`tests/automation/test_system_*.cpp`)
   - 启动 appXDown.exe 进程
   - 通过命令行参数或配置文件传递测试参数
   - 解析 stdout/stderr 获取测试结果

3. **Shell 验证脚本** (`tests/automation/validate_*.sh`)
   - 验证下载文件完整性
   - 验证日志输出正确性
   - 生成测试报告

---

## 三、测试用例详细设计

### 3.1 ST-5.1: 404 错误测试

**测试目标**: 验证程序对 404 错误的处理

**Mock 服务器配置**:
- 返回 HTTP 404 状态码
- 响应体: `{"error": "Not Found"}`

**自动化流程**:
```python
# mock_server.py 配置
{
    "status_code": 404,
    "body": "File not found"
}
```

**预期结果**:
- 任务状态变为 "错误"
- 错误信息包含 "404" 或 "Not Found"

---

### 3.2 ST-5.5: 5xx 服务器错误测试

**测试目标**: 验证程序对服务器错误的处理

**Mock 服务器配置**:
- 返回 HTTP 500 状态码
- 响应体: `{"error": "Internal Server Error"}`

**自动化流程**:
```python
# mock_server.py 配置
{
    "status_code": 500,
    "body": "Internal Server Error"
}
```

**预期结果**:
- 任务状态变为 "错误"
- 错误信息包含 "500" 或 "服务器内部错误"

---

### 3.3 ST-5.2: 网络超时测试

**测试目标**: 验证程序对网络超时的处理

**Mock 服务器配置**:
- 延迟响应时间超过超时阈值 (如 30 秒)
- 或直接不响应

**自动化流程**:
```python
# mock_server.py 配置
{
    "delay": 35,  # 秒
    "close_after": 0
}
```

**预期结果**:
- 任务状态变为 "错误"
- 错误信息包含 "超时" 或 "timeout"

---

### 3.4 ST-8.1: 0字节文件下载测试

**测试目标**: 验证程序对 0 字节文件的处理

**Mock 服务器配置**:
- 返回 HTTP 200
- Content-Length: 0
- 响应体为空

**自动化流程**:
```python
# mock_server.py 配置
{
    "status_code": 200,
    "content_length": 0,
    "body": ""
}
```

**预期结果**:
- 任务状态变为 "已完成"
- 本地文件大小为 0 字节

---

### 3.5 ST-3.2: 多文件并发下载测试

**测试目标**: 验证超过最大并发数时的队列管理

**Mock 服务器配置**:
- 启动 5 个并发下载端点
- 每个返回不同的文件内容

**自动化流程**:
```python
# 同时添加 5 个任务
# 观察: 前 3 个活跃，后 2 个等待
```

**预期结果**:
- 活跃下载数 ≤ 3
- 等待队列数 = 2

---

### 3.6 ST-6.3: 特殊字符文件名测试

**测试目标**: 验证程序对特殊字符文件名的处理

**Mock 服务器配置**:
- Content-Disposition: `attachment; filename="测试 文件(1).zip"`

**自动化流程**:
```python
# mock_server.py 配置
{
    "headers": {
        "Content-Disposition": "attachment; filename=\"测试 文件(1).zip\""
    }
}
```

**预期结果**:
- 文件名包含中文、空格、括号
- 文件能正常保存

---

### 3.7 ST-6.2: 重名处理测试

**测试目标**: 验证程序对同名文件的处理策略

**前置条件**:
- 目标目录预先创建同名文件

**自动化流程**:
```bash
# 预先创建文件
echo "existing" > "D:/Downloads/XDown/test.zip"
# 启动下载同名文件
```

**预期结果**:
- 自动重命名为 `test(1).zip`
- 或提示用户选择

---

### 3.8 ST-5.4: 写入权限测试

**测试目标**: 验证程序对只读目录的处理

**前置条件**:
- 创建只读目录

**自动化流程**:
```bash
mkdir -p /tmp/readonly_test
chmod 444 /tmp/readonly_test
# 尝试下载到该目录
```

**预期结果**:
- 任务状态变为 "错误"
- 错误信息包含 "权限" 或 "无法写入"

---

### 3.9 ST-6.1: 自定义目录测试

**测试目标**: 验证程序对自定义下载目录的支持

**自动化流程**:
```bash
# 启动程序，设置自定义目录
./appXDown.exe --download-dir="D:/CustomPath"
# 添加下载任务
```

**预期结果**:
- 文件保存到 `D:/CustomPath/` 而非默认目录

---

### 3.10 ST-8.2: 重复URL检测测试

**测试目标**: 验证程序对重复URL的检测

**自动化流程**:
```bash
# 添加第一个任务
./appXDown.exe --add-url="http://localhost:8080/file.zip"
# 再次添加相同 URL
./appXDown.exe --add-url="http://localhost:8080/file.zip"
```

**预期结果**:
- 第二次添加时返回错误或提示重复

---

## 四、测试执行脚本

### 4.1 主测试脚本 (`run_automation_tests.py`)

```python
#!/usr/bin/env python3
"""
XDown 自动化系统测试脚本
依赖: Python 3.8+, requests, psutil
"""

import subprocess
import time
import os
import signal
import sys
from http.server import HTTPServer, SimpleHTTPRequestHandler
import threading
import tempfile
import shutil

# 测试配置
APP_PATH = "../build/Debug/appXDown.exe"
MOCK_SERVER_PORT = 8080
TEST_TIMEOUT = 60  # 单个测试超时秒数

class MockServerHandler(SimpleHTTPRequestHandler):
    """自定义 Mock HTTP 服务器处理器"""

    def do_GET(self):
        # 根据路径返回不同的响应
        if self.path == "/404":
            self.send_response(404)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(b'{"error": "Not Found"}')
        elif self.path == "/500":
            self.send_response(500)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(b'{"error": "Internal Server Error"}')
        elif self.path == "/timeout":
            time.sleep(35)  # 延迟响应
            self.send_response(200)
            self.end_headers()
        elif self.path == "/empty":
            self.send_response(200)
            self.send_header("Content-Length", "0")
            self.end_headers()
        elif self.path == "/file1.zip":
            self.send_response(200)
            self.send_header("Content-Length", "1024")
            self.send_header("Content-Disposition", 'attachment; filename="test1.zip"')
            self.end_headers()
            self.wfile.write(b"x" * 1024)
        else:
            super().do_GET()

    def log_message(self, format, *args):
        # 抑制 HTTP 服务器日志
        pass


def start_mock_server(port=8080):
    """启动 Mock HTTP 服务器"""
    server = HTTPServer(('localhost', port), MockServerHandler)
    thread = threading.Thread(target=server.serve_forever)
    thread.daemon = True
    thread.start()
    print(f"[Mock Server] Started on port {port}")
    return server


def run_test(test_name, test_func):
    """运行单个测试"""
    print(f"\n{'='*50}")
    print(f"Running: {test_name}")
    print('='*50)

    try:
        result = test_func()
        if result:
            print(f"✓ PASS: {test_name}")
            return True
        else:
            print(f"✗ FAIL: {test_name}")
            return False
    except Exception as e:
        print(f"✗ ERROR: {test_name} - {e}")
        return False


# 测试用例实现
def test_404_error():
    """ST-5.1: 404 错误测试"""
    # 启动 appXDown.exe，添加 http://localhost:8080/404
    # 检查输出包含 "错误" 和 "404"
    pass


def test_500_error():
    """ST-5.5: 5xx 错误测试"""
    pass


def test_timeout():
    """ST-5.2: 网络超时测试"""
    pass


def test_empty_file():
    """ST-8.1: 0字节文件测试"""
    pass


def test_concurrent_download():
    """ST-3.2: 并发下载测试"""
    pass


def main():
    """主函数"""
    print("XDown 自动化系统测试")
    print("=" * 50)

    # 启动 Mock 服务器
    server = start_mock_server(MOCK_SERVER_PORT)

    # 运行测试
    results = []
    results.append(run_test("ST-5.1: 404错误", test_404_error))
    results.append(run_test("ST-5.5: 500错误", test_500_error))
    results.append(run_test("ST-5.2: 超时测试", test_timeout))
    results.append(run_test("ST-8.1: 0字节文件", test_empty_file))
    results.append(run_test("ST-3.2: 并发下载", test_concurrent_download))

    # 统计结果
    passed = sum(results)
    total = len(results)
    print(f"\n{'='*50}")
    print(f"测试完成: {passed}/{total} 通过")
    print('='*50)

    # 关闭 Mock 服务器
    server.shutdown()


if __name__ == "__main__":
    main()
```

### 4.2 单元测试集成方案

也可以将上述测试集成到 Qt Test 中：

```cpp
// tests/tst_systemtest.cpp 示例结构
class TestSystemWorkflow : public QObject
{
    Q_OBJECT

private slots:
    void testHttp404_data();
    void testHttp404();
    void testHttp500_data();
    void testHttp500();
    void testTimeout_data();
    void testTimeout();
    // ... 其他测试
};
```

---

## 五、测试数据准备

### 5.1 Mock 服务器端点列表

| 端点 | HTTP状态码 | 用途 |
|-----|-----------|------|
| `/404` | 404 | ST-5.1 |
| `/500` | 500 | ST-5.5 |
| `/timeout` | 200 (延迟) | ST-5.2 |
| `/empty` | 200 (空内容) | ST-8.1 |
| `/file1.zip` ~ `/file5.zip` | 200 | ST-3.2 |
| `/special/测试文件(1).zip` | 200 | ST-6.3 |

### 5.2 测试文件准备

```bash
# 创建测试目录结构
tests/
└── automation/
    ├── mock_server.py      # Mock HTTP 服务器
    ├── run_tests.py       # 测试运行脚本
    ├── validate.sh        # 验证脚本
    └── test_data/         # 测试数据目录
        └── existing/      # 预创建文件用于重名测试
```

---

## 六、总结

本方案实现了 **10 个适合自动化** 的系统测试用例，通过以下方式：

1. **本地 Mock HTTP 服务器** - 模拟各种 HTTP 响应
2. **Qt Test 集成** - 与现有单元测试框架统一
3. **Shell 脚本验证** - 文件系统验证和 MD5 校验

**实施优先级建议**:
1. 高优先级: ST-5.1, ST-5.5, ST-5.2, ST-8.1 (核心错误处理)
2. 中优先级: ST-3.2, ST-6.3, ST-6.2 (下载功能)
3. 低优先级: ST-5.4, ST-6.1, ST-8.2 (边界情况)
