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

#endif