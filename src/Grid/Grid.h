#ifndef GAMESERVER_GRID_H
#define GAMESERVER_GRID_H

#include "defines.h"

//@ Poco includes
#include "Poco/SharedPtr.h"

//@ List and Hash Map
#include <list>
#include "hash_map.h"
#include "stack_allocator.h"

using Poco::SharedPtr;

class Object;

class Grid
{
private:
    typedef rde::hash_map<Poco::UInt64 /*guid*/, Poco::SharedPtr<Object>> ObjectMap;

public:
    typedef std::list<Grid*> GridsList;

    Grid(Poco::UInt16 x, Poco::UInt16 y);
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

public:
    static Poco::UInt8 losRange;

private:
    ObjectMap _objects;
    Poco::UInt32 _playersCount;
    Poco::UInt32 _forceLoad;
    Poco::UInt16 _x;
    Poco::UInt16 _y;
};

#endif