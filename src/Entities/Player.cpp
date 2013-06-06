#include "Player.h"
#include "defines.h"
#include "Server.h"
#include "Creature.h"
#include "ObjectManager.h"
#include "Packet.h"
#include "Sector.h"

// Update 5 players at most for each loop
#define MAX_UPDATE_BLOCKS 5

/**
 * Player constructor
 */
Player::Player(std::string name, Client* client):
    Character(name, client)
{
}

/**
 * Player destructor
 */
Player::~Player()
{
}

bool Player::update(const Poco::UInt64 diff)
{
    bool result = Object::update(diff);

    // Process join events on the grid
    if (Sector* sector = getSector())
    {
        if (sector->hasEvents())
        {
            // Get all the join events
            Sector::TypeSectorEvents* sectorEvents = sector->getEvents();

            Packet* spawnData = sServer->buildSpawnPacket(this, false);

            for (Sector::TypeSectorEvents::iterator itr = sectorEvents->begin(); itr != sectorEvents->end();)
            {
                Sector::SectorEvent* sectorEvent = *itr;
                ++itr;

                // Avoid self spawning
                if (GetGUID() == sectorEvent->Who->GetGUID())
                    continue;

                // Send spawn of the visitor
                if (sectorEvent->Who->GetHighGUID() & HIGH_GUID_PLAYER)
                    sServer->sendPacketTo(spawnData, sectorEvent->Who);

                // Send spawn to the visitor
                sServer->sendPacketTo(sectorEvent->EventPacket, this);
            }

            delete spawnData;
        }
    }

    return result;
}


