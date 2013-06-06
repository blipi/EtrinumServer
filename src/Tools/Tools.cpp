#include "Tools.h"
#include "defines.h"

#include <string.h>
#include <cmath>

namespace Tools
{
    union ftou32
    {
        float f;
        Poco::UInt32 u32;
    };

    Poco::UInt32 getU32(float value)
	{
		ftou32 s;
		s.f = value;
		return s.u32;
	}
    
    float u32tof(Poco::UInt32 value)
	{
		float f;
		memcpy(&f, &value, sizeof(Poco::UInt32));
		return f;
	}

    Poco::UInt16 GetXCellFromPos(double x)
    {
        return (int)((x - MAP_MIN_X) / UNITS_PER_CELL) + 1;
    }

    Poco::UInt16 GetYCellFromPos(double z)
    {
        return (int)((z - MAP_MIN_Z) / UNITS_PER_CELL) + 1;
    }

    Poco::UInt16 GetPositionInXCell(Poco::UInt16 cell, float x)
    {
        return Poco::UInt16((((cell - 1) * UNITS_PER_CELL) - (x - MAP_MIN_X)));
    }

    Poco::UInt16 GetPositionInYCell(Poco::UInt16 cell, float z)
    {
        return Poco::UInt16((((cell - 1) * UNITS_PER_CELL) - (z - MAP_MIN_Z)));
    }

    // Hash code 0 is not valid
    Poco::UInt16 GetSector(Poco::UInt16 cellX, Poco::UInt16 cellY, Poco::UInt8 sectorSize)
    {
        return (((cellX / sectorSize) + 1) << 8) | ((Poco::UInt8)(cellY / sectorSize) + 1);
    }
}