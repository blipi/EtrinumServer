#include "ObjectManager.h"
#include "Object.h"
#include "Creature.h"
#include "Player.h"
#include "debugging.h"

Poco::UInt32 ObjectManager::MAX_GUID = std::numeric_limits<Poco::UInt32>::max();

ObjectManager::ObjectManager()
{
    _playersGUID = 1;
    _creaturesGUID = 1;
    _itemsGUID = 1;

    _players.set_empty_key(NULL);
    _players.set_deleted_key(std::numeric_limits<Poco::UInt32>::max());

    _creatures.set_empty_key(NULL);
    _creatures.set_deleted_key(std::numeric_limits<Poco::UInt32>::max());
    
    _items.set_empty_key(NULL);
    _items.set_deleted_key(std::numeric_limits<Poco::UInt32>::max());    
}

Object* ObjectManager::create(Poco::UInt32 highGUID)
{
    Object* object = NULL;
    
    if (Poco::UInt32 lowGUID = newGUID(HIGH_GUID_PLAYER))
    {
        switch (highGUID)
        {
            case HIGH_GUID_CREATURE:
                object = new Creature();
                _creatures.insert(ObjectInserter(lowGUID, object));
                break;

            default:
                return NULL;
        }

        object->SetGUID(MAKE_GUID(highGUID, lowGUID));
    }

    return object;
}

Object* ObjectManager::createPlayer(std::string name, Client* client)
{
    Object* object = NULL;

    if (Poco::UInt32 lowGUID = newGUID(HIGH_GUID_PLAYER))
    {
        object = new Player(name, client);
        object->SetGUID(MAKE_GUID(HIGH_GUID_PLAYER, lowGUID));

        _players.insert(ObjectInserter(lowGUID, object));
    }

    return object;
}

Object* ObjectManager::getObject(Poco::UInt64 GUID)
{
    ObjectsMap::iterator itr;

    switch (HIGUID(GUID))
    {
        case HIGH_GUID_PLAYER:
            if ((itr = _players.find(LOGUID(GUID))) != _players.end())
                return itr->second;
            break;

        case HIGH_GUID_CREATURE:
            if ((itr = _creatures.find(LOGUID(GUID))) != _creatures.end())
                return itr->second;
            break;

        case HIGH_GUID_ITEM:
            break;
    }

    return NULL;
}

void ObjectManager::removeObject(Poco::UInt64 GUID)
{
    switch (HIGUID(GUID))
    {
        case HIGH_GUID_PLAYER:
            _players.erase(LOGUID(GUID));
            break;

        case HIGH_GUID_CREATURE:
            _creatures.erase(LOGUID(GUID));
            break;

        case HIGH_GUID_ITEM:
            break;
    }
}

Poco::UInt32 ObjectManager::newGUID(Poco::UInt32 highGUID)
{
    Poco::UInt32 GUID = MAX_GUID;
    Poco::UInt32* puller = NULL;

    switch (highGUID)
    {
        case HIGH_GUID_PLAYER:
            puller = &_playersGUID;
            break;
        
        case HIGH_GUID_CREATURE:
            puller = &_creaturesGUID;
            break;

        case HIGH_GUID_ITEM:
            puller = &_itemsGUID;
            break;
    }

    if (puller)
    {
        if (*puller != MAX_GUID)
        {
            GUID = *puller;
            *puller = *puller + 1;
        }
    }

    return GUID;
}
