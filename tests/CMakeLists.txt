
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

# 强制为控制台程序，避免 libstdc++ 版本冲突导致的 SegFault
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

# 强制为控制台程序，避免 libstdc++ 版本冲突导致的 SegFault
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

# 强制为控制台程序，避免 libstdc++ 版本冲突导致的 SegFault
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

# ============================================
# 修复 libstdc++ 版本冲突：复制 MSYS2 的 DLL 到测试目录
# 解决 SegFault 问题 (Qt 6.5.3 使用 GCC 11.2，编译器使用 GCC 14.2)
# ============================================
set(MINGW_BIN "E:/DevelopmentTool/mingw64/bin")
set(TEST_DIR "${CMAKE_CURRENT_BINARY_DIR}")

if(WIN32)
    # 复制 libstdc++-6.dll (GCC 14.2 版本)
    add_custom_command(TARGET test_downloadtask POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${MINGW_BIN}/libstdc++-6.dll"
            "${TEST_DIR}/"
        COMMENT "Copying GCC 14.2 libstdc++ to test directory..."
    )
    add_custom_command(TARGET test_systemtest POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${MINGW_BIN}/libstdc++-6.dll"
            "${TEST_DIR}/"
        COMMENT "Copying GCC 14.2 libstdc++ to test directory..."
    )
    add_custom_command(TARGET tst_httptest POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${MINGW_BIN}/libstdc++-6.dll"
            "${TEST_DIR}/"
        COMMENT "Copying GCC 14.2 libstdc++ to test directory..."
    )
endif()
