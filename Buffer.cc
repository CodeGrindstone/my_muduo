#include "Buffer.h"
#include <errno.h>
#include <sys/uio.h>

// 从从fd上读取数据 Poller工作在LT模式
// 如果此次数据没有读完，下一次接着触发读就绪
ssize_t Buffer::readFd(int fd, int *savenoErrno)
{
    char extrabuf[65536] = {0};
    struct iovec vec[2];

    const std::size_t writable = writableBytes();// Buffer底层缓冲区可写的空间大小
    vec[0].iov_base = begin() + writerIndex_; 
    vec[0].iov_len = writable;

    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof extrabuf;

    const int iovcnt = (writable < sizeof extrabuf) ? 2 : 1;
    const ssize_t n = readv(fd, vec, iovcnt);
    if(n < 0)
    {
        *savenoErrno = errno;
    }
    else if(n <= writable)
    {
        writerIndex_ += n;
    }
    else
    {
        writerIndex_ = buffer_.size();
        append(extrabuf, n - writable);
    }
    return n;
}
