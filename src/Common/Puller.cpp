#include "Puller.h"

Poco::UInt32 GuidManager::MAX_GUID = std::numeric_limits<Poco::UInt32>::max();

GuidManager::GuidManager()
{
    for (Poco::UInt8 i = 0; i < MAX_HIGH_GUID; i++)
        _lastGUID[i] = 1;
}

Poco::UInt32 GuidManager::getNewGUID(Poco::UInt32 highGUID)
{
    Poco::UInt32 GUID = MAX_GUID;

    _lock[highGUID].writeLock();
    if (_lastGUID[highGUID] == MAX_GUID)
    {
        if (!_freeGUIDs[highGUID].empty())
        {
            GUID = _freeGUIDs[highGUID].front();
            _freeGUIDs[highGUID].pop_front();
        }
    }
    else
    {
        GUID = _lastGUID[highGUID];
        _lastGUID[highGUID]++;
    }
    _lock[highGUID].unlock();

    return GUID;
}
