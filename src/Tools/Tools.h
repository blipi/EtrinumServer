#ifndef BASIC_SERVER_TOOLS_H
#define BASIC_SERVER_TOOLS_H

#include "Poco/Poco.h"

namespace Tools
{
    Poco::UInt32 getU32(float value);
    float u32tof(Poco::UInt32 value);
    Poco::UInt16 GetCellFromPos(double p);

    Poco::UInt16 GetPositionInCell(Poco::UInt16 cell, float b);
}

/**
At some point, positions in Grid will have to be calculated from a base map
mapId: x -> coordinates (x1, y1)
which will be different from
mapId: y -> coordinates (x1, y1)

(Asumming both are the same x1 and y1)
**/

#endif