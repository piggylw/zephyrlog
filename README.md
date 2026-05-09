# ZephyrLog

C++17 高性能异步日志库，采用"先编码后格式化"（encode-first, format-later）设计。

## 整体介绍

- **异步架构** — 用户线程只负责将参数编码到 LogLine 的二进制缓冲区，后台线程串行读取并写入文件，用户线程延迟极低
- **双缓冲区模式** — Quick（固定环形缓冲，写满覆盖）和 Safe（动态队列，永不丢日志）
- **小缓冲区优化（SBO）** — 每条 LogLine 内置 224 字节栈缓冲，短日志零堆分配
- **零开销过滤** — 日志级别检查发生在 LogLine 构造之前，被过滤的消息无任何编码成本
- **文件滚动** — 按文件大小自动滚动，默认 10 MiB
- **终端输出** — 可选同时输出到终端，方便开发调试
- **多日志器** — 支持多个命名日志器，各自独立配置级别和输出文件
- **线程安全** — MPSC 架构，SpinLock + 原子操作保证并发安全
- **优雅关闭** — 析构时自动排空缓冲区中的剩余日志

输出格式：`[YYYY-MM-DD HH:MM:SS.微秒][LEVEL][thread-id][file:func:line] msg...`

## 快速开始

```cpp
#include "zephyrlog/zephyrlog.h"

int main()
{
    using namespace zephyrlog;

    // 初始化默认日志器（必须调用，否则日志被静默丢弃）
    // 最后一个参数 true 表示同时输出到终端
    initializeQuick(4096, "./logs/", "app", 10 * 1024 * 1024, true);

    setLogLevel(LogLevel::DEBUG);

    ZDEBUG << "Debug message";
    ZINFO  << "Hello, ZephyrLog!";
    ZWARN  << "Warning: value=" << 3.14;
    ZERROR << "Something went wrong, code=" << 500;
    ZCRIT  << "Critical failure!";

    ZINFO << "Logging numbers: " << 42 << ", " << 3.14159;
    return 0;
}
```

`initializeQuick` 或 `initializeSafe` 必须先调用一次，之后才能使用 `ZINFO` 等宏写入日志文件。

## 详细 API + 示例

### 日志级别

| 级别 | 枚举值 | 行为 |
|------|--------|------|
| DEBUG | `LogLevel::DEBUG` | 调试信息 |
| INFO | `LogLevel::INFO` | 一般信息（默认级别） |
| WARN | `LogLevel::WARN` | 警告 |
| ERROR | `LogLevel::ERROR` | 错误 |
| CRITICAL | `LogLevel::CRITICAL` | 严重错误，写入后立即 flush |

```cpp
zephyrlog::setLogLevel(zephyrlog::LogLevel::INFO);   // 只显示 INFO 及以上
auto lv = zephyrlog::getLogLevel();                   // 查询当前级别
```

### 默认日志器 — 初始化与宏

Quick 模式（固定环形缓冲区，写满覆盖旧数据，适合高频低延迟场景）：

```cpp
zephyrlog::initializeQuick(4096, "./logs/", "app", 10 * 1024 * 1024, true);
//                          ↑buf大小 ↑目录     ↑文件名 ↑滚动大小(默认10MiB) ↑终端输出(默认false)
```

Safe 模式（动态队列，绝不丢日志，适合可靠性要求高的场景）：

```cpp
zephyrlog::initializeSafe("./logs/", "app", 10 * 1024 * 1024, true);
//                         ↑目录     ↑文件名 ↑滚动大小(默认10MiB) ↑终端输出(默认false)
```

日志宏：

```cpp
ZDEBUG << "debug" << var;
ZINFO  << "info"  << var;
ZWARN  << "warn"  << var;
ZERROR << "error" << var;
ZCRIT  << "crit"  << var;
```

### 多日志器 — 创建、获取、切换

不同模块可使用独立的日志器，分别输出到不同文件、配置不同级别：

```cpp
// 创建（最后一个参数 true 表示同时输出到终端）
auto netLog = zephyrlog::createLogger("network", 4096, "./logs/", "net", 10*1024*1024, true);
auto dbLog  = zephyrlog::createSafeLogger("database", "./logs/", "db", 10*1024*1024, true);

// 按名称获取
auto lg = zephyrlog::getLogger("network");

// 设置为默认日志器（影响 ZINFO 等宏的目标）
zephyrlog::setDefaultLogger(netLog);

// 删除
zephyrlog::removeLogger("network");
```

多日志器宏（指定日志器，不依赖默认日志器）：

