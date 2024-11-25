#include "Poller.h"
#include <stdlib.h>

Poller* Poller::newDefaultPoller(Eventloop* loop)
{
    if(::getenv("MUDUO_USE_POLL")){
        return nullptr;
    }
    return nullptr;
}