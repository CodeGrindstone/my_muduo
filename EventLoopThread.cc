#include "EventLoopThread.h"

EventLoopThread::EventLoopThread(ThreadInitCallback const &cb,
                                 std::string const &name) :
    loop_(nullptr),
    exiting_(false),
    thread_(std::bind(&EventLoopThread::threadFunc, this), name),
    mutex_(),
    cond_(),
    Callback_(cb)
{}

EventLoopThread::~EventLoopThread()
{
    exiting_ = true;
    if(loop_ != nullptr)
    {
        loop_->quit();
        thread_.join();
    }
}

/*
此函数作用：
    1. 创建一个新线程
    2. 在新线程中定义一个局部变量EventLoop（栈上）
    3. EventLoop开启循环
    4. 返回EventLoop地址
*/
EventLoop *EventLoopThread::startLoop()
{
    // 在EventLoopThread::EventLoopThread中为成员变量thread_绑定了函数对象EventLoopThread::threadFunc，但未创建新线程。
    // 在start中，创建了一个新的线程，并得到新线程的线程号
    thread_.start();

    EventLoop* loop = nullptr;

    {
        std::unique_lock<std::mutex> lock(mutex_);
        while( loop_ == nullptr)
        {
            cond_.wait(lock);
        }
        loop = loop_;
    }
    return loop;
}

// Thread::start()中使用此函数创建了新的线程，此线程就是subloop
// 在startLoop中，利用条件变量等待此函数创建好EventLoop
void EventLoopThread::threadFunc()
{
    EventLoop loop;

    if(Callback_)
    {
        Callback_(&loop);
    }

    {
        std::unique_lock<std::mutex> lock(mutex_);
        loop_ = &loop;
        cond_.notify_one();
    }

    loop.loop();
    std::unique_lock<std::mutex> lock(mutex_);
    loop_ = nullptr;
}
