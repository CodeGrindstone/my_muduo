#include "EpollPoller.h"
#include <errno.h>
#include <string.h>

/*
    表示Channel的状态
*/
const int kNew = -1;
const int kAdded = 1;
const int kDeleted = 2;

EpollPoller::EpollPoller(EventLoop *loop):
    Poller(loop),
    // EPOLL_CLOEXEC：当进程执行exec时，自动关闭该文件描述符
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


/*
功能:
    向EventLoop提供的接口，监听哪些fd发生事件
参数:
    timeout(传入参数): 超时时间
    activeChannels(传出参数): 通过fillActiveChannels函数push所有发生事件的Channel
*/
Timestamp EpollPoller::poll(int timeout, ChannelList *activeChannels)
{
    //LOG_DEBUG("poll start");
    int numEvents = epoll_wait(epollfd_, &*events_.begin(), events_.size(), timeout);
    Timestamp now(Timestamp::now());
    int saveError = errno;
    if(numEvents > 0)
    {
        LOG_DEBUG("%d Events happened", numEvents);
        fillActiveChannels(numEvents, activeChannels);
        if(numEvents == events_.size())
        {
            events_.resize(2 * events_.size());
        }
    }
    else if(numEvents == 0) 
    {
        //LOG_DEBUG("%s timeout!", __FUNCTION__);
    }
    else
    {
        if(saveError != EINTR)
        {
            errno = saveError;
            LOG_ERROR("EpollPoller::poll() err!");
        }
    }
    return now;
}

/*
功能: 
    向EventLoop提供接口，修改Channel所注册的事件
*/
void EpollPoller::updateChannel(Channel* channel)
{
    const int index = channel->index(); 
    LOG_INFO("EpollPoller::%s => fd=%d events=%d index=%d", __FUNCTION__, channel->fd(), channel->events(), channel->index());
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

/*
向EventLoop提供的接口，删除Channel
*/
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
/*
功能:
    为Channel封装的文件描述符和Event注册进epoll的实施动作
参数:
    operation:
        1) EPOLL_CTL_ADD
        2) EPOLL_CTL_MOD
        3) EPOLL_CTL_DEL
*/
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


/*
功能:
    1.设置所有Channel的就绪事件Channel->revents
    2.向ChannelList中push发生事件的Channel 
*/
void EpollPoller::fillActiveChannels(int numEvents,
                                     ChannelList *activeChannel)
{
    for(int i = 0; i < numEvents; i++)
    {
        Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
        channel->set_revents(events_[i].events);
        activeChannel->push_back(channel);
    }
}
