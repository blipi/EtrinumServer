#include "Sector.h"
#include "Grid.h"
#include "GridLoader.h"

#include "Creature.h"
#include "Object.h"
#include "Packet.h"
#include "Player.h"
#include "Server.h"
#include "Tools.h"

Sector::JoinEvent::JoinEvent(SharedPtr<Object> object, SharedPtr<Packet> packet)
{
    Who = object;
    SpawnPacket = packet;
}

Sector::JoinEvent::~JoinEvent()
{
    Who = NULL;
}

Sector::Sector(Poco::UInt16 hash, Grid* grid):
    _hash(hash),
    _grid(grid),
    _state(SECTOR_NOT_INITIALIZED)
{
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
        std::set<Poco::UInt16> sectors = getNearSectors(object->GetPosition(), Grid::LOSRange);
        std::set<Poco::UInt16>::iterator itr = sectors.begin();
        std::set<Poco::UInt16>::iterator end = sectors.end();

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

    leave(object);
}

void Sector::join(SharedPtr<Object> who, SharedPtr<Packet> packet)
{
    // Join events must (ideally) be done at the next update
    // As it reduces the amount of time and loops being done
    // Building the spawn packet is time expensive, do it now
    _joinEvents.push_back(new JoinEvent(who, packet));
}

void Sector::visit(SharedPtr<Object> who)
{
    if (Creature* creature = who->ToCreature())
        for (TypeObjectsMap::iterator itr = _objects.begin(); itr != _objects.end(); ++itr)
            creature->onMoveInLOS(itr->second);
}

void Sector::leave(SharedPtr<Object> who)
{
    for (TypeObjectsMap::iterator itr = _objects.begin(); itr != _objects.end(); )
    {
        Poco::UInt64 GUID = itr->first;
        SharedPtr<Object> object = itr->second;
        ++itr;

        // Send despawn of the visitor
        if (HIGUID(GUID) & HIGH_GUID_PLAYER)
            sServer->sendDespawnPacket(GUID, object);

        // Send despawn to the visitor
        if (who->GetHighGUID() & HIGH_GUID_PLAYER)
            sServer->sendDespawnPacket(object->GetGUID(), who);
    }
}

void Sector::clearJoinEvents()
{
    while (!_joinEvents.empty())
    {
        delete _joinEvents.front();
        _joinEvents.pop_front();
    }
}

bool Sector::hasJoinEvents()
{
    return !_joinEvents.empty();
}

Sector::TypeJoinEvents* Sector::getJoinEvents()
{
    return &_joinEvents;
}

Poco::UInt16 Sector::hashCode()
{
    return _hash;
}

std::set<Poco::UInt16> Sector::getNearSectors(Vector2D position, Poco::UInt8 losRange)
{
    Poco::UInt8 x = position.sector >> 8;
    Poco::UInt8 y = position.sector & 0xFF;
        
    std::set<Poco::UInt16> list;
    list.insert(hash(x, y));

    if (x > losRange)
    {
        list.insert(hash(x - 1, y));

        if (y > losRange)
            list.insert(hash(x - 1, y - 1));
            
        if (y < UNITS_PER_CELL - losRange)
            list.insert(hash(x - 1, y + 1));
    }

    if (x < UNITS_PER_CELL - losRange)
    {
        list.insert(hash(x + 1, y));

        if (y > losRange)
            list.insert(hash(x + 1, y - 1));
            
        if (y < UNITS_PER_CELL - losRange)
            list.insert(hash(x + 1, y + 1));
    }

    if (y > losRange)
        list.insert(hash(x, y - 1));

    if (y < UNITS_PER_CELL - losRange)
        list.insert(hash(x, y + 1));

    return list;
}

