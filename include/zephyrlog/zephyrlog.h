#pragma once

#include "logline.h"
#include "zephyrlog/logger.h"
#include <memory>
#include <string>

namespace zephyrlog
{

struct ZEPHYRLOG_EXPORT ZephyrLog
{
    bool operator==(LogLine& logline);
};

// ---- Global log level (operates on the default logger) ----
ZEPHYRLOG_EXPORT void setLogLevel(LogLevel level);
ZEPHYRLOG_EXPORT LogLevel getLogLevel();

// ---- Default logger initialization ----
ZEPHYRLOG_EXPORT void initializeQuick(uint32_t bufferSize,
                                       const std::string& logDirectory,
                                       const std::string& logFileName,
                                       uint32_t rollSize = 10 * 1024 * 1024);
ZEPHYRLOG_EXPORT void initializeSafe(const std::string& logDirectory,
                                      const std::string& logFileName,
                                      uint32_t rollSize = 10 * 1024 * 1024);

// ---- Named logger registry ----
ZEPHYRLOG_EXPORT std::shared_ptr<Logger> createLogger(const std::string& name,
                                                       uint32_t bufferSize,
                                                       const std::string& logDirectory,
                                                       const std::string& logFileName,
                                                       uint32_t rollSize = 10 * 1024 * 1024);
ZEPHYRLOG_EXPORT std::shared_ptr<Logger> createSafeLogger(const std::string& name,
                                                           const std::string& logDirectory,
                                                           const std::string& logFileName,
                                                           uint32_t rollSize = 10 * 1024 * 1024);
ZEPHYRLOG_EXPORT std::shared_ptr<Logger> getLogger(const std::string& name);
ZEPHYRLOG_EXPORT void setDefaultLogger(std::shared_ptr<Logger> logger);
ZEPHYRLOG_EXPORT std::shared_ptr<Logger> getDefaultLogger();
ZEPHYRLOG_EXPORT void removeLogger(const std::string& name);

} // namespace zephyrlog

// ========== Default-logger macros (backward compatible) ==========
#define LOG(LEVEL) \
    zephyrlog::ZephyrLog() == zephyrlog::LogLine(LEVEL, __FILE__, __func__, __LINE__)

#define ZDEBUG   zephyrlog::LogLevel::DEBUG >= zephyrlog::getLogLevel() && LOG(zephyrlog::LogLevel::DEBUG)
#define ZINFO    zephyrlog::LogLevel::INFO >= zephyrlog::getLogLevel() && LOG(zephyrlog::LogLevel::INFO)
#define ZWARN    zephyrlog::LogLevel::WARN >= zephyrlog::getLogLevel() && LOG(zephyrlog::LogLevel::WARN)
#define ZERROR   zephyrlog::LogLevel::ERROR >= zephyrlog::getLogLevel() && LOG(zephyrlog::LogLevel::ERROR)
#define ZCRIT    zephyrlog::LogLevel::CRITICAL >= zephyrlog::getLogLevel() && LOG(zephyrlog::LogLevel::CRITICAL)

// ========== Named-logger macros ==========
#define ZD(logger) \
    zephyrlog::LogLevel::DEBUG >= (logger)->level() && \
    zephyrlog::LoggerRef{(logger)} == zephyrlog::LogLine(zephyrlog::LogLevel::DEBUG, __FILE__, __func__, __LINE__)

#define ZI(logger) \
    zephyrlog::LogLevel::INFO >= (logger)->level() && \
    zephyrlog::LoggerRef{(logger)} == zephyrlog::LogLine(zephyrlog::LogLevel::INFO, __FILE__, __func__, __LINE__)

#define ZW(logger) \
    zephyrlog::LogLevel::WARN >= (logger)->level() && \
    zephyrlog::LoggerRef{(logger)} == zephyrlog::LogLine(zephyrlog::LogLevel::WARN, __FILE__, __func__, __LINE__)

#define ZE(logger) \
    zephyrlog::LogLevel::ERROR >= (logger)->level() && \
    zephyrlog::LoggerRef{(logger)} == zephyrlog::LogLine(zephyrlog::LogLevel::ERROR, __FILE__, __func__, __LINE__)

#define ZC(logger) \
    zephyrlog::LogLevel::CRITICAL >= (logger)->level() && \
    zephyrlog::LoggerRef{(logger)} == zephyrlog::LogLine(zephyrlog::LogLevel::CRITICAL, __FILE__, __func__, __LINE__)
