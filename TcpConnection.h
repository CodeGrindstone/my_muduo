#pragma once


#include "InetAddress.h"    
#include "Callbacks.h"
#include "Buffer.h"
#include "Socket.h"
#include "Timestamp.h"
#include "noncopyable.h"
#include <memory>
#include <atomic>
#include <string>

class Channel;
class EventLoop;

/*
    TcpServer => Acceptor => 有一个新用户连接，通过accept函数拿到connfd =>
    TcpConnection 设置回调 => Channel => Poller => Channel的回调

    描述已连接客户端与服务端的联系
*/
class TcpConnection : noncopyable, public std::enable_shared_from_this<TcpConnection>
{
public:
    TcpConnection(EventLoop* loop, 
                const std::string& name,
                int sockfd,
                const InetAddress& localAddr,
                const InetAddress& peerAddr);

    ~TcpConnection();

    EventLoop* getLoop() const { return loop_; }
    const std::string& name() const { return name_; }
    const InetAddress& localAddress() const { return localAddr_; }
    const InetAddress& peerAddress() const { return peerAddr_; }

    bool conncted() const { return state_ == StateE::kConnected; }

    // 发送数据
    void send(const std::string& buf);

    // 关闭连接
    void shutdown();
    
    void setConnectionCallback(const ConnectionCallback& cb)
    { connectionCallback_ = cb; }

    void setMessageCallback(const MessageCallback& cb)
    { messageCallback_ = cb; }

    void setWriteCompleteCallback(const WriteCompleteCallback& cb)
    { writeCompleteCallback_ = cb; }

    void setHighWaterMarkCallback(const HighWaterMarkCallback& cb, size_t highWaterMark)
    { highWaterMarkCallback_ = cb; highWaterMark_ = highWaterMark; }

    void setCloseCallback(const CloseCallback& cb)
    { closeCallback_ = cb; }

    // 建立连接
    void connectEstablished();

    // 销毁连接 
    void connectDestroyed();

private:
    enum StateE
    {
        kDisconnected,
        kConnecting,
        kConnected,
        kDisconnecting
    };

    void handleRead(Timestamp receiveTime);
    void handleWrite();
    void handleClose();
    void handleError();

    void sendInLoop(const char* message, size_t len);

    void shutdownInLoop();

    void setState(StateE state) { state_ = state; }
    EventLoop* loop_;
    const std::string name_;
    std::atomic_int state_;
    bool reading_;

    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;

    const InetAddress localAddr_;
    const InetAddress peerAddr_; 

    ConnectionCallback connectionCallback_; // 有新连接的回调
    MessageCallback messageCallback_;       // 有读写消息的回调
    WriteCompleteCallback writeCompleteCallback_;    // 消息发送完成以后的回调
    CloseCallback closeCallback_;
    HighWaterMarkCallback highWaterMarkCallback_;   // 高水位回调
    
    // 高水位的值
    size_t highWaterMark_;

    Buffer inputBuffer_;
    Buffer outputBuffer_;
};