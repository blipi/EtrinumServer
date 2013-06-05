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

    void queue(Poco::Task* task);
    void dequeue();
    void start();
    void wait();

    inline Poco::UInt8 getMaxThreads()
    {
        return _maxThreads;
    }

private:
    Poco::Task* getQueuedTask();

private:
    Poco::UInt8 _maxThreads;
    Poco::UInt32 _pendingThreads;
    Poco::UInt32 _dequeuedThreads;
    std::queue<Poco::Task*> _queue;
    Poco::Mutex _mutex;
};

#endif