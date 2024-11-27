#include "Acceptor.h"

static int CreateNonblocking()
{
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if(sockfd < 0)
    {
        LOG_FATAL("%s:%s:%d listen socket create error:%d", __FILE__, __FUNCTION__, __LINE__, errno);
    }
    return sockfd;
}

Acceptor::Acceptor(EventLoop *loop, InetAddress const &ListenAddr,
                   bool reuseport) : 
    loop_(loop),
    acceptSocket_(CreateNonblocking()),
    acceptChannel(loop_, acceptSocket_.fd()),
    listening_(false)
{
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.setReusePort(true);
    acceptSocket_.bindAddress(ListenAddr);
    acceptChannel.setReadCallback(std::bind(&Acceptor::handleRead, this));
}

Acceptor::~Acceptor()
{
    acceptChannel.disableAll();
    acceptChannel.remove();
}

void Acceptor::listen()
{
    listening_ = true;
    acceptSocket_.listen();
    acceptChannel.enableReading();
}

void Acceptor::handleRead()
{
    InetAddress peerAddr;
    int connfd = acceptSocket_.accpet(&peerAddr);
    if(connfd >= 0)
    {
        if(NewConnectionCallback_)
        {
            NewConnectionCallback_(connfd, peerAddr);
        }
        else
        {
            ::close(connfd);
        }
    }  
    else
    {
        LOG_ERROR("%s:%s:%d listen socket accept error:%d", __FILE__, __FUNCTION__, __LINE__, errno);
        if(errno == EMFILE)
        {
             LOG_ERROR("%s:%s:%d listen socket limit error:%d", __FILE__, __FUNCTION__, __LINE__, errno);
        }
    }

}
