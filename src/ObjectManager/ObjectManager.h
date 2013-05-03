#ifndef BASIC_SERVER_PULLER_H
#define BASIC_SERVER_PULLER_H

//@ Avoid MSV defining max/min and overriding numeric_limits
#if defined(_WIN32) || defined(_WIN64)
    #define NOMINMAX
#endif

#include <list>

#include "Poco/Poco.h"
#include "Poco/RWLock.h"
#include "Poco/SingletonHolder.h"

#include "defines.h"

class ObjectManager
{
public:
    ObjectManager();
    static ObjectManager& instance()
    {
        static Poco::SingletonHolder<ObjectManager> sh;
        return *sh.get();
    }

    Poco::UInt32 getNewGUID(Poco::UInt32 highGUID);

public:
    static Poco::UInt32 MAX_GUID;

private:
    std::list<Poco::UInt32> _freeGUIDs[MAX_HIGH_GUID];
    Poco::UInt32 _lastGUID[MAX_HIGH_GUID];
    Poco::RWLock _lock[MAX_HIGH_GUID];
};

#define sObjectManager ObjectManager::instance()

#endif