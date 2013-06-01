#include "GridManager.h"

#include "Poco/Thread.h"

GridManager::GridManager(Poco::UInt8 maxThreads):
    TaskManager(),
    _maxThreads(maxThreads),
    _pendingThreads(0), _dequeuedThreads(0)
{
}

void GridManager::start(Poco::Task* task)
{
    if (count() < _maxThreads)
        TaskManager::start(task);
    else
    {
        Poco::Mutex::ScopedLock lock(_mutex);
        _queue.push(task);
    }
}

void GridManager::queue(Poco::UInt32 queue)
{
    _pendingThreads = queue;
}

void GridManager::dequeue()
{
    _dequeuedThreads++;
    Poco::Mutex::ScopedLock lock(_mutex);

    if (!_queue.empty())
    {
        Poco::Task* task = _queue.front();
        _queue.pop();
        TaskManager::start(task);
    }
}

void GridManager::wait()
{
    while (_pendingThreads != _dequeuedThreads)
        Poco::Thread::sleep(2);

    _pendingThreads = 0;
    _dequeuedThreads = 0;
}
