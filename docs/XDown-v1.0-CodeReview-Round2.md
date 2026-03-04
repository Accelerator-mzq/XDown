# XDown v1.0 MVP 代码审查报告（第二轮）

**审查日期**: 2026-03-04  
**对比基线**: 第一轮审查报告 (P0-1 ~ P2-7)  
**审查重点**: 验证修复 + 识别修复引入的新问题

---

## 一、上轮问题修复验证

### ✅ 已正确修复

| ID | 问题 | 修复方式 | 验证结论 |
|----|------|----------|----------|
| P0-1 | onFinished 双重连接 | 删除了 `initNetworkManager()` 中的 `m_netManager::finished` 连接 | ✅ 正确 |
| P0-4 | processEvents 不安全 | 删除了手动事件循环，改用 QThread 默认 `exec()` | ✅ 正确 |
| P1-1 | 完成信号重复处理 | `onDownloadFinished` 不再移除下载器和处理队列 | ✅ 正确 |
| P1-2 | pauseTask 内存未更新 | 添加了 `m_tasks[id].status = TaskStatus::Paused` | ✅ 正确 |
| P1-4 | 速度写入未节流 | 新增 `m_pendingSpeedUpdates`，在 `onFlushTimer` 中批量写入 | ✅ 正确 |
| P2-1 | TaskDelegate 高度问题 | 改为 `height: visible ? 80 : 0`，加了动画 | ✅ 正确 |
| P2-2 | 重复校验范围窄 | `checkDuplicateTask` 增加了 Waiting 状态检查，DB 层也同步 | ✅ 正确 |
| P2-4 | DB 连接名冲突 | 改用命名连接 `"xdown_main"` | ✅ 正确 |
| P2-5 | m_workerThread 死代码 | 从 DBManager 中移除 | ✅ 正确 |
| P2-6 | m_eventLoop 死代码 | 从 HttpDownloader.h 中移除 | ✅ 正确 |
| P2-7 | main.cpp 信号重复连接 | 合并为单个 lambda | ✅ 正确 |

### ⚠️ 修复不完整 / 引入了新问题

| ID | 问题 | 状态 | 详见 |
|----|------|------|------|
| P0-2 | 重定向信号累积 | ✅ 信号不再累积，但引入 m_file 泄漏 | NEW-2 |
| P0-3 | 析构跨线程删除 | ❌ deleteLater 在线程退出后无效 | NEW-1 |
| P1-3 | 200 响应重新下载 | ⚠️ 逻辑实现了，但有无限循环风险和 m_file 泄漏 | NEW-2, NEW-4 |
| P1-5 | Content-Length 解析时机 | ❌ 用了 static 变量，引入严重并发 Bug | NEW-0 |
| P2-3 | Content-Disposition 解析 | ⚠️ 代码写了但实际是死代码 | NEW-5 |

---

## 二、新发现的问题

### 🔴 NEW-0: `static` 局部变量被所有下载实例共享（严重并发 Bug）

**文件**: `HttpDownloader.cpp` 第254行

```cpp
void HttpDownloader::onReadyRead() {
    static qint64 prevDownloaded = 0;  // ← 所有实例共享！
```

`static` 局部变量在所有 `HttpDownloader` 实例之间共享。当 3 个下载任务同时运行时：

1. 任务 A 下载了 50MB，`prevDownloaded = 50MB`
2. 任务 B 首次收到 Content-Length=100MB，计算 `totalBytes = 100MB + 50MB = 150MB` ← **错误！**
3. 任务 B 的进度条显示永远到不了 100%

**影响**: 并发下载时所有任务的进度条和文件大小显示完全错乱。

**修复**: 改用成员变量：

```cpp
// HttpDownloader.h 中添加:
qint64 m_initialDownloadedBytes = 0;

// start() 中记录起始值:
m_initialDownloadedBytes = m_task.downloadedBytes;

// onReadyRead() 中使用:
if (m_task.totalBytes == 0 && m_reply->hasRawHeader("Content-Length")) {
    qint64 contentLength = m_reply->rawHeader("Content-Length").toLongLong();
    if (m_initialDownloadedBytes > 0 && !m_isRedirecting) {
        m_task.totalBytes = contentLength + m_initialDownloadedBytes;
    } else {
        m_task.totalBytes = contentLength;
    }
}
// 删除 prevDownloaded 相关的所有代码
```

---

### 🔴 NEW-1: 析构函数中 `deleteLater()` 在线程退出后无效（内存泄漏）

**文件**: `HttpDownloader.cpp` 第50-82行

