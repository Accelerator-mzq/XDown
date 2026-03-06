# Qt 项目开发技能

本技能提供 Qt (C++/QML) 项目的开发规范和最佳实践。

## 适用场景

- 新建 Qt 项目
- Qt C++ 开发
- QML 界面开发
- Qt 单元测试
- Qt 部署发布

## 项目结构规范

```
MyProject/
├── src/                    # C++ 源代码
│   ├── main.cpp           # 程序入口
│   ├── common/            # 公共/通用代码
│   ├── core/              # 核心业务逻辑
│   └── gui/               # GUI 相关代码
├── resources/             # 资源文件
│   └── qml/              # QML 文件
├── tests/                 # 测试代码
├── docs/                  # 文档
├── CMakeLists.txt         # 主构建文件
└── README.md
```

## Qt 规范:
   - 跨线程通信强制使用 Qt Signals & Slots (`Qt::QueuedConnection`)。
   - 数据库操作使用 Qt SQL 模块 (`QSqlDatabase`, `QSqlQuery`)。

## C++ 开发规范

### 1. 头文件规范

```cpp
// 头文件保护
#ifndef MYCLASS_H
#define MYCLASS_H

#include <QObject>
#include <QString>
#include <QDebug>

// 类声明
class MyClass : public QObject {
    Q_OBJECT

public:
    explicit MyClass(QObject *parent = nullptr);
    ~MyClass();

    // Q_PROPERTY 暴露给 QML
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(int value READ value WRITE setValue)

    // Q_INVOKABLE 可调用方法
    Q_INVOKABLE void doSomething();

signals:
    void nameChanged(const QString &name);
    void valueChanged(int value);

private:
    QString m_name;
    int m_value;
};

#endif // MYCLASS_H
```

### 2. 信号槽规范

```cpp
// 连接方式（推荐）
connect(sender, &SenderClass::signalName,
        receiver, &ReceiverClass::slotName);

// Lambda 方式（谨慎使用）
connect(sender, &SenderClass::signalName,
        this, [=](const QString &param) {
            // 处理逻辑
        });

// 跨线程必须使用 Qt::QueuedConnection
connect(sender, &SenderClass::signalName,
        receiver, &ReceiverClass::slotName,
        Qt::QueuedConnection);
```

### 3. 内存管理

```cpp
// 优先使用智能指针
#include <QSharedPointer>
#include <QPointer>

// QObject 子类使用 parent 管理内存
class MyWorker : public QObject {
    Q_OBJECT
public:
    explicit MyWorker(QObject *parent = nullptr) : QObject(parent) {}
};

// 避免内存泄漏：使用 QPointer 防止野指针
QPointer<MyClass> m_weakPtr;
```

### 4. 线程规范

```cpp
// 工作线程
class Worker : public QObject {
    Q_OBJECT
public slots:
    void doWork() {
        // 在这里执行耗时操作
        emit finished(result);
    }

signals:
    void finished(const QString &result);
    void progress(int percent);
};

// 使用 QThread
QThread *thread = new QThread;
Worker *worker = new Worker();
worker->moveToThread(thread);
```

## QML 开发规范

### 1. 基本结构

```qml
import QtQuick 2.15
import QtQuick.Controls 2.15

// 根元素
Item {
    id: root

    // 属性
    property string title: "默认值"
    property int count: 0

    // 信号
    signal clicked(var item)

    // 子元素
    Rectangle {
        id: background
        anchors.fill: parent
        color: "blue"
    }

    // 组件
    Component {
        id: delegateComponent
        Text { text: modelData }
    }

    // 函数
    function doSomething() {
        console.log("Clicked")
    }

    // 动画
    Behavior on opacity {
        NumberAnimation { duration: 200 }
    }
}
```

### 2. 导入检查清单

每个 QML 文件必须检查所需的导入：

| 组件 | 导入语句 |
|-----|---------|
| 基础控件 | `import QtQuick 2.15` |
| 按钮/输入框 | `import QtQuick.Controls 2.15` |
| 对话框 | `import QtQuick.Dialogs 2.15` |
| 动画 | `import QtQuick 2.15` (内置) |
| 网络 | `import QtQuick 2.15` + C++ 处理 |
| 本地存储 | `import QtQuick.LocalStorage 2.15` |

### 3. 禁止在 QML 中做的事情

- ❌ 禁止：直接进行网络请求（必须通过 C++）
- ❌ 禁止：直接操作数据库（必须通过 C++）
- ❌ 禁止：复杂业务逻辑（放在 C++ 中）
- ❌ 禁止：大文件 IO（必须通过 C++）

## Qt 组件速查

| 组件 | CMake 名称 | QML 导入 | 典型类 |
|-----|------------|----------|--------|
| Core | Core | 内置 | QObject, QString, QList |
| Gui | Gui | 内置 | QPainter, QColor |
| Quick | Quick | `QtQuick 2.x` | QQuickView |
| QuickControls2 | QuickControls2 | `QtQuick.Controls 2.x` | Button, TextField |
| Network | Network | `QtQuick 2.x` | QNetworkAccessManager |
| Sql | Sql | C++ 使用 | QSqlDatabase |
| Test | Test | C++ 使用 | QTest, QSignalSpy |
| Widgets | Widgets | `QtWidgets` | QMainWindow |

