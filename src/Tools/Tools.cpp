#include "Tools.h"

#include "defines.h"

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
		memcpy( &f, &value, sizeof(Poco::UInt32));
		return f;
	}

    Poco::UInt16 GetCellFromPos(double p)
    {
        return (int)(p/UNITS_PER_CELL) + 1;
    }

    Poco::UInt16 GetPositionInCell(Poco::UInt16 cell, float b)
    {
        return Poco::UInt16(abs(((cell - 1) * UNITS_PER_CELL) - b));
    }
}