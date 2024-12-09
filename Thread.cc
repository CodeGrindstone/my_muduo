#include "Thread.h"
#include "CurrentThread.h"
#include "semaphore.h"

std::atomic_int Thread::numCreated_(0);

Thread::Thread(ThreadFunc func, std::string const &name) :
    started_(false),
    joined_(false),
    tid_(0),
    func_(std::move(func)),
    name_(name)
{
    setDefaultName();
}

Thread::~Thread()
{
    if(started_ && !joined_)
    {
        thread_->detach();
    }
}

void Thread::start()
{
    started_ = true;

    sem_t sem;
    sem_init(&sem, false, 0);     

    // 创建一个线程，执行EventLoopThread::threadFunc
    // 为了获取新线程的线程号，通过信号量机制同步线程
    thread_ = std::shared_ptr<std::thread>(new std::thread([&](){
        tid_ = CurrentThread::tid();

        sem_post(&sem);

        func_();
    }));
    
    sem_wait(&sem);    
}

void Thread::join() 
{
    joined_ = true;
    thread_->join();
}

void Thread::setDefaultName()
{
    int num = ++numCreated_;
    if(!name_.empty())
    {
        char buf[32];
        snprintf(buf, sizeof buf, "Thread%d", num);
        name_ = buf;
    }
}
