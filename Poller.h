#pragma once

#include "noncopyable.h"
#include "Timestamp.h"
#include "Channel.h"
#include <vector>
#include <unordered_map>

class EventLoop;
class Poller : noncopyable
{
public:
    using ChannelList = std::vector<Channel*>;

    Poller(EventLoop*);
    virtual ~Poller() = default;

    // 给所有IO复用提供统一的接口
    virtual Timestamp poll(int timeout, ChannelList* activeChannels) = 0;
    virtual void updateChannel(Channel*) = 0;
    virtual void removeChannel(Channel*) = 0;
    bool hasChannel(Channel* channel) const;
    // Eventloop可以通过该接口获取默认的IO复用的对象
    static Poller* newDefaultPoller(EventLoop* loop);
protected:
    using ChannelMap = std::unordered_map<int, Channel*>; 
    ChannelMap channels_;
private:
    EventLoop* ownerLoop_;
};