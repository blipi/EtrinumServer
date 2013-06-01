#include "Player.h"
#include "defines.h"
#include "Server.h"
#include "Creature.h"
#include "ObjectManager.h"

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

/**
 * Updates the objects in LoS with the player
 *
 * @param newObjectsInSight A list containing the new objects in sight
 */
void Player::UpdateLoS(TypeObjectsMap newObjectsInSight)
{
    int updates = 0;
    for (TypeObjectsMap::iterator itr = _objectsInSight.begin(); itr != _objectsInSight.end(); itr++)
    {
        if (newObjectsInSight.find(itr->first) == newObjectsInSight.end())
        {
            _objectsInSight.erase(itr->first);
            
            switch (HIGUID(itr->first))
            {
                case HIGH_GUID_PLAYER:
                    sServer->sendDespawnPacket(itr->first, ToObject());
                    updates++;
                    break;
            }
        }
        
        if (updates >= MAX_UPDATE_BLOCKS)
            return;
    }

    for (TypeObjectsMap::iterator itr = newObjectsInSight.begin(); itr != newObjectsInSight.end(); itr++)
    {
        if (_objectsInSight.find(itr->first) == _objectsInSight.end())
        {
            _objectsInSight.insert(rde::make_pair(itr->first, itr->second));
            
            switch (HIGUID(itr->first))
            {
                case HIGH_GUID_PLAYER:
                    sServer->UpdateVisibilityOf(itr->second, ToObject());
                    updates++;
                    break;                

                case HIGH_GUID_CREATURE:
                    if (Creature* creature = itr->second->ToCreature())
                        creature->addPlayerToLoS(sObjectManager.getObject(GetGUID()));
                    break;
            }
        }
        
        if (updates >= MAX_UPDATE_BLOCKS)
            return;
    }
}
