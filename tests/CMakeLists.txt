
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
# v2.0 新增单元测试
# ============================================

# ========== ChunkInfo 测试 ==========
# 注意：现在使用 GCC 11.2（与 Qt 6.5.3 配套），可以直接使用 QTest 框架
qt_add_executable(tst_chunkinfo src/tst_chunkinfo.cpp)
set_target_properties(tst_chunkinfo PROPERTIES WIN32_EXECUTABLE FALSE)
target_include_directories(tst_chunkinfo PRIVATE ${XDOWN_ROOT_DIR}/src)
target_link_libraries(tst_chunkinfo PRIVATE Qt6::Core Qt6::Test)
add_test(NAME ChunkInfoTest COMMAND tst_chunkinfo)

# ========== TaskState 测试 ==========
qt_add_executable(tst_taskstate src/tst_taskstate.cpp)
set_target_properties(tst_taskstate PROPERTIES WIN32_EXECUTABLE FALSE)
target_include_directories(tst_taskstate PRIVATE ${XDOWN_ROOT_DIR}/src)
target_link_libraries(tst_taskstate PRIVATE Qt6::Core Qt6::Test)
add_test(NAME TaskStateTest COMMAND tst_taskstate)

# ========== AdaptiveThreadController 测试 ==========
qt_add_executable(tst_adaptivethread src/tst_adaptivethread.cpp)
set_target_properties(tst_adaptivethread PROPERTIES WIN32_EXECUTABLE FALSE)
target_include_directories(tst_adaptivethread PRIVATE ${XDOWN_ROOT_DIR}/src)
target_sources(tst_adaptivethread PRIVATE
    ${XDOWN_ROOT_DIR}/src/core/download/AdaptiveThreadController.cpp
)
target_link_libraries(tst_adaptivethread PRIVATE Qt6::Core Qt6::Test)
add_test(NAME AdaptiveThreadControllerTest COMMAND tst_adaptivethread)

# ========== BTDownloadEngine 测试 ==========
qt_add_executable(tst_btdownloadengine src/tst_btdownloadengine.cpp)
set_target_properties(tst_btdownloadengine PROPERTIES WIN32_EXECUTABLE FALSE)
target_include_directories(tst_btdownloadengine PRIVATE ${XDOWN_ROOT_DIR}/src)
target_sources(tst_btdownloadengine PRIVATE
    ${XDOWN_ROOT_DIR}/src/core/download/BTDownloadEngine.cpp
    ${XDOWN_ROOT_DIR}/src/core/download/IDownloadEngine.cpp
)
target_link_libraries(tst_btdownloadengine PRIVATE Qt6::Core Qt6::Test Qt6::Network)
add_test(NAME BTDownloadEngineTest COMMAND tst_btdownloadengine)

# ============================================
# 复制 Qt DLL 和 GCC 11.2 运行时库到测试目录
# 编译器已统一为 GCC 11.2，但仍需要运行时库
# ============================================
set(QT_DIR "E:/Qt/6.5.3/mingw_64")
set(MINGW1120_DIR "E:/Qt/Tools/mingw1120_64/bin")
set(TEST_DIR "${CMAKE_CURRENT_BINARY_DIR}")

