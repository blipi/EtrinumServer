#include "Grid.h"
#include "Server.h"
#include "Object.h"
#include "Player.h"
#include "Tools.h"
#include "Config.h"
#include "Log.h"

#include "debugging.h"

/**
 * Initializes a Grid object
 *
 */
Grid::Grid(Poco::UInt16 x, Poco::UInt16 y):
    _x(x), _y(y)
{
    _lastTick = clock();
}

/**
 * Updates the Grid and its objects
 *
 * @return false if the grid must be deleted, true otherwise
 */
bool Grid::update()
{
    // Update only if it's more than 1ms since last tick
    if (clock() - _lastTick > 0)
    {
        for (ObjectMap::const_iterator itr = _players.begin(); itr != _players.end(); )
        {
            SharedPtr<Object> object = itr->second;
            ++itr;

            // Find near grids
            // Must be done before moving, other we may run into LoS problems
            GridsList nearGrids = findNearGrids(object);

            // Update AI, movement, everything if there is any or we have to
            // If update returns false, that means the object is no longer in this grid!
            if (!object->update(clock() - _lastTick))
                _players.erase(object->GetGUID());

            // Update near mobs
            GuidsSet objects;
            visit(object, objects);

            // Update other grids near mobs
            for (GridsList::iterator nGrid = nearGrids.begin(); nGrid != nearGrids.end(); nGrid++)
                (*nGrid)->visit(object, objects);

            // Update players LoS
            // Players will update mobs LoS in its method
            object->ToPlayer()->UpdateLoS(objects);
        }

        _lastTick = clock();
    }
    return true;
}

/**
 * Returns all near grids to the object
 *
 * @param object Object which we must find near grids
 * @return the list of near grids
 */
Grid::GridsList Grid::findNearGrids(SharedPtr<Object> object)
{
    Vector2D position = object->GetPosition();
    Poco::UInt16 gridX = Tools::GetPositionInXCell(GetPositionX(), position.x);
    Poco::UInt16 gridY = Tools::GetPositionInYCell(GetPositionY(), position.z);

    std::list<Grid*> nearGrids;
    // Near at left
    if (gridX < 35 && GetPositionX() > 1)
    {
        Grid* grid = sGridLoader.GetGridOrLoad(GetPositionX() - 1, GetPositionY());
        grid->forceLoad();
        nearGrids.push_back(grid);

        // Near at left top corner
        if (gridY < 35 && GetPositionY() > 1)
        {
            Grid* grid = sGridLoader.GetGridOrLoad(GetPositionX() - 1, GetPositionY() - 1);
            grid->forceLoad();
            nearGrids.push_back(grid);
        }

        // Near at left bottom corner
        if (gridY > UNITS_PER_CELL - 35 && GetPositionY() < 0xFF)
        {
            Grid* grid = sGridLoader.GetGridOrLoad(GetPositionX() - 1, GetPositionY() + 1);
            grid->forceLoad();
            nearGrids.push_back(grid);
        }
    }

    // Near at top
    if (gridY < 35 && GetPositionY() > 1)
    {
        Grid* grid = sGridLoader.GetGridOrLoad(GetPositionX(), GetPositionY() - 1);
        grid->forceLoad();
        nearGrids.push_back(grid);
    }

    // Near at right
    if (gridX > UNITS_PER_CELL - 35 && GetPositionX() < 0xFF)
    {
        Grid* grid = sGridLoader.GetGridOrLoad(GetPositionX() + 1, GetPositionY());
        grid->forceLoad();
        nearGrids.push_back(grid);

        // Near at right top corner
        if (gridY < 35 && GetPositionY() > 1)
        {
            Grid* grid = sGridLoader.GetGridOrLoad(GetPositionX() + 1, GetPositionY() - 1);
            grid->forceLoad();
            nearGrids.push_back(grid);
        }

        // Near at right bottom corner
        if (gridY > UNITS_PER_CELL - 35 && GetPositionY() < 0xFF)
        {
            Grid* grid = sGridLoader.GetGridOrLoad(GetPositionX() + 1, GetPositionY() + 1);
            grid->forceLoad();
            nearGrids.push_back(grid);
        }
    }

    // Near at bottom
    if (gridY > UNITS_PER_CELL - 35 && GetPositionY() < 0xFF)
    {
        Grid* grid = sGridLoader.GetGridOrLoad(GetPositionX(), GetPositionY() + 1);
        grid->forceLoad();
        nearGrids.push_back(grid);
    }

    return nearGrids;
}

/**
 * Finds an object in the Grid near to a given position
 *
 * @param it dense_hash_map element
 * @param c Position taken as center
 * @return true if it's the searched object
 */
