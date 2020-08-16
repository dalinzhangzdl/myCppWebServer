# 基于C++11静态WebServer设计

## 项目目的

**技术栈是C++，学习了C++以及网络编程的基础知识，但缺乏相应的实战。因此需要一个实战项目来把所学习的知识串接成一个整体，在实战项目中加深理解。考虑到静态服务器的实现涉及的知识面比较多，因此选择实现一个简易版高性能的静态服务器。通过这个项目将自己的理论学习核项目实战结合起来提升自己。**

**C++11增加了多线程编程库，增加了处理日期和时间的chrono库，支持lambda表达式，增加了智能指针shared_ptr，增加了std::function和std::bind处理可调用对象和其他一些新特性。**

**项目开发的目标：**

1. 实现一个简易版的高性能静态服务器，支持静态文件的访问，处理常见的HTTP客户端错误。
2. 实现定时器功能，处理HTTP的长连接和超时断开，具备高并发的能力。
3. 实现服务器的云端部署，公网能够访问，并进行压力测试。
4. 使用一些C++11的新特性(线程库、chrono时间库、智能指针等），使用C++标准库，不使用第三方库。
5. 学习和使用基本的开发调试工具(g++, gdb, git等)，在Linux平台开发。

[基础知识](./WebServer编程基础.md)

## 并发模型

**Non-blocking I/O + I/O复用+ 线程池** 

本项目采用的核心架构

1. **非阻塞IO**，当数据未准备好时，使用read和write读取数据时立即返回并设置errno为EAGAIN，而不是一直阻塞到IO数据就绪。当IO数据就绪时，write和read系统调用是阻塞的，本质是阻塞IO。
2. **I/O复用**，使用epoll同时监听多个文件描述符，采用半同步半反应堆模型，epoll为事件循环，接收客户端连接，向任务队列中添加任务。为每个socket事件注册**EPOLLONESHOT事件**，避免出现两个线程操作一个socket事件（当一个线程在读取完socket上的数据后进行计算处理时，该socket又有新的数据可读，此时会触发另一个线程读取这些数据）
3. **线程池**，线程池从任务队列中获取任务，完成任务后睡眠或者继续执行任务。
4. **定时器**，定时器的作用是剔除超时的连接，为了避免过多的连接导致fd耗尽，为每个请求注册定时器。
5. **注册事件回调**

**核心结构：**

```C++
void HttpServer::run() {
    // 注册监听套接字描述符 
    m_epoll->add(m_listenFd, m_listenRequest.get(), (EPOLLIN | EPOLLET));

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
        int timeMs = m_timerManager->getNextExpireTime();   // 返回epoll 超时时间 
        
        // IO复用 监听多个描述符
        int events = m_epoll->wait(timeMs);   // 封装了epoll_wait 超时时间-1是阻塞 等于0立即执行

        // 连接和任务分发
        if (events > 0) {
            m_epoll->handleEvent(m_listenFd, m_threadPool, events);
        }

        // 定时器管理过期连接
        m_timerManager->handleExpireTimers();    // 定时器tick 管理过期连接
    }
}
```



## 核心类

---

1. Socket接口和Epoll封装(Utils.h 和 Epoll.h)

   1. 封装socket通信的socket创建、绑定、监听。
   2. 封装epoll，主要是封装epoll_wait和监听就绪文件描述符后的事件分发。

2. 线程池和同步队列实现(ThreadPool.h)

   1. 使用C++线程库，多线程编程实现线程池。
   2. 同步队列为简单的队列，后期改进为生产者和消费者模型。

3. 定时器和定时器管理实现(Timer.h)

   1. 基于chrono库处理定时时间。
   2. 采用小顶堆管理定时器，堆顶定时器为下一个即将过期的定时事件。
   3. 回调机制踢掉过期连接。

4. 实现输入输出缓冲区Buffer(Buffer.h)

   1. Non-blocking IO的核心思想是避免阻塞在read()或write()或其他IO系统调用上，这样可以最大限度的复用线程，让一个线程服务多个socket连接。IO线程只能阻塞在IO复用函数上，因此对每个TCP socket需要设计输入输出缓冲。
   2. 参考**[muduo](https://github.com/chenshuo/muduo)**的Buffer类实现的缓冲区，缓冲区可自动增长，底层为vector。

5. HTTP请求报文解析(HttpRequest.h)

   1. 网络通信相关: 文件描述符、输入缓冲区、输出缓冲区。
   2. 指向定时器的指针。
   3. 报文解析相关，一些报文解析状态。
   4. 状态机编程实现报文解析

6. HTTP响应类(HttpResponse.h)

   1. 用于创建HTTP响应报文
      + 错误报文响应 状态码
      + 正确报文响应 状态码+资源

7. **HttpServer类(HttpServer.h)**

   1. HTTP服务器类，拥有监听描述符、线程池、定时器管理器和Epoll。
   2. 注册事件回调，运行于oneEventLoop状态，处理连接请求、任务分发以及使用定时器跳动处理过期连接。

   

   