#include "ThreadPool.h"

#include <iostream>
#include <cassert>


ThreadPool::ThreadPool(int numWorkers)
    : m_stop(false)
{
    numWorkers = numWorkers <= 0 ? 1 : numWorkers;
    for(int i = 0; i < numWorkers; ++i) {
        m_threads.emplace_back(std::bind(&ThreadPool::runThread, this));
    }
}


void ThreadPool::runThread() {

    while(1) {
        TaskFunction task;
        {
            std::unique_lock<std::mutex> locker(m_mutex);
            while(!m_stop && m_task.empty())
                m_cond.wait(locker);
            if (m_task.empty() && m_stop)
                return;
            task = m_task.front();
            m_task.pop_front();
        }

        // 执行任务
        if (task) {
            task();
        }
    }
}

void ThreadPool::stopThread() {
    
    m_stop = true;
    m_cond.notify_all();
    for(auto& thread : m_threads) {
        thread.join();
    }
    m_threads.clear();
    printf("~ThreadPool success!\n");
}

ThreadPool::~ThreadPool() {
    // 多线程析构安全
    std::call_once(m_flag, [this]{stopThread();});
}

/*
ThreadPool::~ThreadPool()
{
    //{
       // std::unique_lock<std::mutex> locker(m_mutex);
        m_stop = true;
    //} 
    m_cond.notify_all();
    for(auto& thread: m_threads)
        thread.join();
    m_threads.clear();
}
*/

void ThreadPool::addTask(const TaskFunction& task)
{
    {
        std::unique_lock<std::mutex> locker(m_mutex);
        m_task.push_back(task);    // emplace_back 减少内存拷贝和移动
    }
    // printf("push new job\n");
    m_cond.notify_one();
}

void ThreadPool::addTask(TaskFunction&& task) {
	{
		std::unique_lock<std::mutex> locker(m_mutex);
		m_task.push_back(std::forward<TaskFunction>(task));
	}
	m_cond.notify_one();
}


/*
// test
void testThreadPool() {
    ThreadPool pool;
    std::thread thd1([&pool] {
        for(int i = 0; i < 10; i++) {
            auto id = std::this_thread::get_id();
            pool.addTask([id] {
                std::cout << "TestThread1: " << id << std::endl;
            });
        }
    });

    std::thread thd2([&pool] {
        for(int i = 0; i < 10; i++) {
            auto id = std::this_thread::get_id();
            pool.addTask([id] {
                std::cout << "TestThread2: " << id << std::endl;
            });
        }
    });

    std::this_thread::sleep_for(std::chrono::seconds(2));
    pool.stopThread();
    thd1.join();
    thd2.join();
}
*/