## 单元测试规范

### 1. 测试框架

```cpp
#include <QObject>
#include <QSignalSpy>
#include <QtTest>

class TestMyClass : public QObject {
    Q_OBJECT

private slots:
    void init() { }        // 每个测试前执行
    void cleanup() { }      // 每个测试后执行

    void testCase1() {
        MyClass obj;
        QVERIFY(obj.isValid());
        QCOMPARE(obj.value(), 42);
    }

    void testSignalEmission() {
        MyClass obj;
        QSignalSpy spy(&obj, &MyClass::valueChanged);

        obj.setValue(10);

        QVERIFY(spy.count() == 1);
        QVERIFY(spy.takeFirst().at(0).toInt() == 10);
    }
};

QTEST_MAIN(TestMyClass)
#include "test_myclass.moc"
```

### 2. Mock 对象

```cpp
// 使用 QSignalSpy 测试信号
QSignalSpy spy(object, &Object::signalName);

// 使用 fake 实现
class FakeNetworkAccessManager : public QNetworkAccessManager {
public:
    QNetworkReply *createRequest(Operation op, const QNetworkRequest &request, QIODevice *outgoingData = nullptr) override {
        // 返回模拟数据
        return new MockReply();
    }
};
```

## 部署与发布

### Windows

```cmake
# CMakeLists.txt 中添加（目标名根据项目实际可执行文件名修改）
if(WIN32)
    add_custom_command(TARGET appXDown POST_BUILD
        COMMAND windeployqt $<TARGET_FILE:appXDown> --qmldir ${CMAKE_PREFIX_PATH}/qml
    )
endif()
```

### 打包检查清单

- [ ] Qt 核心 DLL 是否包含
- [ ] 平台插件是否包含 (platforms/)
- [ ] QML 模块是否完整
- [ ] SQL 驱动是否包含 (sqldrivers/)
- [ ] 图标资源是否包含

## Qt Windows 部署与环境陷阱 (Deployment Gotchas) - 强制必读
由于 Windows 系统的 DLL 加载机制，如果 `.exe` 同级目录下缺少 Qt 的核心动态库（如 Qt6Gui.dll, Qt6Core.dll 等），系统会直接拦截进程并弹出 GUI 错误弹窗。
**注意：这种拦截绝对不会在终端（stderr/stdout）输出任何文字日志！你会看到终端被正常挂起，从而误以为程序运行成功。**

为了彻底杜绝此类"幽灵成功"现象，你必须严格遵守以下 CMake 编写规则：
1. **强制自动化部署**：在编写或修改 `CMakeLists.txt` 时，必须在可执行文件目标（如 `appXDown`）的最后，添加使用 `windeployqt` 的 `POST_BUILD` 钩子。
2. **标准实现代码**（必须包含在 CMake 脚本中）：
   ```cmake
   if(WIN32)
       add_custom_command(TARGET appXDown POST_BUILD
           COMMAND windeployqt $<TARGET_FILE:appXDown>
           COMMENT "Running windeployqt to copy Qt DLLs..."
       )
   endif()
   ```

## 常见问题处理

### 1. QML 类型未找到
- 检查 import 语句是否正确
- 确认模块已安装

### 2. 信号槽连接失败
- 检查信号和槽的参数类型是否匹配
- 确认对象已创建

### 3. 内存泄漏
- 检查 QObject 的 parent 是否正确设置
- 确认 moveToThread 后不再跨线程操作

### 4. 中文显示乱码
- 确保源文件保存为 UTF-8 编码
- CMake 添加：`-DCHARSET=utf-8`

### 5. 程序启动失败
- 检查 Qt DLL 是否在 exe 同目录
- 使用 `windeployqt` 自动部署

## 必检清单

每次修改 CMakeLists.txt 后，必须检查：

### find_package 完整性
- [ ] 使用 QNetworkAccessManager → 添加 Network
- [ ] 使用 QSqlDatabase → 添加 Sql
- [ ] 使用 QML/Quick → 添加 Quick, QuickControls2
- [ ] 使用 Widgets → 添加 Widgets
- [ ] 单元测试 → 添加 Test

### target_link_libraries 完整性
- [ ] 链接静态库时必须包含该库所有依赖的 Qt 组件
- [ ] 特别注意：Core 通常必须链接
- [ ] 数据库相关必须链接 Sql
- [ ] 网络相关必须链接 Network

### 路径兼容性
- [ ] 子目录中使用 CMAKE_CURRENT_SOURCE_DIR
- [ ] 不要假设 CMAKE_SOURCE_DIR 在子项目中可用

### Qt Creator 兼容性
- [ ] 修改 CMakeLists.txt 后删除 build 目录重新配置
- [ ] 验证所有目标都能编译