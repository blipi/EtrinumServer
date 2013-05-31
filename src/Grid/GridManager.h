#ifndef GAMESERVER_GRID_MANAGER_H
#define GAMESERVER_GRID_MANAGER_H

#include "Poco/TaskManager.h"
#include "Poco/Task.h"
#include "Poco/Mutex.h"

#include <queue>

class GridManager: public Poco::TaskManager
{
public:
    GridManager(Poco::UInt8 maxThreads);

    void start(Poco::Task* task);
    void queue(Poco::UInt32 pending);
    void dequeue();
    void wait();

    inline Poco::UInt8 getMaxThreads()
    {
        return _maxThreads;
    }

private:
    Poco::UInt8 _maxThreads;
    Poco::UInt32 _pendingThreads;
    Poco::UInt32 _dequeuedThreads;
    std::queue<Poco::Task*> _queue;
    Poco::Mutex _mutex;
};

#endif