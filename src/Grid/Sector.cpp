#include "Sector.h"
#include "Grid.h"
#include "GridLoader.h"

#include "Creature.h"
#include "Object.h"
#include "Packet.h"
#include "Player.h"
#include "Server.h"
#include "Tools.h"

Sector::SectorEvent::SectorEvent(SharedPtr<Object> object, SharedPtr<Packet> packet, Poco::UInt8 eventType)
{
    Who.assign(object);
    EventPacket.assign(packet);
    EventType = eventType;
}

Sector::SectorEvent::~SectorEvent()
{
    Who = NULL;
}

Sector::Sector(Poco::UInt16 hash, Grid* grid):
    _hash(hash),
    _grid(grid),
    _playersInSector(0)
{
    _x = hash >> 8;
    _y = hash & 0xFF;
    _sectors = getNearSectors();
}

Sector::~Sector()
{
    clearJoinEvents();
}

Poco::UInt16 Sector::hash(Poco::UInt8 x, Poco::UInt8 y)
{
    return ((Poco::UInt16)x << 8) | y;
}

bool Sector::update(Poco::UInt64 diff)
{
    Poco::Mutex::ScopedLock lock(_mutex);
    
    // Update all players
    for (TypeObjectsMap::iterator itr = _objects.begin(); itr != _objects.end(); )
    {
        SharedPtr<Object> object = itr->second;
        ++itr;

        // Get last update time (in case the object switches grid)
        Poco::UInt64 objectDiff = object->getLastUpdate();
        if (objectDiff > diff)
            objectDiff = diff;

        // Update AI, movement, everything if there is any or we have to
        // If update returns false, that means the object is no longer in this grid!
        Poco::UInt16 prevSector = object->GetPosition().sector;
        bool updateResult = false;
    
        switch (object->GetHighGUID())
        {
            case HIGH_GUID_PLAYER:
                updateResult = object->ToPlayer()->update(objectDiff);
                break;

            case HIGH_GUID_CREATURE:
                updateResult = object->ToCreature()->update(objectDiff);
                break;

            default:
                updateResult = object->update(objectDiff);
                break;
        }

        // Change Grid if we have to
        if (!updateResult)
        {
            sGridLoader.addObject(object); // Add to the new Grid
            remove_i(object); // Delete from the Sector (and Grid)
        }
        else if (object->hasFlag(FLAGS_TYPE_MOVEMENT, FLAG_MOVING))
        {
            Poco::UInt16 actSector = object->GetPosition().sector;

            // Have we changed sector?
            if (prevSector != actSector)
            {
                Poco::UInt8 aX = (actSector - prevSector) >> 8;
                Poco::UInt8 aY = (actSector - prevSector) & 0xFF;

                _grid->getOrLoadSector_i(actSector)->add(object, &aX, &aY); // Add us to the new sector
                remove_i(object, &aX, &aY); // Remove from this sector
            }
        }
    }
    
    // Theorically, no join events can occure during the sector update, the mutex avoids it
    // Delete them all
    clearJoinEvents();

    return hasObjects();
}

SharedPtr<Object> Sector::selectTargetInAggroRange()
{
    // If there are no players, simply return
    if (!_playersInSector)
        return NULL;

    // All the sector is in aggro range ;)
    for (TypeObjectsMap::iterator itr = _objects.begin(), end = _objects.end(); itr != end; ++itr)
    {
        SharedPtr<Object> object = itr->second;

        if (object->GetHighGUID() & HIGH_GUID_PLAYER)
            return object;
    }

    // Nothing found, should never happen as the first if avoids it
    // Just to avoid compiler warnings
    return NULL;
}

bool Sector::add(SharedPtr<Object> object, Poco::UInt8* aX /*= NULL*/, Poco::UInt8* aY /*= NULL*/)
{
    Poco::Mutex::ScopedLock lock(_mutex);

    if (_objects.insert(rde::make_pair(object->GetGUID(), object)).second)
    {
        object->setSector(this);
        if (object->GetHighGUID() & HIGH_GUID_PLAYER)
        {
            _playersInSector++;
            _grid->onPlayerAdded();
        }
        
        SharedPtr<Packet> packet = sServer->buildSpawnPacket(object, false);

        // Join all near sectors
        if (!aX || !aY)
        {
            TypeHashList::iterator itr = _sectors.begin();
            TypeHashList::iterator end = _sectors.end();
            while (itr != end)
            {
                _grid->getOrLoadSector_i(*itr)->join(object, packet);
                ++itr;
            }
        }
        else if (*aX == 0)
        {
            _grid->getOrLoadSector_i(hash(_x, _y + (2 * *aY)))->join(object, packet);
            _grid->getOrLoadSector_i(hash(_x - 1, _y + (2 * *aY)))->join(object, packet);
            _grid->getOrLoadSector_i(hash(_x + 1, _y + (2 * *aY)))->join(object, packet);
        }
        else if (*aY == 0)
        {
            _grid->getOrLoadSector_i(hash(_x + (2 * *aX), _y))->join(object, packet);
            _grid->getOrLoadSector_i(hash(_x + (2 * *aX), _y - 1))->join(object, packet);
            _grid->getOrLoadSector_i(hash(_x + (2 * *aX), _y + 1))->join(object, packet);
        }
        else
        {
            _grid->getOrLoadSector_i(hash(_x, _y + (2 * *aY)))->join(object, packet);
            _grid->getOrLoadSector_i(hash(_x + *aX, _y + (2 * *aY)))->join(object, packet);
            _grid->getOrLoadSector_i(hash(_x + (2 * *aX), _y + (2 * *aY)))->join(object, packet);
            _grid->getOrLoadSector_i(hash(_x + (2 * *aX), _y + *aY))->join(object, packet);
            _grid->getOrLoadSector_i(hash(_x + (2 * *aX), _y))->join(object, packet);
        }

        return true;
    }

    return false;
}

