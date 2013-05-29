#ifndef BASIC_SERVER_ENTITIES_OBJECT_H
#define BASIC_SERVER_ENTITIES_OBJECT_H

#include <algorithm>
#include <list>

#include "Poco/Poco.h"
#include "Poco/SharedPtr.h"

#include "defines.h"
#include "MotionMaster.h"

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

class Player;
class Creature;
class Character;
class Grid;
class Client;

class Object
{
public:
    Object(Client* client = NULL);
    virtual ~Object();
    
    bool update(const Poco::UInt32 diff);
    void Despawn();

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

    inline Vector2D& GetPosition()
    {
        return _position;
    }

    inline void Relocate(Vector2D position)
    {
        _position = position;
    }

    inline Grid* GetGrid()
    {
        return _grid;
    }

    inline void SetGrid(Grid* grid)
    {
        _grid = grid;
    }

    inline bool IsOnGrid()
    {
        return _grid != NULL;
    }

    inline Client* getClient()
    {
        return _client;
    }
    
    inline Poco::UInt32 getLastUpdate(Poco::UInt32 update)
    {
        Poco::UInt32 r = _lastUpdate;
        _lastUpdate = update;
        return r;
    }

    MotionMaster motionMaster;

    Player* ToPlayer();
    Creature* ToCreature();
    Character* ToCharacter();
    Object* ToObject();    

protected:
    GuidsSet _objectsInSight;

private:
    Poco::UInt64 _GUID;
    Poco::UInt64 _flags[MAX_FLAGS_TYPES];
    Poco::UInt32 _lastUpdate;
    Vector2D _position;
    Grid* _grid;
    Client* _client;
};

#endif