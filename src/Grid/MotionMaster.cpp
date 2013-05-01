#include "MotionMaster.h"
#include "Object.h"
#include "debugging.h"

void MotionMaster::StartSimpleMovement(Object* object, Vector2D to, float speed)
{
    object->motionMaster.addPoint(object->GetPosition());
    object->motionMaster.addPoint(to);
    object->motionMaster.set(speed);
    object->setFlag(FLAGS_TYPE_MOVEMENT, FLAG_MOVING);
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

void MotionMaster::set(float speed, float elapsed)
{
    _movement.speed = speed;
        
    Vector2D c = current();
    Vector2D n = next();

    _movement.dx = (float)((Poco::Int32)n.x - (Poco::Int32)c.x);
    _movement.dy = (float)((Poco::Int32)n.y - (Poco::Int32)c.y);

    float distance = sqrt(pow(_movement.dx, 2) + pow(_movement.dy, 2));
    _time = distance/speed;
    _elapsed = elapsed;

    #if defined(SERVER_FRAMEWORK_TESTING)
        printf("\t\tObject starting movement, t=%f\n", _time);
    #endif
}

bool MotionMaster::evaluate(Poco::UInt32 diff, Vector2D& pos)
{
    _elapsed += diff/1000.0f;

    float r = _elapsed;
    if (_elapsed > _time)
        r = _time;
        
    Vector2D c = current();
    pos.x = c.x + _movement.dx * _elapsed / _time;
    pos.y = c.y + _movement.dy * _elapsed / _time;

    if (r >= _time)
    {
        _movement.points.erase(_movement.points.begin());
        if (hasNext())
            set(_movement.speed, _max(0, _elapsed - r));
    }

    return r >= _time;
}

void MotionMaster::clear()
{
    _movement.points.clear();
    _movement.dx = _movement.dy = 0;
    _movement.angle = 0;
    _movement.speed = 0;
}
