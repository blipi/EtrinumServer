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
    _grid(grid)
{
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
            if (prevSector != object->GetPosition().sector)
            {
                _grid->getOrLoadSector_i(actSector)->add(object); // Add us to the new sector
                remove_i(object); // Remove from this sector
            }
        }
    }
    
    // Theorically, no join events can occure during the sector update, the mutex avoids it
    // Delete them all
    clearJoinEvents();

    return !_objects.empty();
}

bool Sector::add(SharedPtr<Object> object)
{
    Poco::Mutex::ScopedLock lock(_mutex);

    if (_objects.insert(rde::make_pair(object->GetGUID(), object)).second)
    {
        object->setSector(this);
        if (object->GetHighGUID() & HIGH_GUID_PLAYER)
            _grid->onPlayerAdded();
        
        SharedPtr<Packet> packet = sServer->buildSpawnPacket(object, false);

        // Join all near sectors
        TypeHashList::iterator itr = _sectors.begin();
        TypeHashList::iterator end = _sectors.end();
        while (itr != end)
        {
            _grid->getOrLoadSector_i(*itr)->join(object, packet);
            ++itr;
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

void Sector::remove_i(SharedPtr<Object> object)
{
    if (object->GetHighGUID() & HIGH_GUID_PLAYER)
        _grid->onPlayerErased();

    _objects.erase(object->GetGUID());

    SharedPtr<Packet> packet = sServer->buildDespawnPacket(object->GetGUID());
    leave(object, packet);
}

void Sector::join(SharedPtr<Object> who, SharedPtr<Packet> packet)
{
    // Join events must (ideally) be done at the next update
    // As it reduces the amount of time and loops being done
    // Building the spawn packet is time expensive, do it now
    _sectorEvents.push_back(new SectorEvent(who, packet, EVENT_BROADCAST_JOIN));
}

void Sector::visit(SharedPtr<Object> who)
{
    if (Creature* creature = who->ToCreature())
        for (TypeObjectsMap::iterator itr = _objects.begin(); itr != _objects.end(); ++itr)
            creature->onMoveInLOS(itr->second);
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
    Poco::UInt8 x = _hash >> 8;
    Poco::UInt8 y = _hash & 0xFF;
        
    TypeHashList list;
    list.push_back(hash(x, y));

    if (x > 1)
    {
        list.push_back(hash(x - 1, y));

        if (y > 1)
            list.push_back(hash(x - 1, y - 1));
            
        if (y < 0xFF)
            list.push_back(hash(x - 1, y + 1));
    }

    if (x < 0xFF)
    {
        list.push_back(hash(x + 1, y));

        if (y > 1)
            list.push_back(hash(x + 1, y - 1));
            
        if (y < 0xFF)
            list.push_back(hash(x + 1, y + 1));
    }

    if (y > 1)
        list.push_back(hash(x, y - 1));

    if (y < 0xFF)
        list.push_back(hash(x, y + 1));

    return list;
}

