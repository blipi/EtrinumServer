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

typedef rde::hash_map<Poco::UInt64 /*guid*/, Poco::SharedPtr<Object>> TypeObjectsMap;

class Grid;

class Sector
{
private:
    enum SECTOR_STATES
    {
        SECTOR_NOT_INITIALIZED  = 0,
        SECTOR_ACTIVE,
        SECTOR_IDLE,
        SECTOR_INACTIVE
    };

public:
    Sector(Poco::UInt16 hash, Grid* grid):
        _hash(hash),
        _grid(grid),
        _state(SECTOR_NOT_INITIALIZED)
    {
    }

    bool add(SharedPtr<Object> object);
    void remove(Poco::UInt64 GUID);
    void remove_i(Poco::UInt64 GUID);

    bool update(Poco::UInt64 diff);

    Poco::UInt16 hashCode();

private:
    Grid* _grid;
    Poco::UInt16 _hash;
    Poco::UInt8 _state;
    TypeObjectsMap _objects;
    Poco::Mutex _mutex;
};

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
    static Poco::UInt8 losRange;
    static Poco::UInt32 gridRemove;

private:
    TypeSectorsMap _sectors;
    Poco::UInt32 _playersCount;
    Poco::Mutex _mutex;
    Timestamp _forceLoad;
    Poco::UInt16 _x;
    Poco::UInt16 _y;
};

#endif