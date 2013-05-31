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
#include <queue>

//@ Basic server information
// >> Server runs on multiple threads, grids are in a thread pool
#include "Poco/ErrorHandler.h"
#include "Poco/Task.h"
#include "Poco/TaskManager.h"
#include "Poco/TaskNotification.h"
#include "Poco/Observer.h"
#include "Poco/RunnableAdapter.h"
#include "Poco/SingletonHolder.h"
#include "Poco/Mutex.h"
#include "Poco/Condition.h"

//@ Everything is stored in SharedPtrs
#include "Poco/SharedPtr.h"

//@ Hash Map
#include "hash_map.h"
#include "stack_allocator.h"

using Poco::Observer;
using Poco::SharedPtr;

class Object;
class Server;
class GridManager;

class Grid
{
private:
    typedef rde::hash_map<Poco::UInt64 /*guid*/, Poco::SharedPtr<Object>> ObjectMap;

public:
    typedef std::list<Grid*> GridsList;

    Grid(Poco::UInt16 x, Poco::UInt16 y);
    bool update(Poco::UInt64 diff);

    GuidsSet getObjects(Poco::UInt32 highGUID);
    SharedPtr<Object> getObject(Poco::UInt64 GUID);

    bool addObject(SharedPtr<Object> object);
    void removeObject(Poco::UInt64 GUID);

    GridsList findNearGrids(SharedPtr<Object> object);
    void visit(SharedPtr<Object> object, GuidsSet& objects, Poco::UInt64 diff = 0);
    
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

public:
    static Poco::UInt8 losRange;

private:
    ObjectMap _objects;
    ObjectMap _players;
    Poco::UInt32 _forceLoad;
    Poco::UInt16 _x;
    Poco::UInt16 _y;
};

class GridManager: public Poco::TaskManager
{
public:
    GridManager();

    void start(Poco::Task* task);
    void dequeue();
    void wait();

private:
    std::queue<Poco::Task*> _queue;
    Poco::Mutex _mutex;
};

class GridLoader
{
private:    
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
    
    void update(Poco::UInt64 diff);
    void gridUpdated(Poco::TaskFinishedNotification* nf);
    
private:
    Grid* addObjectTo(Poco::UInt16 x, Poco::UInt16 y, SharedPtr<Object> object);

private:
    Server* _server;
    
    GridManager _gridManager;
    GridsMap _grids;
    bool _isGridLoaded[MAX_X][MAX_Y];
};

class GridTask: public Poco::Task
{
public:
    GridTask(Grid* grid, Poco::UInt64 diff);

    void runTask();
    bool getResult();

    inline Grid* getGrid()
    {
        return _grid;
    }

private:
    Grid* _grid;
    Poco::UInt64 _diff;
    bool _result;
};

#define sGridLoader GridLoader::instance()

#endif