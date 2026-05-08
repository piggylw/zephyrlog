cmake_minimum_required(VERSION 3.10)
project(ZephyrLog)

# 生成 compile_commands.json 供 IDE 使用
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 编译选项
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -O2)
endif()

# 头文件目录
include_directories(${PROJECT_SOURCE_DIR}/include)

# 库文件
add_library(zephyrlog STATIC
    src/zephyrlog.cpp
    src/logline.cpp
    src/asynclogger.cpp
    src/ringbuffer.cpp
    src/queuebuffer.cpp
)

# 示例程序
add_executable(test examples/test.cpp)
target_link_libraries(test zephyrlog pthread)