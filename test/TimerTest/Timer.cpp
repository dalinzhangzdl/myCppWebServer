#include "Timer.h"
#include "HttpRequest.h"

#include <cassert>

void TimeManager::addTimer(HttpRequest* request, const int& timeout, const TimeoutCBFunc& cb) {
    // 注册http request 的过期事件
    std::unique_lock<std::mutex> lock(m_mutex);
    assert(request != nullptr);

    updateTime();  // 获取当前时间
    Timer* timer = new Timer(m_now + MS(timeout), cb);      // 为当前request 事件 添加定时事件和到期时间
    m_timerQueue.push(timer);

    // 对同一个request连续调用两次addTimer, 需要删除前一个定时器
    if (request->getTimer() != nullptr)
        delTimer(request);

    request->setTimer(timer);   // 设置定时器
}

// 不需加锁
void TimeManager::delTimer(HttpRequest* request) {

    assert(request != nullptr);

    Timer* timer = request->getTimer();
    if (timer == nullptr)
        return;
    // 不可以从队列中直接删除 timer 会导致对应的timer指针变成悬垂指针
    // 正确方法采用惰性删除
    timer->del();
    // 指针置空
    request->setTimer(nullptr);
}

// 定时器心博   处理过期事件
void TimeManager::handleExpireTimers() {
    std::unique_lock<std::mutex> lock(m_mutex);
    updateTime();
    while(!m_timerQueue.empty()) {
        Timer* timer = m_timerQueue.top();
        assert(timer != nullptr) ;
        if (timer->isDeleted()) {
            // 定时器被标记删除
            std::cout << "[TimerManager::handleExpireTimers] timer = " << Clock::to_time_t(timer->getExpireTime()) << "is deleted" << std::endl;
            m_timerQueue.pop();
            delete timer;
            continue;
        }

        // 判断小顶堆堆顶定时器是否超时 
        if (std::chrono::duration_cast<MS>(timer->getExpireTime() - m_now).count() > 0) {
            std::cout << "[TimerManager::handleExpireTimers] there is no timeout timer" << std::endl;
            return;
        }
        // 超时  执行超时回调函数  删除定时器
       // std::cout << "[TimerManager::handleExpireTimers] exist timeout" << std::endl;
        timer->runCallBack();
        m_timerQueue.pop();

        // timer = nullptr;
        delete timer;
    }
}

// 获取下一次超时时间
int TimeManager::getNextExpireTime() {
    std::unique_lock<std::mutex> lock(m_mutex);
    updateTime();   // 更新m_now
    int res = -1;
    while(!m_timerQueue.empty()) {
        Timer* timer = m_timerQueue.top();
        assert(timer != nullptr);
        if (timer->isDeleted()) {
            // 惰性删除 被标记删除的定时器
            m_timerQueue.pop();
            delete timer;
            continue;
        }
        // 获取下一个超时的时间
        res = std::chrono::duration_cast<MS>(timer->getExpireTime() - m_now).count();   
        res = (res < 0) ? 0 : res;
        break;
    }
    return res;
}