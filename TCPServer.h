#pragma once

#include "noncopyable.h"
#include "EventLoopThreadPool.h"
#include "Acceptor.h"
#include "Callbacks.h"
#include "TcpConnection.h"
#include <unordered_map>
/*
    用户使用muduo编写服务器程序
*/
class TcpServer : noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    enum Option
    {
        KnoReusePort,
        kReusePort
    };

    TcpServer(EventLoop* loop,
                const InetAddress& listenAddr,
                const std::string& argName,
                Option option = KnoReusePort);
    
    ~TcpServer();

    void setThreadInitCallback(const ThreadInitCallback& cb) { threadInitCallback_ = std::move(cb); }
    void setConnectionCallback(const ConnectionCallback& cb) { connectionCallback_ = std::move(cb); }
    void setMessageCallback(const MessageCallback& cb) { messageCallback_ = std::move(cb); }
    void setWriteCallback(const WriteCompleteCallback& cb) { writeCompleteCallback_ = std::move(cb); }

    // 设置subloop数量
    void setThreadNum(int numThreads);

    // 开启监听
    void start();

private:
    void NewConnection(int sockfd, const InetAddress& peerAddr);
    void removeConnection(const TcpConnectionPtr& conn);
    void removeConnectionInLoop(const TcpConnectionPtr& conn);

private:
    using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>;

    EventLoop* loop_;   //用户定义的mainloop
    const std::string ipPort_;
    const std::string name_;
    std::unique_ptr<Acceptor> accpetor_;
    std::shared_ptr<EventLoopThreadPool> threadPool_;   // one loop per thread

    ConnectionCallback connectionCallback_; // 有新连接的回调
    MessageCallback messageCallback_;       // 有读写消息的回调
    WriteCompleteCallback writeCompleteCallback_;    // 消息发送完成以后的回调

    ThreadInitCallback threadInitCallback_; // loop线程初始化回调
    std::atomic_int  started_;

    int nextConnId_;
    ConnectionMap connections_;
};