#include "zephyrlog/zephyrlog.h"
#include "zephyrlog/asynclogger.h"
#include <atomic>
#include <iostream>
#include <memory>

namespace zephyrlog
{
static std::unique_ptr<AsyncLogger> g_logger;
static std::atomic<AsyncLogger*> g_atomicLogger{nullptr};
static std::atomic<LogLevel> g_logLevel{LogLevel::DEBUG};

void setLogLevel(LogLevel level)
{
    g_logLevel.store(level);
}

LogLevel getLogLevel()
{
    return g_logLevel.load();
}

void initializeQuick(uint32_t bufferSize, 
                     const std::string& logDirectory, 
                     const std::string& logFileName, 
                     uint32_t rollSize)
{
    g_logger = std::make_unique<AsyncLogger>(bufferSize, logDirectory, logFileName, rollSize);
    g_atomicLogger.store(g_logger.get());
}

void initializeSafe(const std::string& logDirectory, 
                    const std::string& logFileName, 
                    uint32_t rollSize)
{
    g_logger = std::make_unique<AsyncLogger>(logDirectory, logFileName, rollSize);
    g_atomicLogger.store(g_logger.get());
}

bool ZephyrLog::operator==(LogLine& logline)
{
    g_atomicLogger.load()->add(std::move(logline));
    return true;
}

}
