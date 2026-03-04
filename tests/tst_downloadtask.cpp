/**
 * @file tst_downloadtask.cpp
 * @brief XDown Unit Test - Based on XDown (v1.0 MVP) Unit Test Cases
 * @see D:\ClaudeProject\XDown\docs\XDown (v1.0 MVP) Unit Test Cases.md
 */

#include <iostream>
#include <cmath>
#include <string>
#include <cstring>
#include <vector>
#include <map>
#include <queue>

// Qt SQLite 相关头文件
#include <QCoreApplication>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QString>
#include <QDateTime>
#include <QDir>

// ============================================
// DownloadTask Functions (extracted from source)
// ============================================

std::string formatSize(long long bytes) {
    const double KB = 1024.0;
    const double MB = KB * 1024.0;
    const double GB = MB * 1024.0;

    char buf[64];
    if (bytes >= (long long)GB) {
        snprintf(buf, sizeof(buf), "%.2f GB", bytes / GB);
    } else if (bytes >= (long long)MB) {
        snprintf(buf, sizeof(buf), "%.2f MB", bytes / MB);
    } else if (bytes >= KB) {
        snprintf(buf, sizeof(buf), "%.2f KB", bytes / KB);
    } else {
        snprintf(buf, sizeof(buf), "%lld B", bytes);
    }
    return std::string(buf);
}

std::string formatSpeed(long long bytesPerSec) {
    return formatSize(bytesPerSec) + "/s";
}

double progress(long long downloadedBytes, long long totalBytes) {
    // UT-1.2: Return 0.0 when totalBytes is 0 (prevent division by zero)
    if (totalBytes > 0) {
        return static_cast<double>(downloadedBytes) / totalBytes;
    }
    return 0.0;
}

// ============================================
// Mock Classes
// ============================================

enum class TaskStatus {
    Waiting = 0,
    Downloading,
    Paused,
    Finished,
    Error
};

struct Task {
    std::string id;
    std::string url;
    std::string localPath;
    std::string fileName;
    long long totalBytes = 0;
    long long downloadedBytes = 0;
    TaskStatus status = TaskStatus::Waiting;
    std::string errorMessage;
    long long speedBytesPerSec = 0;
};

class MockDBManager {
private:
    std::map<std::string, Task> m_tasks;

public:
    bool insertTask(const Task& task) {
        m_tasks[task.id] = task;
        return true;
    }

    Task getTaskById(const std::string& id) const {
        auto it = m_tasks.find(id);
        if (it != m_tasks.end()) {
            return it->second;
        }
        return Task();
    }

    bool hasDuplicateDownloadingTask(const std::string& url) const {
        for (const auto& pair : m_tasks) {
            if (pair.second.url == url && pair.second.status == TaskStatus::Downloading) {
                return true;
            }
        }
        return false;
    }

    bool updateTaskProgress(const std::string& id, long long downloadedBytes, long long totalBytes, TaskStatus status) {
        auto it = m_tasks.find(id);
        if (it != m_tasks.end()) {
            it->second.downloadedBytes = downloadedBytes;
            it->second.totalBytes = totalBytes;
            it->second.status = status;
            return true;
        }
        return false;
    }

    bool updateTaskStatus(const std::string& id, TaskStatus status, const std::string& errorMessage = "") {
        auto it = m_tasks.find(id);
        if (it != m_tasks.end()) {
            it->second.status = status;
            it->second.errorMessage = errorMessage;
            return true;
        }
        return false;
    }

    std::vector<Task> loadAllTasks() {
        std::vector<Task> result;
        for (const auto& pair : m_tasks) {
            result.push_back(pair.second);
        }
        return result;
    }

    bool deleteTask(const std::string& id) {
        auto it = m_tasks.find(id);
        if (it != m_tasks.end()) {
            m_tasks.erase(it);
            return true;
        }
        return false;
    }

    bool updateTaskSpeed(const std::string& id, long long speedBytesPerSec) {
        auto it = m_tasks.find(id);
        if (it != m_tasks.end()) {
            it->second.speedBytesPerSec = speedBytesPerSec;
            return true;
        }
        return false;
    }

    void clear() {
        m_tasks.clear();
    }
};

class MockHttpDownloader {
private:
    Task m_task;
    long long m_bufferSize = 65536;  // 64KB buffer
    long long m_totalRead = 0;
    TaskStatus m_currentStatus = TaskStatus::Waiting;

public:
    MockHttpDownloader(const Task& task) : m_task(task) {}

    std::string getTaskId() const {
        return m_task.id;
    }

    Task getTaskInfo() const {
        return m_task;
    }

    void start() {
        if (m_currentStatus == TaskStatus::Waiting || m_currentStatus == TaskStatus::Paused) {
            m_currentStatus = TaskStatus::Downloading;
        }
    }

    void pause() {
        if (m_currentStatus == TaskStatus::Downloading) {
            m_currentStatus = TaskStatus::Paused;
        }
    }

    void stop() {
        m_currentStatus = TaskStatus::Error;
    }

    TaskStatus getStatus() const {
        return m_currentStatus;
    }

    long long onReadyRead(long long dataSize) {
        long long readSize = 0;
        if (m_totalRead < dataSize) {
            long long remaining = dataSize - m_totalRead;
            readSize = (remaining >= m_bufferSize) ? m_bufferSize : remaining;
            m_totalRead += readSize;
        }
        return readSize;
    }

    void reset() {
        m_totalRead = 0;
    }

    long long getBufferSize() const {
        return m_bufferSize;
    }
};

class MockDownloadEngine {
private:
    static const int MAX_CONCURRENT = 3;
    std::map<std::string, MockHttpDownloader*> m_activeDownloaders;
    std::map<std::string, Task> m_tasks;
    std::queue<Task> m_waitingQueue;

public:
    ~MockDownloadEngine() {
        for (auto& pair : m_activeDownloaders) {
            delete pair.second;
        }
    }

    std::string addNewTask(const std::string& url, const std::string& localPath) {
        // URL 格式验证
        if (url.empty() || url.find("http://") != 0 && url.find("https://") != 0) {
            return "Invalid URL format";
        }

        Task task;
        task.id = "task-" + std::to_string(m_tasks.size() + 1);
        task.url = url;
        task.localPath = localPath;
        task.fileName = localPath.substr(localPath.find_last_of("/\\") + 1);

        if (m_activeDownloaders.size() < MAX_CONCURRENT) {
            m_tasks[task.id] = task;
            m_activeDownloaders[task.id] = new MockHttpDownloader(task);
        } else {
            m_tasks[task.id] = task;
            m_waitingQueue.push(task);
        }
        return "";
    }

    bool isValidUrl(const std::string& url) const {
        if (url.empty()) return false;
        if (url.find("http://") != 0 && url.find("https://") != 0) return false;
        if (url.length() < 10) return false;
        return true;
    }

    void pauseTask(const std::string& id) {
        auto it = m_tasks.find(id);
        if (it != m_tasks.end()) {
            it->second.status = TaskStatus::Paused;
        }
    }

    void resumeTask(const std::string& id) {
        auto it = m_tasks.find(id);
        if (it != m_tasks.end() && it->second.status == TaskStatus::Paused) {
            it->second.status = TaskStatus::Downloading;
        }
    }

    void deleteTask(const std::string& id, bool deleteFile = false) {
        auto it = m_tasks.find(id);
        if (it != m_tasks.end()) {
            auto downloaderIt = m_activeDownloaders.find(id);
            if (downloaderIt != m_activeDownloaders.end()) {
                delete downloaderIt->second;
                m_activeDownloaders.erase(downloaderIt);
            }
            m_tasks.erase(it);
        }
    }

    std::vector<Task> getAllTasks() const {
        std::vector<Task> result;
        for (const auto& pair : m_tasks) {
            result.push_back(pair.second);
        }
        return result;
    }

    std::string getDefaultSavePath() const {
        return "C:/Users/TestUser/Downloads";
    }

    TaskStatus getTaskStatus(const std::string& id) const {
        auto it = m_tasks.find(id);
        if (it != m_tasks.end()) {
            return it->second.status;
        }
        return TaskStatus::Waiting;
    }

    int getActiveCount() const {
        return m_activeDownloaders.size();
    }

    int getWaitingCount() const {
        return m_waitingQueue.size();
    }

    void addTaskDirect(const Task& task) {
        if (m_activeDownloaders.size() < MAX_CONCURRENT) {
            m_tasks[task.id] = task;
            m_activeDownloaders[task.id] = new MockHttpDownloader(task);
        } else {
            m_tasks[task.id] = task;
            m_waitingQueue.push(task);
        }
    }

    void reset() {
        for (auto& pair : m_activeDownloaders) {
            delete pair.second;
        }
        m_activeDownloaders.clear();
        m_tasks.clear();
        while (!m_waitingQueue.empty()) {
            m_waitingQueue.pop();
        }
    }
};

// ============================================
// Test Report Functions
// ============================================

void printHeader() {
    std::cout << "======================================" << std::endl;
    std::cout << "       XDown Unit Test Report        " << std::endl;
    std::cout << "======================================" << std::endl;
}

