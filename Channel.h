#pragma once
#include <functional>
#include <memory>
#include "noncopyable.h"

class Timestamp;
class Eventloop;
class Channel : noncopyable
{
public:
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(Timestamp)>;

    Channel(Eventloop* eventloop, int fd);
    ~Channel();

    void handleEvent(Timestamp receiveTime);

    void setReadCallbak(ReadEventCallback cb) { readCallback_ = std::move(cb); }
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

    Eventloop* ownerLoop() const { return loop_; }
    void remove();
private:
    void update();
    void handleEventWithGuard(Timestamp receiveTime);

private:
    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    Eventloop* loop_;
    const int fd_;
    int events_;
    int revents_;
    int index_;

    std::weak_ptr<void> tie_;
    bool tied_;

    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;
};