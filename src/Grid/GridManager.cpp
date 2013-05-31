#include "GridManager.h"

#include "Poco/Thread.h"

GridManager::GridManager():
    TaskManager()
{
}

void GridManager::start(Poco::Task* task)
{
    // @todo MAX Threads Configurable
    if (count() < 4)
        TaskManager::start(task);
    else
    {
        Poco::Mutex::ScopedLock lock(_mutex);
        _queue.push(task);
    }
}

void GridManager::dequeue()
{
    _mutex.lock();
    if (!_queue.empty())
    {
        Poco::Task* task = _queue.front();
        _queue.pop();
        _mutex.unlock();

        start(task);
    }
    else
        _mutex.unlock();
}

void GridManager::wait()
{
    while (count())
        Poco::Thread::sleep(1);
}
