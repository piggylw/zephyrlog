#include "zephyrlog/ringbuffer.h"
#include "zephyrlog/spinlock.h"
#include <cstddef>
#include <cstdlib>

namespace zephyrlog
{

RingBuffer::RingBuffer(size_t size)
    : m_size(size),
      m_ring(static_cast<Item*>(std::malloc(size * sizeof(Item)))),
      m_writeIndex(0),
      m_readIndex(0)
{
    for(size_t i = 0; i < size; ++i)
    {
        new (&m_ring[i]) Item();
    }
    static_assert(sizeof(Item) == 256, "Item size must be 256 bytes");
}

RingBuffer::~RingBuffer()
{
    // 手动调用析构函数
    for (size_t i = 0; i < m_size; ++i)
    {
        m_ring[i].~Item();
    }
    std::free(m_ring);
}

void RingBuffer::push(LogLine&& logline)
{
    /*
    * 多生产者写入流程：
    * 
    * 1. 原子地获取写索引（fetch_add 返回旧值）
    * 2. 取模得到实际位置
    * 3. 加锁（SpinLock）
    * 4. 移动日志行到 Item
    * 5. 标记已写入
    * 6. 解锁（SpinLock 析构）
    */
    unsigned int index = m_writeIndex.fetch_add(1, std::memory_order_relaxed) % m_size;
    Item& item = m_ring[index];
    //自旋锁
    SpinLock lock(item.flag);
    item.logline = std::move(logline);
    item.written = 1;
}

bool RingBuffer::tryPop(LogLine& logline)
{
    /*
    * 单消费者读取流程：
    * 
    * 1. 计算读索引位置
    * 2. 加锁（与生产者互斥）
    * 3. 检查是否已写入
    * 4. 如果已写入：移动数据，清除标记，递增索引
    * 5. 解锁
    */
    Item& item = m_ring[m_readIndex % m_size];
    //自旋锁
    SpinLock lock(item.flag);
    if(item.written == 1)
    {
        logline = std::move(item.logline);
        item.written = 0;
        ++m_readIndex;
        return true;
    }
    return false;
}

}// zephyrlog