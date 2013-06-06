#ifndef GAMESERVER_SECTOR_H
#define GAMESERVER_SECTOR_H

#include "defines.h"
#include "Poco/Mutex.h"
#include "Poco/SharedPtr.h"
#include "Poco/Timestamp.h"

#include <list>
#include <vector>
#include "hash_map.h"
#include "stack_allocator.h"

#include "Position.h"

using Poco::SharedPtr;
using Poco::Timestamp;

class Object;
class Grid;
class Packet;

typedef rde::hash_map<Poco::UInt64 /*guid*/, Poco::SharedPtr<Object>> TypeObjectsMap;
typedef std::vector<Poco::UInt16> TypeHashList;

enum SECTOR_EVENTS
{
    EVENT_BROADCAST_JOIN,
    EVENT_BROADCAST_LEAVE,
    EVENT_BROADCAST_WALK
};

class Sector
{
    friend class Sector;

public:    
    struct SectorEvent
    {
        SectorEvent(SharedPtr<Object> object, SharedPtr<Packet> packet, Poco::UInt8 eventType);
        ~SectorEvent();

        SharedPtr<Object> Who;
        SharedPtr<Packet> EventPacket;
        Poco::UInt8 EventType;
    };

    typedef std::vector<SectorEvent*> TypeSectorEvents;

    Sector(Poco::UInt16 hash, Grid* grid);
    ~Sector();

    static Poco::UInt16 hash(Poco::UInt8 x, Poco::UInt8 y);

    bool add(SharedPtr<Object> object);
    void remove(SharedPtr<Object> object);
    void remove_i(SharedPtr<Object> object);

    bool update(Poco::UInt64 diff);

    bool hasEvents();
    TypeSectorEvents* getEvents();

    Poco::UInt16 hashCode();

private:
    void join(SharedPtr<Object> who, SharedPtr<Packet> packet);
    void leave(SharedPtr<Object> who, SharedPtr<Packet> packet);
    void visit(SharedPtr<Object> who);

    void clearJoinEvents();

    TypeHashList getNearSectors();

private:
    Grid* _grid;
    Poco::UInt16 _hash;
    TypeHashList _sectors;
    TypeObjectsMap _objects;
    TypeSectorEvents _sectorEvents;
    Poco::Mutex _mutex;
};

#endif