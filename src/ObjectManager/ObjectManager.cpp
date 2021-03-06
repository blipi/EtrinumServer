#include "ObjectManager.h"
#include "Object.h"
#include "Creature.h"
#include "Player.h"
#include "debugging.h"

#include <limits>

Poco::UInt32 ObjectManager::MAX_GUID = std::numeric_limits<Poco::UInt32>::max();

ObjectManager::ObjectManager()
{
    _playersGUID = 1;
    _creaturesGUID = 1;
    _itemsGUID = 1;   
}

SharedPtr<Object> ObjectManager::create(Poco::UInt32 highGUID)
{
    SharedPtr<Object> object = NULL;
    
    Poco::UInt32 lowGUID = newGUID(highGUID);
    if (lowGUID != MAX_GUID)
    {
        switch (highGUID)
        {
            case HIGH_GUID_CREATURE:
                object.assign(new Creature());
                _creatures.insert(rde::make_pair(lowGUID, object));
                break;

            default:
                return NULL;
        }

        object->SetGUID(MAKE_GUID(highGUID, lowGUID));
    }

    return object;
}

SharedPtr<Player> ObjectManager::createPlayer(std::string name, Client* client)
{
    SharedPtr<Object> object = NULL;

    Poco::UInt32 lowGUID = newGUID(HIGH_GUID_PLAYER);
    if (lowGUID != MAX_GUID)
    {
        object.assign(new Player(name, client));
        object->SetGUID(MAKE_GUID(HIGH_GUID_PLAYER, lowGUID));

        _players.insert(rde::make_pair(lowGUID, object));
    }

    return object.cast<Player>();
}

SharedPtr<Object> ObjectManager::getObject(Poco::UInt64 GUID)
{
    switch (HIGUID(GUID))
    {
        case HIGH_GUID_PLAYER:
            {
                ObjectsMap::iterator itr = _players.find(LOGUID(GUID));
                if (itr != _players.end())
                    return itr->second;
            }
            break;

        case HIGH_GUID_CREATURE:
            {
                ObjectsMap::iterator itr = _creatures.find(LOGUID(GUID));
                if (itr != _creatures.end())
                    return itr->second;
            }
            break;

        case HIGH_GUID_ITEM:
            break;
    }

    return NULL;
}

void ObjectManager::removeObject(Poco::UInt64 GUID)
{
    // An object removed from here should also be removed from grid
    switch (HIGUID(GUID))
    {
        case HIGH_GUID_PLAYER:
            //@todo: DB set guid = 0
            _players.erase(LOGUID(GUID));
            _freePlayers.insert(LOGUID(GUID));
            break;

        case HIGH_GUID_CREATURE:
            _creatures.erase(LOGUID(GUID));
            _freeCreatures.insert(LOGUID(GUID));
            break;

        case HIGH_GUID_ITEM:
            break;
    }
}

Poco::UInt32 ObjectManager::newGUID(Poco::UInt32 highGUID)
{
    Poco::UInt32 GUID = MAX_GUID;

    switch (highGUID)
    {
        case HIGH_GUID_PLAYER:
            _playersMutex.lock();
            if (!_freePlayers.empty())
            {
                GUID = *(_freePlayers.begin());
                _freePlayers.erase(GUID);
            }
            else if (_playersGUID != MAX_GUID)
            {
                GUID = _playersGUID;
                _playersGUID++;
            }
            _playersMutex.unlock();
            break;
        
        case HIGH_GUID_CREATURE:
            _creaturesMutex.lock();
            if (!_freeCreatures.empty())
            {
                GUID = *(_freeCreatures.begin());
                _freeCreatures.erase(GUID);
            }
            else if (_creaturesGUID != MAX_GUID)
            {
                GUID = _creaturesGUID;
                _creaturesGUID++;
            }
            _creaturesMutex.unlock();
            break;

        case HIGH_GUID_ITEM:
            _itemsMutex.lock();
            if (!_freeItems.empty())
            {
                GUID = *(_freeItems.begin());
                _freeItems.erase(GUID);
            }
            else if (_itemsGUID != MAX_GUID)
            {
                GUID = _itemsGUID;
                _itemsGUID++;
            }
            _itemsMutex.unlock();
            break;
    }

    return GUID;
}
