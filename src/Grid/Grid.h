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
#include "hash_map.h"
#include "stack_allocator.h"


using Poco::SharedPtr;

class Object;
class Server;

class Grid
{
private:
    typedef rde::hash_map<Poco::UInt64 /*guid*/, Poco::SharedPtr<Object>> ObjectMap;

public:
    typedef std::list<Grid*> GridsList;

    Grid(Poco::UInt16 x, Poco::UInt16 y);
    bool update();

    GuidsSet getObjects(Poco::UInt32 highGUID);
    SharedPtr<Object> getObject(Poco::UInt64 GUID);

    bool addObject(SharedPtr<Object> object);
    void removeObject(Poco::UInt64 GUID);

    GridsList findNearGrids(SharedPtr<Object> object);
    void visit(SharedPtr<Object> object, GuidsSet& objects);
    
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
        return !_players.empty();
    }

    void forceLoad();
    bool isForceLoaded();

private:
    ObjectMap _objects;
    ObjectMap _players;
    Poco::UInt32 _forceLoad;
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
            return hash1 == hash2;
        }
    };
    
    typedef rde::hash_map<Poco::UInt32 /*hash*/, Grid* /*object*/> GridsMap;

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

    Grid* addObject(SharedPtr<Object> object);
    bool removeObject(Object* object);

    virtual void run()
    {
        run_impl();
    }
    void run_impl();
    
private:
    Grid* addObjectTo(Poco::UInt16 x, Poco::UInt16 y, SharedPtr<Object> object);

private:
    Server* _server;
    
    Poco::ThreadPool* _gridsPool;
    GridsMap _grids;
    bool _isGridLoaded[MAX_X][MAX_Y];
};

#define sGridLoader GridLoader::instance()

#endif