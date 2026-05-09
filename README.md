# ZephyrLog

C++17 高性能异步日志库，采用"先编码后格式化"（encode-first, format-later）设计。

## 特性

- **异步日志** — 日志写入在后台线程执行，用户线程仅负责编码，延迟极低
- **双缓冲区模式**
  - `Quick` 模式：固定大小环形缓冲区（RingBuffer），写满后覆盖旧数据，适合高频低延迟场景
  - `Safe` 模式：动态队列缓冲区（QueueBuffer），保证不丢日志，适合可靠性要求高的场景
- **小缓冲区优化（SBO）** — 每条 LogLine 内置 224 字节栈缓冲，短日志不触发堆分配
- **文件滚动** — 按文件大小自动滚动（默认 10 MiB）
- **多日志器** — 支持创建多个命名日志器，各自独立配置级别和输出文件
- **线程安全** — 多生产者单消费者（MPSC）架构，SpinLock + 原子操作保证并发安全
- **零开销过滤** — 日志级别检查在 LogLine 构造之前，被过滤的消息几乎无性能消耗
- **优雅关闭** — 析构时自动排空缓冲区剩余日志

## 快速开始

```cpp
#include "zephyrlog/zephyrlog.h"

int main()
{
    // 初始化默认日志器（Quick 模式：环形缓冲区 4096 条）
    zephyrlog::initializeQuick(4096, "./logs/", "app", 10 * 1024 * 1024);

    // 设置日志级别
    zephyrlog::setLogLevel(zephyrlog::LogLevel::DEBUG);

    ZINFO << "Hello, ZephyrLog!";
    ZDEBUG << "Debug info: " << 42;
    ZWARN << "Warning: value=" << 3.14;
    ZERROR << "Something went wrong, code=" << 500;
    ZCRIT << "Critical failure!";

    return 0;
}
```

输出格式：
```
[2025-03-15 10:30:45.123456][INFO ][140123456789][main.cpp:main:15] Hello, ZephyrLog!
```

## 构建

### 编译（默认：动态库 .so）

```bash
cmake -B build
cmake --build build
```

生成 `build/libzephyrlog.so` + 头文件。

### 编译静态库

```bash
cmake -B build -DBUILD_SHARED_LIBS=OFF
cmake --build build
```

生成 `build/libzephyrlog.a`。

> **推荐使用动态库**：ZephyrLog 内部有全局状态（logger 注册表、默认日志器）。如果多个 `.so` 各自静态链接 ZephyrLog，会各自持有一份独立的全局状态，导致日志器注册表不共享。动态库保证整个进程只有一份全局状态。

### 运行示例

```bash
./build/test
```

### 安装为 SDK

```bash
cmake -B build -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build
cmake --install build
```

安装后的文件结构：
```
/usr/local/
├── include/zephyrlog/    # 头文件
├── lib/libzephyrlog.so   # 动态库
└── lib/cmake/ZephyrLog/  # CMake find_package 配置
```

其他项目通过 CMake 使用：

```cmake
find_package(ZephyrLog REQUIRED)
target_link_libraries(myapp zephyrlog::zephyrlog)
```

也可以直接通过 `add_subdirectory` 引入：

```cmake
add_subdirectory(path/to/ZephyrLog)
target_link_libraries(myapp zephyrlog)
```

## API 文档

### 日志级别

| 级别 | 枚举值 | 说明 |
|------|--------|------|
| DEBUG | `LogLevel::DEBUG` | 调试信息 |
| INFO | `LogLevel::INFO` | 一般信息 |
| WARN | `LogLevel::WARN` | 警告 |
| ERROR | `LogLevel::ERROR` | 错误 |
| CRITICAL | `LogLevel::CRITICAL` | 严重错误（强制 flush） |

### 默认日志器 API

```cpp
// 初始化（二选一）
void initializeQuick(uint32_t bufferSize, const std::string& dir,
                     const std::string& file, uint32_t rollSize = 10*1024*1024);
void initializeSafe(const std::string& dir,
                    const std::string& file, uint32_t rollSize = 10*1024*1024);

// 日志级别
void setLogLevel(LogLevel level);
LogLevel getLogLevel();
```

### 默认日志器宏

```cpp
ZDEBUG << "msg" << value;   // 调试
ZINFO  << "msg" << value;   // 信息
ZWARN  << "msg" << value;   // 警告
ZERROR << "msg" << value;   // 错误
ZCRIT  << "msg" << value;   // 严重
```

### 多日志器 API

```cpp
// 创建命名日志器
auto log1 = zephyrlog::createLogger("module_a", 4096, "./logs/", "mod_a");
auto log2 = zephyrlog::createSafeLogger("module_b", "./logs/", "mod_b");

// 按名称获取
auto lg = zephyrlog::getLogger("module_a");

// 设置默认日志器
zephyrlog::setDefaultLogger(log1);

// 获取默认日志器
auto def = zephyrlog::getDefaultLogger();

// 删除日志器
zephyrlog::removeLogger("module_a");
```

### 多日志器宏

```cpp
auto lg = zephyrlog::getLogger("module_a");

ZLOG_DEBUG(lg) << "debug msg";   // 调试
ZLOG_INFO(lg)  << "info msg";    // 信息
ZLOG_WARN(lg)  << "warn msg";    // 警告
ZLOG_ERROR(lg) << "error msg";   // 错误
ZLOG_CRIT(lg)  << "crit msg";    // 严重
```

> 注意：这些宏会先检查该日志器的级别，级别不满足时跳过 LogLine 构造，基本无开销。

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

## 两种缓冲区模式对比

| | Quick (RingBuffer) | Safe (QueueBuffer) |
|---|---|---|
| 缓冲区 | 固定大小环形缓冲 | 动态分配队列 |
| 写满行为 | 覆盖旧数据 | 分配新 Buffer |
| 内存上限 | 固定 | 动态增长 |
| 适用场景 | 高频日志，可以容忍丢失 | 必须保留所有日志 |
| 吞吐量 | 更高 | 略低 |

## 项目结构

```
ZephyrLog/
├── include/zephyrlog/
│   ├── zephyrlog.h        # 公共 API、宏定义
│   ├── logger.h           # Logger 类、LoggerRef
│   ├── logline.h          # LogLine 编码核心
│   ├── asynclogger.h      # AsyncLogger（后台线程 + Buffer + FileWriter）
│   ├── bufferbase.h       # 缓冲区抽象接口
│   ├── ringbuffer.h       # 环形缓冲区（MPSC，固定大小）
│   ├── queuebuffer.h      # 队列缓冲区（动态分配，保证交付）
│   ├── filewriter.h       # 文件写入器（支持滚动）
│   └── spinlock.h         # RAII 自旋锁
├── src/                   # 实现文件
├── examples/
│   └── test.cpp           # 集成测试程序
└── cmake/                 # CMake 打包配置
```

## 许可证

MIT License
