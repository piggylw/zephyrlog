#include "zephyrlog/queuebuffer.h"
#include "zephyrlog/spinlock.h"
#include <atomic>
#include <cstddef>
#include <cstdlib>
#include <functional>
#include <memory>
#include <utility>

namespace zephyrlog
{

Buffer::Buffer()
    : m_buffer(static_cast<Item*>(std::malloc(size * sizeof(Item))))
{
    for (size_t i = 0; i <= size; ++i)
    {
        m_writeState[i].store(0, std::memory_order_relaxed);
    }
    static_assert(sizeof(Item) == 256, "Item size must be 256 bytes");
}

Buffer::~Buffer()
{
    unsigned int writeNum = m_writeState[size].load();
    for (size_t i = 0; i < writeNum; ++i)
    {
        m_buffer[i].~Item();
    }
    std::free(m_buffer);
}

bool Buffer::push(LogLine&& logline, unsigned int writeIndex)
{
    new (&m_buffer[writeIndex]) Item(std::move(logline));
    m_writeState[writeIndex].store(1, std::memory_order_release);
    return m_writeState[size].fetch_add(1) + 1 == size;
}

bool Buffer::tryPop(LogLine& logline, unsigned int readIndex)
{
    if(m_writeState[readIndex].load(std::memory_order_acquire) == 1)
    {
        Item& item = m_buffer[readIndex];
        logline = std::move(item.logline);
        return true;
    }
    return false;
}

QueueBuffer::QueueBuffer()
    : m_currentWriteBuffer{nullptr}
    , m_currentReadBuffer{nullptr}
    , m_writeIndex(0)
    , m_flag{ATOMIC_FLAG_INIT}
    , m_readIndex(0)
{
    setupNextWriteBuffer();
}

void QueueBuffer::setupNextWriteBuffer()
{
    SpinLock lock(m_flag);
    std::unique_ptr<Buffer> next = std::make_unique<Buffer>();
    m_currentWriteBuffer.store(next.get());
    m_buffers.push(std::move(next));
    m_writeIndex.store(0);
}

Buffer* QueueBuffer::getNextReadBuffer()
{
    SpinLock lock(m_flag);
    return m_buffers.empty() ? nullptr : m_buffers.front().get();
}

void QueueBuffer::push(LogLine&& logline)
{
    unsigned int writeIndex = m_writeIndex.fetch_add(1, std::memory_order_relaxed);
        
        if (writeIndex < Buffer::size)
        {
            if (m_currentWriteBuffer.load(std::memory_order_acquire)->push(std::move(logline), writeIndex))
            {
                setupNextWriteBuffer();
            }
        }
        else
        {
            while (m_writeIndex.load(std::memory_order_acquire) >= Buffer::size)
            {
                // 自旋等待（空循环）
            }
            push(std::move(logline));
        }
}

bool QueueBuffer::tryPop(LogLine& logline)
{
    if(m_currentReadBuffer == nullptr)
    {
        m_currentReadBuffer = getNextReadBuffer();
    }

    Buffer* readBuffer = m_currentReadBuffer;
    if(readBuffer == nullptr)
        return false;

    if(readBuffer->tryPop(logline, m_readIndex))
    {
        m_readIndex++;
        if(m_readIndex == Buffer::size)
        {
            m_readIndex = 0;
            m_currentReadBuffer = nullptr;
            SpinLock lock(m_flag);
            m_buffers.pop();
        }
        return true;
    }
    return false;
}

}