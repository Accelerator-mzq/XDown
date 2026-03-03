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
    tst_downloadtask.cpp
)

qt_add_executable(test_downloadtask ${UT_SOURCES})

target_include_directories(test_downloadtask PRIVATE
    ${XDOWN_ROOT_DIR}/src
)

target_link_libraries(test_downloadtask PRIVATE
    Qt6::Core
    Qt6::Test
)

add_test(NAME DownloadTaskTest COMMAND test_downloadtask)

# ========== 系统测试: HttpDownloader 测试 ==========
set(ST_SOURCES
    tst_systemtest.cpp
)

qt_add_executable(test_systemtest ${ST_SOURCES})

target_include_directories(test_systemtest PRIVATE
    ${XDOWN_ROOT_DIR}/src
)

# 检查 XDownCore 库是否存在
set(XDOWNCORE_LIB "")
if(EXISTS ${XDOWN_ROOT_DIR}/build/libXDownCore.a)
    set(XDOWNCORE_LIB ${XDOWN_ROOT_DIR}/build/libXDownCore.a)
elseif(EXISTS ${XDOWN_ROOT_DIR}/build/Desktop_Qt_6_5_3_MinGW_64_bit-Debug/libXDownCore.a)
    set(XDOWNCORE_LIB ${XDOWN_ROOT_DIR}/build/Desktop_Qt_6_5_3_MinGW_64_bit-Debug/libXDownCore.a)
endif()

# 添加源文件直接编译（而不是使用静态库）
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
