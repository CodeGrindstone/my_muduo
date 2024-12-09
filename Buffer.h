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
    static const std::size_t kCheapPrepend = 8; // 存储额外的元数据
    static const std::size_t kInitialSize = 1024;

    explicit Buffer(std::size_t initialSize = kInitialSize) : 
        buffer_(kCheapPrepend + initialSize),
        readerIndex_(kCheapPrepend),
        writerIndex_(kCheapPrepend)
    {  }

    // 返回可读字节大小
    size_t readableBytes() const { return writerIndex_ - readerIndex_; }
    // 返回可写字节大小
    size_t writableBytes() const { return buffer_.size() - writerIndex_; }
    // 返回额外存储元数据大小
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

    // OnMessage string <- Buffer
    // 移动readerIndex,丢弃指定长度len的数据
    void retrieve(size_t len)
    {
        if(len < readableBytes())//丢弃长度小于可读字节长度
        {
            readerIndex_ += len;// 应用只读取了缓冲区的一部分
        }
        else
        {
            retrieveAll();
        }
    }
    
    // 把OnMessage函数上报的Buffer数据，转成string类型返回
    std::string retrieveAllAsString()
    {
        return retrieveAsString(readableBytes());        
    }

    // 读取指定字节
    std::string retrieveAsString(size_t len)
    {
        std::string result(peek(), len);
        retrieve(len);
        return result;
    }
    
    ssize_t readFd(int fd, int* saveErrno);
    ssize_t writeFd(int fd, int* saveErrno);

private:
    char* begin()
    { return &*buffer_.begin(); }

    const char* begin() const 
    { return &*buffer_.begin(); }

    // 返回可读数据的起始地址
    const char* peek() const
    { return begin() + readerIndex_; }

    // 相当于复位操作
    void retrieveAll()
    {
        readerIndex_ = writerIndex_ = kCheapPrepend;
    }

    // 缓冲区剩余空间不足时，重新分配或整理空间。
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

    std::vector<char> buffer_; // 存储实际数据的缓冲区
    std::size_t readerIndex_;   // 读指针，指向可读取数据的起始位置
    std::size_t writerIndex_;   // 写指针，指向可写入数据的起始位置
};