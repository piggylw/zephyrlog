#include "zephyrlog/logline.h"
#include <algorithm>
#include <cstring>
#include <chrono>
#include <ostream>
#include <thread>
#include <ctime>

namespace zephyrlog
{

// 辅助函数
static uint64_t now()
{
    return std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

static void formatTimestamp(std::ostream& os, uint64_t timestamp)
{
    std::time_t time_t = timestamp / 1000000;
    std::tm tm_buf;
    localtime_r(&time_t, &tm_buf);
    char buffer[32];
    strftime(buffer, 32, "%Y-%m-%d %T.", &tm_buf);
    char microseconds[7];
    snprintf(microseconds, sizeof(microseconds), "%06llu",
             (unsigned long long)(timestamp % 1000000));
    os << '[' << buffer << microseconds << ']';
}

static std::thread::id thisThreadId()
{
    static thread_local const std::thread::id id = std::this_thread::get_id();
    return id;
}

const char* toString(LogLevel level)
{
    switch (level)
    {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO ";
        case LogLevel::WARN:  return "WARN ";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::CRITICAL:  return "CRIT ";
    }
    return "UNKN ";
}

// ---------- LogLine ----------
LogLine::LogLine(LogLevel level, const char* file, const char* function, uint32_t line)
    : m_bytesUsed(0),
      m_bufferSize(STACK_BUFFER_SIZE)
{
    *reinterpret_cast<uint64_t*>(buffer()) = now();
    m_bytesUsed += sizeof(uint64_t);
    *reinterpret_cast<std::thread::id*>(buffer()) = thisThreadId();
    m_bytesUsed += sizeof(std::thread::id);
    *reinterpret_cast<string_literal_t*>(buffer()) = string_literal_t(file);
    m_bytesUsed += sizeof(string_literal_t);
    *reinterpret_cast<string_literal_t*>(buffer()) = string_literal_t(function);
    m_bytesUsed += sizeof(string_literal_t);
    *reinterpret_cast<uint32_t*>(buffer()) = line;
    m_bytesUsed += sizeof(uint32_t);
    *reinterpret_cast<LogLevel*>(buffer()) = level;
    m_bytesUsed += sizeof(LogLevel);
}

LogLine::~LogLine() = default;

char* LogLine::buffer()
{
    return !m_heapBuffer ? &m_stackBuffer[m_bytesUsed] : &m_heapBuffer[m_bytesUsed];
}

void LogLine::resizeBufferIfNeeded(size_t additionalBytes)
{
    size_t required = m_bytesUsed + additionalBytes;
    if (required <= m_bufferSize)
        return;

    if (!m_heapBuffer)
    {
        m_bufferSize = std::max<size_t>(512, required);
        m_heapBuffer = std::make_unique<char[]>(m_bufferSize);
        std::memcpy(m_heapBuffer.get(), m_stackBuffer, m_bytesUsed);
    }
    else
    {
        m_bufferSize = std::max<size_t>(2 * m_bufferSize, required);
        auto newHeapBuffer = std::make_unique<char[]>(m_bufferSize);
        std::memcpy(newHeapBuffer.get(), m_heapBuffer.get(), m_bytesUsed);
        m_heapBuffer.swap(newHeapBuffer);
    }
}

void LogLine::encodeChar(char arg)
{
    resizeBufferIfNeeded(sizeof(uint8_t) + sizeof(char));
    *reinterpret_cast<uint8_t*>(buffer()) = static_cast<uint8_t>(ArgType::CHAR);
    m_bytesUsed += sizeof(uint8_t);
    *reinterpret_cast<char*>(buffer()) = arg;
    m_bytesUsed += sizeof(char);
}

void LogLine::encodeInt32(int32_t arg)
{
    resizeBufferIfNeeded(sizeof(uint8_t) + sizeof(int32_t));
    *reinterpret_cast<uint8_t*>(buffer()) = static_cast<uint8_t>(ArgType::INT32);
    m_bytesUsed += sizeof(uint8_t);
    *reinterpret_cast<int32_t*>(buffer()) = arg;
    m_bytesUsed += sizeof(int32_t);
}

void LogLine::encodeUint32(uint32_t arg)
{
    resizeBufferIfNeeded(sizeof(uint8_t) + sizeof(uint32_t));
    *reinterpret_cast<uint8_t*>(buffer()) = static_cast<uint8_t>(ArgType::UINT32);
    m_bytesUsed += sizeof(uint8_t);
    *reinterpret_cast<uint32_t*>(buffer()) = arg;
    m_bytesUsed += sizeof(uint32_t);
}

void LogLine::encodeInt64(int64_t arg)
{
    resizeBufferIfNeeded(sizeof(uint8_t) + sizeof(int64_t));
    *reinterpret_cast<uint8_t*>(buffer()) = static_cast<uint8_t>(ArgType::INT64);
    m_bytesUsed += sizeof(uint8_t);
    *reinterpret_cast<int64_t*>(buffer()) = arg;
    m_bytesUsed += sizeof(int64_t);
}

void LogLine::encodeUint64(uint64_t arg)
{
    resizeBufferIfNeeded(sizeof(uint8_t) + sizeof(uint64_t));
    *reinterpret_cast<uint8_t*>(buffer()) = static_cast<uint8_t>(ArgType::UINT64);
    m_bytesUsed += sizeof(uint8_t);
    *reinterpret_cast<uint64_t*>(buffer()) = arg;
    m_bytesUsed += sizeof(uint64_t);
}

void LogLine::encodeDouble(double arg)
{
    resizeBufferIfNeeded(sizeof(uint8_t) + sizeof(double));
    *reinterpret_cast<uint8_t*>(buffer()) = static_cast<uint8_t>(ArgType::DOUBLE);
    m_bytesUsed += sizeof(uint8_t);
    *reinterpret_cast<double*>(buffer()) = arg;
    m_bytesUsed += sizeof(double);
}

void LogLine::encodeStringLiteral(const char* s)
{
    resizeBufferIfNeeded(sizeof(uint8_t) + sizeof(string_literal_t));
    *reinterpret_cast<uint8_t*>(buffer()) = static_cast<uint8_t>(ArgType::STRING_LITERAL);
    m_bytesUsed += sizeof(uint8_t);
    *reinterpret_cast<string_literal_t*>(buffer()) = string_literal_t(s);
    m_bytesUsed += sizeof(string_literal_t);
}

void LogLine::encodeCString(const char* s, size_t len)
{
    resizeBufferIfNeeded(sizeof(uint8_t) + sizeof(size_t) + len);
    *reinterpret_cast<uint8_t*>(buffer()) = static_cast<uint8_t>(ArgType::CSTRING);
    m_bytesUsed += sizeof(uint8_t);
    *reinterpret_cast<size_t*>(buffer()) = len;
    m_bytesUsed += sizeof(size_t);
    if (len > 0)
    {
        std::memcpy(buffer(), s, len);
        m_bytesUsed += len;
    }
}

void LogLine::encodeCString(const char* s)
{
    encodeCString(s, s ? std::strlen(s) : 0);
}

LogLine& LogLine::operator<<(char arg)
{
    encodeChar(arg);
    return *this;
}

LogLine& LogLine::operator<<(int32_t arg)
{
    encodeInt32(arg);
    return *this;
}

LogLine& LogLine::operator<<(uint32_t arg)
{
    encodeUint32(arg);
    return *this;
}

LogLine& LogLine::operator<<(int64_t arg)
{
    encodeInt64(arg);
    return *this;
}

LogLine& LogLine::operator<<(uint64_t arg)
{
    encodeUint64(arg);
    return *this;
}

LogLine& LogLine::operator<<(double arg)
{
    encodeDouble(arg);
    return *this;
}

LogLine& LogLine::operator<<(const std::string& arg)
{
    encodeCString(arg.c_str(), arg.size());
    return *this;
}

void LogLine::stringify(std::ostream& os)
{
    char* b = !m_heapBuffer ? m_stackBuffer : m_heapBuffer.get();
    const char* end = b + m_bytesUsed;

    uint64_t timestamp = *reinterpret_cast<uint64_t*>(b);
    b += sizeof(uint64_t);
    std::thread::id threadid = *reinterpret_cast<std::thread::id*>(b);
    b += sizeof(std::thread::id);
    string_literal_t file = *reinterpret_cast<string_literal_t*>(b);
    b += sizeof(string_literal_t);
    string_literal_t function = *reinterpret_cast<string_literal_t*>(b);
    b += sizeof(string_literal_t);
    uint32_t line = *reinterpret_cast<uint32_t*>(b);
    b += sizeof(uint32_t);
    LogLevel loglevel = *reinterpret_cast<LogLevel*>(b);
    b += sizeof(LogLevel);

    formatTimestamp(os, timestamp);
    os << '[' << toString(loglevel) << ']'
       << '[' << threadid << ']'
       << '[' << file.m_s << ':' << function.m_s << ':' << line << "] ";

    while (b < end)
    {
        ArgType type = static_cast<ArgType>(*b);
        ++b;

        switch (type)
        {
            case ArgType::CHAR:
            {
                char val = *reinterpret_cast<char*>(b);
                os << val;
                b += sizeof(char);
                break;
            }
            case ArgType::INT32:
            {
                int32_t val = *reinterpret_cast<int32_t*>(b);
                os << val;
                b += sizeof(int32_t);
                break;
            }
            case ArgType::UINT32:
            {
                uint32_t val = *reinterpret_cast<uint32_t*>(b);
                os << val;
                b += sizeof(uint32_t);
                break;
            }
            case ArgType::INT64:
            {
                int64_t val = *reinterpret_cast<int64_t*>(b);
                os << val;
                b += sizeof(int64_t);
                break;
            }
            case ArgType::UINT64:
            {
                uint64_t val = *reinterpret_cast<uint64_t*>(b);
                os << val;
                b += sizeof(uint64_t);
                break;
            }
            case ArgType::DOUBLE:
            {
                double val = *reinterpret_cast<double*>(b);
                os << val;
                b += sizeof(double);
                break;
            }
            case ArgType::STRING_LITERAL:
            {
                string_literal_t s = *reinterpret_cast<string_literal_t*>(b);
                os << s.m_s;
                b += sizeof(string_literal_t);
                break;
            }
            case ArgType::CSTRING:
            {
                size_t len = *reinterpret_cast<const size_t*>(b);
                b += sizeof(size_t);
                os.write(b, static_cast<std::streamsize>(len));
                b += len;
                break;
            }
            default:
                break;
        }
    }
    os << std::endl;
    if (loglevel >= LogLevel::CRITICAL)
        os.flush();
}

}
