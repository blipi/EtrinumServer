#include "Creature.h"
#include "Grid.h"

Creature::Creature():
    Character("")
{
}

Creature::~Creature()
{
}

void Creature::onMoveInLOS(Object* who)
{
    if (distanceTo(who) <= Grid::AggroRange)
    {
        // AttackStart
    }
}
