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
│   ├── common/            # 公共模块
│   ├── core/              # 核心业务逻辑
│   └── gui/               # GUI 相关
├── resources/             # 资源文件
│   └── qml/              # QML 界面
├── tests/                  # 测试代码
│   ├── automation/        # 自动化测试
│   └── tst_*.cpp         # 单元测试
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

- 单元测试: `tests/tst_downloadtask.cpp`
- 系统测试: `tests/tst_systemtest.cpp`
- 自动化测试: `tests/automation/`

## 许可证

MIT License
