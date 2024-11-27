#include "Socket.h"

Socket::~Socket()
{
    ::close(sockfd_);
}

void Socket::bindAddress(InetAddress const &localaddr)
{
    if( 0 != ::bind(sockfd_, localaddr.getSockAddr(), sizeof(sockaddr)))
    {
        LOG_FATAL("bind sockfd:%d fail", sockfd_);
    }
}

void Socket::listen()
{
    if(0 != ::listen(sockfd_, 1024))
    {
        LOG_FATAL("listen sockfd:%d fail", sockfd_);
    }
}

int Socket::accpet(InetAddress *peeraddr)
{
    sockaddr_in addr; 
    socklen_t len;
    bzero(&addr, sizeof addr);
    int connfd = accept(sockfd_, (sockaddr*)&addr, &len);
    if(connfd >= 0)
    {
        peeraddr->setSockAddr(addr);
    }
    return connfd;
}

void Socket::shutdownWrite()
{
    if(0 != ::shutdown(sockfd_, SHUT_WR))
    {
        LOG_ERROR("shutdownWrite error ");
    }
}


void Socket::setTcpNoDelay(bool on)
{
  int optval = on ? 1 : 0;
  ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY,
               &optval, static_cast<socklen_t>(sizeof optval));
  // FIXME CHECK
}

void Socket::setReuseAddr(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR,
                &optval, static_cast<socklen_t>(sizeof optval));
  // FIXME CHECK
}

void Socket::setReusePort(bool on)
{
#ifdef SO_REUSEPORT
    int optval = on ? 1 : 0;
    int ret = ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT,
                         &optval, static_cast<socklen_t>(sizeof optval));
    if (ret < 0 && on)
    {
        LOG_ERROR("SO_REUSEPORT failed.");
    }
#else
    if (on)
    {
        LOG_ERROR << "SO_REUSEPORT is not supported.";
    }
#endif
}

void Socket::setKeepAlive(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE,
                &optval, static_cast<socklen_t>(sizeof optval));
    // FIXME CHECK
}