void Sector::remove(SharedPtr<Object> object)
{
    Poco::Mutex::ScopedLock lock(_mutex);
    remove_i(object);
}

void Sector::remove_i(SharedPtr<Object> object, Poco::UInt8* aX /*= NULL*/, Poco::UInt8* aY /*= NULL*/)
{
    if (object->GetHighGUID() & HIGH_GUID_PLAYER)
    {
        _playersInSector--;
        _grid->onPlayerErased();
    }

    _objects.erase(object->GetGUID());

    SharedPtr<Packet> packet = sServer->buildDespawnPacket(object->GetGUID());

    // Leave all near sectors
    if (!aX || !aY)
    {
        TypeHashList::iterator itr = _sectors.begin();
        TypeHashList::iterator end = _sectors.end();
        while (itr != end)
        {
            _grid->getOrLoadSector_i(*itr)->leave(object, packet);
            ++itr;
        }
    }
    else if (*aX == 0)
    {
        _grid->getOrLoadSector_i(hash(_x, _y - *aY))->leave(object, packet);
        _grid->getOrLoadSector_i(hash(_x - 1, _y - *aY))->leave(object, packet);
        _grid->getOrLoadSector_i(hash(_x + 1, _y - *aY))->leave(object, packet);
    }
    else if (*aY == 0)
    {
        _grid->getOrLoadSector_i(hash(_x - *aX, _y))->leave(object, packet);
        _grid->getOrLoadSector_i(hash(_x - *aX, _y - 1))->leave(object, packet);
        _grid->getOrLoadSector_i(hash(_x - *aX, _y + 1))->leave(object, packet);
    }
    else
    {
        _grid->getOrLoadSector_i(hash(_x - *aX, _y - *aY))->leave(object, packet);
        _grid->getOrLoadSector_i(hash(_x - *aX, _y))->leave(object, packet);
        _grid->getOrLoadSector_i(hash(_x - *aX, _y + *aY))->leave(object, packet);
        _grid->getOrLoadSector_i(hash(_x, _y - *aY))->leave(object, packet);
        _grid->getOrLoadSector_i(hash(_x + *aX, _y - *aY))->leave(object, packet);
    }
}

void Sector::join(SharedPtr<Object> who, SharedPtr<Packet> packet)
{
    // Join events must (ideally) be done at the next update
    // As it reduces the amount of time and loops being done
    // Building the spawn packet is time expensive, do it now
    _sectorEvents.push_back(new SectorEvent(who, packet, EVENT_BROADCAST_JOIN));
}

void Sector::leave(SharedPtr<Object> who, SharedPtr<Packet> packet)
{
    _sectorEvents.push_back(new SectorEvent(who, packet, EVENT_BROADCAST_LEAVE));
}

void Sector::clearJoinEvents()
{
    while (!_sectorEvents.empty())
    {
        delete _sectorEvents.back();
        _sectorEvents.pop_back();
    }
}

bool Sector::hasObjects()
{
    return !_objects.empty();
}

bool Sector::hasEvents()
{
    return !_sectorEvents.empty();
}

Sector::TypeSectorEvents* Sector::getEvents()
{
    return &_sectorEvents;
}

Poco::UInt16 Sector::hashCode()
{
    return _hash;
}

TypeHashList Sector::getNearSectors()
{        
    TypeHashList list;
    list.push_back(hash(_x, _y));

    if (_x > 1)
    {
        list.push_back(hash(_x - 1, _y));

        if (_y > 1)
            list.push_back(hash(_x - 1, _y - 1));
            
        if (_y < 0xFF)
            list.push_back(hash(_x - 1, _y + 1));
    }

    if (_x < 0xFF)
    {
        list.push_back(hash(_x + 1, _y));

        if (_y > 1)
            list.push_back(hash(_x + 1, _y - 1));
            
        if (_y < 0xFF)
            list.push_back(hash(_x + 1, _y + 1));
    }

    if (_y > 1)
        list.push_back(hash(_x, _y - 1));

    if (_y < 0xFF)
        list.push_back(hash(_x, _y + 1));

    return list;
}
