#include "GridManager.h"
#include "debugging.h"

#include "Poco/Thread.h"

GridManager::GridManager(Poco::UInt8 maxThreads):
    TaskManager(),
    _maxThreads(maxThreads),
    _pendingThreads(0), _dequeuedThreads(0)
{
}

GridManager::~GridManager()
{
    ASSERT(count() == 0);
}

void GridManager::queue(Poco::Task* task)
{
    _pendingThreads++;
    _queue.push(task);
}

void GridManager::start()
{
    while (count() < _maxThreads)
    {
        if (Poco::Task* task = getQueuedTask())
            TaskManager::start(task);
        else
            break;
    }
}

void GridManager::dequeue()
{
    ++_dequeuedThreads;
    
    if (Poco::Task* task = getQueuedTask())
        TaskManager::start(task);
}

void GridManager::wait()
{
    while (_pendingThreads != _dequeuedThreads)
        Poco::Thread::sleep(2);

    _pendingThreads = 0;
    _dequeuedThreads = 0;
}

Poco::Task* GridManager::getQueuedTask()
{
    Poco::Mutex::ScopedLock lock(_mutex);

    if (_queue.empty())
        return NULL;

    Poco::Task* task = _queue.front();
    _queue.pop();
    return task;
}
