#include "zephyrlog/asynclogger.h"
#include <iostream>
#include <thread>

using namespace zephyrlog;

int main()
{
    std::cout << "========== Day 1: 基础日志测试 ==========" << std::endl;
    
    // 测试1：五种日志级别
    LOG_DEBUG << "这是调试信息";
    LOG_INFO << "这是普通信息";
    LOG_WARN << "这是警告信息";
    LOG_ERROR << "这是错误信息";
    LOG_CRIT << "这是严重错误";

    std::cout << "\n========== 测试多种数据类型 ==========" << std::endl;
    
    // 测试2：整数类型
    int32_t i32 = -12345;
    uint32_t u32 = 67890;
    int64_t i64 = -9876543210LL;
    uint64_t u64 = 1234567890123ULL;
    
    LOG_INFO << "int32: " << i32 << ", uint32: " << u32;
    LOG_INFO << "int64: " << i64 << ", uint64: " << u64;

    // 测试3：浮点数
    double pi = 3.14159265358979;
    LOG_INFO << "PI = " << pi;

    // 测试4：字符串
    std::string str = "C++ 日志库";
    const char* cstr = "字符串指针";
    LOG_INFO << "std::string: " << str;
    LOG_INFO << "const char*: " << cstr;

    // 测试5：混合类型
    LOG_INFO << "用户 " << 10086 << " 登录成功，IP: " << "192.168.1.100";

    std::cout << "\n========== 测试多线程 ==========" << std::endl;
    
    // 测试6：多线程（验证 thread_id）
    auto worker = [](int id) {
        for (int i = 0; i < 3; ++i) {
            LOG_INFO << "线程 " << id << " - 消息 " << i;
        }
    };

    std::thread t1(worker, 1);
    std::thread t2(worker, 2);
    t1.join();
    t2.join();

    std::cout << "\n========== 测试完成 ==========" << std::endl;
    return 0;
}