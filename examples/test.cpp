#include "zephyrlog/zephyrlog.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <dirent.h>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

using namespace zephyrlog;

namespace
{
constexpr uint32_t kRollSize1MiB = 1U * 1024U * 1024U;
constexpr uint32_t kRollSize2MiB = 2U * 1024U * 1024U;
constexpr uint32_t kRollSize10MiB = 10U * 1024U * 1024U;
constexpr uint32_t kQuickRingSlots = 2U;
} // namespace

// ========== 辅助函数：列出目录中的文件 ==========
void listLogFiles(const std::string& directory)
{
    std::cout << "\n当前日志文件：" << std::endl;

    DIR* dir = opendir(directory.c_str());
    if (!dir)
    {
        std::cout << "无法打开目录: " << directory << std::endl;
        return;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr)
    {
        std::string filename = entry->d_name;
        if (filename.find(".txt") != std::string::npos)
        {
            std::cout << "  - " << filename << std::endl;
        }
    }
    closedir(dir);
}

// ========== 辅助函数：清理日志文件 ==========
void cleanupLogs(const std::string& directory, const std::string& prefix)
{
    DIR* dir = opendir(directory.c_str());
    if (!dir)
        return;

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr)
    {
        std::string filename = entry->d_name;
        if (filename.find(prefix) == 0 &&
            filename.find(".txt") != std::string::npos)
        {
            std::string fullpath = directory + "/" + filename;
            std::remove(fullpath.c_str());
        }
    }
    closedir(dir);
}

// ========== 测试1：基础文件写入 ==========
void testBasicFileWrite()
{
    std::cout << "\n========== 测试1：基础文件写入 ==========" << std::endl;

    cleanupLogs("./", "test1");

    initializeSafe("./", "test1", kRollSize10MiB);

    ZINFO << "测试文件写入功能";
    ZDEBUG << "这条日志会被写入文件";
    ZWARN << "文件路径：./test1.1.txt";

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    listLogFiles("./");

    std::cout << "\n提示：请检查 ./test1.1.txt 文件是否存在" << std::endl;
}

// ========== 测试2：文件滚动 ==========
void testFileRolling()
{
    std::cout << "\n========== 测试2：文件滚动 ==========" << std::endl;

    cleanupLogs("./", "test2");

    initializeSafe("./", "test2", kRollSize1MiB);

    std::cout << "写入大量日志，触发文件滚动..." << std::endl;

    const int messageCount = 15000;

    for (int index = 0; index < messageCount; ++index)
    {
        ZINFO << "File rolling test message " << index
              << " with some additional data to increase size: " << 3.141592653589793
              << " " << index * 2;

        if (index > 0 && index % 7000 == 0)
        {
            std::cout << "已写入 " << index << " 条，应该触发滚动" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
    }

    std::cout << "\n等待后台线程处理..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));

    listLogFiles("./");

    std::cout << "\n预期：应该有 test2.1.txt, test2.2.txt 等多个文件" << std::endl;
}

// ========== 测试3：高负载持久化 ==========
void testHighLoadPersistence()
{
    std::cout << "\n========== 测试3：高负载持久化 ==========" << std::endl;

    cleanupLogs("./", "test3");

    initializeQuick(kQuickRingSlots, "./", "test3", kRollSize1MiB);

    const int messageCount = 100000;

    std::cout << "写入 " << messageCount << " 条日志..." << std::endl;

    auto start = std::chrono::high_resolution_clock::now();

    for (int index = 0; index < messageCount; ++index)
    {
        ZINFO << "High load test " << index << " data: " << 3.14159;
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    const auto durMs = std::max<long long>(1, duration.count());
    std::cout << "\n写入完成：" << duration.count() << " ms" << std::endl;
    std::cout << "吞吐量：" << (messageCount * 1000.0 / durMs) << " logs/s"
              << std::endl;

    std::cout << "\n等待后台线程写入文件..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(5));

    listLogFiles("./");
}

// ========== 测试4：优雅关闭 ==========
void testGracefulShutdown()
{
    std::cout << "\n========== 测试4：优雅关闭 ==========" << std::endl;

    cleanupLogs("./", "test4");

    initializeSafe("./", "test4", kRollSize10MiB);

    const int messageCount = 10000;

    std::cout << "快速写入 " << messageCount << " 条日志..." << std::endl;

    for (int index = 0; index < messageCount; ++index)
    {
        ZINFO << "Shutdown test " << index;
    }

    std::cout << "写入完成，立即触发析构（测试优雅关闭）..." << std::endl;

    initializeSafe("./", "test4_new", kRollSize10MiB);

    std::cout << "析构完成，检查日志文件..." << std::endl;

    listLogFiles("./");

    std::cout << "\n提示：test4.1.txt 应该包含所有 10000 条日志（无丢失）" << std::endl;
}

// ========== 测试5：多线程文件写入 ==========
void testMultithreadFileWrite()
{
    std::cout << "\n========== 测试5：多线程文件写入 ==========" << std::endl;

    cleanupLogs("./", "test5");

    initializeSafe("./", "test5", kRollSize2MiB);

    const int numThreads = 8;
    const int logsPerThread = 5000;

    auto worker = [](int threadId) {
        for (int i = 0; i < logsPerThread; ++i)
        {
            ZINFO << "Thread " << threadId << " message " << i;
        }
    };

    std::cout << "启动 " << numThreads << " 个线程..." << std::endl;

    auto start = std::chrono::high_resolution_clock::now();

    std::vector<std::thread> threads;
    for (int i = 0; i < numThreads; ++i)
    {
        threads.emplace_back(worker, i);
    }

    for (auto& t : threads)
    {
        t.join();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    const auto durMs = std::max<long long>(1, duration.count());
    std::cout << "\n写入完成：" << duration.count() << " ms" << std::endl;
    std::cout << "吞吐量："
              << (numThreads * logsPerThread * 1000.0 / durMs) << " logs/s"
              << std::endl;

    std::cout << "\n等待后台线程写入文件..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));

    listLogFiles("./");
}

// ========== 测试6：CRIT 日志立即刷新 ==========
void testCritFlush()
{
    std::cout << "\n========== 测试6：CRIT 日志立即刷新 ==========" << std::endl;

    cleanupLogs("./", "test6");

    initializeSafe("./", "test6", kRollSize10MiB);

    ZINFO << "普通日志（可能在缓冲区）";
    ZCRIT << "严重错误（立即刷新）";

    std::cout << "\n提示：CRIT 日志应该立即写入文件（无需等待）" << std::endl;

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    listLogFiles("./");
}

int main()
{
    std::cout << "========== Day 5: 文件管理测试 ==========" << std::endl;

    // testBasicFileWrite();
    // testFileRolling();
    testHighLoadPersistence();
    // testGracefulShutdown();
    // testMultithreadFileWrite();
    // testCritFlush();

    std::cout << "\n========== 所有测试完成 ==========" << std::endl;
    std::cout << "请检查当前目录下的 test*.txt 文件" << std::endl;

    return 0;
}
