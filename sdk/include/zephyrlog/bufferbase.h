#pragma once

namespace zephyrlog
{

class LogLine;
// ========== BufferBase：缓冲区接口 ==========
/*
* 抽象基类，定义缓冲区的统一接口
* RingBuffer
* QueueBuffer
*/
struct BufferBase
{
    virtual ~BufferBase() = default;
    virtual void push(LogLine&& logline) = 0;
    virtual bool tryPop(LogLine& logline) = 0;
};

}//zephyrlog