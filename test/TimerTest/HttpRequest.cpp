#include "HttpRequest.h"

#include <cassert>
#include <iostream>
#include <unistd.h>

HttpRequest::HttpRequest(int fd) 
    : m_fd(fd),
    m_working(false), 
    m_timer(nullptr),
    m_state(ExpectRequestLine),
    m_method(Invalid), 
    m_version(Unknown) 
    {
        // printf("%d", m_fd); // 测试初始化状态
        assert(m_fd >= 0);
    }

HttpRequest::~HttpRequest() {
    close(m_fd);
}

int HttpRequest::read(int* savedErrno) {
    int ret = m_inBuf.readFd(m_fd, savedErrno);
    return ret;
}

int HttpRequest::write(int* savedErrno) {
    int ret = m_outBuf.writeFd(m_fd, savedErrno);
    return ret;
}

bool HttpRequest::parseRequest() {
    bool ok = true;
    bool hasMore = true;

    while(hasMore) {
        if (m_state == ExpectRequestLine) {
            // 处理请求行
            const char* crlf = m_inBuf.findCRLF();
            if (crlf) {
                ok = _parseRequestLine(m_inBuf.peek(), crlf);   // 解析报文
                if (ok) {
                    m_inBuf.retrieveUntil(crlf + 2);            // 更新缓冲区的下标索引
                    m_state = ExpectHeaders;
                } else {
                    hasMore = false;
                }
            } else {
                hasMore = false;
            }
        } else if(m_state == ExpectHeaders) {
            // 处理报文头
            const char* crlf = m_inBuf.findCRLF();
            if (crlf) {
                const char* colon = std::find(m_inBuf.peek(), crlf, ':');
                if (colon != crlf){
                    _addHeader(m_inBuf.peek(), colon, crlf);
                } else {
                    m_state = GotAll;
                    hasMore = false;
                }
                m_inBuf.retrieveUntil(crlf + 2);
            } else {
                hasMore = false;
            }
        } else if (m_state == ExpectBody) {
            // 处理报文体
            // ..
        }
    }
    return ok;
}

// 请求行解析
bool HttpRequest::_parseRequestLine(const char* begin, const char* end){
    bool succeed = false;
    const char* start = begin;
    const char* space = std::find(start, end, ' ');   // 确定请求方法
    if (space != end && _setMethod(start, space)) {
        start = space + 1;
        space = std::find(start, end, ' ');
        if (space != end) {
            const char* question = std::find(start, end, '?');   // 确定URL
            if (question != space) {
                _setPath(start, question);
                _setQuery(question, space);
            } else {
                _setPath(start, space);
            }
            start = space + 1;
            succeed = end - start == 8 && std::equal(start, end - 1, "HTTP/1.");   // 确定HTTP版本
            if (succeed) {
                if (*(end - 1) == '1') {
                    _setVersion(HTTP11);
                } else if (*(end - 1) == '0') {
                    _setVersion(HTTP10);
                } else 
                    succeed = false;
            }
        }
    }
    return succeed;
}

bool HttpRequest::_setMethod(const char* begin, const char* end) {
    std::string m(begin, end);
    if (m == "GET")
        m_method = Get;
    else if (m == "POST")
        m_method = Post;
    else if (m == "HEAD")
        m_method = Head;
    else if (m == "PUT")
        m_method = Put;
    else if(m == "DELETE")
        m_method = Delete;
    else
        m_method = Invalid;
    return m_method != Invalid;
}

void HttpRequest::_addHeader(const char* begin, const char* colon, const char* end) {
    std::string field(begin, colon);
    ++colon;
    while(colon < end && *colon == ' ')
        ++colon;
    std::string value(colon, end);
    while(!value.empty() && value[value.size() - 1] == ' ')
        value.resize(value.size() - 1);

    m_headers[field] = value;
}

std::string HttpRequest::getMethod() const {
    std::string res;
    if (m_method == Get)
        res = "GET";
    else if (m_method == Post)
        res = "POST";
    else if (m_method == Head)
        res = "HEAD";
    else if (m_method == Put)
        res = "PUT";
    else if (m_method == Delete)
        res = "DELETE";

    return res;
}

std::string HttpRequest::getHeader(const std::string& field) const {
    std::string res;
    auto iter = m_headers.find(field);
    if (iter != m_headers.end())
        res = iter->second;
    
    return res;
}

bool HttpRequest::keepAlive() const {
    std::string connection = getHeader("Connection");
    bool res = connection == "Keep-Alive" || (m_version == HTTP11 && connection != "close");

    return res;
}

void HttpRequest::resetParse() {
    m_state = ExpectRequestLine;     // 报文解析状态
    m_method = Invalid;              // HTTP 方法
    m_version = Unknown;             // HTTP 版本
    m_path = "";                     // URL 路径
    m_query = "";                    // URL 参数
    m_headers.clear();
}