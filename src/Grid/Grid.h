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
class Sector;

class Grid
{
    friend class Sector;

private:
    typedef rde::hash_map<Poco::UInt16 /*hash*/, Sector*> TypeSectorsMap;

public:
    typedef std::list<Grid*> GridsList;

    Grid(Poco::UInt16 x, Poco::UInt16 y);
    ~Grid();
    bool update(Poco::UInt64 diff);

    Sector* getOrLoadSector(Poco::UInt16 hash);

    bool addObject(SharedPtr<Object> object);
    void removeObject(SharedPtr<Object> object);

    GridsList findNearGrids(SharedPtr<Object> object);
    
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
    inline void onPlayerErased()
    {
        _playersCount--;
    }

    inline void onPlayerAdded()
    {
        _playersCount++;
    }

    Sector* getOrLoadSector_i(Poco::UInt16 hash);
    
public:
    static Poco::UInt8 LOSRange;
    static Poco::UInt8 AggroRange;
    static Poco::UInt32 GridRemove;

private:
    TypeSectorsMap _sectors;
    Poco::UInt32 _playersCount;
    Poco::Mutex _mutex;
    Timestamp _forceLoad;
    Poco::UInt16 _x;
    Poco::UInt16 _y;
};

#endif