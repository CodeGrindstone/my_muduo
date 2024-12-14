#include "TcpConnection.h"
#include "Eventloop.h"
#include "Logger.h"
#include "Socket.h"
#include "Channel.h"
#include <string>
static EventLoop* CheckLoopNotNull(EventLoop* loop)
{
    if(loop == nullptr)
    {
        LOG_FATAL("%s:%s:%d TcpConnection is null!", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}

TcpConnection::TcpConnection(EventLoop *loop, std::string const &name,
                             int sockfd, InetAddress const &localAddr,
                             InetAddress const &peerAddr) : 
    loop_(CheckLoopNotNull(loop)),
    name_(name),
    state_(StateE::kConnecting),
    reading_(true),
    socket_(new Socket(sockfd)),
    channel_(new Channel(loop_, sockfd)),
    localAddr_(localAddr),
    peerAddr_(peerAddr),
    highWaterMark_(64*1024*1024)
{
    channel_->setReadCallback(
        std::bind(&TcpConnection::handleRead, this, std::placeholders::_1)
    );

    channel_->setWriteCallback(
        std::bind(&TcpConnection::handleWrite, this)
    );

    channel_->setCloseCallback(
        std::bind(&TcpConnection::handleClose, this)
    );

    channel_->setErrorCallback(
        std::bind(&TcpConnection::handleError, this)
    );

    LOG_INFO("TcpConnection::ctor[%s] at fd=%d", name_.c_str(), sockfd);

    socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection()
{
    LOG_INFO("TcpConnection::dtor[%s] at fd=%d state=%d",
        name_.c_str(), channel_->fd(), (int)state_);
}


void TcpConnection::send(std::string const &msg)
{
    if(state_ == kConnected)
    {
        if(loop_->isInLoopThread())
        {
            // 在一个线程
            sendInLoop(msg.c_str(), msg.size());    
        }
        else
        {
            loop_->runInLoop(std::bind(&TcpConnection::sendInLoop, this, msg.c_str(), msg.size()));
        }
    }
}


/*
    发送数据，应用程序写的快，内核发送慢,需要把带发送数据写入缓冲区，而且设置了水位回调
*/
void TcpConnection::sendInLoop(const char *message, size_t len)
{
    ssize_t nwrote = 0;         // 本次已写字节
    size_t remaining = len;     // 剩余字节
    bool faultError = false;    // 是否发生错误

    if(state_ == kDisconnected)
    {
        LOG_ERROR("disconnected, give up writing!");
        return;
    }

    // 表示channel第一次写数据=> fd未注册写事件&&发送缓冲区可读字节为0
    if(!channel_->isWriting() && outputBuffer_.readableBytes() == 0)
    {
        nwrote = ::write(channel_->fd(), message, len); // 向内核缓冲区写数据
        if(nwrote >= 0)
        {
            remaining = len - nwrote;
            if(remaining == 0 && writeCompleteCallback_)   
            {
                // 表示数据已全部发送，调用发送完毕回调
                loop_->queueInLoop(
                    std::bind(writeCompleteCallback_, shared_from_this())
                );
            }

        }
        else // 出错
        {
            nwrote = 0;
            if(errno != EWOULDBLOCK)
            {
                LOG_ERROR("TcpConnection::sendInLoop");
                if(errno == EPIPE || errno == ECONNRESET)
                {
                    faultError = true;
                }
            }
        }
    }

    /*
    下面此判断逻辑说明：
        1.当前这一次write并没有全部发送完毕,需要将剩余的数据保存到缓冲区outputBuffer_中
        2.给Channel注册EPOLLOUT事件，poller发现tcp的发送缓冲区有空间，会通知sock - channel，调用writeCallback_回调
        3.就是调用TcpConnection::handleWrite方法，把发送缓冲区中的数据全部发送完为止
    */
    if(!faultError && remaining > 0)    // 这次write系统调用无错误 && 还有剩余数据待发送
    {
        /*
            如果在某次调用sendInLoop并未一次性地把数据全部发送完，会把数据存到缓冲区；
            待下一次调用sendInLoop会取到上次未读完的数据
        */
        size_t oldLen = outputBuffer_.readableBytes();  
        if(oldLen + remaining >= highWaterMark_
            && oldLen < highWaterMark_
            && highWaterMarkCallback_)  
            // 旧数据 + 此次未写数据 >= 高水位标志 && 旧数据 < 高水位标志 
            // => 意味着 此次未写数据要使写缓冲区待发送数据（缓冲区待发送数据 = 旧数据 + 此次未写数据）>=高水位标志
        {
            loop_->queueInLoop(
                std::bind(highWaterMarkCallback_, shared_from_this(), oldLen + remaining)
            );
        }
        // 将此次未写数据添加到 缓冲区
        outputBuffer_.append(static_cast<const char*>(message) + nwrote, remaining);
        if(!channel_->isWriting())
        {
            channel_->enableWriting();  // 设置EPOLLOUT事件
        }
    }
}

// 关闭连接
void TcpConnection::shutdown() 
{   
    if(state_ == kConnected)
    {
        setState(kDisconnecting);
        loop_->runInLoop(std::bind(
            &TcpConnection::shutdownInLoop, this
        ));
    }
}

// 建立连接
void TcpConnection::connectEstablished()
{
    setState(kConnected);
    channel_->tie(shared_from_this());  // 将TcpConnection绑定到Channel上
    channel_->enableReading();      // 向Poller注册EPOLLIN事件

    // 新连接建立，执行回调
    connectionCallback_(shared_from_this());
}

// 连接销毁
void TcpConnection::connectDestroyed()
{
    if(state_ == kConnected)
    {
        setState(kDisconnected);
        channel_->disableAll(); // 把channel的所以有感兴趣的事件从Poller中delete
        connectionCallback_(shared_from_this());
    }
    channel_->remove();     
}

void TcpConnection::handleRead(Timestamp receiveTime)
{
    int saveErrno = 0;
    ssize_t n = inputBuffer_.readFd(channel_->fd(), &saveErrno);
    if(n > 0)
    {
        // 已建立连接的用户，有可读事件发生，
        messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
    }
    else if(n == 0)
    {
        // 客户端断开连接
        handleClose();
    }
    else
    {
        errno = saveErrno;
        LOG_ERROR("TcpConnection::handleRead");
        handleError();
    }
}

void TcpConnection::handleWrite()
{
    if(channel_->isWriting())
    {
        int saveErrno = 0;
        ssize_t n = outputBuffer_.writeFd(channel_->fd(), &saveErrno);
        if(n > 0)
        {
            outputBuffer_.retrieve(n);
            if(outputBuffer_.readableBytes() == 0) // 表示已发送完 
            {
                channel_->disableWriting();
                // 消息发送完之后的回调函数
                if(writeCompleteCallback_)
                {
                    loop_->queueInLoop(
                        std::bind(writeCompleteCallback_, shared_from_this())
                    );
                }
                /*
                    为什么要判断连接状态？
                    1.保证在断开连接前，所有待发送的数据都已发送完毕。
                    2.实现优雅关闭（）
                */
                if(state_ == kDisconnecting)
                {
                    shutdownInLoop();
                }
            }
        }
        else
        {
            LOG_ERROR("TcpConnection::handleWrite");
        }
    }
    else
    {
        LOG_ERROR("TcpConnection fd=%d is down, no more writing", channel_->fd());
    }
}

void TcpConnection::handleClose() 
{
    LOG_INFO("TcpConnection::handleClose fd=%d state=%d", channel_->fd(), (int)state_);
    state_ = kDisconnected;
    channel_->disableAll();

    TcpConnectionPtr connPtr(shared_from_this());
    connectionCallback_(connPtr);   // 执行连接关闭的回调
    closeCallback_(connPtr);        // 执行关闭连接的回调
}

void TcpConnection::handleError()
{
    int optval;
    socklen_t optlen = sizeof optval;
    int err;
    if(::getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
    {
        err = errno;
    }
    else
    {
        err = optval;
    }
    LOG_ERROR("TcpConnection::handleError name:%s - SO_ERROR:%d", name_.c_str(), err);
}

void TcpConnection::shutdownInLoop()
{
    if(!channel_->isWriting()) // 表示写缓冲区内的数据全部发送完
    {
        socket_->shutdownWrite();// 关闭写端，触发EPOLLHUP;
        // =》channel::closeCallback_->TcpConnection::handleClose
    }
}
