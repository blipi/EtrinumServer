#ifndef GAMESERVER_GRID_LOADER_H
#define GAMESERVER_GRID_LOADER_H

//@ Poco includes
#include "Poco/SingletonHolder.h"
#include "Poco/SharedPtr.h"
#include "Poco/TaskNotification.h"

//@ List and Hash Map
#include "hash_map.h"
#include "stack_allocator.h"

#include "defines.h"

using Poco::SharedPtr;

class GridManager;
class Grid;
class Object;
class Server;

class GridLoader
{
private:    
    typedef rde::hash_map<Poco::UInt32 /*hash*/, Grid* /*object*/> GridsMap;
    typedef std::set<Grid*> GridsSet;

public:
    GridLoader();
    ~GridLoader();
    static GridLoader& instance()
    {
        static Poco::SingletonHolder<GridLoader> sh;
        return *sh.get();
    }
    
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
    GridManager* _gridManager;
    GridsMap _grids;
    GridsSet _remove;
    bool _isGridLoaded[MAX_X][MAX_Y];
};

#define sGridLoader GridLoader::instance()

#endif