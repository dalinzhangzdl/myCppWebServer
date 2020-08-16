#include "HttpResponse.h"
#include "Buffer.h"

#include <string>
#include <iostream>
#include <cassert>
#include <cstring>

#include <fcntl.h>   // 文件控制
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>   // mmap, munmap

// 初始化静态成�
const std::map<int, std::string> HttpResponse::statusCode2Message = {
    {200, "OK"},
    {400, "Bad Request"},
    {403, "Forbidden"},
    {404, "Not Found"},
    {500, "Internal Server Error"}
};

const std::map<std::string, std::string> HttpResponse::suffix2Type = {
    {".html", "text/html"},
    {".xml", "text/xml"},
    {".xhtml", "application/xhtml+xml"},
    {".txt", "text/plain"},
    {".rtf", "application/rtf"},
    {".pdf", "application/pdf"},
    {".word", "application/nsword"},
    {".png", "image/png"},
    {".gif", "image/gif"},
    {".jpg", "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".au", "audio/basic"},
    {".mpeg", "video/mpeg"},
    {".avi", "video/x-msvideo"},
    {".gz", "application/x-gzip"},
    {".tar", "application/x-tar"},
    {".css", "text/css"}
};

Buffer HttpResponse::makeResponse() {
    Buffer output;

    // 请求语法错误
    if (m_statusCode == 400) {
        doErrorResponse(output, "Server can't parse the message, Please check it");
        return output;
    }
    // stat 函数  获取文件信息 成功返回0失败返回-1
    // 404 Error
    struct stat sbuf;
    if (stat(m_path.data(), &sbuf) < 0) {
        m_statusCode = 404;    // 找不到资�        doErrorResponse(output, "Server can't find the file");
        return output;
    }
    // 403 Error  Forbidden
    // S_IRUSR  文件所有者具有可读取权限
    // S_ISREG  是否为一般文�    if (!(S_ISREG(sbuf.st_mode) || !(S_IRUSR & sbuf.st_mode))) {
        m_statusCode = 403;
        doErrorResponse(output, "Server can't read the file");
        return output;
    }
    
    // 处理静态文件请�    doStaticResponse(output, sbuf.st_size);
    return output;
}

void HttpResponse::doErrorResponse(Buffer& output, std::string message) {
    std::string body;

    auto iter = statusCode2Message.find(m_statusCode);

    if (iter == statusCode2Message.end())
        return;     // 不在状态码�
    // 添加一些报文头�组成报文头部
    body += "<html><title>Server Error</title>";
    body += "<body bgcolor=\"ffffff\">";
    body += std::to_string(m_statusCode) + " : " + iter->second + "\n";
    body += "<p>" + message + "</p>";
    body += "<hr><em>Web server by mumu</em></body></html>";

    // 响应�    output.append("HTTP/1.1 " + std::to_string(m_statusCode) + " " + iter->second + "\r\n");
    // 报文�    output.append("Server: Simplify web server\r\n");
    output.append("Content-type: text/html\r\n");
    output.append("Connection: close\r\n");
    output.append("Content-length: " + std::to_string(body.size()) + "\r\n\r\n");
    // 报文�    output.append(body);
} 


void HttpResponse::doStaticResponse(Buffer& output, long fileSize) {
    
    assert(fileSize >= 0);

    auto iter = statusCode2Message.find(m_statusCode);
    if (iter == statusCode2Message.end()) {
        m_statusCode = 400;
        doErrorResponse(output, "Unknown status code");
        return;
    }

    // 响应�    
	output.append("HTTP/1.1 " + std::to_string(m_statusCode) + " " + iter->second + "\r\n");
    // 报文�    
	if (m_keepAlive) {
        output.append("Connection: Keep-Alive\r\n");
        output.append("Keep-Alive: timeout=" + std::to_string(CONNECT_TIMEOUT) + "\r\n");   // keep-Alive 500ms
    } else {
        output.append("Connection: colose\r\n");   
    }
    output.append("Content-type: " + _getFileType() + "\r\n");
    output.append("Content-length: " + std::to_string(fileSize) + "\r\n");

    output.append("Server: Simplify web server\r\n");
    output.append("\r\n");

    // 报文�
    // 打开资源文件
    int srcFd = open(m_path.data(), O_RDONLY, 0);   // read-only
    // 内存映射
    void* mmapPtr = mmap(NULL, fileSize, PROT_READ, MAP_PRIVATE, srcFd, 0);    // 映射成功则返回映射区的指�    close(srcFd);
    if (mmapPtr == (void*) -1) {
        munmap(mmapPtr, fileSize);   // 解除映射关系
        output.retrieveAll();        // Buffer清空
        m_statusCode = 404;
        doErrorResponse(output, "Server can't find the file");
        return;
    }

    // 成功映射
    char* ret = static_cast<char*>(mmapPtr);
    output.append(ret, fileSize);
    // 解除映射
    munmap(ret, fileSize);
}

