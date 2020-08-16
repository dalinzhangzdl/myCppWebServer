#ifndef __TIMER_H__
#define __TIMER_H__

#include <functional>
#include <chrono>
#include <queue>
#include <vector>
#include <iostream>
#include <cassert>
#include <mutex>


using TimeoutCallBack = std::function<void()>;
using Clock = std::chrono::high_resolution_clock;
using MS = std::chrono::milliseconds;
using Timestamp = Clock::time_point;

class HttpRequest;

class Timer {
public:
    Timer(const Timestamp& when, const TimeoutCallBack& cb)
        : m_expireTime(when),
          m_callBack(cb),
          m_delete(false) {}
    ~Timer() {}

    // 惰性删除
    void del() { 
        m_delete = true; 
    }

    bool isDeleted() { 
        return m_delete; 
    }

    // 过期时间
    Timestamp getExpireTime() const { 
        return m_expireTime; 
    }
    // 过期回调
    void runCallBack() { 
        m_callBack(); 
    }
    /*
    bool operator < (const Timer& other) {
        return (m_expireTime > other.getExpireTime());
    }
    */

private:
    Timestamp m_expireTime;
    TimeoutCallBack m_callBack;
    bool m_delete;

}; 


// 比较函数，用于priority_queue，时间值最小的在队头
struct cmp {
    bool operator()(Timer* a, Timer* b)
    {
        assert(a != nullptr && b != nullptr);
        return (a -> getExpireTime()) > (b -> getExpireTime());
    }
};

class TimerManager {
public:
    TimerManager() 
        : m_now(Clock::now()) {}

    ~TimerManager() {}

    void updateTime() { m_now = Clock::now(); }

    void addTimer(HttpRequest* request, const int& timeout, const TimeoutCallBack& cb); // timeout单位ms
    void delTimer(HttpRequest* request);
    void handleExpireTimers();
    int getNextExpireTime(); // 返回超时时间(优先队列中最早超时时间和当前时间差)

private:
    using TimerQueue = std::priority_queue<Timer*, std::vector<Timer*>, cmp>;

    TimerQueue m_timerQueue;
    Timestamp m_now;
    std::mutex m_mutex;
}; // class TimerManager



#endif
