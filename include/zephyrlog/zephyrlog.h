#pragma once

#include "logline.h"
#include "zephyrlog/asynclogger.h"

namespace zephyrlog
{

struct ZephyrLog
{
    bool operator==(LogLine& logline);
};

void setLogLevel(LogLevel level);
LogLevel getLogLevel();
void initializeQuick(uint32_t bufferSize, 
                     const std::string& logDirectory, 
                     const std::string& logFileName, 
                     uint32_t rollSize = 10 * 1024 * 1024);
void initializeSafe(const std::string& logDirectory, 
                    const std::string& logFileName, 
                    uint32_t rollSize = 10 * 1024 * 1024);
}

#define LOG(LEVEL) \
    zephyrlog::ZephyrLog() == zephyrlog::LogLine(LEVEL, __FILE__, __func__, __LINE__)

#define ZDEBUG   zephyrlog::LogLevel::DEBUG >= zephyrlog::getLogLevel() && LOG(zephyrlog::LogLevel::DEBUG)
#define ZINFO    zephyrlog::LogLevel::INFO >= zephyrlog::getLogLevel() && LOG(zephyrlog::LogLevel::INFO)
#define ZWARN    zephyrlog::LogLevel::WARN >= zephyrlog::getLogLevel() && LOG(zephyrlog::LogLevel::WARN)
#define ZERROR   zephyrlog::LogLevel::ERROR >= zephyrlog::getLogLevel() && LOG(zephyrlog::LogLevel::ERROR)
#define ZCRIT    zephyrlog::LogLevel::CRITICAL >= zephyrlog::getLogLevel() && LOG(zephyrlog::LogLevel::CRITICAL)
