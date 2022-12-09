cmake_minimum_required(VERSION 3.0.0)

project(MergeBin)

set(CMAKE_CXX_STANDARD 17)

include_directories (./include)  # 指定incuda头文件路径

file(GLOB SRC_LIST ./src/*.cpp)

add_executable(MergeBin ${SRC_LIST})

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR})