#include "Timer.h"
#include "HttpRequest.h"

#include <iostream>
#include <functional>

#include <sys/epoll.h>

void testFunc(HttpRequest* request) {
    printf("timeout id is %d\n", request->fd());
    return;
}


int main() {
    // 初始化HttpRequest 用于测试
    HttpRequest req1(2), req2(4), req3(10), req4(20);
    TimeManager timer;
    timer.addTimer(&req1, 2000, std::bind(&testFunc, &req1));
    timer.addTimer(&req2, 5000, std::bind(&testFunc, &req2));
    timer.addTimer(&req3, 10000, std::bind(&testFunc, &req3));
    //timer.addTimer(&req1, 2000, std::bind(&testFunc, &req1));
    printf("add Timer success \n");

    // 创建epoll
    int epollFd = epoll_create1(EPOLL_CLOEXEC);    // EPOLL_CLOEXEC 标志 进程被替换时关闭打开的文件描述符
    int m_flag = 0;
    while(1) {
        int time = timer.getNextExpireTime();  // 下一个超时时间
        printf("next time out time is %d ms\n", time);
        if (time == -1)
            break;
        epoll_event events[1];
        epoll_wait(epollFd, events, 1, time);

        timer.handleExpireTimers();   // 处理过期事件
        if (m_flag == 0) {
            timer.delTimer(&req2);
            // 添加定时器
            timer.addTimer(&req4, 5000, std::bind(&testFunc, &req4));
            timer.addTimer(&req4, 6000, std::bind(&testFunc, &req4));
            m_flag = 1;
        }
    }
}
