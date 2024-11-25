#pragma once

#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>

class InetAddress
{
public:
    InetAddress(u_int16_t port,std::string ip = "127.0.0.1");
    explicit InetAddress(const sockaddr_in& addr) : addr_(addr){}

    std::string toIp() const;
    std::string toIpPort() const;
    u_int16_t toPort() const;

    const sockaddr* getSockAddr() const { return (sockaddr*)&addr_; }
private:
    struct sockaddr_in addr_; 
};