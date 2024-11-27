#pragma once

#include <vector>
#include <algorithm>
#include <string>

/// A buffer class modeled after org.jboss.netty.buffer.ChannelBuffer
///
/// @code
/// +-------------------+------------------+------------------+
/// | prependable bytes |  readable bytes  |  writable bytes  |
/// |                   |     (CONTENT)    |                  |
/// +-------------------+------------------+------------------+
/// |                   |                  |                  |
/// 0      <=      readerIndex   <=   writerIndex    <=     size
/// @endcode
class Buffer
{
public:
    static const std::size_t kCheapPrepend = 8;
    static const std::size_t kInitialSize = 1024;

    explicit Buffer(std::size_t initialSize = kInitialSize) : 
        buffer_(kCheapPrepend + initialSize),
        readerIndex_(kCheapPrepend),
        writerIndex_(kCheapPrepend)
    {  }

     size_t readableBytes() const { return writerIndex_ - readerIndex_; }
     size_t writableBytes() const { return buffer_.size() - writerIndex_; }
     size_t prependableBytes() const { return readerIndex_; }

    // 确认写缓冲区的大小满足写 len 字节的长度,不够需要扩容
    void ensureWritableBytes(std::size_t len)
    {
        if(writableBytes() < len)
        {
            makeSpace(len);
        }
    }

    // 把[data, data+len)内存上的数据添加到writable缓冲区中
    void append(const char* data, std::size_t len)
    {
        ensureWritableBytes(len);
        std::copy(data, data+len, beginWrite());
        writerIndex_ += len;
    }

    char* beginWrite()
    {   return begin() + writerIndex_; }

    const char* beginWrite() const
    { return begin() + writerIndex_; }


    ssize_t readFd(int fd, int* savenoErrno);

private:
    char* begin()
    { return &*buffer_.begin(); }

    const char* begin() const 
    { return &*buffer_.begin(); }

    // 返回可读数据的起始地址
    const char* peek() const
    { return begin() + readerIndex_; }

    // OnMessage string <- Buffer
    void retrieve(std::size_t len)
    {
        if(len < readerIndex_)
        {
            readerIndex_ += len;// 应用只读取了缓冲区的一部分
        }
        else
        {
            retrieveAll();
        }
    }

    // 相当于复位操作
    void retrieveAll()
    {
        readerIndex_ = writerIndex_ = kCheapPrepend;
    }

    // 把OnMessage函数上报的Buffer数据，转成string类型返回
    std::string retrieveAllAsString()
    {
        return retrieveAsString(readableBytes());        
    }

    std::string retrieveAsString(size_t len)
    {
        std::string result(peek(), len);
        retrieve(len);
        return result;
    }

    void makeSpace(std::size_t len)
    {
        if(writableBytes() + prependableBytes() < len + kCheapPrepend)
        {
            buffer_.resize(writerIndex_ + len); 
        }
        else
        {
            size_t readable = readableBytes();
            std::copy(begin() + readerIndex_,
                    begin() + writerIndex_,
                    begin() + kCheapPrepend);
            readerIndex_ = kCheapPrepend;
            writerIndex_ = readerIndex_ + readable;
        }
    }

    std::vector<char> buffer_;
    std::size_t readerIndex_;
    std::size_t writerIndex_;
};