if(WIN32)
    # 复制 GCC 11.2 运行时库
    add_custom_command(TARGET test_downloadtask POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${MINGW1120_DIR}/libstdc++-6.dll" "${TEST_DIR}/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${MINGW1120_DIR}/libgcc_s_seh-1.dll" "${TEST_DIR}/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${MINGW1120_DIR}/libwinpthread-1.dll" "${TEST_DIR}/"
    )
    add_custom_command(TARGET test_systemtest POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${MINGW1120_DIR}/libstdc++-6.dll" "${TEST_DIR}/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${MINGW1120_DIR}/libgcc_s_seh-1.dll" "${TEST_DIR}/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${MINGW1120_DIR}/libwinpthread-1.dll" "${TEST_DIR}/"
    )
    add_custom_command(TARGET tst_httptest POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${MINGW1120_DIR}/libstdc++-6.dll" "${TEST_DIR}/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${MINGW1120_DIR}/libgcc_s_seh-1.dll" "${TEST_DIR}/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${MINGW1120_DIR}/libwinpthread-1.dll" "${TEST_DIR}/"
    )
    add_custom_command(TARGET tst_chunkinfo POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${MINGW1120_DIR}/libstdc++-6.dll" "${TEST_DIR}/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${MINGW1120_DIR}/libgcc_s_seh-1.dll" "${TEST_DIR}/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${MINGW1120_DIR}/libwinpthread-1.dll" "${TEST_DIR}/"
    )
    add_custom_command(TARGET tst_taskstate POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${MINGW1120_DIR}/libstdc++-6.dll" "${TEST_DIR}/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${MINGW1120_DIR}/libgcc_s_seh-1.dll" "${TEST_DIR}/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${MINGW1120_DIR}/libwinpthread-1.dll" "${TEST_DIR}/"
    )
    add_custom_command(TARGET tst_adaptivethread POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${MINGW1120_DIR}/libstdc++-6.dll" "${TEST_DIR}/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${MINGW1120_DIR}/libgcc_s_seh-1.dll" "${TEST_DIR}/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${MINGW1120_DIR}/libwinpthread-1.dll" "${TEST_DIR}/"
    )
    add_custom_command(TARGET tst_btdownloadengine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${MINGW1120_DIR}/libstdc++-6.dll" "${TEST_DIR}/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${MINGW1120_DIR}/libgcc_s_seh-1.dll" "${TEST_DIR}/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${MINGW1120_DIR}/libwinpthread-1.dll" "${TEST_DIR}/"
    )

    # 复制 Qt DLL
    add_custom_command(TARGET test_downloadtask POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${QT_DIR}/bin/Qt6Core.dll" "${TEST_DIR}/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${QT_DIR}/bin/Qt6Test.dll" "${TEST_DIR}/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${QT_DIR}/bin/Qt6Sql.dll" "${TEST_DIR}/"
    )
    add_custom_command(TARGET test_systemtest POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${QT_DIR}/bin/Qt6Core.dll" "${TEST_DIR}/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${QT_DIR}/bin/Qt6Test.dll" "${TEST_DIR}/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${QT_DIR}/bin/Qt6Sql.dll" "${TEST_DIR}/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${QT_DIR}/bin/Qt6Network.dll" "${TEST_DIR}/"
    )
    add_custom_command(TARGET tst_httptest POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${QT_DIR}/bin/Qt6Core.dll" "${TEST_DIR}/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${QT_DIR}/bin/Qt6Test.dll" "${TEST_DIR}/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${QT_DIR}/bin/Qt6Sql.dll" "${TEST_DIR}/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${QT_DIR}/bin/Qt6Network.dll" "${TEST_DIR}/"
    )
    add_custom_command(TARGET tst_chunkinfo POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${QT_DIR}/bin/Qt6Core.dll" "${TEST_DIR}/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${QT_DIR}/bin/Qt6Test.dll" "${TEST_DIR}/"
    )
    add_custom_command(TARGET tst_taskstate POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${QT_DIR}/bin/Qt6Core.dll" "${TEST_DIR}/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${QT_DIR}/bin/Qt6Test.dll" "${TEST_DIR}/"
    )
    add_custom_command(TARGET tst_adaptivethread POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${QT_DIR}/bin/Qt6Core.dll" "${TEST_DIR}/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${QT_DIR}/bin/Qt6Test.dll" "${TEST_DIR}/"
    )
    add_custom_command(TARGET tst_btdownloadengine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${QT_DIR}/bin/Qt6Core.dll" "${TEST_DIR}/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${QT_DIR}/bin/Qt6Test.dll" "${TEST_DIR}/"
    )

    # 复制 SQL 驱动插件
    add_custom_command(TARGET test_downloadtask POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${QT_DIR}/plugins/sqldrivers" "${TEST_DIR}/sqldrivers"
    )
    add_custom_command(TARGET test_systemtest POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${QT_DIR}/plugins/sqldrivers" "${TEST_DIR}/sqldrivers"
    )
    add_custom_command(TARGET tst_httptest POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${QT_DIR}/plugins/sqldrivers" "${TEST_DIR}/sqldrivers"
    )
endif()

