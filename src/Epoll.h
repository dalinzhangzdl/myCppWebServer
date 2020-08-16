#ifndef __EPOLL_H__
#define __EPOLL_H__

#include <functional>
#include <vector>
#include <memory>

#include <sys/epoll.h>   // epoll_event

#define MAXEVENTS 1024

class HttpRequest;
class ThreadPool;

class Epoll {
public:
    using NewConnectCallBack = std::function<void()>;
    using CloseConnectCallBack = std::function<void(HttpRequest*)>;
    using HandleRequestCallBack = std::function<void(HttpRequest*)>;
    using HandleResponseCallBack = std::function<void(HttpRequest*)>;

    Epoll();
    ~Epoll();

    // 注册新描述符
    int add(int fd, HttpRequest* request, int events);   // 调用epoll_ctl   EPOLL_CTL_ADD
    // 修改描述符状态
    int mod(int fd, HttpRequest* request, int events);    // epoll_ctl     EPOLL_CTL_MOD
    // 删除描述符
    int del(int fd, HttpRequest* request, int events);    // epoll_ctl     EPOLL_CTL_DEL
    // 等待事件发生  返回
    int wait(int timeoutMs);                              // epoll_wait

    // 调用事件处理函数    重要函数
    void handleEvent(int listenFd, std::shared_ptr<ThreadPool>& threadPool, int eventsNum);    // 智能指针管理线程池  RAII
    
    // 设置新连接回调函数
    void setOnConnection(const NewConnectCallBack& cb) {
        m_onConnection = cb;
    }
    // 设置关闭连接回调函数
    void setOnCloseConnection(const CloseConnectCallBack& cb){
        m_onCloseConnection = cb;
    }
    // 设置请求处理回调函数
    void setOnRequest(const HandleRequestCallBack& cb) {
        m_onRequest = cb;
    }
    // 设置响应请求回调函数
    void setOnResponse(const HandleResponseCallBack& cb) {
        m_onResponse = cb;
    }

private:
    using EventList = std::vector<struct epoll_event>;   // epoll_event 事件

    int m_epollFd;
    EventList m_events;
    NewConnectCallBack m_onConnection;
    CloseConnectCallBack m_onCloseConnection;
    HandleRequestCallBack m_onRequest;
    HandleResponseCallBack m_onResponse;
};

/*
// class Epoll 
struct epoll_event {
    __uint32_t events;    // epoll 事件
    epoll_data_t data;    // 用户数据
};
// 联合
typedef union epoll_data {
    void* ptr;           
    int fd;
    uint32_t u32;
    uint64_t u64;
} epoll_data_t;
*/

#endif