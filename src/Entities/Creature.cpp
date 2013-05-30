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
