#ifndef GAMESERVER_GRID_H
#define GAMESERVER_GRID_H

#include "defines.h"

//@ Poco includes
#include "Poco/SharedPtr.h"
#include "Poco/Timestamp.h"

//@ List and Hash Map
#include <list>
#include "hash_map.h"
#include "stack_allocator.h"

using Poco::SharedPtr;
using Poco::Timestamp;

class Object;

class Sector
{
private:
    typedef rde::hash_map<Poco::UInt64 /*guid*/, Poco::SharedPtr<Object>> ObjectsMap;

    enum SECTOR_STATES
    {
        SECTOR_NOT_INITIALIZED  = 0,
        SECTOR_ACTIVE,
        SECTOR_IDLE,
        SECTOR_INACTIVE
    };

public:
    Sector(Poco::UInt16 hash):
        _hash(hash),
        _state(SECTOR_NOT_INITIALIZED)
    {

    }

    void add(SharedPtr<Object> object)
    {
        Poco::Mutex::ScopedLock lock(_mutex);
        _objects.insert(rde::make_pair(object->GetGUID(), object));
    }

    void update(Poco::UInt64 diff);

private:
    Poco::UInt16 _hash;
    Poco::UInt8 _state;
    ObjectsMap _objects;
    Poco::Mutex _mutex;
};

class Grid
{
private:
    typedef rde::hash_map<Poco::UInt64 /*guid*/, Poco::SharedPtr<Object>> ObjectMap;

public:
    typedef std::list<Grid*> GridsList;

    Grid(Poco::UInt16 x, Poco::UInt16 y);
    ~Grid();
    bool update(Poco::UInt64 diff);

    GuidsSet getObjects(Poco::UInt32 highGUID);
    SharedPtr<Object> getObject(Poco::UInt64 GUID);

    bool addObject(SharedPtr<Object> object);
    void removeObject(Poco::UInt64 GUID);

    GridsList findNearGrids(SharedPtr<Object> object);
    void visit(SharedPtr<Object> object, GuidsSet& objects);
    
    inline Poco::UInt16 GetPositionX()
    {
        return _x;
    }

    inline Poco::UInt16 GetPositionY()
    {
        return _y;
    }

    inline Poco::UInt32 hashCode()
    {
        return (_x << 16) |  _y;
    }
    
    inline bool hasPlayers()
    {
        return _playersCount > 0;
    }

    void forceLoad();
    bool isForceLoaded();

private:
    void removeObject_i(Poco::UInt64 GUID);

public:
    static Poco::UInt8 losRange;
    static Poco::UInt32 gridRemove;

private:
    ObjectMap _objects;
    Poco::UInt32 _playersCount;
    Poco::Mutex _mutex;
    Timestamp _forceLoad;
    Poco::UInt16 _x;
    Poco::UInt16 _y;
};

#endif