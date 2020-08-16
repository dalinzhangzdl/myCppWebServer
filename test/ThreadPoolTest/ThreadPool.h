#ifndef __THREAD_POOL__
#define __THREAD_POOL__

// 半同步半异步线程池
// 服务层接受并发的服务请求
// 同步队列缓冲任务队列   改进循环缓冲队列 无限缓存炸掉
// 工作线程处理任务队列

#include <vector>
#include <list>
#include <thread>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <atomic> 
#include <memory>



class ThreadPool {
public:
    using TaskFunction = std::function<void()>;   // task

    ThreadPool(int numWorkers);    // 线程池初始化即运行态
    ~ThreadPool();                 // 多线程环境下只析构一次 
    void addTask(const TaskFunction& task);   // 任务队列添加人物
    void stopThread();
 
private:
    std::vector<std::thread> m_threads;   // 线程组
    std::mutex m_mutex;                   // 互斥锁  使用时使用RAII 手法使用
    std::condition_variable m_cond;       // 条件变量 阻塞任务队列为空
    std::list<TaskFunction> m_task;       // 任务队列
    // bool m_stop;
    std::atomic_bool m_stop;              // 
    std::once_flag m_flag;                // 保证多线程环境下只调用一次
};



#endif
