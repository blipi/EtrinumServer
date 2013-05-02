#include "Character.h"

Character::Character(std::string name):
    Object()
{
    _name = name;
    _maxhp = _hp = 0;
    _maxmp = _mp = 0;
    _lvl = 0;
    _movementTypeSpeed = MOVEMENT_RUN;
    _speed[0] = SPEED_RUN;
    _speed[1] = SPEED_WALK;
}
