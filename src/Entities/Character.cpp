#include "Character.h"

Character::Character(std::string name, Client* client):
    Object(client)
{
    _name = name;
    _maxhp = _hp = 0;
    _maxmp = _mp = 0;
    _lvl = 0;
    _movementTypeSpeed = MOVEMENT_RUN;
    _speed[0] = SPEED_RUN;
    _speed[1] = SPEED_WALK;
}

Character::~Character()
{
}

bool Character::hasNearPlayers()
{
    for (TypeObjectsMap::iterator it = _objectsInSight.begin(); it != _objectsInSight.end(); it++)
        if (HIGUID(it->first) & HIGH_GUID_PLAYER)
            return true;

    return false;
}
