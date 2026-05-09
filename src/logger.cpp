#include "zephyrlog/logger.h"
#include "zephyrlog/asynclogger.h"
#include <utility>

namespace zephyrlog
{

Logger::Logger(std::string name, uint32_t bufferSize,
               std::string logDir, std::string logFile,
               uint32_t rollSize)
    : m_name(std::move(name))
    , m_impl(std::make_unique<AsyncLogger>(bufferSize, logDir, logFile, rollSize))
{
}

Logger::Logger(std::string name,
               std::string logDir, std::string logFile,
               uint32_t rollSize)
    : m_name(std::move(name))
    , m_impl(std::make_unique<AsyncLogger>(logDir, logFile, rollSize))
{
}

Logger::~Logger() = default;

void Logger::add(LogLine&& logline)
{
    m_impl->add(std::move(logline));
}

bool LoggerRef::operator==(LogLine& logline)
{
    logger->add(std::move(logline));
    return true;
}

} // namespace zephyrlog
