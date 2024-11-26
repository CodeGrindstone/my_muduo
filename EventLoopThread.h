#pragma once

#include "noncopyable.h"
#include "Thread.h"
#include "Eventloop.h"
#include <condition_variable>

class EventLoopThread : noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    explicit EventLoopThread(const ThreadInitCallback& cb, 
                            const std::string& name = std::string());
    ~EventLoopThread();

    EventLoop* startLoop();
private:
    void threadFunc();

private:
    EventLoop* loop_;
    bool exiting_;
    Thread thread_;
    std::mutex mutex_;
    std::condition_variable cond_;
    ThreadInitCallback Callback_;
};