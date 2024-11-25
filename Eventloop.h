#pragma once

#include "noncopyable.h"
#include "EpollPoller.h"
#include "Timestamp.h"
#include "Channel.h"
#include "CurrentThread.h"
#include <functional>
#include <mutex>
#include <memory>
#include <atomic>

/*
        EventLoop类的两大组件
    Chanel              Poller
*/
class EventLoop : noncopyable
{
public:
    using Functor = std::function<void()>;

    EventLoop();
    ~EventLoop();

    // 开启事件循环
    void loop();
    // 退出事件循环
    void quit();

    Timestamp pollReturnTime() const { return pollReturnTime_; }
    // 在当前loop中执行cb
    void runInLoop(Functor cb);
    void queueInLoop(Functor cb);
    // 唤醒loop所在的线程
    void wakeup();

    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);
    // 判断当前线程是否为创建EventLoop中的线程
    bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }

private:
    // WakeupFd的回调函数
    void handleRead();
    void doPendingFunctor();

private:
    using ChannelList = std::vector<Channel*>;

    std::atomic_bool looping_;  // 是否在epoll_wait循环中
    std::atomic_bool quit_;     // 是否退出
    std::atomic_bool eventHanding_;// 是否在处理注册的回调函数
    std::atomic_bool callingPendingFunctors_; /* atomic */
    const pid_t threadId_;   // 用来标识此EventLoop的线程号
    Timestamp pollReturnTime_;// poller返回发生事件的时间点
    std::unique_ptr<Poller> poller_;// EventLoop绑定的poller

    // mainloop与subloop的通信方式, 在subloop中监听这个文件描述符，如果读事件就绪，代表mainloop派发了一个新的Channel 
    int wakeupFd_;  
    // 监听wakeupFd的Channel
    std::unique_ptr<Channel> wakeupChannel_;

    // epoll_wait中返回的活跃的Channel集合
    ChannelList activeChannels;

    std::mutex mutex_; // 保护pendingFunctors的线程安全
    std::vector<Functor> pendingFunctors_; // 存储此loop需要执行的所有回调函数

};