#ifndef GAMESERVER_TOOLS_H
#define GAMESERVER_TOOLS_H

#include "Poco/Poco.h"
#include <set>

#include "Position.h"

namespace Tools
{
    Poco::UInt32 getU32(float value);
    float u32tof(Poco::UInt32 value);

    Poco::UInt16 GetXCellFromPos(double x);
    Poco::UInt16 GetYCellFromPos(double z);

    Poco::UInt16 GetPositionInXCell(Poco::UInt16 cell, float x);
    Poco::UInt16 GetPositionInYCell(Poco::UInt16 cell, float z);

    Poco::UInt16 GetSector(Poco::UInt16 cellX, Poco::UInt16 cellY, Poco::UInt8 sectorSize);
}

/**
At some point, positions in Grid will have to be calculated from a base map
mapId: x -> coordinates (x1, y1)
which will be different from
mapId: y -> coordinates (x1, y1)

(Asumming both are the same x1 and y1)
**/

#endif