#ifndef GAMESERVER_SECTOR_H
#define GAMESERVER_SECTOR_H

#include "defines.h"
#include "Poco/Mutex.h"
#include "Poco/SharedPtr.h"
#include "Poco/Timestamp.h"

#include <list>
#include "hash_map.h"
#include "stack_allocator.h"

#include "Position.h"

using Poco::SharedPtr;
using Poco::Timestamp;

class Object;
class Grid;
class Packet;

typedef rde::hash_map<Poco::UInt64 /*guid*/, Poco::SharedPtr<Object>> TypeObjectsMap;

class Sector
{
    friend class Sector;

private:
    enum SECTOR_STATES
    {
        SECTOR_NOT_INITIALIZED  = 0,
        SECTOR_ACTIVE,
        SECTOR_IDLE,
        SECTOR_INACTIVE
    };

public:    
    struct JoinEvent
    {
        JoinEvent(SharedPtr<Object> object, SharedPtr<Packet> packet);
        ~JoinEvent();

        SharedPtr<Object> Who;
        SharedPtr<Packet> SpawnPacket;
    };

    typedef std::list<JoinEvent*> TypeJoinEvents;

    Sector(Poco::UInt16 hash, Grid* grid);
    ~Sector();

    static Poco::UInt16 hash(Poco::UInt8 x, Poco::UInt8 y);

    bool add(SharedPtr<Object> object);
    void remove(SharedPtr<Object> object);
    void remove_i(SharedPtr<Object> object);

    bool update(Poco::UInt64 diff);

    bool hasJoinEvents();
    TypeJoinEvents* getJoinEvents();

    Poco::UInt16 hashCode();

private:
    void leave(SharedPtr<Object> who);
    void join(SharedPtr<Object> who, SharedPtr<Packet> packet);
    void visit(SharedPtr<Object> who);

    void clearJoinEvents();

    std::set<Poco::UInt16> getNearSectors(Vector2D position, Poco::UInt8 losRange);

private:
    Grid* _grid;
    Poco::UInt16 _hash;
    Poco::UInt8 _state;
    TypeObjectsMap _objects;
    TypeJoinEvents _joinEvents;
    Poco::Mutex _mutex;
};

#endif