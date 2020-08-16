# myCppWebServer
**使用C++11实现的高性能静态web服务器，可处理静态资源请求，定时器管理超时连接，具备并发能力。**

## 开发工具

- **操作系统**： Ubuntu18.04LTS
- **编辑器**：  vscode + vim 
- **编译器**： g++ 7.5.0
- **调试工具**： gdb
- **压测工具**： webbench 1.5

## 代码统计

![ ](C:\Users\USER\AppData\Roaming\Typora\typora-user-images\image-20200815204900235.png)

## 使用方式

```shell
cd src
g++ -g -c *.cpp -std=c++11 -lpthread
g++ -g -o server *.o -std=c++11 -lpthread
./server 8080 4 // 默认本地8080端口 线程池大小为4
```

## 项目要点

- Reactor 并发模型。
- Non-blocking IO + IO复用 + 线程池。
- 基于优先队列实现定时器，实现对定时事件的统一管理。采用小顶堆实现，堆顶定时器为所有定时器中超时时间最小的一个定时器，将该超时时间作为心博间隔，一旦到达超时时间，删除该定时器和对应的资源，并从剩余的定时器中找出超时时间最小的一个作为下次定时器超时时间。
- 实现缓冲区，设计了可自动增长的缓冲区，实现HTTP请求和响应的数据缓冲区。
- 封装Socket通信和Epoll IO复用。