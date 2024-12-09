#include "TCPServer.h"
#include <strings.h>

static EventLoop* CheckLoopNotNull(EventLoop* loop)
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

TcpServer::~TcpServer()
{
    for(auto& item : connections_)
    {
        // 临时保存TcpConnectionPtr智能指针，防止在reset后被析构
        TcpConnectionPtr conn(item.second);
        item.second.reset();
        // 销毁conn的连接
        conn->getLoop()->runInLoop(
            std::bind(&TcpConnection::connectDestroyed, conn)
        );
    }
}

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


// 在构造TcpServer时，创建了accpetor_，并把TcpServer::NewConnection绑定到Acceptor
// 当有新连接时->在MainLoop中的Acceptor::handleRead()->TcpServer::NewConnection
void TcpServer::NewConnection(int sockfd, InetAddress const &peerAddr)
{
    // 根据轮询算法选择一个subloop来管理对应的channel
    EventLoop* ioloop = threadPool_->getNextLoop();
    char buf[64] = {0};
    snprintf(buf, sizeof(buf), "-%s#%d", ipPort_.c_str(), nextConnId_);
    ++nextConnId_;
    // TcpConnection的名字
    std::string connName = name_ + buf; 

    LOG_INFO("TcpServer::newConnecton [%s] - new connection[%s] from %s\n", 
        name_.c_str(), connName.c_str(), peerAddr.toIpPort().c_str()
    );
   // 通过sofkfd，获取其绑定的本地的ip地址和端口
   sockaddr_in local;
   ::bzero(&local, sizeof local);
   socklen_t addrlen = sizeof local;
   if(::getsockname(sockfd, (sockaddr*)&local, &addrlen) < 0)
   {
        LOG_ERROR("sockets::getLocalAddr");
   }
   InetAddress localAddr(local);

   // 根据连接成功的sockfd, 创建TcpConnection对象
   TcpConnectionPtr conn(
        new TcpConnection(
            ioloop,
            connName,
            sockfd,
            localAddr,
            peerAddr
    ));

    connections_[connName] = conn;
    // 以下回调都是用户设置给 TcpServer
    // TcpServer -> Channel -> poller => notify channel 调用回调
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    
    conn->setCloseCallback(
        std::bind(&TcpServer::removeConnection, 
        this, std::placeholders::_1)
    );

    // 执行此语句时是在mainLoop，将其入队到ioloop的任务队列，调用TcpConnection::connectionEstablish
    ioloop->runInLoop(
        std::bind(&TcpConnection::connectEstablished,
        conn)
    );

}

void TcpServer::removeConnection(TcpConnectionPtr const &conn)
{
    loop_->runInLoop(
        std::bind(&TcpServer::removeConnectionInLoop, this, conn)
    );
}

void TcpServer::removeConnectionInLoop(TcpConnectionPtr const &conn)
{
    LOG_INFO("TcpServer::removeConnection [%s] - connection %s",
        name_.c_str(), conn->name().c_str());
    connections_.erase(conn->name());
    EventLoop* ioLoop = conn->getLoop();
    ioLoop->queueInLoop(
        std::bind(&TcpConnection::connectDestroyed, conn)
    );
}