static bool findObjectsIf(rde::pair<Poco::UInt64, SharedPtr<Object> > it, Vector2D c)
{
    Poco::UInt32 x = it.second->GetPosition().x;
    Poco::UInt32 z = it.second->GetPosition().z;

    return (_max(x, 20) - 20 <= c.x && c.x <= x + 20 && _max(z, 20) - 20 <= c.z && c.z <= z + 20);
}

/**
 * Enables a player to visit a grid, be it its grid or a nearby grid
 *
 * @param object Visiting object
 * @param objets List where near objects are included
 */
void Grid::visit(SharedPtr<Object> object, GuidsSet& objects)
{
    Vector2D c(object->GetPosition().x, object->GetPosition().z);
    ObjectMap::iterator it = rde::find_if(_objects.begin(), _objects.end(), c, findObjectsIf);
    while (it != _objects.end())
    {
        // Update the object, if it fails, it means it is in a new grid
        SharedPtr<Object> obj = it->second;
        // Find next near object now, avoid issues
        it = rde::find_if(++it, _objects.end(),c, findObjectsIf);

        if (!obj->update(clock() - _lastTick))
            _objects.erase(obj->GetGUID());
        else
            objects.insert(obj->GetGUID());
    }

    it = rde::find_if(_players.begin(), _players.end(), c, findObjectsIf);
    while (it != _players.end())
    {
        if (it->first != object->GetGUID())
            objects.insert(it->first);

        it = rde::find_if(++it, _players.end(),c, findObjectsIf);
    }
}

/**
 * Returns a list of GUIDs
 *
 * @param highGUID object type to gather
 * @return the objects list
 */
GuidsSet Grid::getObjects(Poco::UInt32 highGUID)
{
    GuidsSet objects;

    if (highGUID & HIGH_GUID_PLAYER)
    {
        for (ObjectMap::const_iterator itr = _players.begin(); itr != _players.end(); )
        {
            Poco::UInt64 GUID = itr->first;
            itr++;
            objects.insert(GUID);
        }
    }

    if (highGUID & ~HIGH_GUID_PLAYER)
    {
        for (ObjectMap::const_iterator itr = _objects.begin(); itr != _objects.end(); )
        {
            Poco::UInt64 GUID = itr->first;
            itr++;

            if (GUID & highGUID)
                objects.insert(GUID);
        }
    }

    return objects;
}

/**
 * Returns the object specified by its complete GUID
 *
 * @param GUID object's GUID
 * @return NULL SharedPtr if not found, the object otherwise
 */
SharedPtr<Object> Grid::getObject(Poco::UInt64 GUID)
{
    SharedPtr<Object> object = NULL;
    
    if (HIGUID(GUID) & HIGH_GUID_PLAYER)
    {
        ObjectMap::iterator itr = _players.find(GUID);
        if (itr != _players.end())
            object = itr->second;
    }
    else
    {
        ObjectMap::iterator itr = _objects.find(GUID);
        if (itr != _objects.end())
            object = itr->second;
    }

    return object;
}

/**
 * Adds an object to the grid
 *
 * @param object Object to be added
 * @return true if the object has been added
 */
bool Grid::addObject(SharedPtr<Object> object)
{
    bool inserted = false;
    
    if (object->GetHighGUID() & HIGH_GUID_PLAYER)
        inserted = _players.insert(rde::make_pair(object->GetGUID(), object)).second;
    else
        inserted = _objects.insert(rde::make_pair(object->GetGUID(), object)).second;    

    if (inserted)
        object->SetGrid(this);

    return inserted;
}

/**
 * Removes an object from the Grid
 *
 */
void Grid::removeObject(Poco::UInt64 GUID)
{
    if (HIGUID(GUID) & HIGH_GUID_PLAYER)
        _players.erase(GUID);
    else
        _objects.erase(GUID);
}


/**
 * Forces a Grid to remain loaded (this happens when a players gets near to a grid!)
 *
 */
void Grid::forceLoad()   
{
    _forceLoad = clock();
}

/**
 * Checks whether a grid has been force loaded or not
 *
 * @return true if it's force loaded
 */
bool Grid::isForceLoaded()
{
    return (clock() - _forceLoad) < 5000;
}

/**
 * Initializes the grid loader, which manages and handles all grids
 *
 */
GridLoader::GridLoader():
    _server(NULL)
{
    //@todo: We should be able to have more than one grid handler
    // and they should all balance themselves, in order to keep
    // update rate really low (in ms)
    _gridsPool = new Poco::ThreadPool(1, 1);

    for (Poco::UInt16 x = 0; x < MAX_X; x++)
        for (Poco::UInt16 y = 0; y < MAX_Y; y++)
            _isGridLoaded[x][y] = false;
}

