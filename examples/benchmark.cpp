#include "zephyrlog/zephyrlog.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <chrono>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <thread>
#include <vector>
#include <unistd.h>
#include <fcntl.h>

constexpr int kSingleIters = 500000;
constexpr int kMultiItersPer = 100000;
constexpr int kThreads = 4;

// Redirect stdout → /dev/null for duration of scope, restore on destruction
struct NullStdout
{
    NullStdout()  { m_fd = dup(1); int n = open("/dev/null", O_WRONLY); dup2(n, 1); close(n); }
    ~NullStdout() { fflush(stdout); dup2(m_fd, 1); close(m_fd); }
    int m_fd;
};

int main()
{
    std::ostringstream report;

    report << "========== ZephyrLog vs spdlog Benchmark ==========\n";
    report << "Platform: Linux arm64, GCC, both to /dev/null\n";

    // ===== Single-thread =====
    report << "\n=== Single-thread (" << kSingleIters << " iterations) ===\n";

    double z_st;
    {
        NullStdout ns;
        zephyrlog::setLogLevel(zephyrlog::LogLevel::INFO);
        auto t1 = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < kSingleIters; ++i)
            ZINFO << "bench msg " << i << " val=" << 3.14159;
        auto t2 = std::chrono::high_resolution_clock::now();
        long long us = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
        z_st = (kSingleIters * 1e6) / (us > 0 ? us : 1);
        report << "  ZephyrLog: " << us / 1000.0 << " ms  (" << static_cast<long long>(z_st) << " logs/s)\n";
    }

    double s_st;
    {
        auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("/dev/null", true);
        auto spd = std::make_shared<spdlog::logger>("b1", sink);
        spd->set_level(spdlog::level::info);
        auto t1 = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < kSingleIters; ++i)
            spd->info("bench msg {} val={}", i, 3.14159);
        auto t2 = std::chrono::high_resolution_clock::now();
        long long us = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
        s_st = (kSingleIters * 1e6) / (us > 0 ? us : 1);
        report << "  spdlog   : " << us / 1000.0 << " ms  (" << static_cast<long long>(s_st) << " logs/s)\n";
    }
    report << "  => ZephyrLog " << (z_st / s_st) << "x vs spdlog\n";

    // ===== Multi-thread =====
    report << "\n=== Multi-thread (" << kThreads << " threads x " << kMultiItersPer << " msgs) ===\n";
    long long total = kMultiItersPer * kThreads;

    {
        NullStdout ns;
        zephyrlog::setLogLevel(zephyrlog::LogLevel::INFO);
        auto t1 = std::chrono::high_resolution_clock::now();
        std::vector<std::thread> ths;
        for (int t = 0; t < kThreads; ++t)
            ths.emplace_back([t]() {
                for (int i = 0; i < kMultiItersPer; ++i)
                    ZINFO << "thread " << t << " msg " << i << " val=" << 3.14159;
            });
        for (auto& th : ths) th.join();
        auto t2 = std::chrono::high_resolution_clock::now();
        long long us = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
        long long rate = static_cast<long long>((total * 1e6) / (us > 0 ? us : 1));
        report << "  ZephyrLog: " << us / 1000.0 << " ms  (" << rate << " logs/s)\n";
    }

    {
        auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("/dev/null", true);
        auto spd = std::make_shared<spdlog::logger>("b2", sink);
        spd->set_level(spdlog::level::info);
        auto t1 = std::chrono::high_resolution_clock::now();
        std::vector<std::thread> ths;
        for (int t = 0; t < kThreads; ++t)
            ths.emplace_back([&spd, t]() {
                for (int i = 0; i < kMultiItersPer; ++i)
                    spd->info("thread {} msg {} val={}", t, i, 3.14159);
            });
        for (auto& th : ths) th.join();
        auto t2 = std::chrono::high_resolution_clock::now();
        long long us = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
        long long rate = static_cast<long long>((total * 1e6) / (us > 0 ? us : 1));
        report << "  spdlog   : " << us / 1000.0 << " ms  (" << rate << " logs/s)\n";
    }

    report << "\n========== Done ==========\n";
    std::cout << report.str();
    return 0;
}
