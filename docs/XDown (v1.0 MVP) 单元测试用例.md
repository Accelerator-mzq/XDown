###  单元测试用例 (Unit Test Cases)

**责任人：研发端 (C++ 开发 / Claude Code)** **测试工具建议：Qt Test 或 Google Test (GTest)** **核心目标：验证各个模块的边界条件、逻辑运算和数据库读写，不依赖外部真实网络和 UI。**

| 用例编号     | 测试模块/类      | 测试函数/方法                   | 测试场景 (输入/前置条件)                                     | 预期结果 (断言 Assertion)                                    |
| ------------ | ---------------- | ------------------------------- | ------------------------------------------------------------ | ------------------------------------------------------------ |
| **UT-1.1-1** | `DownloadTask`   | `formatSize()`                  | 输入 `1024` 字节；                                           | 返回字符串 `"1.00 KB"`；                                     |
| **UT-1.1-2** | `DownloadTask`   | `formatSize()`                  | 输入 `1048576` 字节;                                         | 返回 `"1.00 MB"`。                                           |
| **UT-1.2-1** | `DownloadTask`   | `progress()`                    | `downloadedBytes`=50, `totalBytes`=100；                     | 返回 `0.5`；                                                 |
| **UT-1.2-2** | `DownloadTask`   | `progress()`                    | `totalBytes`=0                                               | 当总大小为 0 时返回 `0.0` (防除零崩溃)。                     |
| **UT-1.3-1** | `DownloadTask`   | `formatSpeed()`                 | 输入 `1024` 字节/秒；                                        | 返回 `"1.00 KB/s"`；                                         |
| **UT-1.3-2** | `DownloadTask`   | `formatSpeed()`                 | 输入 `1048576` 字节/秒;                                      | 返回 `"1.00 MB/s"`；                                         |
| **UT-2.1**   | `DBManager`      | `insertTask()`                  | 构造一个状态为 `Waiting` 的 mock 任务实例并插入。            | 函数返回 `true`，使用 SQL 查询该 ID 能精准读出一致的数据。   |
| **UT-2.2**   | `DBManager`      | `hasDuplicateDownloadingTask()` | 数据库已存在一个 URL 为 `http://test.com/a.zip` 且状态为 `Downloading` 的记录。 | 传入相同 URL 查询，返回 `true`；传入不同 URL 或状态为 `Finished` 的 URL，返回 `false`。 |
| **UT-2.3**   | `DBManager`      | `updateTaskProgress()`          | 数据库已存在一个 ID 为 `test-id-001` 的任务；传入 `downloadedBytes`=50, `totalBytes`=100。 | 返回 `true`；再次查询该任务，确认 `downloadedBytes`=50, `totalBytes`=100。 |
| **UT-2.4**   | `DBManager`      | `updateTaskStatus()`            | 数据库已存在一个 ID 为 `test-id-002` 状态为 `Waiting` 的任务；传入状态 `Downloading`。 | 返回 `true`；再次查询该任务，确认状态已变为 `Downloading`。  |
| **UT-2.5**   | `DBManager`      | `loadAllTasks()`                | 数据库已插入 3 个不同状态的任务。                            | 返回包含 3 个任务的列表；每个任务字段与插入时一致。          |
| **UT-2.6**   | `DBManager`      | `deleteTask()`                  | 数据库已存在一个 ID 为 `test-id-003` 的任务。                | 返回 `true`；再次查询该 ID，返回空任务或查询失败。           |
| **UT-2.7**   | `DBManager`      | `updateTaskSpeed()`             | 数据库已存在一个 ID 为 `test-id-004` 的任务；传入 `speedBytesPerSec`=102400。 | 返回 `true`；再次查询该任务，确认 `speedBytesPerSec`=102400。 |
| **UT-3.1**   | `HttpDownloader` | `onReadyRead()`                 | 模拟一个 100KB 的网络回包，缓冲区设定为 64KB。               | 循环读取两次，第一次读取 64KB，第二次读取 36KB，不发生内存溢出。 |
| **UT-3.2**   | `HttpDownloader` | `getTaskId() / getTaskInfo()`   | 构造一个 taskId 为 `task-001` 的 HttpDownloader 实例。       | `getTaskId()` 返回 `"task-001"`；`getTaskInfo()` 返回的 task.id 一致。 |
| **UT-3.3**   | `HttpDownloader` | `start() / pause() / stop()`    | 构造一个 HttpDownloader 实例，任务状态为 `Waiting`。         | `start()` 后状态变为 `Downloading`；`pause()` 后状态变为 `Paused`；`stop()` 后清理资源。 |
| **UT-4.1**   | `DownloadEngine` | 调度逻辑                        | 设定最大并发数为 3，连续压入 4 个合法任务。                  | 活跃下载器字典 `m_activeDownloaders` 大小为 3，等待队列 `m_waitingQueue` 大小为 1。 |
| **UT-4.2-1** | `DownloadEngine` | `addNewTask()`                  | 调用 `addNewTask()` 传入合法 URL 和本地路径；                | 合法 URL 返回空字符串（成功）；                              |
| **UT-4.2-2** | `DownloadEngine` | `addNewTask()`                  | 传入无效 URL；                                               | 无效 URL 返回错误信息；                                      |
| **UT-4.3-1** | `DownloadEngine` | `isValidUrl()`                  | 传入合法 URL `http://test.com/file.zip`；                    | 合法 URL 返回 `true`；                                       |
| **UT-4.3-2** | `DownloadEngine` | `isValidUrl()`                  | 传入非法 URL `invalid-url`；                                 | 非法 URL 返回 `false`；                                      |
| **UT-4.4**   | `DownloadEngine` | `pauseTask() / resumeTask()`    | 引擎中存在一个状态为 `Downloading` 的任务；调用 `pauseTask()` 后再调用 `resumeTask()`。 | `pauseTask()` 后任务状态变为 `Paused`；`resumeTask()` 后任务状态恢复为 `Downloading`。 |
| **UT-4.5**   | `DownloadEngine` | `deleteTask()`                  | 引擎中存在一个 ID 为 `test-id-005` 的任务。                  | 删除后任务从 `m_tasks` 中移除；若 `deleteFile=true` 则本地文件也被删除。 |
| **UT-4.6**   | `DownloadEngine` | `getDefaultSavePath()`          | 系统默认下载路径为 `C:/Users/xxx/Downloads`。                | 返回的路径包含 `Downloads` 目录且路径存在。                  |
| **UT-4.7**   | `DownloadEngine` | `getAllTasks()`                 | 引擎中已添加 3 个任务。                                      | 返回包含 3 个任务的列表。                                    |
| **UT-5.1-1** | `TaskListModel`  | `addTask()`                     | 构造一个 TaskListModel 实例；添加一个任务后再移除。          | `rowCount()` 返回 1；                                        |
| **UT-5.1-2** | `TaskListModel`  | `removeTask()`                  | 构造一个 TaskListModel 实例后再移除。                        | 移除后 `rowCount()` 返回 0；                                 |
| **UT-5.2**   | `TaskListModel`  | `updateTaskProgress()`          | 模型中已存在一个 ID 为 `task-001` 的任务；调用 `updateTaskProgress()` 更新进度。 | 再次调用 `data()` 获取该任务，进度相关字段已更新。           |
| **UT-5.3**   | `TaskListModel`  | `rowCount() / data()`           | 模型中添加 2 个任务；调用 `data()` 获取索引 0 的任务。       | `rowCount()` 返回 2；返回的任务字段与添加时一致。            |
| **UT-5.4**   | `TaskListModel`  | `getTaskIndex()`                | 模型中添加 3 个任务，ID 分别为 `a`, `b`, `c`。               | `getTaskIndex("b")` 返回 1；`getTaskIndex("nonexistent")` 返回 -1。 |
| **UT-5.5**   | `TaskListModel`  | `clearAll()`                    | 模型中已添加多个任务。                                       | 调用后 `rowCount()` 返回 0。                                 |
| **UT-5.6**   | `TaskListModel`  | `roleNames()`                   | 构造 TaskListModel 实例。                                    | 返回的 Hash 包含所有定义的角色（如 IdRole, UrlRole, FileNameRole 等）。 |
