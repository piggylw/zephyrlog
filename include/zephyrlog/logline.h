#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <iosfwd>
#include <type_traits>

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

    //获取缓冲区信息
    size_t bufferUsedSize() const { return m_bytesUsed; }
    size_t bufferSize() const { return m_bufferSize; }
    bool isHeapAllocated() const { return m_heapBuffer != nullptr; }

    //重载
    LogLine& operator<<(char arg);
    LogLine& operator<<(int32_t arg);
    LogLine& operator<<(uint32_t arg);
    LogLine& operator<<(int64_t arg);
    LogLine& operator<<(uint64_t arg);
    LogLine& operator<<(double arg);
    LogLine& operator<<(const std::string& arg);

    //char* ,const char*, 字符串字面量 const char[N]
    template <typename T>
    auto operator<<(T&& arg) -> std::enable_if_t<
        std::is_same_v<std::remove_reference_t<T>, char*> ||
        std::is_same_v<std::remove_reference_t<T>, const char*> ||
        std::is_array_v<std::remove_reference_t<T>>,
        LogLine&>
    {
        if constexpr (std::is_array_v<std::remove_reference_t<T>>)
        {
            // 字符串字面量 const char[N]
            encodeStringLiteral(arg);
        }
        else
        {
            // 运行时 const char* / char*
            encodeCString(arg);
        }
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
    // 栈缓冲区大小计算：
    // 目标：256 字节对齐
    // 已用：sizeof(size_t)*2 + sizeof(unique_ptr) + 预留(8) = 32
    // 剩余：256 - 32 = 224
    static constexpr size_t STACK_BUFFER_SIZE = 224;
    char m_stackBuffer[STACK_BUFFER_SIZE];
};

}
