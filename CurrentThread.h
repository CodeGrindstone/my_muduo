#pragma once
#include <unistd.h>
#include <sys/syscall.h>
namespace CurrentThread
{
    /*
        每个线程都有一个唯一的ID
    */
    extern thread_local int t_cachedTid;

    void cacheTid();

    inline int tid()
    {
        if(__builtin_expect(t_cachedTid == 0, 0)){
            cacheTid();
        }
        return t_cachedTid;
    }
}