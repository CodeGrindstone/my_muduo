#pragma once
#include <functional>
#include <memory>
#include "noncopyable.h"

class Timestamp;
class EventLoop;
/*
    Channel是文件描述符以及对文件描述符的事件回调函数的封装
*/
class Channel : noncopyable
{
public:
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(Timestamp)>;

    Channel(EventLoop* eventloop, int fd);
    ~Channel();

    void handleEvent(Timestamp receiveTime);

    void setReadCallback(ReadEventCallback cb) { readCallback_ = std::move(cb); }
    void setWriteCallback(EventCallback cb)   { writeCallback_ = std::move(cb); }
    void setCloseCallback(EventCallback cb)   { closeCallback_ = std::move(cb); }
    void setErrorCallback(EventCallback cb)   { errorCallback_ = std::move(cb); }
    
    // 将channel绑定到shared_ptr管理的owner对象，
    // 防止在handleEvent中销毁owner对象
    void tie(const std::shared_ptr<void>&);

    int fd() const { return fd_; }
    int events() const { return events_; }
    // 被poller调用设置已就绪的事件
    void set_revents(int revt) { revents_ = revt; }
    bool isNonEvent() const { return events_ == kNoneEvent; }

    void enableReading() { events_ |= kReadEvent; update();}
    void disableReading(){ events_ &= ~kReadEvent; update();}
    void enableWriting() { events_ |= kWriteEvent; update();}
    void disableWriting(){ events_ &= ~kWriteEvent; update();}
    void disableAll()    { events_ = kNoneEvent; update();}
    bool isReading()const{ return events_ & kReadEvent; }
    bool isWriting()const{ return events_ & kWriteEvent; }

    int index() const { return index_; }
    void set_index(int index) { index_ = index; }

    EventLoop* ownerLoop() const { return loop_; }
    void remove();
private:
    void update();
    void handleEventWithGuard(Timestamp receiveTime);

private:
    static const int kNoneEvent; // 无事件
    static const int kReadEvent; // 读时间
    static const int kWriteEvent;// 写事件

    EventLoop* loop_;   // 该Channel所绑定的EventLoop
    const int fd_;      // 封装的fd
    int events_;        // 注册事件
    int revents_;       // 就绪事件
    /*描述当前Channel的状态:
        -1 : 新添加，还未注册到epoll
        1 : 已注册并添加到epoll
        2 : 已删除 */
    int index_;

    std::weak_ptr<void> tie_;
    bool tied_;

    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;
};