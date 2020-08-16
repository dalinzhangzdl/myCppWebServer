#ifndef __HTTP_REQUEST_H__
#define __HTTP_REQUEST_H__

#include "Buffer.h"

#include <string>
#include <map>
#include <iostream>

#define STATIC_ROOT "../www"



class Timer;

class HttpRequest {
public:
    enum HttpRequestParseState { // 报文解析状态
        ExpectRequestLine,
        ExpectHeaders,
        ExpectBody,
        GotAll
    };

    enum Method { // HTTP方法
        Invalid, Get, Post, Head, Put, Delete
    };

    enum Version { // HTTP版本
        Unknown, HTTP10, HTTP11
    };

    HttpRequest(int fd);
    ~HttpRequest();

    int fd() { return m_fd; } // 返回文件描述符
    int read(int* savedErrno); // 读数据
    int write(int* savedErrno); // 写数据

    void appendOutBuffer(const Buffer& buf) { m_outBuff.append(buf); }
    int writableBytes() { return m_outBuff.readableBytes(); }

    void setTimer(Timer* timer) { m_timer = timer; }
    Timer* getTimer() { return m_timer; }

    void setWorking() { m_working = true; }
    void setNoWorking() { m_working = false; }
    bool isWorking() const { return m_working; }

    bool parseRequest(); // 解析Http报文
    bool parseFinish() { return m_state == GotAll; } // 是否解析完一个报文
    void resetParse(); // 重置解析状态
    std::string getPath() const { return m_path; }
    std::string getQuery() const { return m_query; }
    std::string getHeader(const std::string& field) const;
    std::string getMethod() const;
    bool keepAlive() const; // 是否长连接

private:
    // 解析请求行
    bool _parseRequestLine(const char* begin, const char* end);
    // 设置HTTP方法
    bool _setMethod(const char* begin, const char* end);
    // 设置URL路径
    void _setPath(const char* begin, const char* end)
    { 
        std::string subPath;
        subPath.assign(begin, end);
        if(subPath == "/")
            subPath = "/index.html";
        m_path = STATIC_ROOT + subPath;
    }
    // 设置URL参数
    void _setQuery(const char* begin, const char* end)
    { m_query.assign(begin, end); }
    // 设置HTTP版本
    void _setVersion(Version version) 
    { m_version = version; }
    // 增加报文头
    void _addHeader(const char* start, const char* colon, const char* end);

private:
    // 网络通信相关
    int m_fd; // 文件描述符
    Buffer m_inBuff; // 读缓冲区
    Buffer m_outBuff; // 写缓冲区
    bool m_working; // 若正在工作，则不能被超时事件断开连接

    // 定时器相关
    Timer* m_timer;

    // 报文解析相关
    HttpRequestParseState m_state; // 报文解析状态
    Method m_method; // HTTP方法
    Version m_version; // HTTP版本
    std::string m_path; // URL路径
    std::string m_query; // URL参数
    std::map<std::string, std::string> m_headers; // 报文头部
}; // class HttpRequest


#endif
