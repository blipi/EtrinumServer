#include "Creature.h"

Creature::Creature():
    Character("")
{
}

void Creature::UpdateVisibilityOf(Poco::UInt64 GUID, bool visible)
{
    if (visible)
        _objectsInSight.insert(GUID);
    else
        _objectsInSight.erase(GUID);
}
