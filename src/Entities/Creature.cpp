#include "Creature.h"

Creature::Creature():
    Character("")
{
}

Creature::~Creature()
{
}

void Creature::UpdateLoS(GuidsSet newObjectsInSight)
{
    _objectsInSight = newObjectsInSight;
}

void Creature::addPlayerToLoS(Poco::UInt64 GUID)
{
    _objectsInSight.insert(GUID);
}
