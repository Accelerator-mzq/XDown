cmake_minimum_required(VERSION 3.20)

# 项目配置
project(XDown VERSION 1.0.0 LANGUAGES CXX)

# 项目根目录 (供子目录使用)
set(XDOWN_SOURCE_DIR ${CMAKE_SOURCE_DIR})

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 启用 Qt 自动处理
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 查找 Qt 组件
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Quick
    QuickControls2
    Network
    Sql
    Widgets
)

# 开启测试功能
enable_testing()

# ========== 核心库: XDownCore (静态库) ==========
# 将功能代码编译成静态库，主程序和测试都链接这个库
set(CORE_SOURCES
    src/core/DBManager.cpp
    src/core/HttpDownloader.cpp
    src/core/DownloadEngine.cpp
    src/gui/TaskListModel.cpp
)

add_library(XDownCore STATIC ${CORE_SOURCES})

target_include_directories(XDownCore PUBLIC
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(XDownCore PUBLIC
    Qt6::Core
    Qt6::Network
    Qt6::Sql
)

# ========== 主程序: appXDown ==========
set(SOURCES
    src/main.cpp
)

set(RESOURCES
    resources/qml.qrc
)

add_executable(appXDown ${SOURCES} ${RESOURCES})

target_include_directories(appXDown PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(appXDown PRIVATE
    XDownCore
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Widgets
)

# QML 静态检查
set_target_properties(appXDown PROPERTIES
    QML_LINT "WARN"
)

# Windows 自动化部署
if(WIN32)
    add_definitions(-DUNICODE -D_UNICODE)

    # 尝试找到依赖 Qt 版本对应的 windeployqt
    get_target_property(_qmake_executable Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")

    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET appXDown POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}" $<TARGET_FILE:appXDown>
            COMMENT "Running windeployqt to copy Qt DLLs..."
        )
    else()
        message(WARNING "windeployqt not found! Qt DLLs will not be deployed automatically.")
    endif()
endif()

# 安装配置
install(TARGETS appXDown
    RUNTIME DESTINATION bin
)

# 打印配置信息
message(STATUS "XDown v${PROJECT_VERSION}")
message(STATUS "Qt Version: ${Qt6_VERSION}")
message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "QML Lint: ENABLED (WARN mode)")

# 引入测试子目录
add_subdirectory(tests)
