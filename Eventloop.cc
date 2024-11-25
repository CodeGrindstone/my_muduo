#include "Eventloop.h"
#include <sys/eventfd.h>
#include <unistd.h>
#include "Logger.h"

// 防止一个线程创建多个Eventloop
__thread EventLoop* t_loopInThisThread = nullptr;
// 定义默认的Poller IO复用接口的超时函数
const int kPoolTimeMs = 10000;


int createEventfd()
{
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if(evtfd< 0)
    {
        LOG_FATAL("threadId : %d create eventfd error", CurrentThread::tid());
    }
    return evtfd;
}

EventLoop::EventLoop() :
    looping_(false),
    quit_(false),
    eventHanding_(false),
    callingPendingFunctors_(false),
    threadId_(CurrentThread::tid()),
    poller_(Poller::newDefaultPoller(this)),
    wakeupFd_(createEventfd()),
    wakeupChannel_(new Channel(this, wakeupFd_)) 
{
    LOG_DEBUG("thread %d Eventloop created", CurrentThread::tid());
    if(t_loopInThisThread){
        LOG_FATAL("Another EventLoop %p exists in this thread %d.", t_loopInThisThread, threadId_);
    }else{
        t_loopInThisThread = this;
    }

    wakeupChannel_->setReadCallback(
        std::bind(&EventLoop::handleRead, this)
    );
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop()
{
    LOG_DEBUG("EventLoop %p of thread %d destructs in thread %d", 
        this, threadId_, CurrentThread::tid());
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    t_loopInThisThread = nullptr;
}

void EventLoop::loop()
{
    looping_ = true;
    quit_ = false;
    LOG_DEBUG("EventLoop %p start looping", this);

    while(!quit_)
    {
        activeChannels.clear();
        pollReturnTime_ = poller_->poll(kPoolTimeMs, &activeChannels);

        eventHanding_ = true;
        for(auto channel : activeChannels)
        {
            // Poller监听哪些channel发生事件了，上报给Event
            channel->handleEvent(pollReturnTime_);
        }
        eventHanding_ = false;
        // 执行当前EventLoop事件循环需要处理的回调操作
        /*
        
        */
        doPendingFunctor();
    }

    LOG_DEBUG("EventLoop %p stop looping", this);
    looping_ = false;
}


void EventLoop::quit()
{
    /*
    事件循环退出时机:
        1. 在自己的线程内调用
        2. 在其他线程内调用该函数
             2.1 如果EventLoop在阻塞中,wakeup可以唤醒进而退出循环
    */
    quit_ = true;

    if(!isInLoopThread())
    {
        wakeup();
    }
}

void EventLoop::runInLoop(Functor cb)
{
    /*
    让某个任务（回调函数cb）在当前线程内执行。
    主要用于线程安全和任务调度，确保任务的执行环境和EventLoop的线程一致
    */
    if(isInLoopThread())
    {
        cb();
    }
    else
    {
        queueInLoop(std::move(cb));
    }
}

void EventLoop::queueInLoop(Functor cb)
{
    /*
    将任务放入pendingFunctors:
        1. 外部线程调用queueInLoop -- !isInLoopThread
        2. 本线程正在执行回调函数 -- callingPendingFunctors_
            2.1 如果不添加这个判断条件，很有可能导致EventLoop一直阻塞
    */
    {
        std::lock_guard<std::mutex> mutex(mutex_);
        pendingFunctors_.emplace_back(std::move(cb));
    }
    if (!isInLoopThread() || callingPendingFunctors_)
    {
        wakeup();
    }
}

void EventLoop::updateChannel(Channel *channel)
{
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel *channel)
{
    poller_->removeChannel(channel);
}

void EventLoop::handleRead()
{
    u_int64_t one = 1;
    size_t n = ::read(wakeupFd_, &one, sizeof one);
    if(n != sizeof one)
    {
        LOG_ERROR("EventLoop::handleRead() reads %ld bytes instead of 8", n);
    }
}

void EventLoop::doPendingFunctor()
{
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;

    {
        std::lock_guard<std::mutex> mutex(mutex_);
        functors.swap(pendingFunctors_);
    }

    for(const Functor& functor : functors)
    {
        functor();
    }

    callingPendingFunctors_ = false;
}

void EventLoop::wakeup()
{
    u_int64_t one = 1;
    size_t n = ::write(wakeupFd_, &one, sizeof one);
    if(n != sizeof one)
    {
        LOG_ERROR("EventLoop::wakeup() writes %ld bytes instead of 8", n);
    }
}
