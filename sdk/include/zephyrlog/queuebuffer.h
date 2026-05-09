#pragma once
#include "zephyrlog/bufferbase.h"
#include "zephyrlog/spinlock.h"
#include "zephyrlog/logline.h"
#include <atomic>
#include <cstddef>
#include <queue>

namespace zephyrlog
{

// ========== Buffer：QueueBuffer 的基本单元 ==========
   /*
    * 单个 Buffer：
    * - 固定大小：32768 个 Item（8MB）
    * - 原子状态数组：标记每个 Item 是否已写入
    * - 生命周期：创建 → 写满 → 读完 → 释放
    */   
class Buffer
{
public:
    struct alignas(64) Item
    {
        Item(LogLine&& line)
            : logline(std::move(line))
        {}

        char padding[256 - sizeof(LogLine)];
        LogLine logline;
    };

    // 每个 Buffer 容纳 32768 个 Item（8MB）
    static constexpr const size_t size = 32768;

    Buffer();
    ~Buffer();
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    bool push(LogLine&& logline, unsigned int writeIndex);
    bool tryPop(LogLine& logline, unsigned int readIndex);

private:
    Item* m_buffer;
        /*
         * 写入状态数组：
         * m_write_state[i] = 0: 未写入
         * m_write_state[i] = 1: 已写入
         * m_write_state[size] = 当前写入计数（用于判断是否写满）
         */
    std::atomic<unsigned int> m_writeState[size + 1];
};

// ========== QueueBuffer：多 Buffer 队列 ==========
    /*
     * GuaranteedLogger 的核心：
     * - 动态 Buffer 队列：写满后创建新 Buffer
     * - 读写分离：当前写入和当前读取在不同 Buffer
     * - 零丢失：内存不足前不会丢失日志
     */
class QueueBuffer : public BufferBase
{
public:
    QueueBuffer();
    ~QueueBuffer() = default;

    void push(LogLine&& logline) override;
    bool tryPop(LogLine& logline) override;

    QueueBuffer(const QueueBuffer&) = delete;
    QueueBuffer& operator=(const QueueBuffer&) = delete;

private:
    void setupNextWriteBuffer();
    Buffer* getNextReadBuffer();

private:
    std::queue<std::unique_ptr<Buffer>> m_buffers;  
    std::atomic<Buffer*> m_currentWriteBuffer;
    Buffer* m_currentReadBuffer; 
    std::atomic<unsigned int> m_writeIndex; 
    std::atomic_flag m_flag;
    unsigned int m_readIndex;
};

}