#include "Epoll.h"
#include "ThreadPool.h"
#include "HttpRequest.h"

#include <iostream>
#include <cassert>
#include <cstring>

#include <unistd.h>



Epoll::Epoll() 
       : m_epollFd(epoll_create1(EPOLL_CLOEXEC)),    // EPOLL_CLOEXEC 表示进程被替换时会关闭文件描述符
       m_events(MAXEVENTS)
{
    assert(m_epollFd >= 0);
}

Epoll::~Epoll() {
    close(m_epollFd);      // ?
}

int Epoll::add(int fd, HttpRequest* request, int events) {
    struct epoll_event event;
    event.data.ptr = static_cast<void*> (request);
    event.events = events;
    int ret = epoll_ctl(m_epollFd, EPOLL_CTL_ADD, fd, &event);
   // printf("[Epoll::add] fd add to epoll_core finish\n");
    return ret;
}

int Epoll::mod(int fd, HttpRequest* request, int events) {
    struct epoll_event event;
    event.data.ptr = static_cast<void*> (request);
    event.events = events;
    int ret = epoll_ctl(m_epollFd, EPOLL_CTL_MOD, fd, &event);
    return ret;
}

int Epoll::del(int fd, HttpRequest* request, int events) {
    struct epoll_event event;
    event.data.ptr = static_cast<void*> (request);
    event.events = events;
    int ret = epoll_ctl(m_epollFd, EPOLL_CTL_DEL, fd, &event);
    return ret;
}

int Epoll::wait(int timeoutMs) {
    int ret = epoll_wait(m_epollFd, &*m_events.begin(), static_cast<int>(m_events.size()), timeoutMs);
    if (ret == 0){
        printf("[Epoll::wait] nothing happen, epoll timeout");
    } else if (ret < 0) {
        printf("[Epoll::wait] epoll: %s\n", strerror(errno));
    }
    return ret;
}

// oneEventLoop
void Epoll::handleEvent(int listenFd, std::shared_ptr<ThreadPool>& threadPool, int eventsNum) {
    assert(eventsNum > 0);
    // 取出就绪文件描述符
    for(int i = 0; i < eventsNum; i++) {
        HttpRequest* request = static_cast<HttpRequest*>(m_events[i].data.ptr);    // 第i个就绪文件的数据
        int fd = request->fd();
        if (fd == listenFd) {
            m_onConnection();   // 连接回调
        } else {
            // 出现错误   EPOLLERR 错误 EPOLLHUP 挂起
            if ((m_events[i].events & EPOLLERR) || (m_events[i].events & EPOLLHUP) || (!m_events[i].events & EPOLLIN) ) {
                request->setNoWorking();
                m_onCloseConnection(request);
            } else if (m_events[i].events & EPOLLIN){
                // 可读事件
                request->setWorking();
                threadPool->addTask(std::bind(m_onRequest, request));    // 向线程池队列添加任务 读数据
            } else if (m_events[i].events & EPOLLOUT) {
                // 可写事件
                request->setWorking();
                threadPool->addTask(std::bind(m_onResponse, request));   // 向线程池添加人物 写数据
            } else {
                printf("[Epoll::handleEvent] unexpected event\n");
            }
        }
    }
    return;
}

