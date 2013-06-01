#include "Player.h"
#include "defines.h"
#include "Server.h"
#include "Creature.h"
#include "ObjectManager.h"

// Update 5 players at most for each loop
#define MAX_UPDATE_BLOCKS 5

/**
 * Player constructor
 */
Player::Player(std::string name, Client* client):
    Character(name, client)
{
}

/**
 * Player destructor
 */
Player::~Player()
{
}
