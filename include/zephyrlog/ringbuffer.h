#pragma once
#include "zephyrlog/bufferbase.h"
#include "zephyrlog/logline.h"
#include <atomic>
#include <cstddef>

namespace zephyrlog
{
// ========== RingBuffer：环形缓冲区（MPSC） ==========
/*
* Multi-Producer Single-Consumer Ring Buffer
* 
* 特点：
* - 固定大小，写满后覆盖旧数据（NonGuaranteed）
* - 多生产者：用 SpinLock 保护
* - 单消费者：简化设计，无锁读取
* - 256 字节对齐：避免伪共享
*/

class RingBuffer : public BufferBase
{
public:
    //缓冲区存储的结构
    struct alignas(64) Item
    {
        Item()
            : flag(ATOMIC_FLAG_INIT),
              written(0),
              logline(LogLevel::INFO, nullptr, nullptr, 0)
        {

        }
        std::atomic_flag flag;
        char written;
        char padding[256 - sizeof(std::atomic_flag) - sizeof(char) - sizeof(LogLine)];
        LogLine logline;
    };

    explicit RingBuffer(size_t size);
    ~RingBuffer();

    void push(LogLine&& logline) override;
    bool tryPop(LogLine& logline) override;

    // 禁用拷贝
    RingBuffer(const RingBuffer&) = delete;
    RingBuffer& operator=(const RingBuffer&) = delete;

private:
    size_t m_size;                          // 环形缓冲区大小
    Item* m_ring;                           // Item 数组
    std::atomic<unsigned int> m_writeIndex; // 写索引（原子）
    char pad[64];                           // 填充（避免 false sharing）
    unsigned int m_readIndex;              // 读索引（仅消费者访问）
};


}//zephyrlog