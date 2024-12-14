#pragma once

#include "noncopyable.h"
#include <thread>
#include <memory>
#include <string>
#include <atomic>
#include <functional>

class Thread : noncopyable
{
public:
    using ThreadFunc = std::function<void()>;

    explicit Thread(ThreadFunc, const std::string& name = std::string());
    ~Thread();

    void start();
    void join();
    
    bool started() const { return started_; }
    pid_t tid() const { return tid_; }
    const std::string& name() const { return name_; }

    static int numCreated() { return numCreated_; }
private:
    void setDefaultName();
private:
    static std::atomic_int numCreated_; // 已创建线程的数量

    bool started_;  // 是否开始运行函数
    bool joined_;   // 是否join
    std::shared_ptr<std::thread> thread_;   // 线程
    pid_t tid_;  // TID
    ThreadFunc func_;   // 要运行的函数
    std::string name_;  // 线程的名字
};