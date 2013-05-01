#ifndef BASIC_SERVER_GRID_H
#define BASIC_SERVER_GRID_H

#include <list>
#include <map>

//@ Basic server information
// >> Server runs on multiple threads, grids are in a thread pool
#include "Poco/ErrorHandler.h"
#include "Poco/ThreadPool.h"
#include "Poco/RunnableAdapter.h"
#include "Poco/SingletonHolder.h"
#include "Poco/RWLock.h"

//@ Everything is stored in SharedPtrs
#include "Poco/SharedPtr.h"

#include "defines.h"

using Poco::SharedPtr;

class Object;
class Server;

class Grid
{
public:
    typedef std::map<Poco::UInt64 /*guid*/, Poco::SharedPtr<Object> /*client*/> ObjectMap;
    typedef std::pair<Poco::UInt64 /*guid*/, Poco::SharedPtr<Object> /*client*/> ObjectMapInserter;
    typedef std::list<Poco::UInt64> ObjectList;

    Grid(Poco::UInt16 x, Poco::UInt16 y);
    bool update();

    std::list<Poco::UInt64> getObjects();
    SharedPtr<Object> getObject(Poco::UInt64 GUID);
    bool addObject(SharedPtr<Object> object);
    void removeObject(Poco::UInt64 GUID);

    inline Poco::UInt16 GetPositionX()
    {
        return _x;
    }
    inline Poco::UInt16 GetPositionY()
    {
        return _y;
    }
    
    inline Poco::UInt32 getNumberOfPlayers()
    {
        return _playersInGrid;
    }

    bool mustDoRemoveCheck();

private:
    Poco::UInt32 _playersInGrid;
    Poco::UInt32 _lastRemoveCheck;
    
    ObjectMap _objects;    
    Poco::RWLock _objectsLock;

    ObjectList _moveList;
    Poco::RWLock _moveListLock;

    Poco::UInt16 _x;
    Poco::UInt16 _y;
    Poco::UInt32 _lastTick;
};

class GridLoader: public Poco::Runnable
{
private:
    typedef std::list<Grid*> GridsList;

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
    bool checkNearbyGrid(Poco::UInt16 x, Poco::UInt16 y);
    void removeGrid(Grid* grid);

private:
    Server* _server;
    
    Poco::ThreadPool* _gridsPool;
    GridsList _grids;
    GridsList _removeList;
    GridsList::const_iterator _gridsIterator;

    Grid::ObjectList _moveList;
    Poco::RWLock _gridsLock;
    bool _isGridLoaded[MAX_X][MAX_Y];
};

#define sGridLoader GridLoader::instance()

#endif