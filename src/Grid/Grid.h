#ifndef BASIC_SERVER_GRID_H
#define BASIC_SERVER_GRID_H

#include "defines.h"

//@ Avoid MSV defining max/min and overriding numeric_limits
#if defined(_WIN32) || defined(_WIN64)
    #define NOMINMAX
#endif

#include <list>
#include <map>
#include <set>
#include <unordered_map>

//@ Basic server information
// >> Server runs on multiple threads, grids are in a thread pool
#include "Poco/ErrorHandler.h"
#include "Poco/ThreadPool.h"
#include "Poco/RunnableAdapter.h"
#include "Poco/SingletonHolder.h"
#include "Poco/RWLock.h"

//@ Everything is stored in SharedPtrs
#include "Poco/SharedPtr.h"

//@ Hash Map
#include "google/dense_hash_map"


using Poco::SharedPtr;

class Object;
class Server;

class Grid
{
private:
    typedef google::dense_hash_map<Poco::UInt64 /*guid*/, Poco::SharedPtr<Object> /*object*/, std::hash<Poco::UInt64>, equint64> ObjectMap;
    typedef std::pair<Poco::UInt64 /*guid*/, Poco::SharedPtr<Object> /*client*/> ObjectMapInserter;

public:
    typedef std::list<Poco::UInt64> ObjectList;

    Grid(Poco::UInt16 x, Poco::UInt16 y);
    bool update();

    ObjectList getObjects(Poco::UInt32 highGUID);
    SharedPtr<Object> getObject(Poco::UInt64 GUID);

    bool addObject(SharedPtr<Object> object);
    void removeObject(Poco::UInt64 GUID);

    inline void addToMoveList(Poco::UInt64 GUID)
    {
        _moveList.push_back(GUID);
    }

    inline Poco::UInt16 GetPositionX()
    {
        return _x;
    }

    inline Poco::UInt16 GetPositionY()
    {
        return _y;
    }

    inline Poco::UInt32 hashCode()
    {
        return (_x << 16) |  _y;
    }
    
    inline bool hasPlayers()
    {
        return _playersInGrid > 0;
    }

private:
    ObjectMap _objects;
    ObjectList _moveList;
    Poco::UInt32 _playersInGrid;
    Poco::UInt16 _x;
    Poco::UInt16 _y;
    Poco::UInt32 _lastTick;
};

class GridLoader: public Poco::Runnable
{
private:
    struct GridCompare
    {
        bool operator() (Poco::UInt32 hash1, Poco::UInt32 hash2) const
        {
            return hash1 < hash2;
        }
    };
    
    typedef google::dense_hash_map<Poco::UInt32 /*hash*/, Grid* /*object*/, std::hash<Poco::UInt32>, GridCompare> GridsMap;
    typedef std::pair<Poco::UInt32 /*hash*/, Grid* /*grid*/> GridInserter;

public:
    GridLoader();
    ~GridLoader();
    static GridLoader& instance()
    {
        static Poco::SingletonHolder<GridLoader> sh;
        return *sh.get();
    }
    void initialize(Server* server);
    
    bool checkAndLoad(Poco::UInt16 x, Poco::UInt16 y);
    Grid* GetGrid(Poco::UInt16 x, Poco::UInt16 y);
    Grid* GetGridOrLoad(Poco::UInt16 x, Poco::UInt16 y);

    Grid* addObjectTo(Poco::UInt16 x, Poco::UInt16 y, SharedPtr<Object> object);
    Grid* addObject(SharedPtr<Object> object);
    bool removeObject(Object* object);

    std::list<Poco::UInt64> ObjectsInGrid(Object* object);
    std::list<Poco::UInt64> ObjectsInGridNear(Object* object, float distance);

    void addToMoveList(Poco::UInt64 GUID);

    virtual void run()
    {
        run_impl();
    }
    void run_impl();
    
private:
    Server* _server;
    
    Poco::ThreadPool* _gridsPool;
    GridsMap _grids;
    bool _isGridLoaded[MAX_X][MAX_Y];
};

#define sGridLoader GridLoader::instance()

#endif