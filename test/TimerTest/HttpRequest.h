#ifndef __HTTPREQUEST_H__
#define __HTTPREQUEST_H__

#include "Buffer.h"

#include <string>
#include <map>
#include <iostream>

// 状态机编程
#define STATIC_ROOT "../www"

class Timer;

class HttpRequest {
public:
    // 报文解析状态
    enum HttpRequestParseState{
        ExpectRequestLine,
        ExpectHeaders,
        ExpectBody,
        GotAll
    };
    // HTTP 方法
    enum Method {
        Invalid, Get, Post, Head, Put, Delete
    };
    // HTTP 版本
    enum Version {
        Unknown, HTTP10, HTTP11
    };

    HttpRequest(int fd);
    ~HttpRequest();

    int fd() {
        return m_fd;
    }

    int read(int* savedErrno);
    int write(int* savedRrrno);

    // 添加buffer
    void appendOutBuffer(const Buffer& buf) {
        m_outBuf.append(buf);
    }
    int writableBytes() {
        return m_outBuf.readableBytes();
    }

    void setTimer(Timer* timer) {
        m_timer = timer;
    }
    Timer* getTimer() {
        return m_timer;
    }

    void setWorking() {
        m_working = true;
    }
    void setNoWorking() {
        m_working = false;
    }
    bool isWorking() const {
        return m_working;
    }

    // HTTP 报文解析
    bool parseRequest();
    bool parseFinish() {
        return m_state == GotAll;
    }
    // 重置解析状态
    void resetParse();

    std::string getPath() const {
        return m_path;
    }
    std::string getQuery() const {
        return m_query;
    }
    std::string getHeader(const std::string& field) const;
    std::string getMethod() const;
    bool keepAlive() const;    // Http 长连接

private:
    // 解析请求行
    bool _parseRequestLine(const char* begin, const char* end);
    // 设置HTTP 方法
    bool _setMethod(const char* begin, const char* end);
    // 设置URL 路径
    void _setPath(const char* begin, const char* end) {
        std::string subPath;
        subPath.assign(begin, end);
        if (subPath == "/")
            subPath = "/index.html";
        m_path = STATIC_ROOT + subPath;
    }
    // 设置URL 参数
    void _setQuery(const char* begin, const char* end){
        m_query.assign(begin, end);   // assign 先清空字符串 再赋值字符串
    }
    // 设置URL 版本
    void _setVersion(Version version) {
        m_version = version;
    }
    // 添加报文头
    void _addHeader(const char* begin, const char* colon, const char* end);
private:
    // 定时器
    Timer* m_timer;

    // 网络通信相关
    int m_fd;   // 文件描述符
    Buffer m_inBuf;   // 读缓冲区
    Buffer m_outBuf;  // 写缓冲区
    bool m_working;   // 若正在工作，则不能被超时事件断开连接

    // http报文解析相关
    HttpRequestParseState m_state;    // 报文解析状态
    Method m_method;                  // HTTP 方法 get post ..
    Version m_version;                // HTTP 版本
    std::string m_path;               // URL 路径
    std::string m_query;              // URL 参数
    std::map<std::string, std::string> m_headers;  // 报文头部
};

#endif