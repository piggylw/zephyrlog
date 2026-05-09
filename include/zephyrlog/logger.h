#pragma once

#include "zephyrlog/logline.h"
#include <atomic>
#include <memory>
#include <string>

namespace zephyrlog
{

class AsyncLogger;

class ZEPHYRLOG_EXPORT Logger
{
public:
    Logger(std::string name, uint32_t bufferSize,
           std::string logDir, std::string logFile,
           uint32_t rollSize = 10 * 1024 * 1024);
    Logger(std::string name,
           std::string logDir, std::string logFile,
           uint32_t rollSize = 10 * 1024 * 1024);

    ~Logger();

    void add(LogLine&& logline);

    void setLevel(LogLevel level) { m_level.store(level, std::memory_order_relaxed); }
    LogLevel level() const { return m_level.load(std::memory_order_relaxed); }
    const std::string& name() const { return m_name; }

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

private:
    std::string m_name;
    std::unique_ptr<AsyncLogger> m_impl;
    std::atomic<LogLevel> m_level{LogLevel::DEBUG};
};

struct ZEPHYRLOG_EXPORT LoggerRef
{
    Logger* logger;
    bool operator==(LogLine& logline);
};

} // namespace zephyrlog
