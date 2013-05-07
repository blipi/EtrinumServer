#include "MotionMaster.h"
#include "Object.h"
#include "Character.h"
#include "Log.h"
#include "debugging.h"

#include <cmath>

void MotionMaster::StartSimpleMovement(Object* object, Vector2D to, float speed)
{
    object->motionMaster.clear();
    object->motionMaster.addPoint(object->GetPosition());
    object->motionMaster.addPoint(to);
    object->motionMaster.set(speed, MOVEMENT_TO_POINT);
    object->setFlag(FLAGS_TYPE_MOVEMENT, FLAG_MOVING);

    sLog.out(Message::PRIO_INFORMATION, "\t\tObject starting movement, t=%f", object->motionMaster._time);
}

void MotionMaster::StartAngleMovement(Object* object, float angle, float speed)
{
    object->motionMaster.clear();
    object->motionMaster.addPoint(object->GetPosition());
    object->motionMaster.set(speed, MOVEMENT_BY_ANGLE);
    object->motionMaster.angle(angle);
    object->setFlag(FLAGS_TYPE_MOVEMENT, FLAG_MOVING);
    
    sLog.out(Message::PRIO_INFORMATION, "\t\tObject starting movement, a=%f", angle);
}

void MotionMaster::addPoint(Vector2D point)
{
    _movement.points.push_back(point);
}

Vector2D& MotionMaster::current()
{
    ASSERT(!_movement.points.empty())
    return _movement.points[0];
}

bool MotionMaster::hasNext()
{
    return _movement.points.size() > 1;
}

Vector2D& MotionMaster::next()
{
    ASSERT(_movement.points.size() > 1)
    return _movement.points[1];
}

void MotionMaster::angle(float angle)
{
    _movement.angle = angle;
}

void MotionMaster::set(float speed, Poco::UInt8 movementType, float elapsed)
{
    _movement.movementType = movementType;
    _movement.speed = speed;

    if (movementType == MOVEMENT_TO_POINT)
    {
        Vector2D c = current();
        Vector2D n = next();

        _movement.dx = (float)((Poco::Int32)n.x - (Poco::Int32)c.x);
        _movement.dy = (float)((Poco::Int32)n.y - (Poco::Int32)c.y);

        float distance = std::sqrt(std::pow(_movement.dx, 2) + std::pow(_movement.dy, 2));
        _time = distance/speed;
        _elapsed = elapsed;
    }
    else if (movementType == MOVEMENT_BY_ANGLE)
        _elapsed = 0;
}

bool MotionMaster::evaluate(Poco::UInt32 diff, Vector2D& pos)
{
    _elapsed += diff/1000.0f;
    Vector2D c = current();

    if (_movement.movementType == MOVEMENT_TO_POINT)
    {
        float r = _elapsed;
        if (_elapsed > _time)
            r = _time;

        pos.x = c.x + _movement.dx * _elapsed / _time;
        pos.y = c.y + _movement.dy * _elapsed / _time;

        if (r >= _time)
        {
            _movement.points.erase(_movement.points.begin());
            if (hasNext())
                set(_movement.speed, _movement.movementType, _max(0, _elapsed - r));
        }

        return r >= _time && !hasNext();
    }
    else if (_movement.movementType == MOVEMENT_BY_ANGLE)
    {
        pos.x = c.x + (_movement.speed * _elapsed * std::cos(_movement.angle));
        pos.y = c.y + (_movement.speed * _elapsed * std::sin(_movement.angle));

        return false;
    }

    return true;
}

Poco::UInt8 MotionMaster::getMovementType()
{
    current(); // Verify there's some kind of movement
    return _movement.movementType;
}

void MotionMaster::clear()
{
    _movement.points.clear();
    _movement.dx = _movement.dy = 0;
    _movement.angle = 0;
    _movement.speed = 0;
}
