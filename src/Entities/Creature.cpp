#include "Creature.h"
#include "Sector.h"

Creature::Creature():
    Character("")
{
}

Creature::~Creature()
{
}

bool Creature::update(const Poco::UInt64 diff)
{
    if (checkLOS())
    {
        SharedPtr<Object> target = getSector()->selectTargetInAggroRange();

        if (!target.isNull())
        {
            // attackStart(target);
        }
    }

    return Object::update(diff);
}
