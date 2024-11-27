#pragma once

#include "noncopyable.h"
#include "Socket.h"
#include "Channel.h"
class EventLoop;

class Acceptor : noncopyable
{
public:
    using NewConnectionCallback = std::function<void(int, const InetAddress&)>;

    Acceptor(EventLoop* loop, const InetAddress& ListenAddr, bool reuseport);
    ~Acceptor();

    void setNewConnectionCallback(const NewConnectionCallback& cb)
    {
        NewConnectionCallback_ = std::move(cb);
    }

    bool listening() const { return listening_; }
    void listen();
private:
    void handleRead();

    EventLoop* loop_;
    Socket acceptSocket_;
    Channel acceptChannel;
    NewConnectionCallback NewConnectionCallback_;
    bool listening_;
};