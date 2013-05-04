#include "Object.h"
#include "ObjectManager.h"
#include "Player.h"
#include "Character.h"
#include "Creature.h"
#include "Grid.h"
#include "Server.h"
#include "Tools.h"

#ifdef SERVER_FRAMEWORK_TESTING

    #include "debugging.h"

#endif

Object::Object(Client* client):
    _client(client),
    _GUID(ObjectManager::MAX_GUID)
{
    // Reset all flags
    for (Poco::UInt8 i = 0; i < MAX_FLAGS_TYPES; i++)
        _flags[i] = 0;

    // Reset Position
    _position.x = 0;
    _position.y = 0;
    _grid = NULL;
}

Object::~Object()
{
#if defined(SERVER_FRAMEWORK_TESTING)
    printf("Object %s deleted\n", Poco::NumberFormatter::formatHex(GetGUID()).c_str());
#endif
}

bool Object::update(const Poco::UInt32 diff)
{
    bool updatedGrid = false;
    if (hasFlag(FLAGS_TYPE_MOVEMENT, FLAG_MOVING))
    {
        // Check if movement has finalized, in which case, remove flag
        Vector2D newPos;
        if (motionMaster.evaluate(diff, newPos))
            clearFlag(FLAGS_TYPE_MOVEMENT, FLAG_MOVING);

        // Update grid if we have to
        Vector2D currentPos = GetPosition();
        if (Tools::GetCellFromPos(newPos.x) != Tools::GetCellFromPos(currentPos.x) || Tools::GetCellFromPos(newPos.y) != Tools::GetCellFromPos(currentPos.y))
        {                    
            // Add it to GridLoader move list, we can't add it directly from here to the new Grid
            // We would end up being deadlocked, so the GridLoader handles it
            _grid = NULL;
            sGridLoader.addObject(this);
            updatedGrid = true;
        }

        // Relocate Object
        Relocate(newPos);
    }

    return !updatedGrid;
}

void Object::Despawn()
{
    // Send despawn packets
    for (GuidsSet::iterator itr = _objectsInSight.begin(); itr != _objectsInSight.end(); itr++)
    {
        // We should only update creatures, players will get notified through the grid
        if (!(*itr & HIGH_GUID_CREATURE))
            continue;

        SharedPtr<Object> object = sObjectManager.getObject(*itr);
        if (object.isNull())
            continue;

        object->ToCreature()->UpdateVisibilityOf(GetGUID(), false);
    }
}

void Object::SetGUID(Poco::UInt64 GUID)
{
    _GUID = GUID;
}

Poco::UInt64 Object::GetGUID()
{
    return _GUID;
}

Poco::UInt32 Object::GetHighGUID()
{
    return (Poco::UInt32)(_GUID >> 32);
}

Poco::UInt32 Object::GetLowGUID()
{
    return (Poco::UInt32)(_GUID & 0xFFFFFFFF);
}

Player* Object::ToPlayer()
{
    if (GetHighGUID() & HIGH_GUID_PLAYER)
        return (Player*)this;
    return NULL;
}

Creature* Object::ToCreature()
{
    if (GetHighGUID() & HIGH_GUID_CREATURE)
        return (Creature*)this;
    return NULL;
}
   
Character* Object::ToCharacter()
{
    if ((GetHighGUID() & HIGH_GUID_PLAYER) || (GetHighGUID() & HIGH_GUID_CREATURE))
        return (Character*)this;
    return NULL;
}

Object* Object::ToObject()
{
    return (Object*)this;
}