```cpp
HttpDownloader::~HttpDownloader() {
    stop();                    // (1) 向事件队列投递 lambda
    if (m_thread) {
        m_thread->quit();      // (2) 立即终止事件循环
        m_thread->wait(3000);  // (3) 等待线程结束

        // (4) 此时事件循环已停止！
        m_reply->deleteLater();  // ← 投递到已停止的事件循环，永远不会执行
        m_file->deleteLater();   // ← 同上，内存泄漏
```

两个问题叠加：

**问题 A**: `stop()` 内部使用 `Qt::QueuedConnection` 投递 lambda 到工作线程事件循环。但下一行 `m_thread->quit()` 立即终止了事件循环。`stop()` 中的 `pause()`（关闭网络连接、flush 文件）**可能根本没有执行**。

**问题 B**: `deleteLater()` 向对象所属线程的事件循环投递删除事件。但线程已经 `quit()` + `wait()` 结束了，事件永远不会被处理。`m_reply` 和 `m_file` 泄漏。

**修复**: 线程结束后直接 `delete`（此时没有线程竞争，直接删除是安全的）：

```cpp
HttpDownloader::~HttpDownloader() {
    // 先尝试在工作线程中优雅停止
    if (m_thread && m_thread->isRunning()) {
        // 用 BlockingQueuedConnection 确保 pause 执行完毕
        QMetaObject::invokeMethod(this, [this]() {
            if (m_reply) {
                m_reply->abort();
                m_reply->deleteLater();
                m_reply = nullptr;
            }
            if (m_speedTimer) m_speedTimer->stop();
            if (m_file && m_file->isOpen()) {
                m_file->flush();
                m_file->close();
            }
        }, Qt::BlockingQueuedConnection);

        m_thread->quit();
        m_thread->wait(3000);
        if (m_thread->isRunning()) {
            m_thread->terminate();
            m_thread->wait(1000);
        }
    }

    // 线程已结束，安全直接删除
    delete m_file;
    m_file = nullptr;
    delete m_thread;
    m_thread = nullptr;
    // m_netManager 和 m_speedTimer 是 this 的子对象，随 this 析构自动删除
}
```

---

### 🟡 NEW-2: `m_file` 在重定向和重新下载路径中泄漏

**文件**: `HttpDownloader.cpp`

**路径 A — 重定向** (第451-467行):
```cpp
void HttpDownloader::onRedirected(const QUrl& url) {
    if (m_file && m_file->isOpen()) {
        m_file->close();
        m_file->remove();
        // ← m_file 没有 delete，也没有置 nullptr
    }
    // ...
    start();  // ← start() 第140行: m_file = new QFile(...)  旧指针泄漏！
}
```

**路径 B — 200 重新下载** (第334-348行):
```cpp
} else if (m_file && m_file->exists()) {
    m_file->close();
    m_file->remove();
    // ← 同样没有 delete
    start();  // ← 泄漏
}
```

**路径 C — pause → resume 路径**: `pause()` 关闭 m_file 但不删除。不过 `resumeTask()` 创建的是新的 `HttpDownloader` 实例，旧实例整体被 `deleteLater`，所以这条路径不泄漏。

**修复**: 在 `onRedirected` 和 `onFinished` 的重新下载分支中，关闭后 delete：

```cpp
if (m_file) {
    if (m_file->isOpen()) m_file->close();
    m_file->remove();
    delete m_file;
    m_file = nullptr;
}
```

同时在 `start()` 的 `m_file = new QFile(...)` 之前加上防御：

```cpp
if (m_file) {
    if (m_file->isOpen()) m_file->close();
    delete m_file;
}
m_file = new QFile(m_task.localPath);
```

---

### 🟡 NEW-3: `onFinished` 200 响应重新下载可能无限循环

**文件**: `HttpDownloader.cpp` 第326-349行

当服务器不支持断点续传（返回 200 而非 206）时，代码会重置 `downloadedBytes=0` 并调用 `start()` 重新下载。但第二次请求完成后，`onFinished` 再次收到 200，此时 `downloadedBytes > 0`，条件 `statusCode == 200 && m_task.downloadedBytes > 0 && !m_isRedirecting` 又成立。

虽然如果第二次下载完整完成了（`downloadedBytes >= totalBytes`），第331行的 guard 会拦住。但如果 Content-Length 未提供（`totalBytes == 0`），两个分支都不匹配，代码会掉入成功分支——这恰好是正确行为。所以**正常情况下不会死循环**。

