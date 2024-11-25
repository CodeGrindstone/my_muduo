#pragma once

#include "Poller.h"
#include "Logger.h"
#include <sys/epoll.h>
/*
    epoll的接口
    1. epoll_create
    2. epoll_ctl    EPOLL_CTL_ADD EPOLL_CTL_MOD EPOLL_CTL_DEL
    3. epoll_wait 
*/
class Eventloop;
class EpollPoller : public Poller
{
public:
    EpollPoller(Eventloop* loop);
    ~EpollPoller() override;

    Timestamp poll(int timeout, ChannelList* activeChannels) override;
    void updateChannel(Channel*) override;
    void removeChannel(Channel*) override;

private:
    void update(int operation, Channel* channel);
    void fillActiveChannels(int numEvents, ChannelList* activeChannel);

private:
    using EventList = std::vector<struct epoll_event>;
    static const int kInitEventListSize = 16;

    int epollfd_;
    EventList events_;
};