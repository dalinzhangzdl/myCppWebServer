#ifndef __HTTPSERVER_H__
#define __HTTPSERVER_H__

#include <memory>   // unique_ptr
#include <mutex>

#define TIMEOUT -1   // epoll_wait超时时间 -1表示不超时
#define CONNECT_TIMEOUT  500   // 连接默认超时时间
#define NUM_WORKERS 4   // 线程池参数

class HttpRequest;
class Epoll;
class ThreadPool;
class TimerManager;

class HttpServer {
public:
    HttpServer(int port, int numThread);
    ~HttpServer();

    // 启动Http服务器
    void run();

private:
    // callback function
    void _acceptConnection();
    void _closeConnection(HttpRequest* request);
    void _doRequest(HttpRequest* request);    // 处理HTTP 报文 
    void _doResponse(HttpRequest* request);

private:
    using ListenRequestPtr = std::unique_ptr<HttpRequest>;
    using EpollPtr = std::unique_ptr<Epoll>;
    using ThreadPoolPtr = std::shared_ptr<ThreadPool>;
    using TimerManagerPtr = std::unique_ptr<TimerManager>;

    // 网络通信相关
    int m_port;
    int m_listenFd;

    // http 相关
    ListenRequestPtr m_listenRequest;    // 监听套接字的HttpRequest实例
    EpollPtr m_epoll; // epoll 实例
    ThreadPoolPtr m_threadPool;   // 线程池
    TimerManagerPtr m_timerManager;  // 定时器
};


#endif
