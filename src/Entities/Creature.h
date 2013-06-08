#ifndef BASIC_SERVER_ENTITIES_CREATURE_H
#define BASIC_SERVER_ENTITIES_CREATURE_H

#include "Character.h"

#include "Poco/Poco.h"

class Creature: public Character
{
public:
    Creature();
    virtual ~Creature();

    bool update(const Poco::UInt64 diff);

private:

};

#endif