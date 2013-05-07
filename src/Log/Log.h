#ifndef EPS_LOG_H
#define EPS_LOG_H

#include "Poco/Poco.h"
#include "Poco/SingletonHolder.h"
#include "Poco/Logger.h"
#include "Poco/Message.h"

using Poco::Logger;
using Poco::Message;

class Log
{
public:
    Log();
    static Log& instance()
    {
        static Poco::SingletonHolder<Log> sh;
        return *sh.get();
    }

    void out(Message::Priority prio, const char* fmt, ...);
    void out(Message::Priority prio, std::string msg);

    inline void setLogLevel(Message::Priority prio)
    {
        _logLevel = prio;
    }

private:
    Logger* _logger;
    Message::Priority _logLevel;
};

#define sLog Log::instance()

#endif