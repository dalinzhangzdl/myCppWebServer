#include "Utils.h"

#include <iostream>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>    // socket bind listen
#include <arpa/inet.h>     // htons 
#include <stdio.h>         // perror

int createSocketFd(int port) {
    // 端口处理
    port = ((port <= 1024) || (port >= 65535)) ? 9999: port;

    // 创建socket 非阻塞fd
    int listenFd = 0;
    if ((listenFd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) == -1) {
        printf("[createSocketFd] fd = %d socket : %s\n", listenFd, strerror(errno));
        return -1;
    }

    // 设置socket选项    SO_REUSEADDR 强制使用被Time_Wait状态占用的socket地址
    int reuse = 1;
    if (setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1) {
        printf("[createSocketFd] fd = %d setsockopt : %s\n", listenFd, strerror(errno));
        return -1;
    }

    // bind and listen
    struct sockaddr_in serverAddr;
    bzero(&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;                   // 协议
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);    // INADDR_ANY  指向地址为0.0.0.0的地址, 事实是表示不确定的地址
    serverAddr.sin_port = htons((unsigned int )port);  // 端口 主机字节序

    // bind
    if (bind(listenFd, (struct sockaddr*)& serverAddr, sizeof(serverAddr)) == -1) {
        printf("[createSocketFd] fd = %d bind : %s\n", listenFd, strerror(errno));
        return -1;        
    }
    // listen
    if (listen(listenFd, LISTENQ) == -1) {
        printf("[createSocketFd] fd = %d listen : %s\n", listenFd, strerror(errno));
        return -1;
    }

    // close invalid fd
    if (listenFd == -1) {
        close(listenFd);
        return -1;
    }
    // printf("listenFd is %d\n", listenFd);
    return listenFd;
}

// 设置fd非阻塞
int setNonBlocking(int fd) {
    int oldOption = fcntl(fd, F_GETFL);
    if (oldOption == -1) {
        printf("[setNonBlocking] fd = %d fcntl : %s\n", fd, strerror(errno));
        return -1;        
    }
    // 设置非阻塞
    int newOption = oldOption | O_NONBLOCK;
    if (fcntl(fd, F_SETFL, newOption) == -1) {
        printf("[setNonBlocking] fd = %d fcntl : %s\n", fd, strerror(errno));
        return -1;  
    }
    
    return 0;
}    