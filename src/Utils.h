#ifndef __UTILS_H__
#define __UTILS_H__

const int LISTENQ = 1024;

int createSocketFd(int port);    // 封装socket 创建与绑定
int setNonBlocking(int fd);      // 设置fd非阻塞
// 预添加 accept接口


#endif