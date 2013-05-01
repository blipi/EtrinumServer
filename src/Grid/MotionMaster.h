#ifndef BASIC_SERVER_GRID_MOTION_MASTER_H
#define BASIC_SERVER_GRID_MOTION_MASTER_H

#include "Poco/Poco.h"
#include <vector>

#include "defines.h"

#define _max(a, b) ((a > b) ? (a) : (b))

//enum BASIC_PARAMS
#define SPEED_WALK          1.5f
#define SPEED_RUN           8.0f
#define GRAVITY             20.0f
#define JUMP_ANIM_SPEED     1.5f
#define LAND_ANIM_SPEED     2.0f

enum MOVEMENT_TYPE
{
    MOVEMENT_RUN,
    MOVEMENT_WALK,
};

class Object;

class MotionMaster
{
public:
    void StartSimpleMovement(Object* object, Vector2D to, float speed);

    void addPoint(Vector2D point);
    Vector2D& current();
    bool hasNext();
    Vector2D& next();

    void set(float speed, float elapsed = 0);
    bool evaluate(Poco::UInt32 diff, Vector2D& pos);

    void clear();

private:
    struct MovementVector
    {
        std::vector<Vector2D> points;
        Poco::UInt16 angle;
        float speed;
        float dx;
        float dy;
    };


private:
    MovementVector _movement;

    float _time;
    float _elapsed;
};

#endif
