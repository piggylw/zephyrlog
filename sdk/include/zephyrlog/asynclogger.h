#pragma once
#include "zephyrlog/bufferbase.h"
#include "zephyrlog/logline.h"
#include "zephyrlog/ringbuffer.h"
#include "zephyrlog/filewriter.h"
#include <atomic>
#include <cstdint>
#include <memory>
#include <thread>

namespace zephyrlog
{
// ========== 异步日志器 ==========
/*
* 
* 架构：
*   用户线程 → push(LogLine) → RingBuffer
*                                  ↓
*   后台线程 ← tryPop(LogLine) ← RingBuffer
*       ↓
*   FileWriter::write() → stdout
*/
class AsyncLogger
{
public:
    explicit AsyncLogger(uint32_t bufferSize,
                        const std::string& logDirectory,
                        const std::string& logFileName,
                        uint32_t rollSize = 10 * 1024 * 1024,
                        bool terminalOutput = false);
    AsyncLogger(const std::string& logDirectory,
                const std::string& logFileName,
                uint32_t rollSize = 10 * 1024 * 1024,
                bool terminalOutput = false);
    ~AsyncLogger();
    void add(LogLine&& logline);

private:
    enum class State
    {
        INIT,
        READY,
        SHUTDOWN
    };

    void pop();

private:
    std::atomic<State> m_state;
    std::unique_ptr<BufferBase> m_buffer;
    FileWriter m_writer;
    std::thread m_thread;
};

}