#include "Object.h"
#include "ObjectManager.h"
#include "Player.h"
#include "Character.h"
#include "Creature.h"
#include "GridLoader.h"
#include "Server.h"
#include "Tools.h"
#include "Log.h"

/**
 * Object constructor, sets the GUID to nothing and saves the client
 * if any
 */
Object::Object(Client* client):
    _client(client),
    _GUID(ObjectManager::MAX_GUID)
{
    // Reset all flags
    for (Poco::UInt8 i = 0; i < MAX_FLAGS_TYPES; ++i)
        _flags[i] = 0;

    // Reset Position
    _position.x = 0;
    _position.z = 0;
    _grid = NULL;
    _lastUpdate = 0;
}

/**
 * Destructor, simply logs the information
 */
Object::~Object()
{
    //sLog.out(Message::PRIO_TRACE, "Object %s deleted", Poco::NumberFormatter::formatHex(GetGUID()).c_str());
}

/**
 * Update the object:
 * -Process movement
 *
 * @param diff Time difference from last tick
 */
bool Object::update(const Poco::UInt64 diff)
{
    if (hasFlag(FLAGS_TYPE_MOVEMENT, FLAG_MOVING))
    {
        // Save the previous grid
        Poco::UInt8 gridX = GetPosition().gridX;
        Poco::UInt8 gridY = GetPosition().gridY;

        // Check if movement has finalized, in which case, remove flag
        Vector2D newPos;
        if (motionMaster.evaluate(diff, newPos))
        {
            //sLog.out(Message::PRIO_TRACE, "\t\tObject finished movement (%.2f, %.2f)", GetPosition().x, GetPosition().z);
            clearFlag(FLAGS_TYPE_MOVEMENT, FLAG_MOVING);
        }

        // Relocate to the new position
        Relocate(newPos);

        // Return if grid has changed
        return gridX == GetPosition().gridX && gridY == GetPosition().gridY;
    }

    return true;
}

/**
 * Sets the object GUID
 *
 * @param GUID Objects new GUID
 */
void Object::SetGUID(Poco::UInt64 GUID)
{
    _GUID = GUID;
}

/**
 * Gets the object GUID
 *
 * @return The object GUID
 */
Poco::UInt64 Object::GetGUID()
{
    return _GUID;
}

/**
 * Gets the object High GUID
 *
 * @return The object High GUID
 */
Poco::UInt32 Object::GetHighGUID()
{
    return (Poco::UInt32)(_GUID >> 32);
}

/**
 * Gets the object Low GUID
 *
 * @return The object Low GUID
 */
Poco::UInt32 Object::GetLowGUID()
{
    return (Poco::UInt32)(_GUID & 0xFFFFFFFF);
}

void Object::Relocate(Vector2D position)
{
    _position.Relocate(position);
    _position.Process();
}

float Object::distanceTo(Object* to)
{
    return GetPosition().Distance(to->GetPosition());
}

/**
 * Converts the object to a player, if it is of that type
 *
 * @return The player pointer or NULL
 */
Player* Object::ToPlayer()
{
    if (GetHighGUID() & HIGH_GUID_PLAYER)
        return (Player*)this;
    return NULL;
}

/**
 * Converts the object to a creature, if it is of that type
 *
 * @return The creature pointer or NULL
 */
Creature* Object::ToCreature()
{
    if (GetHighGUID() & HIGH_GUID_CREATURE)
        return (Creature*)this;
    return NULL;
}

/**
 * Converts the object to a character, if it is of that type
 *
 * @return The character pointer or NULL
 */
Character* Object::ToCharacter()
{
    if ((GetHighGUID() & HIGH_GUID_PLAYER) || (GetHighGUID() & HIGH_GUID_CREATURE))
        return (Character*)this;
    return NULL;
}

/**
 * Converts the (casted) object to an object, if it is of that type
 *
 * @return The object pointer or NULL
 */
Object* Object::ToObject()
{
    return (Object*)this;
}