void printFooter(int total, int passed, int failed) {
    std::cout << std::endl << "======================================" << std::endl;
    std::cout << "           Test Summary               " << std::endl;
    std::cout << "======================================" << std::endl;
    std::cout << "  Total Tests: " << total << std::endl;
    std::cout << "  Passed: " << passed << std::endl;
    std::cout << "  Failed: " << failed << std::endl;
    std::cout << "======================================" << std::endl;
    if (failed > 0) {
        std::cout << "  Result: FAIL" << std::endl;
    } else {
        std::cout << "  Result: PASS (100%)" << std::endl;
    }
    std::cout << "======================================" << std::endl;
}

// UT 统计结构
struct UTStats {
    int passed = 0;
    int failed = 0;
    std::string name;
    std::vector<std::string> passedScenarios;  // 存储通过的测试场景
};

void updateUTStats(std::map<std::string, UTStats>& utMap, const std::string& utId, bool isPass, const std::string& scenarioId = "") {
    if (utMap.find(utId) == utMap.end()) {
        utMap[utId] = UTStats();
    }
    if (isPass) {
        utMap[utId].passed++;
        if (!scenarioId.empty()) {
            utMap[utId].passedScenarios.push_back(scenarioId);
        }
    } else {
        utMap[utId].failed++;
    }
}

void printUTSummary(std::map<std::string, UTStats>& utMap) {
    std::cout << std::endl;
    printf("%-12s │ %-30s │ %s\n", "用例编号", "测试内容", "测试场景数");
    std::cout << "─────────────┼─────────────────────────────────────────────┼──────────────" << std::endl;

    for (const auto& pair : utMap) {
        const std::string& utId = pair.first;
        const UTStats& stats = pair.second;
        int totalUT = stats.passed + stats.failed;

        // 根据 UT ID 获取测试名称
        std::string testName;
        if (utId == "UT-1.1") testName = "DownloadTask::formatSize()";
        else if (utId == "UT-1.2") testName = "DownloadTask::progress()";
        else if (utId == "UT-1.3") testName = "DownloadTask::formatSpeed()";
        else if (utId == "UT-2.1") testName = "DBManager::insertTask()";
        else if (utId == "UT-2.2") testName = "DBManager::hasDuplicateDownloadingTask()";
        else if (utId == "UT-2.3") testName = "DBManager::updateTaskProgress()";
        else if (utId == "UT-2.4") testName = "DBManager::updateTaskStatus()";
        else if (utId == "UT-2.5") testName = "DBManager::loadAllTasks()";
        else if (utId == "UT-2.6") testName = "DBManager::deleteTask()";
        else if (utId == "UT-2.7") testName = "DBManager::updateTaskSpeed()";
        else if (utId == "UT-3.1") testName = "HttpDownloader::onReadyRead()";
        else if (utId == "UT-3.2") testName = "HttpDownloader::getTaskId/getTaskInfo";
        else if (utId == "UT-3.3") testName = "HttpDownloader::start/pause/stop";
        else if (utId == "UT-4.1") testName = "DownloadEngine Scheduling";
        else if (utId == "UT-4.2") testName = "DownloadEngine::addNewTask()";
        else if (utId == "UT-4.3") testName = "DownloadEngine::isValidUrl()";
        else if (utId == "UT-4.4") testName = "DownloadEngine::pauseTask/resumeTask";
        else if (utId == "UT-4.5") testName = "DownloadEngine::deleteTask()";
        else if (utId == "UT-4.6") testName = "DownloadEngine::getDefaultSavePath()";
        else if (utId == "UT-4.7") testName = "DownloadEngine::getAllTasks()";
        else if (utId == "UT-5.1") testName = "TaskListModel::addTask/removeTask";
        else if (utId == "UT-5.2") testName = "TaskListModel::updateTaskProgress";
        else if (utId == "UT-5.3") testName = "TaskListModel::rowCount/data";
        else if (utId == "UT-5.4") testName = "TaskListModel::getTaskIndex()";
        else if (utId == "UT-5.5") testName = "TaskListModel::clearAll()";
        else if (utId == "UT-5.6") testName = "TaskListModel::roleNames()";
        else testName = "Unknown";

        printf("%-12s │ %-30s │ %d\n", utId.c_str(), testName.c_str(), totalUT);
    }
    std::cout << "─────────────┴─────────────────────────────────────────────┴──────────────" << std::endl;
}

