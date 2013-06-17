#ifndef GAMESERVER_ENTITIES_PLAYER_H
#define GAMESERVER_ENTITIES_PLAYER_H

#include <list>

#include "Character.h"

class Client;

class Player: public Character
{
public:
    Player(std::string name, Client* client);
    virtual ~Player();

    bool update(const Poco::UInt64 diff);

private:
    
};

#endif