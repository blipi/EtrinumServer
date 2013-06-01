#include "Creature.h"

Creature::Creature():
    Character("")
{
}

Creature::~Creature()
{
}

void Creature::UpdateLoS(TypeObjectsMap newObjectsInSight)
{
    _objectsInSight = newObjectsInSight;
    _objectsInSight.erase(GetGUID());
}

void Creature::addPlayerToLoS(SharedPtr<Object> object)
{
    _objectsInSight.insert(rde::make_pair(object->GetGUID(), object));
}