```cpp
auto lg = zephyrlog::getLogger("network");
ZI(lg)  << "message";
ZD(lg) << "debug";
ZW(lg)  << "warn";
ZE(lg) << "error";
ZC(lg)  << "crit";
```

> 所有宏都会先检查对应日志器的级别，级别不满足时跳过 LogLine 构造，无编码开销。

### 支持的流操作类型

| 类型 | 说明 |
|------|------|
| `char` | 单字符 |
| `int32_t` / `uint32_t` | 32 位整数 |
| `int64_t` / `uint64_t` | 64 位整数 |
| `double` | 浮点数 |
| `const char*` / `char*` | C 字符串（拷贝存储） |
| `const char[N]` | 字符串字面量（仅存指针，零拷贝） |
| `std::string` | C++ 字符串 |

### 缓冲区模式对比

| | Quick (RingBuffer) | Safe (QueueBuffer) |
|---|---|---|
| 缓冲区 | 固定大小环形缓冲 | 动态分配队列 |
| 写满行为 | 覆盖旧数据 | 分配新 Buffer |
| 内存上限 | 固定 | 动态增长 |
| 吞吐量 | 更高 | 略低 |
| 适用场景 | 高频日志，可容忍丢失 | 必须保留所有日志 |

## 编译使用

### 编译动态库（默认）

```bash
cmake -B build
cmake --build build
# → build/libzephyrlog.so
```

### 编译静态库

```bash
cmake -B build -DBUILD_SHARED_LIBS=OFF
cmake --build build
# → build/libzephyrlog.a
```

> 推荐使用动态库：ZephyrLog 内部有全局状态（logger 注册表、默认日志器）。多个 `.so` 各自静态链接会导致各自持有一份独立的全局状态，动态库保证进程内只有一份。

### 运行示例

```bash
./build/test       # 基本使用演示
./build/benchmark  # 与 spdlog 性能对比
```

### 作为 SDK 安装

```bash
cmake -B build -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build
cmake --install build
```

安装后结构：
```
/usr/local/
├── include/zephyrlog/   # 头文件
├── lib/libzephyrlog.so  # 动态库
└── lib/cmake/ZephyrLog/ # find_package 配置
```

其他项目通过 CMake 引用：

```cmake
find_package(ZephyrLog REQUIRED)
target_link_libraries(myapp zephyrlog::zephyrlog)
```

或直接 `add_subdirectory`：

```cmake
add_subdirectory(path/to/ZephyrLog)
target_link_libraries(myapp zephyrlog)
```

### 使用预编译 SDK（无需编译）

仓库 `sdk/` 目录提供了预编译的动态库和头文件，可直接使用：

```
sdk/
├── include/zephyrlog/      # 所有头文件
└── lib/
    ├── aarch64/libzephyrlog.so   # ARM aarch64
    └── x86_64/libzephyrlog.so    # x86_64
```

**命令行编译：**

```bash
# 根据目标平台选择 -L 路径，-Wl,-rpath 指定运行时 .so 搜索路径
g++ -std=c++17 -Ipath/to/sdk/include -Lpath/to/sdk/lib/x86_64 main.cpp \
    -lzephyrlog -lpthread -Wl,-rpath,path/to/sdk/lib/x86_64
```

**CMake 引用：**

```cmake
add_library(zephyrlog SHARED IMPORTED)
set_target_properties(zephyrlog PROPERTIES
    IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/path/to/sdk/lib/x86_64/libzephyrlog.so
    INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_SOURCE_DIR}/path/to/sdk/include
)
target_link_libraries(myapp zephyrlog pthread)
```

> 根据目标平台选择 `aarch64` 或 `x86_64` 目录。

## 与 spdlog 对比

平台：Linux aarch64, GCC，双方均写入 `/dev/null`（排除磁盘 I/O），反映纯编码+格式化性能。

运行 `./build/benchmark` 可在本机复现。

### 单线程 — 500,000 条日志

| 库 | 耗时 | 吞吐量 |
|---|---|---|
| **ZephyrLog** | 90 ms | 5,550,190 logs/s |
| spdlog | 189 ms | 2,650,481 logs/s |

ZephyrLog 约为 spdlog 的 **2.1x**。

### 多线程 — 4 线程 × 100,000 条日志

| 库 | 耗时 | 吞吐量 |
|---|---|---|
| **ZephyrLog** | 135 ms | 2,968,173 logs/s |
| spdlog | 205 ms | 1,955,225 logs/s |

ZephyrLog 约为 spdlog 的 **1.5x**。

## 许可证

MIT License
