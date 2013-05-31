#ifndef BASIC_SERVER_ENTITIES_CREATURE_H
#define BASIC_SERVER_ENTITIES_CREATURE_H

#include "Character.h"

#include "Poco/Poco.h"

class Creature: public Character
{
public:
    Creature();
    virtual ~Creature();

    void UpdateLoS(GuidsSet newObjectsInSight);
    void addPlayerToLoS(Poco::UInt64 GUID);

private:

};

#endif