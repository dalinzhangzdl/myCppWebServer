#include "Buffer.h"

#include <cstring>
#include <iostream>

#include <unistd.h>
#include <sys/uio.h>   // readv


// 从套接字读到缓冲区
ssize_t Buffer::readFd(int fd, int* savedErrno) {

    // 在栈上额外开辟一个空间  ？
    char extrabuf[65536];
    struct iovec vec[2];    // 注意这个数据结构
    const size_t writable = writableBytes();   // 缓冲区可写大小
    vec[0].iov_base = _begin() + m_writeIndex;
    vec[0].iov_len = writable;
    vec[1].iov_base= extrabuf;
    vec[1].iov_len = sizeof(extrabuf);

    const ssize_t n = readv(fd, vec, 2);

    if (n < 0) {
        printf("[Buffer::readFd] fd = %d readv: %s\n", fd, strerror(errno));
        *savedErrno = errno;
    } else if (static_cast<size_t>(n) < writable) {
        m_writeIndex += n;
    } else {
        m_writeIndex = m_buffer.size();
        append(extrabuf, n - writable);
    }
    return n;
}

// 从缓冲区写道套接字
ssize_t Buffer::writeFd(int fd, int* savedErrno) {
    size_t nLeft = readableBytes();   // 缓冲区的大小
    char* bufptr = _begin() + m_readIndex;   // 获取第一个元素
    ssize_t n;
    if ((n = write(fd, bufptr, nLeft)) <= 0) {
        if (n < 0 && n == EINTR)
            return 0;
        else {
            printf("[Buffer::writeFd]fd = %d write : %s\n", fd, strerror(errno));
            *savedErrno = errno;
            return -1;
        }
    } else {
        m_readIndex += n;
        return n;
    }
}
