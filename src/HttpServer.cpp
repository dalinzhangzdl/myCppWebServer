#include "HttpServer.h"
#include "HttpResponse.h"
#include "HttpRequest.h"
#include "ThreadPool.h"
#include "Timer.h"
#include "Utils.h"
#include "Epoll.h"

#include <iostream>
#include <functional>
#include <cassert>
#include <cstring>

#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>


HttpServer::HttpServer(int port, int numThread)
    : m_port(port),
      m_listenFd(createSocketFd(m_port)),
      m_listenRequest(new HttpRequest(m_listenFd)),
      m_epoll(new Epoll()),
      m_threadPool(new ThreadPool(numThread)),
      m_timerManager(new TimerManager())
      {
          assert(m_listenFd >= 0);
      }

HttpServer::~HttpServer() {

}

void HttpServer::run() {
    // 注册监听套接字描述符 
    m_epoll->add(m_listenFd, m_listenRequest.get(), (EPOLLIN | EPOLLET));

    // 注册回调函数

    // 注册连接回调
    m_epoll->setOnConnection(std::bind(&HttpServer::_acceptConnection, this));
    // 注册关闭连接
    m_epoll->setOnCloseConnection(std::bind(&HttpServer::_closeConnection, this, std::placeholders::_1));
    // 注册请求处理回调
    m_epoll->setOnRequest(std::bind(&HttpServer::_doRequest, this, std::placeholders::_1));
    // 注册响应处理回调函数
    m_epoll->setOnResponse(std::bind(&HttpServer::_doResponse, this, std::placeholders::_1));

    // 事件循环 oneEventLoop
    while(1) {
        int timeMs = m_timerManager->getNextExpireTime();   // 取得过期时间
        // epoll_wait() 等待事件
        int events = m_epoll->wait(timeMs);   // 封装了epoll_wait

        if (events > 0) {
            // 事件分发
            m_epoll->handleEvent(m_listenFd, m_threadPool, events);
        }
        m_timerManager->handleExpireTimers();    // 定时器tick 管理过期连接
    }
}

// accept连接  添加到epoll内核事件表  ET
void HttpServer::_acceptConnection() {
    //printf("[HttpServer::acceptConnection] accept finish\n");
    while(1) {
        int acceptFd = accept4(m_listenFd, nullptr, nullptr, SOCK_NONBLOCK | SOCK_CLOEXEC);
        if (acceptFd == -1) {
            if (errno == EAGAIN) 
                break;
            printf("HttpServer::_acceptConnection] acceptConnect: %s\n", strerror(errno));
            break;
        }
        // 为新的连接套接字分配HttpRequest资源
        HttpRequest* request = new HttpRequest(acceptFd);
        m_timerManager->addTimer(request, CONNECT_TIMEOUT, std::bind(&HttpServer::_closeConnection, this, request));   // 注意this 指针
        // epoll内核事件表注册 acceptFd
        m_epoll->add(acceptFd, request, (EPOLLIN | EPOLLONESHOT));   // 注意需要EPOLLONESHOT 
    }
}

void HttpServer::_closeConnection(HttpRequest* request) {
    int fd = request->fd();
    if (request->isWorking())
        return;
    // 打印调试信息
    //printf("[HttpServer::_closeConnection] connect fd = %d is closed\n", fd);
    // 删除定时器
    m_timerManager->delTimer(request);
    // 删除epoll事件
    m_epoll->del(fd, request, 0);
    // 释放套接字占用的HttpRequest资源， HttpRequest类析构函数close fd
    delete request;    // 调用析构函数
    request = nullptr;
}

// LT 模式和 ET 模式的区别
// LT模式是epoll 默认的工作模式，该模式下为一个高效的poll，LT模式是指当epoll_wait检测到其上有事件发生并将此事件通知应用程序后，应用程序可以不立即处理该事件
// 当程序下一次调用epoll_wait时，epoll_wait还会再次向应用程序通告此事件，直到该事件被处理。
// ET模式下，epoll_wait通知应用程序有事件发生时，应用程序必须立即处理该事件，否则下一次调用epoll_wait时不在通知

