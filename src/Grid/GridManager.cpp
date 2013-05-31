#include "GridManager.h"

#include "Poco/Thread.h"

GridManager::GridManager(Poco::UInt8 maxThreads):
    TaskManager(),
    _maxThreads(maxThreads)
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

void GridManager::dequeue()
{
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
    while (count() || !_queue.empty())
        Poco::Thread::sleep(1);
}