/**
 * Class destructor, frees memory
 *
 */
GridLoader::~GridLoader()
{
    delete _gridsPool;
}

/**
 * Initializes the grid loader, called uppon server start
 *
 * @param server Server reference, to keep this alive
 */
void GridLoader::initialize(Server* server)
{
    _server = server;
    _gridsPool->start(*this);
}

/**
 * Checks if a grid is loaded, and if it's not, it loads it
 *
 * @param x X position of the grid
 * @param y Y position of the grid
 * @return True if the grid is successfully added
 */
bool GridLoader::checkAndLoad(Poco::UInt16 x, Poco::UInt16 y)
{
    ASSERT(x >= 0 && x <= MAX_X)
    ASSERT(y >= 0 && y <= MAX_Y)

    if (_isGridLoaded[x][y])
        return true;

    Grid* grid = new Grid(x, y);
    bool inserted = _grids.insert(rde::make_pair(grid->hashCode(), grid)).second;
    _isGridLoaded[x][y] = true;

    sLog.out(Message::PRIO_DEBUG, "Grid (%d, %d) has been created (%d)", x, y, inserted);
    return inserted;
}

/**
 * Gets a Grid. Should be safe as std::set iterators are safe, but we should keep and eye to this.
 *
 * @param x Cell Position x
 * @param y Cell Position y
 */
Grid* GridLoader::GetGrid(Poco::UInt16 x, Poco::UInt16 y)
{
    Grid* grid = NULL;
    if (!_isGridLoaded[x][y])
        return grid;

    GridsMap::const_iterator itr = _grids.find((x << 16) | y);
    if (itr != _grids.end())
        grid = itr->second;

    return grid;
}

/**
 * Checks if a Grid is loaded, if not it loads it. Finally, returns the Grid
 *
 * @param x X position of the Grid
 * @param y Y position of the Grid
 * @return The grid itself
 */
Grid* GridLoader::GetGridOrLoad(Poco::UInt16 x, Poco::UInt16 y)
{
    if (checkAndLoad(x, y))
        return GetGrid(x, y);
    return NULL;
}

/**
 * Given an object, it adds it to a Grid
 *
 * @param x X position of the Grid
 * @param y Y position of the Grid
 * @param object Object to be added
 * @return Grid where the object has been added
 */
Grid* GridLoader::addObjectTo(Poco::UInt16 x, Poco::UInt16 y, SharedPtr<Object> object)
{
    if (!checkAndLoad(x, y))
        return NULL;
    
    Grid* grid = NULL;
    if (grid = GetGrid(x, y))
        if (!grid->addObject(object))
            grid = NULL;

    return grid;
}

/**
 * Adds an object to the Grid where it should be
 *
 * @param object Object to be added
 * @return Grid where it has been added
 */
Grid* GridLoader::addObject(SharedPtr<Object> object)
{
    Vector2D pos = object->GetPosition();
    return addObjectTo(Tools::GetXCellFromPos(pos.x), Tools::GetYCellFromPos(pos.z), object);
}

/**
 * Removes an object from the Grid where it is
 *
 * @param object Object to be removed
 * @return True if the object has been removed
 */
bool GridLoader::removeObject(Object* object)
{
    Poco::UInt16 x = Tools::GetXCellFromPos(object->GetPosition().x);
    Poco::UInt16 y = Tools::GetYCellFromPos(object->GetPosition().z);

    if (!_isGridLoaded[x][y])
        return false;

    if (Grid* grid = GetGrid(x, y))
    {
        grid->removeObject(object->GetGUID());
        object->SetGrid(NULL);
        return true;
    }

    return false;
}

/**
 * Thread where the Grids are updating
 *
 */
void GridLoader::run_impl()
{
    while (sServer->isRunning())
    {
        // Update all Grids
        for (GridsMap::const_iterator itr = _grids.begin(); itr != _grids.end(); )
        {
            Grid* grid = itr->second;
            itr++;

            // If update fails, something went really wrong, delete this grid
            // If the grid has no players in it, check for nearby grids, if they are not loaded or have no players, remove it
            if (!grid->update() || (!grid->hasPlayers() && !grid->isForceLoaded()))
            {
                sLog.out(Message::PRIO_DEBUG, "Grid (%d, %d) has been deleted (%d %d)", grid->GetPositionX(), grid->GetPositionY(), grid->hasPlayers(), grid->isForceLoaded());
                
                _isGridLoaded[grid->GetPositionX()][grid->GetPositionY()] = false;
                _grids.erase(grid->hashCode());
                delete grid;
            }
        }

        Poco::Thread::sleep(1);
    }
}
