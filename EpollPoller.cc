#include "EpollPoller.h"
#include <string.h>

const int kNew = -1;
const int kAdded = 1;
const int kDeleted = 2;

EpollPoller::EpollPoller(Eventloop *loop):
    Poller(loop),
    epollfd_(::epoll_create1(EPOLL_CLOEXEC)),
    events_(kInitEventListSize)
{
    if(epollfd_ < 0)
    {
        LOG_ERROR("EPollPoller Create epollfd error\n");
    }
}

EpollPoller::~EpollPoller()
{
    ::close(epollfd_);
}

Timestamp EpollPoller::poll(int timeout, ChannelList *activeChannels) {
    return Timestamp();
}

void EpollPoller::updateChannel(Channel* channel)
{
    const int index = channel->index(); 
    LOG_INFO("func=%s => fd=%d events=%d index=%d\n", __FUNCTION__, channel->fd(), channel->events(), channel->index());
    if(index == kNew || index == kDeleted)
    {
        int fd = channel->fd();
        if(index == kNew)
        {
            channels_[fd] = channel;
        }
        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD, channel);
    }
    else
    {
        // update existing one with EPOLL_CTL_MOD/DEL
        int fd = channel->fd();
        if(channel->isNonEvent())
        {
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(kDeleted);
        }
        else
        {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}
// 从poller中删除channel
void EpollPoller::removeChannel(Channel* channel)
{
    int fd = channel->fd();
    LOG_INFO("func=%s => fd=%d \n", __FUNCTION__, fd);
    channels_.erase(fd); 
    int index = channel->index();
    if (index == kAdded)
    {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->set_index(kNew);
}

void EpollPoller::update(int operation, Channel *channel)
{
    struct epoll_event event;
    ::bzero(&event, sizeof event);
    event.data.ptr = channel;
    event.events = channel->events();
    int fd = channel->fd();
    if(::epoll_ctl(epollfd_, operation, fd, &event) < 0){
        if(operation == EPOLL_CTL_DEL)
        {
            LOG_ERROR("epollfd : %d, fd : %d op : %d, epoll_ctl error", epollfd_, fd, EPOLL_CTL_DEL);
        }else{
            LOG_FATAL("epollfd : %d, fd : %d op : %d, epoll_ctl error", epollfd_, fd, fd);
        }
    } 
}

void EpollPoller::fillActiveChannels(int numEvents,
                                     ChannelList *activeChannel) {}
