#ifndef __TIMER_H__
#define __TIMER_H__

#include <functional>
#include <chrono>    // C++11时间库
#include <queue>
#include <vector>
#include <iostream>
#include <cassert>
#include <mutex>

using TimeoutCBFunc = std::function<void()>;
using Clock = std::chrono::high_resolution_clock;
using MS = std::chrono::microseconds;
using Timestamp = Clock::time_point;

class HttpRequest;

class Timer {
public:
    Timer(const Timestamp& when, const TimeoutCBFunc& cb) 
        :m_expireTime(when),
        m_callBack(cb),
        m_delete(false) {}

    ~Timer() {

    }
    void del() {
        m_delete = true;
    }

    bool isDeleted() {
        return m_delete;
    }

    Timestamp getExpireTime() const {
        return m_expireTime;
    }

    void runCallBack() {
        m_callBack();
    }

private:
    Timestamp m_expireTime;    // 定时器生效的绝对时间
    TimeoutCBFunc m_callBack;  // 定时器回调函数
    bool m_delete;             // 标记定时器是否被删除
};

// 基于小顶堆的优先队列管理定时器事件

// timer类比较仿函数 或者重载 < 
struct cmp {
    bool operator() (const Timer* a, const Timer* b) {
        assert(a != nullptr && b != nullptr);
        return (a->getExpireTime() > b->getExpireTime());
    }
};

class TimeManager {
public:
    TimeManager(): m_now(Clock::now()) {}
    ~TimeManager() { }

    void updateTime() {
        m_now = Clock::now();    // 获取当前时间
    }

    void addTimer(HttpRequest* request, const int& timeout, const TimeoutCBFunc& cb);
    void delTimer(HttpRequest* request);
    void handleExpireTimers();  // 处理过期事件
    int getNextExpireTime();    // 获取超时时间

private:
    using TimerPriorityQueue = std::priority_queue<Timer*, std::vector<Timer*>, cmp>;

    TimerPriorityQueue m_timerQueue;
    Timestamp m_now;     // 当前时间点
    std::mutex m_mutex;  // 多线程互斥

};

#endif

// 定时器的作用：定期检测一个客户连接的活动状态。服务器需要管理众多的定时事件，因此有效地组织这些定时事件
// 使之能在预期的时间点被触发且不影响服务器的主要逻辑。将定时时间封装成定时器，使用时间轮、链表、排序链表串联管理
// 以实现对定时事件的统一管理。
// 实现小顶堆定时器