// 使用LT模式
// 解析请求报文 并发送响应报文
void HttpServer::_doRequest(HttpRequest* request) {

    m_timerManager->delTimer(request);
    assert(request != nullptr);

    int fd = request->fd();
    //printf("[HttpServer::doRequest] read Buffer start \n");
    int readErrno;
    int nRead = request->read(&readErrno);  // 从缓冲区读取数据
    //printf("[HttpServer::doRequest] read Buffer end \n");
    //printf("[HttpServer::doRequest] read Buffer %d bytes \n", nRead);
    
    //printf("[HttpServer::doRequest] read Buffer Errno %d \n", readErrno);
    // 返回为0 客户端断开连接
    if (nRead == 0) {
        request->setNoWorking();
        _closeConnection(request);   // 关闭连接
        return;
    }

    // 非EAGAIN错误 断开
    if (nRead < 0 && readErrno != EAGAIN) {
        request->setNoWorking();
        _closeConnection(request);
        return;
    }

    if (nRead < 0 && readErrno == EAGAIN) {
        m_epoll->mod(fd, request, (EPOLLIN | EPOLLONESHOT));
        request->setNoWorking();
        m_timerManager->addTimer(request, CONNECT_TIMEOUT, std::bind(&HttpServer::_closeConnection, this, request));
        return;
    }

    // 解析报文, 出错断开连接
    if (!request->parseRequest()) {
        // 解析不成功 发送400状态码
        HttpResponse response(400, "", false);   // 初始化一个 400 response对象
        request->appendOutBuffer(response.makeResponse());    // 发送 doErrorResponse()

        int writeErrno;
        request->write(&writeErrno);
        request->setNoWorking();
        _closeConnection(request);   // 关闭连接
    }

    // !request->parseRequest() 不成立 则报文解析成功
    // 发送response 并继续监听 注册EPOLLONESHOT
    if (request->parseFinish()) {
        HttpResponse response(200, request->getPath(), request->keepAlive());
        request->appendOutBuffer(response.makeResponse());
        m_epoll->mod(fd, request, (EPOLLIN | EPOLLONESHOT | EPOLLOUT)); 
    }

    return;
}


void HttpServer::_doResponse(HttpRequest* request) {

    m_timerManager->delTimer(request);
    assert(request != nullptr);
    int fd = request->fd();

    int totalWrite = request->writableBytes();

    if (totalWrite == 0) {
        m_epoll->mod(fd, request, (EPOLLIN | EPOLLONESHOT));
        request->setNoWorking();
        m_timerManager->addTimer(request, CONNECT_TIMEOUT, std::bind(&HttpServer::_closeConnection, this, request));
        return;
    }

    int writeErrno;
    int ret = request->write(&writeErrno);

    if(ret < 0 && writeErrno == EAGAIN) {
        m_epoll->mod(fd, request, (EPOLLIN | EPOLLONESHOT | EPOLLOUT));
        return;
    }

    // 断开连接
    if (ret < 0 && (writeErrno != EAGAIN)) {
        request->setNoWorking();
        _closeConnection(request);
        return;
    }

    if (ret == totalWrite) {
        if (request->keepAlive()) {
            // 长连接则保持连接 继续监听
            request->resetParse();
            m_epoll->mod(fd, request, (EPOLLIN | EPOLLONESHOT));
            request->setNoWorking();
            m_timerManager->addTimer(request, CONNECT_TIMEOUT, std::bind(&HttpServer::_closeConnection, this, request));
        } else {
            request->setNoWorking();
            _closeConnection(request);
        }
        return;
    }

    m_epoll->mod(fd, request, (EPOLLIN | EPOLLOUT | EPOLLONESHOT));
    request->setNoWorking();
    m_timerManager->addTimer(request, CONNECT_TIMEOUT, std::bind(&HttpServer::_closeConnection, this, request));
    return;
}


      

