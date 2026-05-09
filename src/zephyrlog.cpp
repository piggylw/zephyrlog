#include "zephyrlog/zephyrlog.h"
#include "zephyrlog/logger.h"
#include <atomic>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <unordered_map>

namespace zephyrlog
{

// ---- Default logger ----
// atomic_load/atomic_store on shared_ptr ensures safe handover when
// re-initializing: any thread that loaded the old pointer keeps it alive
// via its own shared_ptr copy until it finishes using it.
static std::shared_ptr<Logger> g_defaultLogger;
static std::atomic<LogLevel> g_logLevel{LogLevel::DEBUG};

// ---- Named logger registry ----
static std::shared_mutex g_registryMutex;
static std::unordered_map<std::string, std::shared_ptr<Logger>> g_registry;

// ---- Global log level ----
void setLogLevel(LogLevel level)
{
    g_logLevel.store(level, std::memory_order_relaxed);
    auto def = std::atomic_load(&g_defaultLogger);
    if (def)
        def->setLevel(level);
}

LogLevel getLogLevel()
{
    auto def = std::atomic_load(&g_defaultLogger);
    if (def)
        return def->level();
    return g_logLevel.load(std::memory_order_relaxed);
}

// ---- Default logger initialization ----
void initializeQuick(uint32_t bufferSize,
                     const std::string& logDirectory,
                     const std::string& logFileName,
                     uint32_t rollSize)
{
    auto logger = std::make_shared<Logger>("default", bufferSize,
                                           logDirectory, logFileName, rollSize);
    setDefaultLogger(std::move(logger));
}

void initializeSafe(const std::string& logDirectory,
                    const std::string& logFileName,
                    uint32_t rollSize)
{
    auto logger = std::make_shared<Logger>("default",
                                           logDirectory, logFileName, rollSize);
    setDefaultLogger(std::move(logger));
}

// ---- ZephyrLog routing (default logger) ----
bool ZephyrLog::operator==(LogLine& logline)
{
    auto logger = std::atomic_load(&g_defaultLogger);
    if (logger)
        logger->add(std::move(logline));
    return true;
}

// ---- Named logger registry ----
std::shared_ptr<Logger> createLogger(const std::string& name,
                                     uint32_t bufferSize,
                                     const std::string& logDirectory,
                                     const std::string& logFileName,
                                     uint32_t rollSize)
{
    auto logger = std::make_shared<Logger>(name, bufferSize,
                                           logDirectory, logFileName, rollSize);
    std::unique_lock lock(g_registryMutex);
    g_registry[name] = logger;
    return logger;
}

std::shared_ptr<Logger> createSafeLogger(const std::string& name,
                                         const std::string& logDirectory,
                                         const std::string& logFileName,
                                         uint32_t rollSize)
{
    auto logger = std::make_shared<Logger>(name,
                                           logDirectory, logFileName, rollSize);
    std::unique_lock lock(g_registryMutex);
    g_registry[name] = logger;
    return logger;
}

std::shared_ptr<Logger> getLogger(const std::string& name)
{
    std::shared_lock lock(g_registryMutex);
    auto it = g_registry.find(name);
    return it != g_registry.end() ? it->second : nullptr;
}

void setDefaultLogger(std::shared_ptr<Logger> logger)
{
    if (!logger)
        return;
    // Register in the map
    {
        std::unique_lock lock(g_registryMutex);
        g_registry[logger->name()] = logger;
    }
    // Atomic swap: any thread that already loaded the old shared_ptr
    // keeps it alive until it finishes its add() call.
    std::atomic_store(&g_defaultLogger, std::move(logger));
}

std::shared_ptr<Logger> getDefaultLogger()
{
    return std::atomic_load(&g_defaultLogger);
}

void removeLogger(const std::string& name)
{
    std::unique_lock lock(g_registryMutex);
    auto it = g_registry.find(name);
    if (it != g_registry.end())
    {
        // If removing the default logger, clear it
        auto def = std::atomic_load(&g_defaultLogger);
        if (def && def->name() == name)
            std::atomic_store(&g_defaultLogger, std::shared_ptr<Logger>{});
        g_registry.erase(it);
    }
}

} // namespace zephyrlog
