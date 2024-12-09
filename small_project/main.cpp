#include <iostream>
#include "ChatServer.h"

int main()
{
    int port = 10086;
    muduo::net::InetAddress listenAddr(port);
    muduo::net::EventLoop loop;

    ChatServer server(&loop, listenAddr);
    server.start();
    loop.loop();
    return 0;
}
