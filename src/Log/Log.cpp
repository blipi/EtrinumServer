#include "Log.h"

#include "Poco/FileChannel.h"
#include "Poco/ConsoleChannel.h"
#include "Poco/AsyncChannel.h"
#include "Poco/SplitterChannel.h"
#include "Poco/AutoPtr.h"

#include <stdarg.h>

using Poco::FileChannel;
using Poco::ConsoleChannel;
using Poco::AsyncChannel;
using Poco::SplitterChannel;
using Poco::AutoPtr;

Log::Log():
    _logger(Logger::get("Server"))
{
    _logger.setLevel(Message::PRIO_INFORMATION);

    AutoPtr<FileChannel> channel(new FileChannel);
    AutoPtr<ConsoleChannel> cons(new ConsoleChannel);

    AutoPtr<AsyncChannel> asyncFile(new AsyncChannel(channel));
    AutoPtr<AsyncChannel> asyncConsole(new AsyncChannel(cons));

    AutoPtr<SplitterChannel> splitter(new SplitterChannel);
    
    channel->setProperty("path", "Server.log");
    channel->setProperty("rotation", "10 M");
    channel->setProperty("archive", "timestamp");
    channel->setProperty("rotateOnOpen", "true");
    
    splitter->addChannel(asyncFile);
    splitter->addChannel(asyncConsole);

    _logger.setChannel(splitter);
}

Log::~Log()
{
}

void Log::out(Message::Priority prio, const char* fmt, ...)
{
    if (prio > _logger.getLevel())
        return;

    int n, size=100;
    bool b=false;
    va_list marker;
    std::string s;

    while (!b)
    {
        s.resize(size);
        va_start(marker, fmt);
        n = vsnprintf((char*)s.c_str(), size, fmt, marker);
        va_end(marker);
        if ((n>0) && ((b=(n<size))==true)) s.resize(n); else size*=2;
    }

    out(prio, s);
}

void Log::out(Message::Priority prio, std::string msg)
{
    if (prio > _logger.getLevel())
        return;

    switch (prio)
    {
        case Message::PRIO_CRITICAL:
            _logger.critical(msg);
            break;

        case Message::PRIO_DEBUG:
            _logger.debug(msg);
            break;

        case Message::PRIO_ERROR:
            _logger.error(msg);
            break;

        case Message::PRIO_FATAL:
            _logger.fatal(msg);
            break;

        case Message::PRIO_INFORMATION:
            _logger.information(msg);
            break;

        case Message::PRIO_NOTICE:
            _logger.notice(msg);
            break;

        case Message::PRIO_TRACE:
            _logger.trace(msg);
            break;

        case Message::PRIO_WARNING:
            _logger.trace(msg);
            break;
    }
}
