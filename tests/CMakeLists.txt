cmake_minimum_required(VERSION 3.20)

# 测试项目
project(XDownTests LANGUAGES CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找 Qt
find_package(Qt6 REQUIRED COMPONENTS Core Test Network Sql)

# 启用 Qt 自动处理
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 获取项目根目录
get_filename_component(XDOWN_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)

# ========== 单元测试: DownloadTask 测试 ==========
set(UT_SOURCES
    src/tst_downloadtask.cpp
)

qt_add_executable(test_downloadtask ${UT_SOURCES})
set_target_properties(test_downloadtask PROPERTIES WIN32_EXECUTABLE FALSE)

target_include_directories(test_downloadtask PRIVATE
    ${XDOWN_ROOT_DIR}/src
)

# 添加 DBManager 源文件进行编译
target_sources(test_downloadtask PRIVATE
    ${XDOWN_ROOT_DIR}/src/core/DBManager.cpp
)

target_link_libraries(test_downloadtask PRIVATE
    Qt6::Core
    Qt6::Test
    Qt6::Sql
)

add_test(NAME DownloadTaskTest COMMAND test_downloadtask)

# ========== 系统测试: HttpDownloader 测试 ==========
set(ST_SOURCES
    src/tst_systemtest.cpp
)

qt_add_executable(test_systemtest ${ST_SOURCES})
set_target_properties(test_systemtest PROPERTIES WIN32_EXECUTABLE FALSE)

target_include_directories(test_systemtest PRIVATE
    ${XDOWN_ROOT_DIR}/src
)

# 添加源文件直接编译
target_sources(test_systemtest PRIVATE
    ${XDOWN_ROOT_DIR}/src/core/DBManager.cpp
    ${XDOWN_ROOT_DIR}/src/core/HttpDownloader.cpp
    ${XDOWN_ROOT_DIR}/src/core/DownloadEngine.cpp
    ${XDOWN_ROOT_DIR}/src/gui/TaskListModel.cpp
)

target_link_libraries(test_systemtest PRIVATE
    Qt6::Core
    Qt6::Test
    Qt6::Network
    Qt6::Sql
)

add_test(NAME SystemTest COMMAND test_systemtest)

# 简化的 HTTP 测试
qt_add_executable(tst_httptest src/tst_httptest.cpp)
set_target_properties(tst_httptest PROPERTIES WIN32_EXECUTABLE FALSE)

target_include_directories(tst_httptest PRIVATE
    ${XDOWN_ROOT_DIR}/src
)

target_sources(tst_httptest PRIVATE
    ${XDOWN_ROOT_DIR}/src/core/DBManager.cpp
    ${XDOWN_ROOT_DIR}/src/core/HttpDownloader.cpp
    ${XDOWN_ROOT_DIR}/src/core/DownloadEngine.cpp
    ${XDOWN_ROOT_DIR}/src/gui/TaskListModel.cpp
)

target_link_libraries(tst_httptest PRIVATE
    Qt6::Core
    Qt6::Test
    Qt6::Network
    Qt6::Sql
)
