#pragma once

#include "noncopyable.h"
#include "Timestamp.h"
#include <string>

#define LOG_INFO(LogmsgFormat, ...) \
    do  \
    {   \
        Logger& logger = Logger::GetInstance(); \
        logger.setLogLevel(LogLevel::INFO); \
        char buf[512] = {0};    \
        snprintf(buf, 512, LogmsgFormat, ##__VA_ARGS__);\
        logger.Log(buf);\
    }   \
    while(0)

#define LOG_FATAL(LogmsgFormat, ...) \
    do  \
    {   \
        Logger& logger = Logger::GetInstance(); \
        logger.setLogLevel(LogLevel::FATAL); \
        char buf[512] = {0};    \
        snprintf(buf, 512, LogmsgFormat, ##__VA_ARGS__);\
        logger.Log(buf);\
        exit(-1);    \
    }   \
    while(0)


#define LOG_DEBUG(LogmsgFormat, ...) \
    do  \
    {   \
        Logger& logger = Logger::GetInstance(); \
        logger.setLogLevel(LogLevel::DEBUG); \
        char buf[512] = {0};    \
        snprintf(buf, 512, LogmsgFormat, ##__VA_ARGS__);\
        logger.Log(buf);\
    }   \
    while(0)


#define LOG_ERROR(LogmsgFormat, ...) \
    do  \
    {   \
        Logger& logger = Logger::GetInstance(); \
        logger.setLogLevel(LogLevel::ERROR); \
        char buf[512] = {0};    \
        snprintf(buf, 512, LogmsgFormat, ##__VA_ARGS__);\
        logger.Log(buf);\
    }   \
    while(0)

enum LogLevel{
    DEBUG,
    INFO,
    ERROR,
    FATAL
};

class Logger : noncopyable
{
public:
    ~Logger() = default;

    void setLogLevel(LogLevel loglevel);

    void Log(const std::string& msg) const;

    static Logger& GetInstance();
private:
    LogLevel loglevel_;
    Logger() = default;
};