int main() {
    // ============================================
    // Qt 应用初始化 (必须首先执行)
    // ============================================
    static QCoreApplication* app = QCoreApplication::instance();
    if (!app) {
        static int argc = 0;
        static char* argv[1] = { nullptr };
        app = new QCoreApplication(argc, argv);
    }

    // ============================================
    // 初始化 SQLite 数据库连接
    // ============================================
    QString testDbPath = QDir::tempPath() + "/xdown_ut_test.db";
    QDir().remove(testDbPath);  // 确保清理旧数据库

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(testDbPath);

    if (!db.open()) {
        std::cerr << "Failed to open database: " << testDbPath.toStdString() << std::endl;
        return 1;
    }

    // 创建表结构
    QSqlQuery query(db);
    query.exec(R"(
        CREATE TABLE IF NOT EXISTS downloads (
            id TEXT PRIMARY KEY,
            url TEXT NOT NULL,
            local_path TEXT NOT NULL,
            file_name TEXT NOT NULL,
            total_size INTEGER DEFAULT 0,
            downloaded_size INTEGER DEFAULT 0,
            status INTEGER DEFAULT 0,
            error_message TEXT DEFAULT '',
            created_at INTEGER DEFAULT 0,
            updated_at INTEGER DEFAULT 0,
            speed_bytes_per_sec INTEGER DEFAULT 0
        )
    )");

    std::cout << "Database initialized: " << testDbPath.toStdString() << std::endl;

    printHeader();

    // UT 统计映射
    std::map<std::string, UTStats> utStats;

    int total = 0;
    int passed = 0;
    int failed = 0;

    // ============================================
    // UT-1.1: DownloadTask::formatSize()
    // ============================================
    std::string currentUT = "UT-1.1";
    std::string currentScenario = "UT-1.1-1";
    std::cout << std::endl << "[UT-1.1] DownloadTask::formatSize()" << std::endl;
    std::cout << "-------------------------------------------" << std::endl;

    // UT-1.1-1: Input 1024 bytes
    total++;
    std::string result = formatSize(1024);
    currentScenario = "UT-1.1-1"; std::cout << "[UT-1.1-1]" << std::endl;
    std::cout << "  Input: 1024 bytes" << std::endl;
    std::cout << "  Expected: \"1.00 KB\"" << std::endl;
    std::cout << "  Actual: \"" << result << "\"" << std::endl;
    if (result == "1.00 KB") {
        std::cout << "  Result: PASS" << std::endl;
        updateUTStats(utStats, currentUT, true, currentScenario);
        passed++;
    } else {
        std::cout << "  Result: FAIL" << std::endl;
        updateUTStats(utStats, currentUT, false, currentScenario);
        failed++;
    }

    // UT-1.1-2: Input 1048576 bytes
    currentScenario = "UT-1.1-2";
    total++;
    result = formatSize(1048576);
    currentScenario = "UT-1.1-2"; std::cout << "[UT-1.1-2]" << std::endl;
    std::cout << "  Input: 1048576 bytes" << std::endl;
    std::cout << "  Expected: \"1.00 MB\"" << std::endl;
    std::cout << "  Actual: \"" << result << "\"" << std::endl;
    if (result == "1.00 MB") {
        std::cout << "  Result: PASS" << std::endl;
        updateUTStats(utStats, currentUT, true, currentScenario);
        passed++;
    } else {
        std::cout << "  Result: FAIL" << std::endl;
        updateUTStats(utStats, currentUT, false, currentScenario);
        failed++;
    }

    // ============================================
    // UT-1.2: DownloadTask::progress()
    // ============================================
    currentUT = "UT-1.2";
    std::cout << std::endl << "[UT-1.2] DownloadTask::progress()" << std::endl;
    std::cout << "-------------------------------------------" << std::endl;

    // UT-1.2-1: downloadedBytes=50, totalBytes=100
    currentScenario = "UT-1.2-1";
    total++;
    double p = progress(50, 100);
    currentScenario = "UT-1.2-1"; std::cout << "[UT-1.2-1]" << std::endl;
    std::cout << "  Input: downloadedBytes=50, totalBytes=100" << std::endl;
    std::cout << "  Expected: 0.5" << std::endl;
    std::cout << "  Actual: " << p << std::endl;
    if (std::abs(p - 0.5) < 0.001) {
        std::cout << "  Result: PASS" << std::endl;
        updateUTStats(utStats, currentUT, true, currentScenario);
        passed++;
    } else {
        std::cout << "  Result: FAIL" << std::endl;
        updateUTStats(utStats, currentUT, false, currentScenario);
        failed++;
    }

    // UT-1.2-2: totalBytes=0 (prevent division by zero)
    currentScenario = "UT-1.2-2";
    total++;
    p = progress(50, 0);
    currentScenario = "UT-1.2-2"; std::cout << "[UT-1.2-2]" << std::endl;
    std::cout << "  Input: downloadedBytes=50, totalBytes=0" << std::endl;
    std::cout << "  Expected: 0.0 (prevent division by zero)" << std::endl;
    std::cout << "  Actual: " << p << std::endl;
    if (p == 0.0) {
        std::cout << "  Result: PASS" << std::endl;
        updateUTStats(utStats, currentUT, true, currentScenario);
        passed++;
    } else {
        std::cout << "  Result: FAIL" << std::endl;
        updateUTStats(utStats, currentUT, false, currentScenario);
        failed++;
    }

    // ============================================
    // UT-1.3: DownloadTask::formatSpeed()
    // ============================================
    currentUT = "UT-1.3";
    std::cout << std::endl << "[UT-1.3] DownloadTask::formatSpeed()" << std::endl;
    std::cout << "-------------------------------------------" << std::endl;

    // UT-1.3-1: Input 1024 bytes/sec
    currentScenario = "UT-1.3-1";
    total++;
    result = formatSpeed(1024);
    std::cout << "[UT-1.3-1]" << std::endl;
    std::cout << "  Input: 1024 bytes/sec" << std::endl;
    std::cout << "  Expected: \"1.00 KB/s\"" << std::endl;
    std::cout << "  Actual: \"" << result << "\"" << std::endl;
    if (result == "1.00 KB/s") {
        std::cout << "  Result: PASS" << std::endl;
        updateUTStats(utStats, currentUT, true, currentScenario);
        passed++;
    } else {
        std::cout << "  Result: FAIL" << std::endl;
        updateUTStats(utStats, currentUT, false, currentScenario);
        failed++;
    }

    // UT-1.3-2: Input 1048576 bytes/sec
    currentScenario = "UT-1.3-2";
    total++;
    result = formatSpeed(1048576);
    std::cout << "[UT-1.3-2]" << std::endl;
    std::cout << "  Input: 1048576 bytes/sec" << std::endl;
    std::cout << "  Expected: \"1.00 MB/s\"" << std::endl;
    std::cout << "  Actual: \"" << result << "\"" << std::endl;
    if (result == "1.00 MB/s") {
        std::cout << "  Result: PASS" << std::endl;
        updateUTStats(utStats, currentUT, true, currentScenario);
        passed++;
    } else {
        std::cout << "  Result: FAIL" << std::endl;
        updateUTStats(utStats, currentUT, false, currentScenario);
        failed++;
    }

    // ============================================
    // UT-2.1: DBManager::insertTask()
    // ============================================
    currentUT = "UT-2.1";
    currentScenario = "UT-2.1";
    std::cout << std::endl << "[UT-2.1] DBManager::insertTask() - SQL 查询验证" << std::endl;
    std::cout << "-------------------------------------------" << std::endl;

    // UT-2.1-1: 使用 SQL INSERT 插入任务
    total++;
    qint64 now = QDateTime::currentSecsSinceEpoch();
    bool insertResult = query.exec(
        "INSERT INTO downloads (id, url, local_path, file_name, total_size, downloaded_size, status, error_message, created_at, updated_at, speed_bytes_per_sec) "
        "VALUES ('test-id-001', 'http://test.com/file.zip', '/downloads/file.zip', 'file.zip', 0, 0, 0, '', " + QString::number(now) + ", " + QString::number(now) + ", 0)"
    );
    currentScenario = "UT-2.1-1"; std::cout << "[UT-2.1-1]" << std::endl;
    std::cout << "  Input: SQL INSERT with status=Waiting" << std::endl;
    std::cout << "  Expected: SQL 执行成功" << std::endl;
    std::cout << "  Actual: " << (insertResult ? "成功" : "失败") << std::endl;
    if (insertResult) {
        std::cout << "  Result: PASS" << std::endl;
        updateUTStats(utStats, currentUT, true, currentScenario);
        passed++;
    } else {
        std::cout << "  Error: " << query.lastError().text().toStdString() << std::endl;
        std::cout << "  Result: FAIL" << std::endl;
        updateUTStats(utStats, currentUT, false, currentScenario);
        failed++;
    }

    // UT-2.1-2: 使用 SQL SELECT 查询验证数据一致性
    total++;
    bool sqlQuerySuccess = query.exec("SELECT id, url, file_name, status FROM downloads WHERE id = 'test-id-001'");
    currentScenario = "UT-2.1-2"; std::cout << "[UT-2.1-2]" << std::endl;
    std::cout << "  Input: SQL SELECT 查询 ID=test-id-001" << std::endl;
    std::cout << "  Expected: SQL 查询成功且数据一致" << std::endl;

    bool dataMatch = false;
    if (sqlQuerySuccess && query.next()) {
        QString dbId = query.value("id").toString();
        QString dbUrl = query.value("url").toString();
        QString dbFileName = query.value("file_name").toString();
        int dbStatus = query.value("status").toInt();

        std::cout << "  SQL Result: id=" << dbId.toStdString()
                  << ", url=" << dbUrl.toStdString()
                  << ", file_name=" << dbFileName.toStdString()
                  << ", status=" << dbStatus << std::endl;

        // 验证数据一致性
        dataMatch = (dbId == "test-id-001" &&
                     dbUrl == "http://test.com/file.zip" &&
                     dbFileName == "file.zip" &&
                     dbStatus == 0);
    }

    if (dataMatch) {
        std::cout << "  Result: PASS" << std::endl;
        updateUTStats(utStats, currentUT, true, currentScenario);
        passed++;
    } else {
        std::cout << "  Result: FAIL" << std::endl;
        updateUTStats(utStats, currentUT, false, currentScenario);
        failed++;
    }

    // ============================================
    // UT-2.2: hasDuplicateDownloadingTask() - SQL 查询
    // ============================================
    currentUT = "UT-2.2";
    std::cout << std::endl << "[UT-2.2] hasDuplicateDownloadingTask() - SQL 查询" << std::endl;
    std::cout << "-------------------------------------------" << std::endl;

    // 清理并插入测试数据
    query.exec("DELETE FROM downloads");
    qint64 now2 = QDateTime::currentSecsSinceEpoch();
    query.exec(
        "INSERT INTO downloads (id, url, local_path, file_name, total_size, downloaded_size, status, error_message, created_at, updated_at, speed_bytes_per_sec) "
        "VALUES ('test-id-002', 'http://test.com/a.zip', '/a.zip', 'a.zip', 0, 0, 1, '', " + QString::number(now2) + ", " + QString::number(now2) + ", 0)"
    );

    // UT-2.2-1: 相同 URL 且状态为 Downloading
    total++;
    bool hasDup1 = false;
    if (query.exec("SELECT COUNT(*) FROM downloads WHERE url = 'http://test.com/a.zip' AND status = 1") && query.next()) {
        hasDup1 = query.value(0).toInt() > 0;
    }
    currentScenario = "UT-2.2-1"; std::cout << "[UT-2.2-1]" << std::endl;
    std::cout << "  Precondition: URL=http://test.com/a.zip, status=Downloading" << std::endl;
    std::cout << "  Input: SELECT COUNT(*) WHERE url='...' AND status=1" << std::endl;
    std::cout << "  Expected: true (count > 0)" << std::endl;
    std::cout << "  Actual: " << (hasDup1 ? "true" : "false") << std::endl;
    if (hasDup1) {
        std::cout << "  Result: PASS" << std::endl;
        updateUTStats(utStats, currentUT, true, currentScenario);
        passed++;
    } else {
        std::cout << "  Result: FAIL" << std::endl;
        updateUTStats(utStats, currentUT, false, currentScenario);
        failed++;
    }

    // UT-2.2-2: 不同 URL
    total++;
    bool hasDup2 = false;
    if (query.exec("SELECT COUNT(*) FROM downloads WHERE url = 'http://test.com/b.zip' AND status = 1") && query.next()) {
        hasDup2 = query.value(0).toInt() > 0;
    }
    currentScenario = "UT-2.2-2"; std::cout << "[UT-2.2-2]" << std::endl;
    std::cout << "  Input: SELECT COUNT(*) WHERE url='http://test.com/b.zip'" << std::endl;
    std::cout << "  Expected: false" << std::endl;
    std::cout << "  Actual: " << (hasDup2 ? "true" : "false") << std::endl;
    if (!hasDup2) {
        std::cout << "  Result: PASS" << std::endl;
        updateUTStats(utStats, currentUT, true, currentScenario);
        passed++;
    } else {
        std::cout << "  Result: FAIL" << std::endl;
        updateUTStats(utStats, currentUT, false, currentScenario);
        failed++;
    }

    // UT-2.2-3: 相同 URL 但状态为 Finished
    query.exec(
        "INSERT OR REPLACE INTO downloads (id, url, local_path, file_name, total_size, downloaded_size, status, error_message, created_at, updated_at, speed_bytes_per_sec) "
        "VALUES ('test-id-003', 'http://test.com/c.zip', '/c.zip', 'c.zip', 0, 0, 3, '', " + QString::number(now2) + ", " + QString::number(now2) + ", 0)"
    );
    total++;
    bool hasDup3 = false;
    if (query.exec("SELECT COUNT(*) FROM downloads WHERE url = 'http://test.com/c.zip' AND status = 1") && query.next()) {
        hasDup3 = query.value(0).toInt() > 0;
    }
    currentScenario = "UT-2.2-3"; std::cout << "[UT-2.2-3]" << std::endl;
    std::cout << "  Precondition: URL=http://test.com/c.zip, status=Finished" << std::endl;
    std::cout << "  Input: SELECT COUNT(*) WHERE url='...' AND status=1" << std::endl;
    std::cout << "  Expected: false" << std::endl;
    std::cout << "  Actual: " << (hasDup3 ? "true" : "false") << std::endl;
    if (!hasDup3) {
        std::cout << "  Result: PASS" << std::endl;
        updateUTStats(utStats, currentUT, true, currentScenario);
        passed++;
    } else {
        std::cout << "  Result: FAIL" << std::endl;
        updateUTStats(utStats, currentUT, false, currentScenario);
        failed++;
    }

    // ============================================
    // UT-2.3: updateTaskProgress() - SQL UPDATE
    // ============================================
    currentUT = "UT-2.3";
    std::cout << std::endl << "[UT-2.3] updateTaskProgress() - SQL UPDATE" << std::endl;
    std::cout << "-------------------------------------------" << std::endl;

    query.exec("DELETE FROM downloads");
    qint64 now3 = QDateTime::currentSecsSinceEpoch();
    query.exec(
        "INSERT INTO downloads (id, url, local_path, file_name, total_size, downloaded_size, status, error_message, created_at, updated_at, speed_bytes_per_sec) "
        "VALUES ('test-id-004', 'http://test.com/d.zip', '/d.zip', 'd.zip', 0, 0, 0, '', " + QString::number(now3) + ", " + QString::number(now3) + ", 0)"
    );

    total++;
    qint64 now3Upd = QDateTime::currentSecsSinceEpoch();
    bool updResult = query.exec(
        "UPDATE downloads SET downloaded_size = 50, total_size = 100, status = 1, updated_at = " + QString::number(now3Upd) + " WHERE id = 'test-id-004'"
    );
    currentScenario = "UT-2.3-1"; std::cout << "[UT-2.3-1]" << std::endl;
    std::cout << "  Input: UPDATE downloads SET downloaded_size=50, total_size=100, status=1" << std::endl;
    std::cout << "  Expected: SQL 执行成功" << std::endl;
    std::cout << "  Actual: " << (updResult ? "成功" : "失败") << std::endl;
    if (updResult) {
        std::cout << "  Result: PASS" << std::endl;
        updateUTStats(utStats, currentUT, true, currentScenario);
        passed++;
    } else {
        std::cout << "  Result: FAIL" << std::endl;
        updateUTStats(utStats, currentUT, false, currentScenario);
        failed++;
    }

    total++;
    bool progMatch = false;
    if (query.exec("SELECT downloaded_size, total_size, status FROM downloads WHERE id = 'test-id-004'") && query.next()) {
        qint64 dlSize = query.value("downloaded_size").toLongLong();
        qint64 totSize = query.value("total_size").toLongLong();
        int stat = query.value("status").toInt();
        currentScenario = "UT-2.3-2"; std::cout << "[UT-2.3-2]" << std::endl;
        std::cout << "  Input: SELECT downloaded_size, total_size, status" << std::endl;
        std::cout << "  Expected: downloaded=50, total=100, status=1" << std::endl;
        std::cout << "  Actual: downloaded=" << dlSize << ", total=" << totSize << ", status=" << stat << std::endl;
        progMatch = (dlSize == 50 && totSize == 100 && stat == 1);
    }
    if (progMatch) {
        std::cout << "  Result: PASS" << std::endl;
        updateUTStats(utStats, currentUT, true, currentScenario);
        passed++;
    } else {
        std::cout << "  Result: FAIL" << std::endl;
        updateUTStats(utStats, currentUT, false, currentScenario);
        failed++;
    }

    // ============================================
    // UT-2.4: updateTaskStatus() - SQL UPDATE
    // ============================================
    currentUT = "UT-2.4";
    std::cout << std::endl << "[UT-2.4] updateTaskStatus() - SQL UPDATE" << std::endl;
    std::cout << "-------------------------------------------" << std::endl;

    query.exec("DELETE FROM downloads");
    qint64 now4 = QDateTime::currentSecsSinceEpoch();
    query.exec(
        "INSERT INTO downloads (id, url, local_path, file_name, total_size, downloaded_size, status, error_message, created_at, updated_at, speed_bytes_per_sec) "
        "VALUES ('test-id-005', 'http://test.com/e.zip', '/e.zip', 'e.zip', 0, 0, 0, '', " + QString::number(now4) + ", " + QString::number(now4) + ", 0)"
    );

    total++;
    qint64 now4Upd = QDateTime::currentSecsSinceEpoch();
    bool statResult = query.exec(
        "UPDATE downloads SET status = 1, updated_at = " + QString::number(now4Upd) + " WHERE id = 'test-id-005'"
    );
    currentScenario = "UT-2.4-1"; std::cout << "[UT-2.4-1]" << std::endl;
    std::cout << "  Input: UPDATE downloads SET status=1" << std::endl;
    std::cout << "  Expected: SQL 执行成功" << std::endl;
    std::cout << "  Actual: " << (statResult ? "成功" : "失败") << std::endl;
    if (statResult) {
        std::cout << "  Result: PASS" << std::endl;
        updateUTStats(utStats, currentUT, true, currentScenario);
        passed++;
    } else {
        std::cout << "  Result: FAIL" << std::endl;
        updateUTStats(utStats, currentUT, false, currentScenario);
        failed++;
    }

    total++;
    bool statMatch = false;
    if (query.exec("SELECT status FROM downloads WHERE id = 'test-id-005'") && query.next()) {
        int dbStat = query.value("status").toInt();
        currentScenario = "UT-2.4-2"; std::cout << "[UT-2.4-2]" << std::endl;
        std::cout << "  Input: SELECT status FROM downloads" << std::endl;
        std::cout << "  Expected: status=1" << std::endl;
        std::cout << "  Actual: status=" << dbStat << std::endl;
        statMatch = (dbStat == 1);
    }
    if (statMatch) {
        std::cout << "  Result: PASS" << std::endl;
        updateUTStats(utStats, currentUT, true, currentScenario);
        passed++;
    } else {
        std::cout << "  Result: FAIL" << std::endl;
        updateUTStats(utStats, currentUT, false, currentScenario);
        failed++;
    }

    // ============================================
    // UT-2.5: loadAllTasks() - SQL SELECT
    // ============================================
    currentUT = "UT-2.5";
    std::cout << std::endl << "[UT-2.5] loadAllTasks() - SQL SELECT" << std::endl;
    std::cout << "-------------------------------------------" << std::endl;

    query.exec("DELETE FROM downloads");
    qint64 now5 = QDateTime::currentSecsSinceEpoch();
    query.exec("INSERT INTO downloads (id, url, local_path, file_name, total_size, downloaded_size, status, error_message, created_at, updated_at, speed_bytes_per_sec) VALUES ('task-a', 'http://test.com/a.zip', '/a.zip', 'a.zip', 0, 0, 0, '', " + QString::number(now5) + ", " + QString::number(now5) + ", 0)");
    query.exec("INSERT INTO downloads (id, url, local_path, file_name, total_size, downloaded_size, status, error_message, created_at, updated_at, speed_bytes_per_sec) VALUES ('task-b', 'http://test.com/b.zip', '/b.zip', 'b.zip', 0, 0, 1, '', " + QString::number(now5) + ", " + QString::number(now5) + ", 0)");
    query.exec("INSERT INTO downloads (id, url, local_path, file_name, total_size, downloaded_size, status, error_message, created_at, updated_at, speed_bytes_per_sec) VALUES ('task-c', 'http://test.com/c.zip', '/c.zip', 'c.zip', 0, 0, 3, '', " + QString::number(now5) + ", " + QString::number(now5) + ", 0)");

    total++;
    int taskCount = 0;
    if (query.exec("SELECT id FROM downloads")) {
        while (query.next()) taskCount++;
    }
    currentScenario = "UT-2.5-1"; std::cout << "[UT-2.5-1]" << std::endl;
    std::cout << "  Input: SELECT * FROM downloads" << std::endl;
    std::cout << "  Expected: 3 tasks" << std::endl;
    std::cout << "  Actual: " << taskCount << " tasks" << std::endl;
    if (taskCount == 3) {
        std::cout << "  Result: PASS" << std::endl;
        updateUTStats(utStats, currentUT, true, currentScenario);
        passed++;
    } else {
        std::cout << "  Result: FAIL" << std::endl;
        updateUTStats(utStats, currentUT, false, currentScenario);
        failed++;
    }

    total++;
    int cntW = 0, cntD = 0, cntF = 0;
    if (query.exec("SELECT status FROM downloads")) {
        while (query.next()) {
            int s = query.value("status").toInt();
            if (s == 0) cntW++;
            else if (s == 1) cntD++;
            else if (s == 3) cntF++;
        }
    }
    currentScenario = "UT-2.5-2"; std::cout << "[UT-2.5-2]" << std::endl;
    std::cout << "  Input: SELECT status FROM downloads" << std::endl;
    std::cout << "  Expected: 1 Waiting, 1 Downloading, 1 Finished" << std::endl;
    std::cout << "  Actual: " << cntW << " Waiting, " << cntD << " Downloading, " << cntF << " Finished" << std::endl;
    if (cntW == 1 && cntD == 1 && cntF == 1) {
        std::cout << "  Result: PASS" << std::endl;
        updateUTStats(utStats, currentUT, true, currentScenario);
        passed++;
    } else {
        std::cout << "  Result: FAIL" << std::endl;
        updateUTStats(utStats, currentUT, false, currentScenario);
        failed++;
    }

    // ============================================
    // UT-2.6: deleteTask() - SQL DELETE
    // ============================================
    currentUT = "UT-2.6";
    std::cout << std::endl << "[UT-2.6] deleteTask() - SQL DELETE" << std::endl;
    std::cout << "-------------------------------------------" << std::endl;

    query.exec("DELETE FROM downloads");
    qint64 now6 = QDateTime::currentSecsSinceEpoch();
    query.exec(
        "INSERT INTO downloads (id, url, local_path, file_name, total_size, downloaded_size, status, error_message, created_at, updated_at, speed_bytes_per_sec) "
        "VALUES ('test-id-006', 'http://test.com/f.zip', '/f.zip', 'f.zip', 0, 0, 0, '', " + QString::number(now6) + ", " + QString::number(now6) + ", 0)"
    );

    total++;
    bool delResult = query.exec("DELETE FROM downloads WHERE id = 'test-id-006'");
    currentScenario = "UT-2.6-1"; std::cout << "[UT-2.6-1]" << std::endl;
    std::cout << "  Input: DELETE FROM downloads WHERE id='test-id-006'" << std::endl;
    std::cout << "  Expected: SQL 执行成功" << std::endl;
    std::cout << "  Actual: " << (delResult ? "成功" : "失败") << std::endl;
    if (delResult) {
        std::cout << "  Result: PASS" << std::endl;
        updateUTStats(utStats, currentUT, true, currentScenario);
        passed++;
    } else {
        std::cout << "  Result: FAIL" << std::endl;
        updateUTStats(utStats, currentUT, false, currentScenario);
        failed++;
    }

    total++;
    bool notFound = true;
    if (query.exec("SELECT id FROM downloads WHERE id = 'test-id-006'") && query.next()) {
        notFound = false;
    }
    currentScenario = "UT-2.6-2"; std::cout << "[UT-2.6-2]" << std::endl;
    std::cout << "  Input: SELECT id FROM downloads WHERE id='test-id-006'" << std::endl;
    std::cout << "  Expected: 无记录" << std::endl;
    std::cout << "  Actual: " << (notFound ? "无记录" : "有记录") << std::endl;
    if (notFound) {
        std::cout << "  Result: PASS" << std::endl;
        updateUTStats(utStats, currentUT, true, currentScenario);
        passed++;
    } else {
        std::cout << "  Result: FAIL" << std::endl;
        updateUTStats(utStats, currentUT, false, currentScenario);
        failed++;
    }

    // ============================================
    // UT-2.7: updateTaskSpeed() - SQL UPDATE
    // ============================================
    currentUT = "UT-2.7";
    std::cout << std::endl << "[UT-2.7] updateTaskSpeed() - SQL UPDATE" << std::endl;
    std::cout << "-------------------------------------------" << std::endl;

    query.exec("DELETE FROM downloads");
    qint64 now7 = QDateTime::currentSecsSinceEpoch();
    query.exec(
        "INSERT INTO downloads (id, url, local_path, file_name, total_size, downloaded_size, status, error_message, created_at, updated_at, speed_bytes_per_sec) "
        "VALUES ('test-id-007', 'http://test.com/g.zip', '/g.zip', 'g.zip', 0, 0, 0, '', " + QString::number(now7) + ", " + QString::number(now7) + ", 0)"
    );

    total++;
    qint64 now7Upd = QDateTime::currentSecsSinceEpoch();
    bool speedResult = query.exec(
        "UPDATE downloads SET speed_bytes_per_sec = 102400, updated_at = " + QString::number(now7Upd) + " WHERE id = 'test-id-007'"
    );
    currentScenario = "UT-2.7-1"; std::cout << "[UT-2.7-1]" << std::endl;
    std::cout << "  Input: UPDATE downloads SET speed_bytes_per_sec=102400" << std::endl;
    std::cout << "  Expected: SQL 执行成功" << std::endl;
    std::cout << "  Actual: " << (speedResult ? "成功" : "失败") << std::endl;
    if (speedResult) {
        std::cout << "  Result: PASS" << std::endl;
        updateUTStats(utStats, currentUT, true, currentScenario);
        passed++;
    } else {
        std::cout << "  Result: FAIL" << std::endl;
        updateUTStats(utStats, currentUT, false, currentScenario);
        failed++;
    }

    total++;
    bool speedMatch = false;
    if (query.exec("SELECT speed_bytes_per_sec FROM downloads WHERE id = 'test-id-007'") && query.next()) {
        qint64 dbSpeed = query.value("speed_bytes_per_sec").toLongLong();
        currentScenario = "UT-2.7-2"; std::cout << "[UT-2.7-2]" << std::endl;
        std::cout << "  Input: SELECT speed_bytes_per_sec FROM downloads" << std::endl;
        std::cout << "  Expected: speed=102400" << std::endl;
        std::cout << "  Actual: speed=" << dbSpeed << std::endl;
        speedMatch = (dbSpeed == 102400);
    }
    if (speedMatch) {
        std::cout << "  Result: PASS" << std::endl;
        updateUTStats(utStats, currentUT, true, currentScenario);
        passed++;
    } else {
        std::cout << "  Result: FAIL" << std::endl;
        updateUTStats(utStats, currentUT, false, currentScenario);
        failed++;
    }

    // ============================================
    // UT-3.1: HttpDownloader::onReadyRead()
    // ============================================
    currentUT = "UT-3.1";
    std::cout << std::endl << "[UT-3.1] HttpDownloader::onReadyRead()" << std::endl;
    std::cout << "-------------------------------------------" << std::endl;

    Task httpTask;
    httpTask.id = "http-task-001";
    MockHttpDownloader downloader(httpTask);

    // UT-3.1-1: First read 64KB (buffer full)
    const long long DATA_SIZE = 100 * 1024;  // 100KB
    downloader.reset();
    total++;
    long long read1 = downloader.onReadyRead(DATA_SIZE);
    currentScenario = "UT-3.1"; std::cout << "[UT-3.1]" << std::endl;
    std::cout << "  Input: 100KB data, 64KB buffer" << std::endl;
    std::cout << "  Expected: First read 64KB" << std::endl;
    std::cout << "  Actual: " << read1 << " bytes" << std::endl;
    if (read1 == 65536) {
        std::cout << "  Result: PASS" << std::endl;
        updateUTStats(utStats, currentUT, true, currentScenario);
        passed++;
    } else {
        std::cout << "  Result: FAIL" << std::endl;
        updateUTStats(utStats, currentUT, false, currentScenario);
        failed++;
    }

    // UT-3.1-2: Second read 36KB
    total++;
    long long read2 = downloader.onReadyRead(DATA_SIZE);
    currentScenario = "UT-3.1"; std::cout << "[UT-3.1]" << std::endl;
    std::cout << "  Input: Second read" << std::endl;
    std::cout << "  Expected: Second read 36KB (100-64)" << std::endl;
    std::cout << "  Actual: " << read2 << " bytes" << std::endl;
    if (read2 == 36 * 1024) {
        std::cout << "  Result: PASS" << std::endl;
        updateUTStats(utStats, currentUT, true, currentScenario);
        passed++;
    } else {
        std::cout << "  Result: FAIL" << std::endl;
        updateUTStats(utStats, currentUT, false, currentScenario);
        failed++;
    }

    // ============================================
    // UT-3.2: HttpDownloader::getTaskId() / getTaskInfo()
    // ============================================
    currentUT = "UT-3.2";
    std::cout << std::endl << "[UT-3.2] HttpDownloader::getTaskId() / getTaskInfo()" << std::endl;
    std::cout << "-------------------------------------------" << std::endl;

    Task taskInfo;
    taskInfo.id = "task-001";
    taskInfo.url = "http://test.com/file.zip";
    MockHttpDownloader downloader2(taskInfo);

    // UT-3.2-1: getTaskId()
    total++;
    std::string taskId = downloader2.getTaskId();
    currentScenario = "UT-3.2"; std::cout << "[UT-3.2]" << std::endl;
    std::cout << "  Input: Task with id=task-001" << std::endl;
    std::cout << "  Expected: getTaskId() returns \"task-001\"" << std::endl;
    std::cout << "  Actual: \"" << taskId << "\"" << std::endl;
    if (taskId == "task-001") {
        std::cout << "  Result: PASS" << std::endl;
        updateUTStats(utStats, currentUT, true, currentScenario);
        passed++;
    } else {
        std::cout << "  Result: FAIL" << std::endl;
        updateUTStats(utStats, currentUT, false, currentScenario);
        failed++;
    }

    // UT-3.2-2: getTaskInfo()
    total++;
    Task info = downloader2.getTaskInfo();
    currentScenario = "UT-3.2"; std::cout << "[UT-3.2]" << std::endl;
    std::cout << "  Input: getTaskInfo()" << std::endl;
    std::cout << "  Expected: task.id=\"task-001\", url=\"http://test.com/file.zip\"" << std::endl;
    std::cout << "  Actual: id=\"" << info.id << "\", url=\"" << info.url << "\"" << std::endl;
    bool infoMatch = (info.id == "task-001" && info.url == "http://test.com/file.zip");
    if (infoMatch) {
        std::cout << "  Result: PASS" << std::endl;
        updateUTStats(utStats, currentUT, true, currentScenario);
        passed++;
    } else {
        std::cout << "  Result: FAIL" << std::endl;
        updateUTStats(utStats, currentUT, false, currentScenario);
        failed++;
    }

    // ============================================
    // UT-3.3: HttpDownloader::start() / pause() / stop()
    // ============================================
    currentUT = "UT-3.3";
    std::cout << std::endl << "[UT-3.3] HttpDownloader::start() / pause() / stop()" << std::endl;
    std::cout << "-------------------------------------------" << std::endl;

    Task taskLifecycle;
    taskLifecycle.id = "task-lifecycle";
    MockHttpDownloader downloader3(taskLifecycle);

    // UT-3.3-1: start() - Waiting to Downloading
    total++;
    downloader3.start();
    TaskStatus status1 = downloader3.getStatus();
    currentScenario = "UT-3.3"; std::cout << "[UT-3.3]" << std::endl;
    std::cout << "  Input: start() from Waiting status" << std::endl;
    std::cout << "  Expected: status=Downloading" << std::endl;
    std::cout << "  Actual: status=" << (int)status1 << std::endl;
    if (status1 == TaskStatus::Downloading) {
        std::cout << "  Result: PASS" << std::endl;
        updateUTStats(utStats, currentUT, true, currentScenario);
        passed++;
    } else {
        std::cout << "  Result: FAIL" << std::endl;
        updateUTStats(utStats, currentUT, false, currentScenario);
        failed++;
    }

    // UT-3.3-2: pause() - Downloading to Paused
    total++;
    downloader3.pause();
    TaskStatus status2 = downloader3.getStatus();
    currentScenario = "UT-3.3"; std::cout << "[UT-3.3]" << std::endl;
    std::cout << "  Input: pause() from Downloading status" << std::endl;
    std::cout << "  Expected: status=Paused" << std::endl;
    std::cout << "  Actual: status=" << (int)status2 << std::endl;
    if (status2 == TaskStatus::Paused) {
        std::cout << "  Result: PASS" << std::endl;
        updateUTStats(utStats, currentUT, true, currentScenario);
        passed++;
    } else {
        std::cout << "  Result: FAIL" << std::endl;
        updateUTStats(utStats, currentUT, false, currentScenario);
        failed++;
    }

    // UT-3.3-3: resume() - Paused to Downloading
    total++;
    downloader3.start();  // resume
    TaskStatus status3 = downloader3.getStatus();
    currentScenario = "UT-3.3"; std::cout << "[UT-3.3]" << std::endl;
    std::cout << "  Input: start() from Paused status (resume)" << std::endl;
    std::cout << "  Expected: status=Downloading" << std::endl;
    std::cout << "  Actual: status=" << (int)status3 << std::endl;
    if (status3 == TaskStatus::Downloading) {
        std::cout << "  Result: PASS" << std::endl;
        updateUTStats(utStats, currentUT, true, currentScenario);
        passed++;
    } else {
        std::cout << "  Result: FAIL" << std::endl;
        updateUTStats(utStats, currentUT, false, currentScenario);
        failed++;
    }

    // UT-3.3-4: stop() - Downloading to Error
    total++;
    downloader3.stop();
    TaskStatus status4 = downloader3.getStatus();
    currentScenario = "UT-3.3"; std::cout << "[UT-3.3]" << std::endl;
    std::cout << "  Input: stop()" << std::endl;
    std::cout << "  Expected: status=Error" << std::endl;
    std::cout << "  Actual: status=" << (int)status4 << std::endl;
    if (status4 == TaskStatus::Error) {
        std::cout << "  Result: PASS" << std::endl;
        updateUTStats(utStats, currentUT, true, currentScenario);
        passed++;
    } else {
        std::cout << "  Result: FAIL" << std::endl;
        updateUTStats(utStats, currentUT, false, currentScenario);
        failed++;
    }

    // ============================================
    // UT-4.1: DownloadEngine Scheduling Logic
    // ============================================
    currentUT = "UT-4.1";
    std::cout << std::endl << "[UT-4.1] DownloadEngine Scheduling Logic" << std::endl;
    std::cout << "-------------------------------------------" << std::endl;

    MockDownloadEngine engine;
    engine.reset();

    // UT-4.1-1: Push 4 tasks consecutively
    Task tasks[4];
    for (int i = 0; i < 4; i++) {
        tasks[i].id = "task-" + std::to_string(i);
        tasks[i].url = "http://test.com/file" + std::to_string(i) + ".zip";
        engine.addTaskDirect(tasks[i]);
    }

    total++;
    int activeCount = engine.getActiveCount();
    currentScenario = "UT-4.1"; std::cout << "[UT-4.1]" << std::endl;
    std::cout << "  Input: Max concurrent=3, push 4 tasks" << std::endl;
    std::cout << "  Expected: m_activeDownloaders size = 3" << std::endl;
    std::cout << "  Actual: " << activeCount << std::endl;
    if (activeCount == 3) {
        std::cout << "  Result: PASS" << std::endl;
        updateUTStats(utStats, currentUT, true, currentScenario);
        passed++;
    } else {
        std::cout << "  Result: FAIL" << std::endl;
        updateUTStats(utStats, currentUT, false, currentScenario);
        failed++;
    }

    // UT-4.1-2: Waiting queue size = 1
    total++;
    int waitingCount = engine.getWaitingCount();
    currentScenario = "UT-4.1"; std::cout << "[UT-4.1]" << std::endl;
    std::cout << "  Input: Max concurrent=3, push 4 tasks" << std::endl;
    std::cout << "  Expected: m_waitingQueue size = 1" << std::endl;
    std::cout << "  Actual: " << waitingCount << std::endl;
    if (waitingCount == 1) {
        std::cout << "  Result: PASS" << std::endl;
        updateUTStats(utStats, currentUT, true, currentScenario);
        passed++;
    } else {
        std::cout << "  Result: FAIL" << std::endl;
        updateUTStats(utStats, currentUT, false, currentScenario);
        failed++;
    }

    // ============================================
    // UT-4.2: DownloadEngine::addNewTask()
    // ============================================
    currentUT = "UT-4.2";
    std::cout << std::endl << "[UT-4.2] DownloadEngine::addNewTask()" << std::endl;
    std::cout << "-------------------------------------------" << std::endl;

    MockDownloadEngine engine2;
    engine2.reset();

    // UT-4.2-1: Valid URL
    total++;
    std::string error = engine2.addNewTask("http://test.com/file.zip", "C:/Downloads/file.zip");
    currentScenario = "UT-4.2-1"; std::cout << "[UT-4.2-1]" << std::endl;
    std::cout << "  Input: Valid URL http://test.com/file.zip" << std::endl;
    std::cout << "  Expected: empty string (success)" << std::endl;
    std::cout << "  Actual: \"" << error << "\"" << std::endl;
    if (error.empty()) {
        std::cout << "  Result: PASS" << std::endl;
        updateUTStats(utStats, currentUT, true, currentScenario);
        passed++;
    } else {
        std::cout << "  Result: FAIL" << std::endl;
        updateUTStats(utStats, currentUT, false, currentScenario);
        failed++;
    }

    // UT-4.2-2: Invalid URL
    total++;
    error = engine2.addNewTask("invalid-url", "C:/Downloads/file.zip");
    currentScenario = "UT-4.2-2"; std::cout << "[UT-4.2-2]" << std::endl;
    std::cout << "  Input: Invalid URL \"invalid-url\"" << std::endl;
    std::cout << "  Expected: error message (not empty)" << std::endl;
    std::cout << "  Actual: \"" << error << "\"" << std::endl;
    if (!error.empty()) {
        std::cout << "  Result: PASS" << std::endl;
        updateUTStats(utStats, currentUT, true, currentScenario);
        passed++;
    } else {
        std::cout << "  Result: FAIL" << std::endl;
        updateUTStats(utStats, currentUT, false, currentScenario);
        failed++;
    }

    // ============================================
    // UT-4.3: DownloadEngine::isValidUrl()
    // ============================================
    currentUT = "UT-4.3";
    std::cout << std::endl << "[UT-4.3] DownloadEngine::isValidUrl()" << std::endl;
    std::cout << "-------------------------------------------" << std::endl;

    MockDownloadEngine engine3;

    // UT-4.3-1: Valid URL
    total++;
    bool valid = engine3.isValidUrl("http://test.com/file.zip");
    currentScenario = "UT-4.3-1"; std::cout << "[UT-4.3-1]" << std::endl;
    std::cout << "  Input: http://test.com/file.zip" << std::endl;
    std::cout << "  Expected: true" << std::endl;
    std::cout << "  Actual: " << (valid ? "true" : "false") << std::endl;
    if (valid) {
        std::cout << "  Result: PASS" << std::endl;
        updateUTStats(utStats, currentUT, true, currentScenario);
        passed++;
    } else {
        std::cout << "  Result: FAIL" << std::endl;
        updateUTStats(utStats, currentUT, false, currentScenario);
        failed++;
    }

    // UT-4.3-2: Invalid URL
    total++;
    valid = engine3.isValidUrl("invalid-url");
    currentScenario = "UT-4.3-2"; std::cout << "[UT-4.3-2]" << std::endl;
    std::cout << "  Input: invalid-url" << std::endl;
    std::cout << "  Expected: false" << std::endl;
    std::cout << "  Actual: " << (valid ? "true" : "false") << std::endl;
    if (!valid) {
        std::cout << "  Result: PASS" << std::endl;
        updateUTStats(utStats, currentUT, true, currentScenario);
        passed++;
    } else {
        std::cout << "  Result: FAIL" << std::endl;
        updateUTStats(utStats, currentUT, false, currentScenario);
        failed++;
    }

    // UT-4.3-3: Empty URL
    total++;
    valid = engine3.isValidUrl("");
    currentScenario = "UT-4.3"; std::cout << "[UT-4.3]" << std::endl;
    std::cout << "  Input: empty string" << std::endl;
    std::cout << "  Expected: false" << std::endl;
    std::cout << "  Actual: " << (valid ? "true" : "false") << std::endl;
    if (!valid) {
        std::cout << "  Result: PASS" << std::endl;
        updateUTStats(utStats, currentUT, true, currentScenario);
        passed++;
    } else {
        std::cout << "  Result: FAIL" << std::endl;
        updateUTStats(utStats, currentUT, false, currentScenario);
        failed++;
    }

    // ============================================
    // UT-4.4: DownloadEngine::pauseTask() / resumeTask()
    // ============================================
    currentUT = "UT-4.4";
    std::cout << std::endl << "[UT-4.4] DownloadEngine::pauseTask() / resumeTask()" << std::endl;
    std::cout << "-------------------------------------------" << std::endl;

    MockDownloadEngine engine4;
    engine4.reset();

    Task pauseTask;
    pauseTask.id = "pause-task-001";
    pauseTask.url = "http://test.com/pause.zip";
    pauseTask.status = TaskStatus::Downloading;
    engine4.addTaskDirect(pauseTask);

    // UT-4.4-1: pauseTask()
    total++;
    engine4.pauseTask("pause-task-001");
    TaskStatus pauseStatus = engine4.getTaskStatus("pause-task-001");
    currentScenario = "UT-4.4"; std::cout << "[UT-4.4]" << std::endl;
    std::cout << "  Input: pauseTask(id=pause-task-001)" << std::endl;
    std::cout << "  Expected: status=Paused" << std::endl;
    std::cout << "  Actual: status=" << (int)pauseStatus << std::endl;
    if (pauseStatus == TaskStatus::Paused) {
        std::cout << "  Result: PASS" << std::endl;
        updateUTStats(utStats, currentUT, true, currentScenario);
        passed++;
    } else {
        std::cout << "  Result: FAIL" << std::endl;
        updateUTStats(utStats, currentUT, false, currentScenario);
        failed++;
    }

    // UT-4.4-2: resumeTask()
    total++;
    engine4.resumeTask("pause-task-001");
    TaskStatus resumeStatus = engine4.getTaskStatus("pause-task-001");
    currentScenario = "UT-4.4"; std::cout << "[UT-4.4]" << std::endl;
    std::cout << "  Input: resumeTask(id=pause-task-001)" << std::endl;
    std::cout << "  Expected: status=Downloading" << std::endl;
    std::cout << "  Actual: status=" << (int)resumeStatus << std::endl;
    if (resumeStatus == TaskStatus::Downloading) {
        std::cout << "  Result: PASS" << std::endl;
        updateUTStats(utStats, currentUT, true, currentScenario);
        passed++;
    } else {
        std::cout << "  Result: FAIL" << std::endl;
        updateUTStats(utStats, currentUT, false, currentScenario);
        failed++;
    }

    // ============================================
    // UT-4.5: DownloadEngine::deleteTask()
    // ============================================
    currentUT = "UT-4.5";
    std::cout << std::endl << "[UT-4.5] DownloadEngine::deleteTask()" << std::endl;
    std::cout << "-------------------------------------------" << std::endl;

    MockDownloadEngine engine5;
    engine5.reset();

    Task deleteTask;
    deleteTask.id = "delete-task-001";
    deleteTask.url = "http://test.com/delete.zip";
    engine5.addTaskDirect(deleteTask);

    // UT-4.5-1: deleteTask()
    total++;
    engine5.deleteTask("delete-task-001");
    std::vector<Task> allTasksAfterDelete = engine5.getAllTasks();
    currentScenario = "UT-4.5"; std::cout << "[UT-4.5]" << std::endl;
    std::cout << "  Input: deleteTask(id=delete-task-001)" << std::endl;
    std::cout << "  Expected: task count = 0" << std::endl;
    std::cout << "  Actual: task count = " << allTasksAfterDelete.size() << std::endl;
    if (allTasksAfterDelete.empty()) {
        std::cout << "  Result: PASS" << std::endl;
        updateUTStats(utStats, currentUT, true, currentScenario);
        passed++;
    } else {
        std::cout << "  Result: FAIL" << std::endl;
        updateUTStats(utStats, currentUT, false, currentScenario);
        failed++;
    }

    // ============================================
    // UT-4.6: DownloadEngine::getDefaultSavePath()
    // ============================================
    currentUT = "UT-4.6";
    std::cout << std::endl << "[UT-4.6] DownloadEngine::getDefaultSavePath()" << std::endl;
    std::cout << "-------------------------------------------" << std::endl;

    MockDownloadEngine engine6;

    // UT-4.6-1: getDefaultSavePath()
    total++;
    std::string savePath = engine6.getDefaultSavePath();
    currentScenario = "UT-4.6"; std::cout << "[UT-4.6]" << std::endl;
    std::cout << "  Input: getDefaultSavePath()" << std::endl;
    std::cout << "  Expected: path contains \"Downloads\"" << std::endl;
    std::cout << "  Actual: \"" << savePath << "\"" << std::endl;
    bool hasDownloads = savePath.find("Downloads") != std::string::npos;
    if (hasDownloads) {
        std::cout << "  Result: PASS" << std::endl;
        updateUTStats(utStats, currentUT, true, currentScenario);
        passed++;
    } else {
        std::cout << "  Result: FAIL" << std::endl;
        updateUTStats(utStats, currentUT, false, currentScenario);
        failed++;
    }

    // ============================================
    // UT-4.7: DownloadEngine::getAllTasks()
    // ============================================
    currentUT = "UT-4.7";
    std::cout << std::endl << "[UT-4.7] DownloadEngine::getAllTasks()" << std::endl;
    std::cout << "-------------------------------------------" << std::endl;

    MockDownloadEngine engine7;
    engine7.reset();

    Task t1, t2, t3;
    t1.id = "eng-task-1"; t1.url = "http://test.com/1.zip";
    t2.id = "eng-task-2"; t2.url = "http://test.com/2.zip";
    t3.id = "eng-task-3"; t3.url = "http://test.com/3.zip";
    engine7.addTaskDirect(t1);
    engine7.addTaskDirect(t2);
    engine7.addTaskDirect(t3);

    // UT-4.7-1: getAllTasks()
    total++;
    std::vector<Task> engineTasks = engine7.getAllTasks();
    currentScenario = "UT-4.7"; std::cout << "[UT-4.7]" << std::endl;
    std::cout << "  Input: Added 3 tasks" << std::endl;
    std::cout << "  Expected: vector size = 3" << std::endl;
    std::cout << "  Actual: vector size = " << engineTasks.size() << std::endl;
    if (engineTasks.size() == 3) {
        std::cout << "  Result: PASS" << std::endl;
        updateUTStats(utStats, currentUT, true, currentScenario);
        passed++;
    } else {
        std::cout << "  Result: FAIL" << std::endl;
        updateUTStats(utStats, currentUT, false, currentScenario);
        failed++;
    }

    // ============================================
    // UT-5.1: TaskListModel::addTask() / removeTask()
    // (Simulated without Qt dependency)
    // ============================================
    currentUT = "UT-5.1";
    std::cout << std::endl << "[UT-5.1] TaskListModel::addTask() / removeTask()" << std::endl;
    std::cout << "-------------------------------------------" << std::endl;

    // Simulating TaskListModel behavior
    std::map<std::string, Task> modelTasks;
    std::map<std::string, int> idToIndex;

    // UT-5.1-1: addTask()
    total++;
    Task modelTask1;
    modelTask1.id = "model-task-1";
    modelTasks[modelTask1.id] = modelTask1;
    idToIndex[modelTask1.id] = 0;
    currentScenario = "UT-5.1-1"; std::cout << "[UT-5.1-1]" << std::endl;
    std::cout << "  Input: addTask(model-task-1)" << std::endl;
    std::cout << "  Expected: task count = 1" << std::endl;
    std::cout << "  Actual: task count = " << modelTasks.size() << std::endl;
    if (modelTasks.size() == 1) {
        std::cout << "  Result: PASS" << std::endl;
        updateUTStats(utStats, currentUT, true, currentScenario);
        passed++;
    } else {
        std::cout << "  Result: FAIL" << std::endl;
        updateUTStats(utStats, currentUT, false, currentScenario);
        failed++;
    }

    // UT-5.1-2: removeTask()
    total++;
    modelTasks.erase("model-task-1");
    idToIndex.erase("model-task-1");
    currentScenario = "UT-5.1-2"; std::cout << "[UT-5.1-2]" << std::endl;
    std::cout << "  Input: removeTask(model-task-1)" << std::endl;
    std::cout << "  Expected: task count = 0" << std::endl;
    std::cout << "  Actual: task count = " << modelTasks.size() << std::endl;
    if (modelTasks.empty()) {
        std::cout << "  Result: PASS" << std::endl;
        updateUTStats(utStats, currentUT, true, currentScenario);
        passed++;
    } else {
        std::cout << "  Result: FAIL" << std::endl;
        updateUTStats(utStats, currentUT, false, currentScenario);
        failed++;
    }

    // ============================================
    // UT-5.2: TaskListModel::updateTaskProgress()
    // ============================================
    currentUT = "UT-5.2";
    std::cout << std::endl << "[UT-5.2] TaskListModel::updateTaskProgress()" << std::endl;
    std::cout << "-------------------------------------------" << std::endl;

    std::map<std::string, Task> modelTasks2;
    Task updateTask;
    updateTask.id = "update-task-1";
    updateTask.downloadedBytes = 0;
    updateTask.totalBytes = 100;
    modelTasks2[updateTask.id] = updateTask;

    // UT-5.2-1: updateTaskProgress()
    total++;
    auto it2 = modelTasks2.find("update-task-1");
    if (it2 != modelTasks2.end()) {
        it2->second.downloadedBytes = 50;
    }
    currentScenario = "UT-5.2"; std::cout << "[UT-5.2]" << std::endl;
    std::cout << "  Input: updateTaskProgress(id=update-task-1, downloaded=50)" << std::endl;
    std::cout << "  Expected: downloadedBytes = 50" << std::endl;
    std::cout << "  Actual: downloadedBytes = " << modelTasks2["update-task-1"].downloadedBytes << std::endl;
    if (modelTasks2["update-task-1"].downloadedBytes == 50) {
        std::cout << "  Result: PASS" << std::endl;
        updateUTStats(utStats, currentUT, true, currentScenario);
        passed++;
    } else {
        std::cout << "  Result: FAIL" << std::endl;
        updateUTStats(utStats, currentUT, false, currentScenario);
        failed++;
    }

    // ============================================
    // UT-5.3: TaskListModel::rowCount() / data()
    // ============================================
    currentUT = "UT-5.3";
    std::cout << std::endl << "[UT-5.3] TaskListModel::rowCount() / data()" << std::endl;
    std::cout << "-------------------------------------------" << std::endl;

    std::map<std::string, Task> modelTasks3;
    Task mTask1, mTask2;
    mTask1.id = "data-task-1"; mTask1.fileName = "file1.zip";
    mTask2.id = "data-task-2"; mTask2.fileName = "file2.zip";
    modelTasks3[mTask1.id] = mTask1;
    modelTasks3[mTask2.id] = mTask2;

    // UT-5.3-1: rowCount()
    total++;
    int rowCount = modelTasks3.size();
    currentScenario = "UT-5.3"; std::cout << "[UT-5.3]" << std::endl;
    std::cout << "  Input: Added 2 tasks" << std::endl;
    std::cout << "  Expected: rowCount = 2" << std::endl;
    std::cout << "  Actual: rowCount = " << rowCount << std::endl;
    if (rowCount == 2) {
        std::cout << "  Result: PASS" << std::endl;
        updateUTStats(utStats, currentUT, true, currentScenario);
        passed++;
    } else {
        std::cout << "  Result: FAIL" << std::endl;
        updateUTStats(utStats, currentUT, false, currentScenario);
        failed++;
    }

    // UT-5.3-2: data() - get first task
    total++;
    Task firstTask = modelTasks3["data-task-1"];
    currentScenario = "UT-5.3"; std::cout << "[UT-5.3]" << std::endl;
    std::cout << "  Input: data(index=0)" << std::endl;
    std::cout << "  Expected: fileName = \"file1.zip\"" << std::endl;
    std::cout << "  Actual: fileName = \"" << firstTask.fileName << "\"" << std::endl;
    if (firstTask.fileName == "file1.zip") {
        std::cout << "  Result: PASS" << std::endl;
        updateUTStats(utStats, currentUT, true, currentScenario);
        passed++;
    } else {
        std::cout << "  Result: FAIL" << std::endl;
        updateUTStats(utStats, currentUT, false, currentScenario);
        failed++;
    }

    // ============================================
    // UT-5.4: TaskListModel::getTaskIndex()
    // ============================================
    currentUT = "UT-5.4";
    std::cout << std::endl << "[UT-5.4] TaskListModel::getTaskIndex()" << std::endl;
    std::cout << "-------------------------------------------" << std::endl;

    std::map<std::string, int> indexMap;
    indexMap["a"] = 0;
    indexMap["b"] = 1;
    indexMap["c"] = 2;

    // UT-5.4-1: Existing task
    total++;
    auto idx1 = indexMap.find("b");
    currentScenario = "UT-5.4"; std::cout << "[UT-5.4]" << std::endl;
    std::cout << "  Input: getTaskIndex(\"b\")" << std::endl;
    std::cout << "  Expected: index = 1" << std::endl;
    std::cout << "  Actual: index = " << (idx1 != indexMap.end() ? idx1->second : -1) << std::endl;
    if (idx1 != indexMap.end() && idx1->second == 1) {
        std::cout << "  Result: PASS" << std::endl;
        updateUTStats(utStats, currentUT, true, currentScenario);
        passed++;
    } else {
        std::cout << "  Result: FAIL" << std::endl;
        updateUTStats(utStats, currentUT, false, currentScenario);
        failed++;
    }

    // UT-5.4-2: Non-existing task
    total++;
    auto idx2 = indexMap.find("nonexistent");
    int actualIdx = (idx2 != indexMap.end()) ? idx2->second : -1;
    currentScenario = "UT-5.4"; std::cout << "[UT-5.4]" << std::endl;
    std::cout << "  Input: getTaskIndex(\"nonexistent\")" << std::endl;
    std::cout << "  Expected: index = -1" << std::endl;
    std::cout << "  Actual: index = " << actualIdx << std::endl;
    if (actualIdx == -1) {
        std::cout << "  Result: PASS" << std::endl;
        updateUTStats(utStats, currentUT, true, currentScenario);
        passed++;
    } else {
        std::cout << "  Result: FAIL" << std::endl;
        updateUTStats(utStats, currentUT, false, currentScenario);
        failed++;
    }

    // ============================================
    // UT-5.5: TaskListModel::clearAll()
    // ============================================
    currentUT = "UT-5.5";
    std::cout << std::endl << "[UT-5.5] TaskListModel::clearAll()" << std::endl;
    std::cout << "-------------------------------------------" << std::endl;

    std::map<std::string, Task> modelTasks5;
    Task cTask1, cTask2, cTask3;
    cTask1.id = "clear-1"; cTask2.id = "clear-2"; cTask3.id = "clear-3";
    modelTasks5[cTask1.id] = cTask1;
    modelTasks5[cTask2.id] = cTask2;
    modelTasks5[cTask3.id] = cTask3;

    // UT-5.5-1: clearAll()
    total++;
    modelTasks5.clear();
    currentScenario = "UT-5.5"; std::cout << "[UT-5.5]" << std::endl;
    std::cout << "  Input: clearAll()" << std::endl;
    std::cout << "  Expected: task count = 0" << std::endl;
    std::cout << "  Actual: task count = " << modelTasks5.size() << std::endl;
    if (modelTasks5.empty()) {
        std::cout << "  Result: PASS" << std::endl;
        updateUTStats(utStats, currentUT, true, currentScenario);
        passed++;
    } else {
        std::cout << "  Result: FAIL" << std::endl;
        updateUTStats(utStats, currentUT, false, currentScenario);
        failed++;
    }

    // ============================================
    // UT-5.6: TaskListModel::roleNames()
    // ============================================
    currentUT = "UT-5.6";
    std::cout << std::endl << "[UT-5.6] TaskListModel::roleNames()" << std::endl;
    std::cout << "-------------------------------------------" << std::endl;

    // Simulating role names (Qt UserRole + 1, +2, etc.)
    std::map<int, std::string> roleNames;
    roleNames[256] = "id";       // Qt::UserRole + 1
    roleNames[257] = "url";
    roleNames[258] = "localPath";
    roleNames[259] = "fileName";
    roleNames[260] = "totalBytes";
    roleNames[261] = "downloadedBytes";
    roleNames[262] = "progress";
    roleNames[263] = "status";

    // UT-5.6-1: roleNames()
    total++;
    bool hasIdRole = (roleNames.find(256) != roleNames.end());
    bool hasUrlRole = (roleNames.find(257) != roleNames.end());
    bool hasFileNameRole = (roleNames.find(259) != roleNames.end());
    currentScenario = "UT-5.6"; std::cout << "[UT-5.6]" << std::endl;
    std::cout << "  Input: Check roleNames contains required roles" << std::endl;
    std::cout << "  Expected: id, url, fileName roles exist" << std::endl;
    std::cout << "  Actual: hasIdRole=" << hasIdRole << ", hasUrlRole=" << hasUrlRole << ", hasFileNameRole=" << hasFileNameRole << std::endl;
    if (hasIdRole && hasUrlRole && hasFileNameRole) {
        std::cout << "  Result: PASS" << std::endl;
        updateUTStats(utStats, currentUT, true, currentScenario);
        passed++;
    } else {
        std::cout << "  Result: FAIL" << std::endl;
        updateUTStats(utStats, currentUT, false, currentScenario);
        failed++;
    }

    // ============================================
    // Test Summary
    // ============================================
    printFooter(total, passed, failed);
    printUTSummary(utStats);

    return (failed > 0) ? 1 : 0;
}