但万一服务器行为异常（如 Content-Length 不断变化），就可能无限重启。

**修复**: 添加重启计数器：

```cpp
// 头文件中添加:
int m_restartCount = 0;
static const int MAX_RESTARTS = 1;

// onFinished 中:
} else if (m_file && m_file->exists() && m_restartCount < MAX_RESTARTS) {
    m_restartCount++;
    // ... restart logic
```

---

### 🟢 NEW-4: Content-Disposition 解析实际上是死代码

**文件**: `HttpDownloader.cpp` 第500-516行

`parseFileName()` 只在 `onRedirected()` 中被调用，但 `onRedirected()` 在调用 `parseFileName()` 之前已经将 `m_reply` 置为 `nullptr`（第456-457行）。所以 `parseFileName()` 中的 Content-Disposition 解析分支永远不会执行：

```cpp
if (m_reply && m_reply->hasRawHeader("Content-Disposition")) {
    // ← m_reply 在 onRedirected 中已经是 nullptr，永远不会进来
```

要真正实现 Content-Disposition 解析，应该在 `onReadyRead()` 首次收到数据时调用（此时响应头已可用）：

```cpp
void HttpDownloader::onReadyRead() {
    if (!m_reply || !m_file) return;

    // 首次收到数据时解析文件名
    if (m_task.totalBytes == 0) {
        parseFileName();  // 此时 m_reply 有效，可以读取 Content-Disposition
    }
    // ...
```

---

### 🟢 NEW-5: Content-Disposition 正则表达式不完整

**文件**: `HttpDownloader.cpp` 第504行

```cpp
QRegularExpression regex(R"(filename\*=([^;\s]+)|filename=["']([^"']+)["'])");
```

两个问题：

**A. 不匹配无引号文件名**

很多服务器返回 `Content-Disposition: attachment; filename=report.pdf`（无引号），当前正则只匹配有引号的形式。

**B. `filename*=` 未剥离字符集前缀**

RFC 5987 规定 `filename*=UTF-8''%E4%B8%AD%E6%96%87.pdf`，当前代码把 `UTF-8''` 前缀也包含在文件名中。

**修复**:

```cpp
// 匹配三种形式：filename*=, filename="quoted", filename=unquoted
QRegularExpression regex(
    R"(filename\*=(?:UTF-8''|utf-8'')([^;\s]+)|)"
    R"(filename=["']([^"']+)["']|)"
    R"(filename=([^;\s"']+))");
QRegularExpressionMatch match = regex.match(disposition);
if (match.hasMatch()) {
    QString raw = match.captured(1);  // filename*= (已去前缀)
    if (raw.isEmpty()) raw = match.captured(2);  // filename="quoted"
    if (raw.isEmpty()) raw = match.captured(3);  // filename=unquoted
    if (!raw.isEmpty()) {
        name = QUrl::fromPercentEncoding(raw.toUtf8());
    }
}
```

---

## 三、问题优先级总览

| 优先级 | ID | 问题 | 影响 |
|--------|-----|------|------|
| **🔴 P0** | NEW-0 | static 变量并发共享 | 多任务同时下载时进度/大小全部错乱 |
| **🔴 P0** | NEW-1 | 析构 deleteLater 无效 + stop 竞态 | 内存泄漏，网络连接未正确关闭，文件未 flush |
| **🟡 P1** | NEW-2 | m_file 在重定向/重新下载时泄漏 | 每次重定向泄漏一个 QFile 对象 |
| **🟡 P1** | NEW-3 | 200 重新下载无限循环风险 | 异常服务器可能触发 |
| **🟢 P2** | NEW-4 | Content-Disposition 解析是死代码 | 功能不生效 |
| **🟢 P2** | NEW-5 | 正则不匹配无引号文件名 | 部分服务器文件名解析失败 |

---

## 四、总评

上轮 **11 项 P0~P2 问题已修复 8 项**（P0-1, P0-4, P1-1, P1-2, P1-4, P2-1, P2-2, P2-4~P2-7），质量有明显提升。

但修复过程中引入了 **2 个新的 P0 级问题**：

1. **`static qint64 prevDownloaded`** 是最紧迫的——改成成员变量只需几分钟，不改的话并发下载全部出错。
2. **析构函数的生命周期管理**需要将 `deleteLater()` 改为线程退出后直接 `delete`，并用 `BlockingQueuedConnection` 确保 `stop()` 执行完毕。

这两个修完后，剩下的 P1 `m_file` 泄漏也顺手一起清理，代码质量就可以达到 v1.0 发布标准了。
