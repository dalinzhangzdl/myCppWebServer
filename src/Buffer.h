
#ifndef __BUFFER_H__
#define __BUFFER_H__

// 缓冲Buffer
// 建立缓冲Buffer是必须的
// 参考muduo Buffer设计

#include <vector>
#include <string>
#include <algorithm>
#include <cassert>

#define INIT_SIZE 1024   // 初始化Buffer的大小为1Kb

class Buffer {
public:
    Buffer() : m_buffer(INIT_SIZE),
        m_readIndex(0),
		m_writeIndex(0) {
			assert(readableBytes() == 0);
			assert(writableBytes() == INIT_SIZE);
		} 
	~Buffer() {
		
	}
	
	// 使用vector 默认拷贝构造和赋值构造语义可用
	
	// 重要的两个函数 RIO
	// 从套接字读到缓冲区
	ssize_t readFd(int fd, int* savedErrno);
	// 从缓冲区写到套接字
	ssize_t writeFd(int fd, int* saveErrno);

	// 可读字节数
	size_t readableBytes() const {
		return m_writeIndex - m_readIndex;
	} 
	
	// 可写字节数
	size_t writableBytes() const {
		return m_buffer.size() - m_writeIndex;
	}

	// m_readerIndex 前空闲缓冲区大小
	size_t prependableBytes() const {
		return m_readIndex;
	} 

	// 第一个可读位置
	const char* peek() const {
		return _begin() + m_readIndex;
	}

	// 取出len个字节
	void retrieve(size_t len) {
		assert(len <= readableBytes());
		m_readIndex += len;
	}

	// 取出数据直到vector尾部
	void retrieveUntil(const char* end) {
		assert(peek() <= end);
		assert(end <= beginWrite());
		// 取出从m_readIndex 到end个字节
		retrieve(end - peek());
	}

	// 取出Buffer中的所有数据
	// 恢复缓冲区下标
	void retrieveAll() {
		m_readIndex = 0;
		m_writeIndex = 0;
	}

	// string 流取出
	std::string retrieveToString() {
		std::string str(peek(), readableBytes());
		retrieveAll();  
		return str;
	}

	// 插入数据
	void append(const std::string& str) {
		append(str.data(), str.size());
	}

	void append(const char* data, size_t len) {
		ensureWritableBytes(len);
		std::copy(data, data + len, beginWrite());
		hasWritten(len);
	}

	// const void* -> const char*
	void append(const void* data, size_t len) {
		append(static_cast<const char*>(data), len);
	}

	// 拷贝Buffer
	void append(const Buffer& otherBuffer) {
		append(otherBuffer.peek(), otherBuffer.readableBytes());
	}

	// 可写char指针
	char* beginWrite() {
		return _begin() + m_writeIndex;
	}

	const char* beginWrite() const {
		return _begin() + m_writeIndex;
	}

	void ensureWritableBytes(size_t len) {
		if (writableBytes() < len) {
			_makeAdequateSpace(len);
		}
		assert(writableBytes() >= len);
	}
	// 插入数据后移动m_writerIndex
	void hasWritten(size_t len) {
		m_writeIndex += len;
	}

// 报文分包 
	const char* findCRLF() const {
		const char CRLF[] = "\r\n";
		const char* crlf = std::search(peek(), beginWrite(), CRLF, CRLF + 2);
		return crlf == beginWrite() ? nullptr : crlf;
	}

	const char* findCRLF(const char* start) const {
		assert(peek() <= start);
		assert(start <= beginWrite());
		const char CRLF[] = "\r\n";
		const char* crlf = std::search(start, beginWrite(), CRLF, CRLF + 2);
		return crlf == beginWrite() ? nullptr : crlf;
	}

private:
	// 返回缓冲区头指针
	char* _begin() {
		return &*m_buffer.begin();  // &*的含义
	}
	const char* _begin() const {
		return &*m_buffer.begin();
	}

	// 确保缓冲区具备足够的空间
	void _makeAdequateSpace(size_t len) {
		if (writableBytes() + prependableBytes() < len) {
			m_buffer.resize(m_writeIndex + len);
		} else {
			size_t readable = readableBytes();
			// 重拷贝 拷贝到缓冲区的首部
			std::copy(_begin() + m_readIndex, _begin() + m_writeIndex, _begin());
			m_readIndex = 0;
			m_writeIndex = m_readIndex + readable;
			assert(readable == readableBytes());
		}
	}

private:
    std::vector<char> m_buffer;
    size_t m_readIndex;
    size_t m_writeIndex;
};


#endif
