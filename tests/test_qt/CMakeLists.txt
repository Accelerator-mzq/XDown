cmake_minimum_required(VERSION 3.20)
project(TestQt LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Test)

set(CMAKE_AUTOMOC ON)

qt_add_executable(test_qt test_qt.cpp)

target_link_libraries(test_qt PRIVATE Qt6::Core Qt6::Test)
