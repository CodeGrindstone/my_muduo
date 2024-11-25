#include "Channel.h"
#include "Timestamp.h"
#include "Eventloop.h"
#include <sys/epoll.h>

const int Channel::kNoneEvent = 0;
// EPOLLPRI: 当文件描述符上有紧急数据时，触发
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;

Channel::Channel(EventLoop *eventloop, int fd) :
  loop_(eventloop),
  fd_(fd), 
  events_(0),
  revents_(0),
  index_(-1),
  tied_(false)
{}

Channel::~Channel() {}

void Channel::handleEvent(Timestamp receiveTime)
{
  /*
  处理回调函数的向外提供的接口
  在EventLoop中调用
  */
    if(tied_){
      std::shared_ptr<void> guard = tie_.lock();
      if(guard){
        handleEventWithGuard(receiveTime);
      }

    }else{
      handleEventWithGuard(receiveTime);
    }
}

void Channel::tie(const std::shared_ptr<void>& obj)
{
    tie_ = obj;
    tied_ = true;
}

void Channel::remove()
{
  loop_->removeChannel(this);
}

// 当改变channel所表示的fd的events后，需要在poller里面更改fd相应的事件epoll_ctl
void Channel::update()
{
  loop_->updateChannel(this);
}

void Channel::handleEventWithGuard(Timestamp receiveTime)
{
  /*
  调用回调函数的实施接口
  */
    if((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN))
    {
      /*
      EPOLLHUP: 表示文件描述符挂起事件，通常表示远端关闭连接
      */
        if(closeCallback_){
           closeCallback_();
        }
    }
    
    if(revents_ & EPOLLERR){
      if(errorCallback_){
        errorCallback_();
      }
    }

    if(revents_ & (EPOLLIN | EPOLLPRI)){
      if(readCallback_){
        readCallback_(receiveTime);
      }
    }

    if(revents_ & EPOLLOUT){
      if(writeCallback_){
        writeCallback_();
      }
    }
}
