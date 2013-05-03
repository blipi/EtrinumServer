#include "Player.h"
#include "defines.h"
#include "Server.h"
#include "Creature.h"

Player::Player(std::string name, Client* client):
    Character(name, client)
{
}

void Player::UpdateLoS(GuidsSet newObjectsInSight)
{
    // Send spawn packets
    for (GuidsSet::iterator itr = newObjectsInSight.begin(); itr != newObjectsInSight.end(); itr++)
    {
        // Is the object already in the LOS list? If not, send an update packet
        if (_objectsInSight.find(*itr) == _objectsInSight.end())
        {
            // Send update packet to players only
            SharedPtr<Object> object = sServer->GetObject(*itr);
            if (object.isNull())
                continue;

            if (object->GetHighGUID() != HIGH_GUID_PLAYER)
                object->ToCreature()->UpdateVisibilityOf(GetGUID(), true);
            
            sServer->UpdateVisibilityOf(this->ToObject(), object, true);
        }
    }

    // Send despawn packets
    for (GuidsSet::iterator itr = _objectsInSight.begin(); itr != _objectsInSight.end(); itr++)
    {
        // Is the object no longer in sight? Send Despawn packet
        if (newObjectsInSight.find(*itr) == newObjectsInSight.end())
        {
            // Send update packet to players only
            SharedPtr<Object> object = sServer->GetObject(*itr);
            if (object.isNull())
                continue;

            if (object->GetHighGUID() != HIGH_GUID_PLAYER)
                object->ToCreature()->UpdateVisibilityOf(GetGUID(), false);
            
            sServer->UpdateVisibilityOf(this->ToObject(), object, false);
        }
    }

    _objectsInSight = newObjectsInSight;
}