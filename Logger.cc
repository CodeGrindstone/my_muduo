#include "Logger.h"

void Logger::setLogLevel(LogLevel loglevel)
{
    loglevel_ = loglevel;
}

void Logger::Log(const std::string& msg) const
{
    switch (loglevel_)
    {
    case(LogLevel::DEBUG):
        std::cout << "[DEBUG] ";
        break;
    case(LogLevel::INFO):
        std::cout << "[INFO] ";
        break;
    case(LogLevel::ERROR):
        std::cout << "[ERROR] ";
        break;
    case(LogLevel::FATAL):
        std::cout << "[FATAL] ";
        break;
    }

    std::cout << "time : " << Timestamp::now().ToString() << msg << std::endl;
}

Logger& Logger::GetInstance()
{
    static Logger logger;
    return logger;
}

