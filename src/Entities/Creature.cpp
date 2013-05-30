#include "Creature.h"

Creature::Creature():
    Character("")
{
}

Creature::~Creature()
{
}

void UpdateLoS(GuidsSet newObjectsInSight)
{

}

void Creature::UpdateVisibilityOf(Poco::UInt64 GUID, bool visible)
{
    if (visible)
        _objectsInSight.insert(GUID);
    else
        _objectsInSight.erase(GUID);
}
