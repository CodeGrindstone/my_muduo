
#include "../TCPServer.h"

class EchoServer
{
public:
    EchoServer(EventLoop* loop, const InetAddress& addr, const std::string& name) :
        server_(loop, addr, name),
        loop_(loop)
    {
        // 注册回调
        server_.setConnectionCallback(
            std::bind(&EchoServer::onConnection, this, std::placeholders::_1)
        );

        server_.setMessageCallback(
            std::bind(&EchoServer::onMessage, this,
                std::placeholders::_1, std::placeholders::_2,std::placeholders::_3
            )
        );
        // 设置线程数
        server_.setThreadNum(3);
    }

    void start()
    {
        server_.start();
    }

private:
    // 建立连接或断开连接的回调
    void onConnection(const TcpConnectionPtr& conn)
    {
        if(conn->conncted())
        {
            LOG_INFO("Connection UP : %s", conn->peerAddress().toIpPort().c_str());
        }
        else
        {
            LOG_INFO("Connection DOWN : %s", conn->peerAddress().toIpPort().c_str());
        }
    }

    // 可读事件回调
    void onMessage(const TcpConnectionPtr& conn , Buffer* buf, Timestamp time)
    {
        std::string msg = buf->retrieveAllAsString();
        conn->send(msg);
        conn->shutdown();
    }
private:
    EventLoop* loop_;
    TcpServer server_;
};

int main()
{
    EventLoop loop;
    InetAddress addr(8888);
    EchoServer server(&loop, addr, "EchoServer");
    server.start();
    loop.loop();
    return 0;
}