#ifndef BASIC_SERVER_ENTITIES_CHARACTER_H
#define BASIC_SERVER_ENTITIES_CHARACTER_H

#include "Object.h"

class Client;

class Character: public Object
{
public:
    Character(std::string name, Client* client = NULL);
    virtual ~Character();

    virtual void UpdateLoS(TypeObjectsMap newObjectsInSight) = 0;
    bool hasNearPlayers();

    inline std::string GetName()
    {
        return _name;
    }

    inline Poco::UInt32 GetMaxHP()
    {
        return _maxhp;
    }

    inline void SetMaxHP(Poco::UInt32 maxhp)
    {
        _maxhp = maxhp;
    }

    inline Poco::UInt32 GetHP()
    {
        return _hp;
    }

    inline void SetHP(Poco::UInt32 hp)
    {
        _hp = hp;
    }

    inline Poco::UInt32 GetMaxMP()
    {
        return _maxmp;
    }

    inline void SetMaxMP(Poco::UInt32 maxmp)
    {
        _maxmp = maxmp;
    }

    inline Poco::UInt32 GetMP()
    {
        return _mp;
    }

    inline void SetMP(Poco::UInt32 mp)
    {
        _mp = mp;
    }

    inline Poco::UInt8 GetLVL()
    {
        return _lvl;
    }

    inline void SetLVL(Poco::UInt8 lvl)
    {
        _lvl = lvl;
    }

    inline float getFacingTo()
    {
        return _angle;
    }

    inline void setFacingTo(float angle)
    {
        _angle = angle;
    }

    inline float GetSpeed(Poco::UInt8 movementType)
    {
        return _speed[movementType];
    }

    inline Poco::UInt8 MovementTypeSpeed()
    {
        return _movementTypeSpeed;
    }

    inline bool doLoSUpdate()
    {
        bool update = _losTimer.elapsed() > 500;
        _losTimer.update();
        return update;
    }

private:
    std::string _name;
    Poco::UInt32 _maxhp;
    Poco::UInt32 _maxmp;
    Poco::UInt32 _hp;
    Poco::UInt32 _mp;
    Poco::UInt8 _lvl;

    float _angle;

    Poco::Timestamp _losTimer;

    Poco::UInt8 _movementTypeSpeed;
    float _speed[2];
};

#endif