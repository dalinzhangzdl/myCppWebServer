#ifndef __HTTPRESPONSE_H__
#define __HTTPRESPONSE_H__

#include <map>
#include <string>

#define CONNECT_TIMEOUT 500    // 500ms 

class Buffer;

class HttpResponse {
public:
    static const std::map<int, std::string> statusCode2Message;
    static const std::map<std::string, std::string> suffix2Type;    

    HttpResponse(int statusCode, std::string path, bool keepAlive):
                m_statusCode(statusCode),
                m_path(path),
                m_keepAlive(keepAlive){ }

    ~HttpResponse() { }

    Buffer makeResponse();

    void doErrorResponse(Buffer& output, std::string message);
    void doStaticResponse(Buffer& output, long fileSize);

private:
    std::string _getFileType() {
        int index = m_path.find_last_of('.');   // 找最后一个点
        std::string suffix;
        // 查找对应的后缀
        if (index == std::string::npos) {
            return "text/plain";
        }

        suffix = m_path.substr(index);
        auto iter = suffix2Type.find(suffix);
        // 未知的文件后缀，默认为纯文本
        if (iter == suffix2Type.end())
            return "text/plain";
            
        return iter->second;
    }

private:
    std::map<std::string, std::string> m_headers;   // 响应报文头部
    int m_statusCode;                               // 响应状态码   1xx 2xx 3xx 4xx 5xx
    std::string m_path;                             // 请求资源路径
    bool m_keepAlive;                               // 长连接
};

#endif