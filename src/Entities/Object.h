#ifndef BASIC_SERVER_ENTITIES_OBJECT_H
#define BASIC_SERVER_ENTITIES_OBJECT_H

#include <algorithm>
#include <list>
#include "hash_map.h"
#include "stack_allocator.h"

#include "Poco/Poco.h"
#include "Poco/SharedPtr.h"
#include "Poco/Timestamp.h"

#include "defines.h"
#include "MotionMaster.h"
#include "Position.h"

using Poco::SharedPtr;

enum OBJECT_FLAGS_TYPES
{
    FLAGS_TYPE_MOVEMENT,
    MAX_FLAGS_TYPES
};

enum OBJECT_MOVEMENT_FLAGS
{
    FLAG_MOVING = 1,
    FLAG_FLYING = 2,
};

class Character;
class Client;
class Creature;
class Grid;
class Player;
class Sector;

class Object
{
public:
    Object(Client* client = NULL);
    virtual ~Object();
    
    bool update(const Poco::UInt64 diff);

    void SetGUID(Poco::UInt64 GUID);
    Poco::UInt64 GetGUID();
    Poco::UInt32 GetHighGUID();
    Poco::UInt32 GetLowGUID();

    inline void setFlag(Poco::UInt8 flagType, Poco::UInt64 flag)
    {
         _flags[flagType] |= flag;
    }

    inline bool hasFlag(Poco::UInt8 flagType, Poco::UInt64 flag)
    {
        return (_flags[flagType] & flag) == flag;
    }

    inline void clearFlag(Poco::UInt8 flagType, Poco::UInt64 flag)
    {
        _flags[flagType] &= ~flag;
    }

    Vector2D GetPosition()
    {
        return _position;
    }

    void Relocate(Vector2D position);

    inline Grid* GetGrid()
    {
        return _grid;
    }

    inline void SetGrid(Grid* grid)
    {
        _grid = grid;
    }

    Sector* getSector()
    {
        return _sector;
    }

    void setSector(Sector* sector)
    {
        _sector = sector;
    }

    inline bool IsOnGrid()
    {
        return _grid != NULL;
    }

    inline Client* getClient()
    {
        return _client;
    }
    
    inline Poco::UInt64 getLastUpdate()
    {
        Poco::UInt64 elapsed = _lastUpdate.elapsed();
        _lastUpdate.update();
        return elapsed;
    }

    inline bool checkLOS()
    {
        if (_losTrigger.elapsed() / 1000 >= 1000)
        {
            _losTrigger.update();
            return true;
        }

        return false;
    }

    float distanceTo(Object* to);

    MotionMaster motionMaster;

    Player* ToPlayer();
    Creature* ToCreature();
    Character* ToCharacter();
    Object* ToObject();

private:
    Poco::UInt64 _GUID;
    Poco::UInt64 _flags[MAX_FLAGS_TYPES];
    Poco::Timestamp _lastUpdate;
    Poco::Timestamp _losTrigger;
    Vector2D _position;
    Grid* _grid;
    Sector* _sector;
    Client* _client;
};

#endif