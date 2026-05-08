#include "zephyrlog/asynclogger.h"
#include "zephyrlog/ringbuffer.h"
#include "zephyrlog/queuebuffer.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <memory>
#include <thread>

namespace zephyrlog
{
    
AsyncLogger::AsyncLogger(uint32_t bufferSize,
                        const std::string& logDirectory, 
                        const std::string& logFileName, 
                        uint32_t rollSize)
    : m_state(State::INIT),
      m_buffer(std::make_unique<RingBuffer>(bufferSize)),
      m_writer(logDirectory, logFileName, rollSize),
      m_thread(&AsyncLogger::pop, this)
{
    m_state.store(State::READY, std::memory_order_release);
}

AsyncLogger::AsyncLogger(const std::string& logDirectory, 
                        const std::string& logFileName, 
                        uint32_t rollSize)
    : m_state(State::INIT),
      m_buffer(std::make_unique<QueueBuffer>()),
      m_writer(logDirectory, logFileName, rollSize),
      m_thread(&AsyncLogger::pop, this)
{
    m_state.store(State::READY, std::memory_order_release);
}

AsyncLogger::~AsyncLogger()
{
    m_state.store(State::SHUTDOWN);
    m_thread.join();
}

void AsyncLogger::add(LogLine&& logline)
{
    m_buffer->push(std::move(logline));
}

void AsyncLogger::pop()
{
    while(m_state.load(std::memory_order_acquire) == State::INIT)
    {
        std::this_thread::sleep_for(std::chrono::microseconds(50));
    }

    LogLine logline(LogLevel::INFO, nullptr, nullptr, 0);
    while(m_state.load() == State::READY)
    {
        if(m_buffer->tryPop(logline))
        {
            m_writer.write(logline);
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::microseconds(50));
        }
    }

    while (m_buffer->tryPop(logline))
    {
        m_writer.write(logline);
    }
}

}