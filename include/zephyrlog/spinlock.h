#pragma once
#include <atomic>

namespace zephyrlog
{
// ========== SpinLock：自旋锁 ==========
/*
    * RAII 风格的自旋锁
    * 基于 std::atomic_flag（最轻量的原子类型）
    */

class SpinLock
{
public:
    explicit SpinLock(std::atomic_flag& flag)
        : m_flag(flag)
    {
        // test_and_set：原子地设置为 true 并返回旧值
        // 如果旧值是 true（已被占用），继续自旋
        while(m_flag.test_and_set(std::memory_order_acquire))
        {

        }
    }

    ~SpinLock()
    {
        // 释放锁
        m_flag.clear(std::memory_order_release);
    }

    // 禁用拷贝和移动
    SpinLock(const SpinLock&) = delete;
    SpinLock& operator=(const SpinLock&) = delete;

private:
    std::atomic_flag& m_flag;
};

}//zephyrlog