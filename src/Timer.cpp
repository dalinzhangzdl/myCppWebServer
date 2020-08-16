#include "Timer.h"
#include "HttpRequest.h"

#include <cassert>


void TimerManager::addTimer(HttpRequest* request, 
                     const int& timeout, 
                     const TimeoutCallBack& cb)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    assert(request != nullptr);

    updateTime();
    Timer* timer = new Timer(m_now + MS(timeout), cb);
    m_timerQueue.push(timer);

    // 对同一个request连续调用两次addTimer，需要把前一个定时器删除
    if(request -> getTimer() != nullptr)
        delTimer(request);

    request -> setTimer(timer);
}

// 这个函数不必上锁，没有线程安全问题
void TimerManager::delTimer(HttpRequest* request)
{
    assert(request != nullptr);

    Timer* timer = request -> getTimer();
    if(timer == nullptr)
        return;

    // 如果这里写成delete timeNode，会使priority_queue里的对应指针变成垂悬指针
    // 正确的方法是惰性删除
    timer->del();
    // 防止request -> getTimer()访问到垂悬指针
    request -> setTimer(nullptr);
}

void TimerManager::handleExpireTimers()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    updateTime();
    while(!m_timerQueue.empty()) {
        Timer* timer = m_timerQueue.top();
        assert(timer != nullptr);
        // 定时器被删除
        if(timer -> isDeleted()) {
            m_timerQueue.pop();
            delete timer;
            continue;
        }
        // 判断小顶堆堆顶定时器是否超时
        if(std::chrono::duration_cast<MS>(timer -> getExpireTime() - m_now).count() > 0) {
            // std::cout << "[TimerManager::handleExpireTimers] there is no timeout timer" << std::endl;
            return;
        }
        // 超时
        timer -> runCallBack();
        m_timerQueue.pop();
        delete timer;   // 注意这个delete 段错误的源头
    }
}

// 获取下一个超时时间 用于epoll_wait的超时时间
int TimerManager::getNextExpireTime()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    updateTime();
    int res = -1;
    while(!m_timerQueue.empty()) {
        Timer* timer = m_timerQueue.top();
        if(timer-> isDeleted()) {
            m_timerQueue.pop();
            delete timer;
            continue;
        }
        res = std::chrono::duration_cast<MS>(timer -> getExpireTime() - m_now).count();
        res = (res < 0) ? 0 : res;
        break;
    }
    return res;
}
