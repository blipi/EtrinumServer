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

            Packet* spawnPacket = NULL;
            Packet* despawnPacket = NULL;

            for (Sector::TypeSectorEvents::iterator itr = sectorEvents->begin(); itr != sectorEvents->end();)
            {
                Sector::SectorEvent* sectorEvent = *itr;
                ++itr;

                // Avoid self sending
                if (GetGUID() == sectorEvent->Who->GetGUID())
                    continue;

                switch (sectorEvent->EventType)
                {
                    case EVENT_BROADCAST_JOIN:
                    {
                        if (spawnPacket == NULL)
                            spawnPacket = sServer->buildSpawnPacket(this, false);

                        // Send spawn of the visitor
                        if (sectorEvent->Who->GetHighGUID() & HIGH_GUID_PLAYER)
                            sServer->sendPacketTo(spawnPacket, sectorEvent->Who);

                        // Send spawn to the visitor
                        sServer->sendPacketTo(sectorEvent->EventPacket, this);

                        break;
                    }

                    case EVENT_BROADCAST_LEAVE:
                    {
                        if (despawnPacket == NULL)
                            despawnPacket = sServer->buildDespawnPacket(GetGUID());

                        // Send spawn of the visitor
                        if (sectorEvent->Who->GetHighGUID() & HIGH_GUID_PLAYER)
                            sServer->sendPacketTo(despawnPacket, sectorEvent->Who);

                        // Send spawn to the visitor
                        sServer->sendPacketTo(sectorEvent->EventPacket, this);

                        break;
                    }
                }
            }

            if (spawnPacket)
                delete spawnPacket;

            if (despawnPacket)
                delete despawnPacket;
        }
    }

    return result;
}


