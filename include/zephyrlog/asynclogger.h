#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <iosfwd>

namespace zephyrlog
{

enum class LogLevel : uint8_t
{
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3,
    CRITICAL = 4
};

const char* toString(LogLevel level);

class LogLine
{
public:
    LogLine(LogLevel level, const char* file, const char* function, uint32_t line);
    ~LogLine();

    LogLine(LogLine&&) = default;
    LogLine& operator=(LogLine&&) = default;

    LogLine(const LogLine&) = delete;
    LogLine& operator=(const LogLine&) = delete;

    //格式化输出到流
    void stringify(std::ostream& os);

    //重载
    LogLine& operator<<(char arg);
    LogLine& operator<<(int32_t arg);
    LogLine& operator<<(uint32_t arg);
    LogLine& operator<<(int64_t arg);
    LogLine& operator<<(uint64_t arg);
    LogLine& operator<<(double arg);
    LogLine& operator<<(const std::string& arg);
    LogLine& operator<<(const char* arg);
    LogLine& operator<<(char* arg);

    // 字符串字面量（编译期常量，不需要拷贝）
    template<size_t N>
    LogLine& operator<<(const char (&arg)[N])
    {
        encodeStringLiteral(arg);
        return *this;
    }

private:

    enum class ArgType : uint8_t
    {
        CHAR,
        UINT32,
        UINT64,
        INT32,
        INT64,
        DOUBLE,
        STRING_LITERAL,   // pointer to string literal
        CSTRING           // dynamic string (copied)
    };

    struct string_literal_t
    {
        explicit string_literal_t(const char* s) : m_s(s) {}
        const char* m_s;
    };

    //编码
    void encodeChar(char arg);
    void encodeInt32(int32_t arg);
    void encodeUint32(uint32_t arg);
    void encodeInt64(int64_t arg);
    void encodeUint64(uint64_t arg);
    void encodeDouble(double arg);
    void encodeStringLiteral(const char* s);
    void encodeCString(const char* s, size_t len);
    void encodeCString(const char* s);

    //获取当前写入位置
    char* buffer();
    //缓冲区管理
    void resizeBufferIfNeeded(size_t additionalBytes);

private:
    size_t m_bytesUsed;
    size_t m_bufferSize;
    std::unique_ptr<char[]> m_heapBuffer;
    char m_stackBuffer[256 - 2 * sizeof(size_t) - sizeof(decltype(m_heapBuffer))];
};

struct Logger
{
    bool operator==(LogLine& logline);
};

}

#define LOG(LEVEL) \
    zephyrlog::Logger() == zephyrlog::LogLine(LEVEL, __FILE__, __func__, __LINE__)

#define LOG_DEBUG   LOG(zephyrlog::LogLevel::DEBUG)
#define LOG_INFO    LOG(zephyrlog::LogLevel::INFO)
#define LOG_WARN    LOG(zephyrlog::LogLevel::WARN)
#define LOG_ERROR   LOG(zephyrlog::LogLevel::ERROR)
#define LOG_CRIT    LOG(zephyrlog::LogLevel::CRITICAL)