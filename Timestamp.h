#pragma once

#include <unistd.h>
#include <iostream>
#include <string>
#include <time.h>

class Timestamp
{
public:
    Timestamp() : microSecondsSinceEpoch_(0) {}

    explicit Timestamp(long microSecondsSinceEpoch) : microSecondsSinceEpoch_(microSecondsSinceEpoch) {}

    ~Timestamp() = default;

    static Timestamp now(){
        time_t tm = time(NULL);
        return Timestamp(tm);
    }

    std::string ToString(){
        struct tm* localtm = localtime(&microSecondsSinceEpoch_);
        char buf[128] = {0};
        snprintf(buf, 128, "%04d/%02d/%02d %02d:%02d:%02d ", 
            localtm->tm_year + 1900, 
            localtm->tm_mon  + 1,
            localtm->tm_mday,
            localtm->tm_hour,
            localtm->tm_min,
            localtm->tm_sec);
        return std::string(buf);
    }
private:
    time_t microSecondsSinceEpoch_;
};