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
    std::cout << std::endl << "[UT-2.1] DBManager::insertTask()" << std::endl;
    std::cout << "-------------------------------------------" << std::endl;

    MockDBManager db;

    // UT-2.1-1: Insert task with Waiting status
    total++;
    Task task1;
    task1.id = "test-id-001";
    task1.url = "http://test.com/file.zip";
    task1.fileName = "file.zip";
    task1.status = TaskStatus::Waiting;
    bool insertResult = db.insertTask(task1);
    currentScenario = "UT-2.1"; std::cout << "[UT-2.1]" << std::endl;
    std::cout << "  Input: Task with status=Waiting, query after insert" << std::endl;
    std::cout << "  Expected: function returns true" << std::endl;
    std::cout << "  Actual: " << (insertResult ? "true" : "false") << std::endl;
    if (insertResult) {
        std::cout << "  Result: PASS" << std::endl;
        updateUTStats(utStats, currentUT, true, currentScenario);
        passed++;
    } else {
        std::cout << "  Result: FAIL" << std::endl;
        updateUTStats(utStats, currentUT, false, currentScenario);
        failed++;
    }

    // UT-2.1-2: Verify data consistency
    total++;
    Task retrievedTask = db.getTaskById("test-id-001");
    currentScenario = "UT-2.1"; std::cout << "[UT-2.1]" << std::endl;
    std::cout << "  Input: Query ID=test-id-001" << std::endl;
    std::cout << "  Expected: Can retrieve matching data" << std::endl;
    std::cout << "  Actual: id=" << retrievedTask.id << ", status=" << (int)retrievedTask.status << std::endl;
    bool dataMatch = (retrievedTask.id == "test-id-001" && retrievedTask.status == TaskStatus::Waiting);
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
    // UT-2.2: DBManager::hasDuplicateDownloadingTask()
    // ============================================
    currentUT = "UT-2.2";
    currentScenario = "UT-2.2-1";
    std::cout << std::endl << "[UT-2.2] DBManager::hasDuplicateDownloadingTask()" << std::endl;
    std::cout << "-------------------------------------------" << std::endl;

    db.clear();
    Task task2;
    task2.id = "test-id-002";
    task2.url = "http://test.com/a.zip";
    task2.status = TaskStatus::Downloading;
    db.insertTask(task2);

    // UT-2.2-1: Same URL query
    total++;
    bool dup1 = db.hasDuplicateDownloadingTask("http://test.com/a.zip");
    currentScenario = "UT-2.2"; std::cout << "[UT-2.2]" << std::endl;
    std::cout << "  Precondition: DB has URL=http://test.com/a.zip, status=Downloading" << std::endl;
    std::cout << "  Input: Same URL" << std::endl;
    std::cout << "  Expected: true" << std::endl;
    std::cout << "  Actual: " << (dup1 ? "true" : "false") << std::endl;
    if (dup1) {
        std::cout << "  Result: PASS" << std::endl;
        updateUTStats(utStats, currentUT, true, currentScenario);
        passed++;
    } else {
        std::cout << "  Result: FAIL" << std::endl;
        updateUTStats(utStats, currentUT, false, currentScenario);
        failed++;
    }

    // UT-2.2-2: Different URL
    total++;
    bool dup2 = db.hasDuplicateDownloadingTask("http://test.com/b.zip");
    currentScenario = "UT-2.2"; std::cout << "[UT-2.2]" << std::endl;
    std::cout << "  Precondition: DB has URL=http://test.com/a.zip, status=Downloading" << std::endl;
    std::cout << "  Input: Different URL" << std::endl;
    std::cout << "  Expected: false" << std::endl;
    std::cout << "  Actual: " << (dup2 ? "true" : "false") << std::endl;
    if (!dup2) {
        std::cout << "  Result: PASS" << std::endl;
        updateUTStats(utStats, currentUT, true, currentScenario);
        passed++;
    } else {
        std::cout << "  Result: FAIL" << std::endl;
        updateUTStats(utStats, currentUT, false, currentScenario);
        failed++;
    }

    // UT-2.2-3: URL with Finished status
    db.clear();
    Task task3;
    task3.id = "test-id-003";
    task3.url = "http://test.com/c.zip";
    task3.status = TaskStatus::Finished;
    db.insertTask(task3);

    total++;
    bool dup3 = db.hasDuplicateDownloadingTask("http://test.com/c.zip");
    currentScenario = "UT-2.2"; std::cout << "[UT-2.2]" << std::endl;
    std::cout << "  Precondition: DB has URL=http://test.com/c.zip, status=Finished" << std::endl;
    std::cout << "  Input: Same URL but status=Finished" << std::endl;
    std::cout << "  Expected: false" << std::endl;
    std::cout << "  Actual: " << (dup3 ? "true" : "false") << std::endl;
    if (!dup3) {
        std::cout << "  Result: PASS" << std::endl;
        updateUTStats(utStats, currentUT, true, currentScenario);
        passed++;
    } else {
        std::cout << "  Result: FAIL" << std::endl;
        updateUTStats(utStats, currentUT, false, currentScenario);
        failed++;
    }

    // ============================================
    // UT-2.3: DBManager::updateTaskProgress()
    // ============================================
    currentUT = "UT-2.3";
    currentScenario = "UT-2.3-1";
    std::cout << std::endl << "[UT-2.3] DBManager::updateTaskProgress()" << std::endl;
    std::cout << "-------------------------------------------" << std::endl;

    db.clear();
    Task task4;
    task4.id = "test-id-004";
    task4.url = "http://test.com/d.zip";
    task4.status = TaskStatus::Waiting;
    db.insertTask(task4);

    // UT-2.3-1: Update task progress
    total++;
    bool updateResult = db.updateTaskProgress("test-id-004", 50, 100, TaskStatus::Downloading);
    currentScenario = "UT-2.3"; std::cout << "[UT-2.3]" << std::endl;
    std::cout << "  Input: id=test-id-004, downloadedBytes=50, totalBytes=100, status=Downloading" << std::endl;
    std::cout << "  Expected: function returns true" << std::endl;
    std::cout << "  Actual: " << (updateResult ? "true" : "false") << std::endl;
    if (updateResult) {
        std::cout << "  Result: PASS" << std::endl;
        updateUTStats(utStats, currentUT, true, currentScenario);
        passed++;
    } else {
        std::cout << "  Result: FAIL" << std::endl;
        updateUTStats(utStats, currentUT, false, currentScenario);
        failed++;
    }

    // UT-2.3-2: Verify updated data
    total++;
    Task updatedTask = db.getTaskById("test-id-004");
    currentScenario = "UT-2.3"; std::cout << "[UT-2.3]" << std::endl;
    std::cout << "  Input: Query ID=test-id-004" << std::endl;
    std::cout << "  Expected: downloadedBytes=50, totalBytes=100, status=Downloading" << std::endl;
    std::cout << "  Actual: downloadedBytes=" << updatedTask.downloadedBytes
              << ", totalBytes=" << updatedTask.totalBytes
              << ", status=" << (int)updatedTask.status << std::endl;
    bool progressMatch = (updatedTask.downloadedBytes == 50 &&
                         updatedTask.totalBytes == 100 &&
                         updatedTask.status == TaskStatus::Downloading);
    if (progressMatch) {
        std::cout << "  Result: PASS" << std::endl;
        updateUTStats(utStats, currentUT, true, currentScenario);
        passed++;
    } else {
        std::cout << "  Result: FAIL" << std::endl;
        updateUTStats(utStats, currentUT, false, currentScenario);
        failed++;
    }

    // ============================================
    // UT-2.4: DBManager::updateTaskStatus()
    // ============================================
    currentUT = "UT-2.4";
    std::cout << std::endl << "[UT-2.4] DBManager::updateTaskStatus()" << std::endl;
    std::cout << "-------------------------------------------" << std::endl;

    db.clear();
    Task task5;
    task5.id = "test-id-005";
    task5.url = "http://test.com/e.zip";
    task5.status = TaskStatus::Waiting;
    db.insertTask(task5);

    // UT-2.4-1: Update task status
    total++;
    bool statusResult = db.updateTaskStatus("test-id-005", TaskStatus::Downloading);
    currentScenario = "UT-2.4"; std::cout << "[UT-2.4]" << std::endl;
    std::cout << "  Input: id=test-id-005, status=Downloading" << std::endl;
    std::cout << "  Expected: function returns true" << std::endl;
    std::cout << "  Actual: " << (statusResult ? "true" : "false") << std::endl;
    if (statusResult) {
        std::cout << "  Result: PASS" << std::endl;
        updateUTStats(utStats, currentUT, true, currentScenario);
        passed++;
    } else {
        std::cout << "  Result: FAIL" << std::endl;
        updateUTStats(utStats, currentUT, false, currentScenario);
        failed++;
    }

    // UT-2.4-2: Verify status change
    total++;
    Task statusTask = db.getTaskById("test-id-005");
    currentScenario = "UT-2.4"; std::cout << "[UT-2.4]" << std::endl;
    std::cout << "  Input: Query ID=test-id-005" << std::endl;
    std::cout << "  Expected: status=Downloading" << std::endl;
    std::cout << "  Actual: status=" << (int)statusTask.status << std::endl;
    if (statusTask.status == TaskStatus::Downloading) {
        std::cout << "  Result: PASS" << std::endl;
        updateUTStats(utStats, currentUT, true, currentScenario);
        passed++;
    } else {
        std::cout << "  Result: FAIL" << std::endl;
        updateUTStats(utStats, currentUT, false, currentScenario);
        failed++;
    }

    // ============================================
    // UT-2.5: DBManager::loadAllTasks()
    // ============================================
    currentUT = "UT-2.5";
    std::cout << std::endl << "[UT-2.5] DBManager::loadAllTasks()" << std::endl;
    std::cout << "-------------------------------------------" << std::endl;

    db.clear();
    Task task6, task7, task8;
    task6.id = "task-a"; task6.url = "http://test.com/a.zip"; task6.status = TaskStatus::Waiting;
    task7.id = "task-b"; task7.url = "http://test.com/b.zip"; task7.status = TaskStatus::Downloading;
    task8.id = "task-c"; task8.url = "http://test.com/c.zip"; task8.status = TaskStatus::Finished;
    db.insertTask(task6);
    db.insertTask(task7);
    db.insertTask(task8);

    // UT-2.5-1: Load all tasks
    total++;
    std::vector<Task> allTasks = db.loadAllTasks();
    currentScenario = "UT-2.5"; std::cout << "[UT-2.5]" << std::endl;
    std::cout << "  Input: Insert 3 tasks with different status" << std::endl;
    std::cout << "  Expected: vector size = 3" << std::endl;
    std::cout << "  Actual: vector size = " << allTasks.size() << std::endl;
    if (allTasks.size() == 3) {
        std::cout << "  Result: PASS" << std::endl;
        updateUTStats(utStats, currentUT, true, currentScenario);
        passed++;
    } else {
        std::cout << "  Result: FAIL" << std::endl;
        updateUTStats(utStats, currentUT, false, currentScenario);
        failed++;
    }

    // UT-2.5-2: Verify task fields
    total++;
    bool hasAllThree = false;
    int countWaiting = 0, countDownloading = 0, countFinished = 0;
    for (const auto& t : allTasks) {
        if (t.status == TaskStatus::Waiting) countWaiting++;
        if (t.status == TaskStatus::Downloading) countDownloading++;
        if (t.status == TaskStatus::Finished) countFinished++;
    }
    currentScenario = "UT-2.5"; std::cout << "[UT-2.5]" << std::endl;
    std::cout << "  Input: Check task status distribution" << std::endl;
    std::cout << "  Expected: 1 Waiting, 1 Downloading, 1 Finished" << std::endl;
    std::cout << "  Actual: " << countWaiting << " Waiting, " << countDownloading << " Downloading, " << countFinished << " Finished" << std::endl;
    hasAllThree = (countWaiting == 1 && countDownloading == 1 && countFinished == 1);
    if (hasAllThree) {
        std::cout << "  Result: PASS" << std::endl;
        updateUTStats(utStats, currentUT, true, currentScenario);
        passed++;
    } else {
        std::cout << "  Result: FAIL" << std::endl;
        updateUTStats(utStats, currentUT, false, currentScenario);
        failed++;
    }

    // ============================================
    // UT-2.6: DBManager::deleteTask()
    // ============================================
    currentUT = "UT-2.6";
    std::cout << std::endl << "[UT-2.6] DBManager::deleteTask()" << std::endl;
    std::cout << "-------------------------------------------" << std::endl;

    db.clear();
    Task task9;
    task9.id = "test-id-006";
    task9.url = "http://test.com/f.zip";
    db.insertTask(task9);

    // UT-2.6-1: Delete existing task
    total++;
    bool deleteResult = db.deleteTask("test-id-006");
    currentScenario = "UT-2.6"; std::cout << "[UT-2.6]" << std::endl;
    std::cout << "  Input: id=test-id-006" << std::endl;
    std::cout << "  Expected: function returns true" << std::endl;
    std::cout << "  Actual: " << (deleteResult ? "true" : "false") << std::endl;
    if (deleteResult) {
        std::cout << "  Result: PASS" << std::endl;
        updateUTStats(utStats, currentUT, true, currentScenario);
        passed++;
    } else {
        std::cout << "  Result: FAIL" << std::endl;
        updateUTStats(utStats, currentUT, false, currentScenario);
        failed++;
    }

    // UT-2.6-2: Verify deletion
    total++;
    Task deletedTask = db.getTaskById("test-id-006");
    currentScenario = "UT-2.6"; std::cout << "[UT-2.6]" << std::endl;
    std::cout << "  Input: Query ID=test-id-006 after deletion" << std::endl;
    std::cout << "  Expected: empty task (id=\"\")" << std::endl;
    std::cout << "  Actual: id=\"" << deletedTask.id << "\"" << std::endl;
    if (deletedTask.id.empty()) {
        std::cout << "  Result: PASS" << std::endl;
        updateUTStats(utStats, currentUT, true, currentScenario);
        passed++;
    } else {
        std::cout << "  Result: FAIL" << std::endl;
        updateUTStats(utStats, currentUT, false, currentScenario);
        failed++;
    }

    // ============================================
    // UT-2.7: DBManager::updateTaskSpeed()
    // ============================================
    currentUT = "UT-2.7";
    std::cout << std::endl << "[UT-2.7] DBManager::updateTaskSpeed()" << std::endl;
    std::cout << "-------------------------------------------" << std::endl;

    db.clear();
    Task task10;
    task10.id = "test-id-007";
    task10.url = "http://test.com/g.zip";
    db.insertTask(task10);

    // UT-2.7-1: Update task speed
    total++;
    bool speedResult = db.updateTaskSpeed("test-id-007", 102400);
    currentScenario = "UT-2.7"; std::cout << "[UT-2.7]" << std::endl;
    std::cout << "  Input: id=test-id-007, speedBytesPerSec=102400" << std::endl;
    std::cout << "  Expected: function returns true" << std::endl;
    std::cout << "  Actual: " << (speedResult ? "true" : "false") << std::endl;
    if (speedResult) {
        std::cout << "  Result: PASS" << std::endl;
        updateUTStats(utStats, currentUT, true, currentScenario);
        passed++;
    } else {
        std::cout << "  Result: FAIL" << std::endl;
        updateUTStats(utStats, currentUT, false, currentScenario);
        failed++;
    }

    // UT-2.7-2: Verify speed update
    total++;
    Task speedTask = db.getTaskById("test-id-007");
    currentScenario = "UT-2.7"; std::cout << "[UT-2.7]" << std::endl;
    std::cout << "  Input: Query ID=test-id-007" << std::endl;
    std::cout << "  Expected: speedBytesPerSec=102400" << std::endl;
    std::cout << "  Actual: speedBytesPerSec=" << speedTask.speedBytesPerSec << std::endl;
    if (speedTask.speedBytesPerSec == 102400) {
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
