#include "zephyrlog/zephyrlog.h"

int main()
{
    using namespace zephyrlog;

    // 初始化默认日志器（Quick 模式，缓冲区 4096 条，滚动 10 MiB，同时输出到终端）
    initializeQuick(4096, "./logs/", "app", 10 * 1024 * 1024, true);

    // 设置日志级别
    setLogLevel(LogLevel::DEBUG);

    // 五种日志级别
    ZDEBUG << "Debug message";
    ZINFO  << "Hello, ZephyrLog!";
    ZWARN  << "Warning: value=" << 3.14;
    ZERROR << "Something went wrong, code=" << 500;
    ZCRIT  << "Critical failure!";

    // 支持的数据类型
    char ch = 'A';
    int32_t i32 = -100;
    uint32_t u32 = 200;
    int64_t i64 = -9999999999;
    uint64_t u64 = 9999999999;
    double d = 3.141592653589793;
    const char* cstr = "C string";
    std::string s = "std::string";

    ZINFO << "char: " << ch;
    ZINFO << "int32: " << i32 << ", uint32: " << u32;
    ZINFO << "int64: " << i64 << ", uint64: " << u64;
    ZINFO << "double: " << d;
    ZINFO << "C string: " << cstr;
    ZINFO << "std::string: " << s;
    ZINFO << "string literal with int: " << 42;

    // ---- 多日志器 ----
    auto netLog = createLogger("network", 4096, "./logs/", "net");
    auto dbLog  = createSafeLogger("database", "./logs/", "db");

    netLog->setLevel(LogLevel::DEBUG);
    dbLog->setLevel(LogLevel::WARN);

    auto net = netLog.get();   // Logger* for macros
    auto db  = dbLog.get();

    ZI(net) << "Network request from 192.168.1.1";
    ZD(net) << "Connection accepted, fd=" << 5;

    ZW(db) << "Slow query detected, took " << 3.5 << "s";
    ZE(db) << "Connection pool exhausted, retries=" << 3;
    ZD(db) << "This won't be logged (level is WARN)";

    // 切换到其他默认日志器
    setDefaultLogger(netLog);
    ZINFO << "This now goes to net log file";

    return 0;
}
