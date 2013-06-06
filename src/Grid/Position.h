#ifndef GAMESERVER_POSITION_H
#define GAMESERVER_POSITION_H

#include "Poco/Poco.h"

#include "Tools.h"

struct Vector2D
{
    Vector2D();
    Vector2D(float x, float z);

    float x;
    float z;

    Poco::UInt8 gridX;
    Poco::UInt8 gridY;
    Poco::UInt16 sector;

    void Relocate(Vector2D vector2D);
    float Distance(Vector2D to);

private:
    void calculateGrid();
    void calculateSector();

    Poco::UInt16 _inCellX;
    Poco::UInt16 _inCellY;
};

#endif