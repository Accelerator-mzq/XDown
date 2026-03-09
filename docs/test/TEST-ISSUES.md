# XDown 测试问题记录

本文档记录测试环境中遇到的问题及解决方案，每次修改 CMakeLists.txt 后需验证测试是否正常。

---

## ✅ 已解决：编译器版本统一

### 问题
- Qt 6.5.3 使用 GCC 11.2 编译
- 用户之前使用 GCC 14.2
- 导致 libstdc++ 版本冲突和 MOC 兼容性问题

### 解决方案
在 `CMakeLists.txt` 中指定使用 Qt 自带的 GCC 11.2 编译器：

```cmake
# CMakeLists.txt 开头添加
set(CMAKE_C_COMPILER "E:/Qt/Tools/mingw1120_64/bin/gcc.exe")
set(CMAKE_CXX_COMPILER "E:/Qt/Tools/mingw1120_64/bin/g++.exe")
```

### 编译命令
```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_PREFIX_PATH="E:/Qt/6.5.3/mingw_64" \
  -DCMAKE_C_COMPILER="E:/Qt/Tools/mingw1120_64/bin/gcc.exe" \
  -DCMAKE_CXX_COMPILER="E:/Qt/Tools/mingw1120_64/bin/g++.exe"
```

---

## ✅ 已解决：ChunkInfoTest Qt Test 框架问题

### 症状
- 测试编译通过
- 运行时无任何输出（stdout/stderr 均为空）
- exit code = 1

### 原因
- Qt 6.5.3 的 MOC 与测试框架存在兼容性问题
- 简单结构体测试更容易触发此问题

### 解决方案
使用**纯 C++ 方式 + QVERIFY 宏**（不使用 QTest 类）：

```cpp
// tests/src/tst_chunkinfo.cpp
#include <QCoreApplication>
#include <QtTest/QtTest>
#include "core/download/ChunkInfo.h"

// 定义测试函数（不使用 Q_OBJECT）
void testDefaultConstructor() {
    ChunkInfo chunk;
    QVERIFY(chunk.index == 0);
    // ...
}

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    testDefaultConstructor();
    // ...
    return 0;
}
```

---

## ✅ 已解决：测试 DLL 依赖

### 解决方案
在 `tests/CMakeLists.txt` 中添加 POST_BUILD 命令，复制必要的 DLL：

```cmake
# tests/CMakeLists.txt 末尾添加
set(QT_DIR "E:/Qt/6.5.3/mingw_64")
set(MINGW1120_DIR "E:/Qt/Tools/mingw1120_64/bin")
set(TEST_DIR "${CMAKE_CURRENT_BINARY_DIR}")

if(WIN32)
    # 复制 GCC 11.2 运行时库
    add_custom_command(TARGET tst_chunkinfo POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${MINGW1120_DIR}/libstdc++-6.dll" "${TEST_DIR}/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${MINGW1120_DIR}/libgcc_s_seh-1.dll" "${TEST_DIR}/"
    )

    # 复制 Qt DLL
    add_custom_command(TARGET tst_chunkinfo POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${QT_DIR}/bin/Qt6Core.dll" "${TEST_DIR}/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${QT_DIR}/bin/Qt6Test.dll" "${TEST_DIR}/"
    )
endif()
```

---

## 验证清单

每次修改 CMakeLists.txt 或添加新测试后，执行以下验证：

- [ ] 编译成功
- [ ] `cd build && ctest -C Debug` 所有测试通过
- [ ] 检查无 "Failed" 或 "Error" 输出

---

## 环境信息

| 组件 | 版本 |
|------|------|
| Qt | 6.5.3 |
| 编译器 | GCC 11.2 (Qt Tools mingw1120_64) |
| CMake | 3.20+ |
| 构建类型 | Debug |
