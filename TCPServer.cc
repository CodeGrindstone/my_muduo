#include "TCPServer.h"


EventLoop* CheckLoopNotNull(EventLoop* loop)
{
    if(loop == nullptr)
    {
        LOG_FATAL("%s:%s:%d mainloop is null!", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}

TcpServer::TcpServer(EventLoop *loop, InetAddress const &listenAddr, const std::string& argName, Option option) :
    loop_(loop),
    ipPort_(listenAddr.toIpPort()),
    name_(argName),
    accpetor_(new Acceptor(loop, listenAddr, option==kReusePort)),
    threadPool_(new EventLoopThreadPool(loop, name_)),
    connectionCallback_(),
    messageCallback_(),
    started_(0),
    nextConnId_(1)
{
    // 当有新用户连接时，此函数作为回调函数
    accpetor_->setNewConnectionCallback(
        std::bind(&TcpServer::NewConnection, 
        this, std::placeholders::_1, std::placeholders::_2));
}

TcpServer::~TcpServer() {}

void TcpServer::setThreadNum(int numThreads)
{
    threadPool_->setThreadNum(numThreads);
}

void TcpServer::start()
{
    if(started_++ == 0)// 防止一个TCPServer对象被start多次
    {
        // 启动底层的线程池
        threadPool_->start(threadInitCallback_);
        loop_->runInLoop(std::bind(&Acceptor::listen, accpetor_.get()));
    }
}

void TcpServer::NewConnection(int sockfd, InetAddress const &peerAddr) {}

void TcpServer::removeConnection(TcpConnectionPtr const &conn) {}

void TcpServer::removeConnectionInLoop(TcpConnectionPtr const &conn) {}
