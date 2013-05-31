#include "Grid.h"
#include "Server.h"
#include "Object.h"
#include "Player.h"
#include "Creature.h"
#include "Tools.h"
#include "ServerConfig.h"
#include "Log.h"
#include "debugging.h"

#include "Poco/Timestamp.h"

using Poco::Timestamp;

Poco::UInt8 Grid::losRange;

/**
 * Initializes a Grid object
 *
 */
Grid::Grid(Poco::UInt16 x, Poco::UInt16 y):
    _x(x), _y(y),
    _playersCount(0)
{
}

/**
 * Updates the Grid and its objects
 *
 * @return false if the grid must be deleted, true otherwise
 */
bool Grid::update(Poco::UInt64 diff)
{
    // Update all objects
    for (ObjectMap::const_iterator itr = _objects.begin(); itr != _objects.end(); )
    {
        SharedPtr<Object> object = itr->second;
        ++itr;

        // For non players, update only if a player is near
        if (!(object->GetHighGUID() & HIGH_GUID_PLAYER))
            if (!object->hasNearPlayers())
                continue;

        // Find near grids
        // Must be done before moving, other we may run into LoS problems
        GridsList nearGrids = findNearGrids(object);

        // Get last update time (in case the object switches grid)
        Poco::UInt64 objectDiff = object->getLastUpdate();
        if (objectDiff > diff)
            objectDiff = diff;

        // Update AI, movement, everything if there is any or we have to
        // If update returns false, that means the object is no longer in this grid!
        if (!object->update(objectDiff))
            removeObject(object->GetGUID());

        // Visit near objects as to update LoS
        GuidsSet objects;
        visit(object, objects);

        // Update other grids near objects
        for (GridsList::iterator nGrid = nearGrids.begin(); nGrid != nearGrids.end(); nGrid++)
            (*nGrid)->visit(object, objects);

        // Object the object LoS if it's a player or a creature
        if (Character* character = object->ToCharacter())
            character->UpdateLoS(objects);
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
    if (gridX < losRange && GetPositionX() > 1)
    {
        Grid* grid = sGridLoader.GetGridOrLoad(GetPositionX() - 1, GetPositionY());
        grid->forceLoad();
        nearGrids.push_back(grid);

        // Near at left top corner
        if (gridY < losRange && GetPositionY() > 1)
        {
            Grid* grid = sGridLoader.GetGridOrLoad(GetPositionX() - 1, GetPositionY() - 1);
            grid->forceLoad();
            nearGrids.push_back(grid);
        }

        // Near at left bottom corner
        if (gridY > UNITS_PER_CELL - losRange && GetPositionY() < 0xFF)
        {
            Grid* grid = sGridLoader.GetGridOrLoad(GetPositionX() - 1, GetPositionY() + 1);
            grid->forceLoad();
            nearGrids.push_back(grid);
        }
    }

    // Near at top
    if (gridY < losRange && GetPositionY() > 1)
    {
        Grid* grid = sGridLoader.GetGridOrLoad(GetPositionX(), GetPositionY() - 1);
        grid->forceLoad();
        nearGrids.push_back(grid);
    }

    // Near at right
    if (gridX > UNITS_PER_CELL - losRange && GetPositionX() < 0xFF)
    {
        Grid* grid = sGridLoader.GetGridOrLoad(GetPositionX() + 1, GetPositionY());
        grid->forceLoad();
        nearGrids.push_back(grid);

        // Near at right top corner
        if (gridY < losRange && GetPositionY() > 1)
        {
            Grid* grid = sGridLoader.GetGridOrLoad(GetPositionX() + 1, GetPositionY() - 1);
            grid->forceLoad();
            nearGrids.push_back(grid);
        }

        // Near at right bottom corner
        if (gridY > UNITS_PER_CELL - losRange && GetPositionY() < 0xFF)
        {
            Grid* grid = sGridLoader.GetGridOrLoad(GetPositionX() + 1, GetPositionY() + 1);
            grid->forceLoad();
            nearGrids.push_back(grid);
        }
    }

    // Near at bottom
    if (gridY > UNITS_PER_CELL - losRange && GetPositionY() < 0xFF)
    {
        Grid* grid = sGridLoader.GetGridOrLoad(GetPositionX(), GetPositionY() + 1);
        grid->forceLoad();
        nearGrids.push_back(grid);
    }

    return nearGrids;
}

/**
 * Finds an object in radius in a Grid given a centre 
 *
 * @param it dense_hash_map element
 * @param c Position taken as centre
 * @return true if it's the searched object
 */
static bool findObjectsIf(rde::pair<Poco::UInt64, SharedPtr<Object> > it, Vector2D c)
{
    float x = it.second->GetPosition().x;
    float z = it.second->GetPosition().z;

    return (_max(x, Grid::losRange) - Grid::losRange <= c.x && c.x <= x + Grid::losRange && _max(z, Grid::losRange) - Grid::losRange <= c.z && c.z <= z + Grid::losRange);
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
        it = rde::find_if(++it, _objects.end(), c, findObjectsIf);

        // Don't add the object to itself list
        if (object->GetGUID() == obj->GetGUID())
            continue;

        // Add the object to the seen objects list
        objects.insert(obj->GetGUID());

        // If the visitor is a player and the object seen is a creature, tell the creature there is a player
        if ((object->GetHighGUID() & HIGH_GUID_PLAYER) && (obj->GetHighGUID() & HIGH_GUID_CREATURE))
            if (Creature* creature = obj->ToCreature())
                creature->addPlayerToLoS(object->GetGUID());
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

    for (ObjectMap::const_iterator itr = _objects.begin(); itr != _objects.end(); )
    {
        Poco::UInt64 GUID = itr->first;
        itr++;

        if (GUID & highGUID)
            objects.insert(GUID);
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
    
    ObjectMap::iterator itr = _objects.find(GUID);
    if (itr != _objects.end())
        object = itr->second;

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
    bool inserted = _objects.insert(rde::make_pair(object->GetGUID(), object)).second;
    if (inserted)
    {
        object->SetGrid(this);
        
        if (object->GetHighGUID() & HIGH_GUID_PLAYER)
            _playersCount++;
    }

    return inserted;
}

/**
 * Removes an object from the Grid
 *
 */
void Grid::removeObject(Poco::UInt64 GUID)
{
    _objects.erase(GUID);
    if (HIGUID(GUID) & HIGH_GUID_PLAYER)
        _playersCount--;
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
    // Adds a Grid finished update callback
    _gridManager.addObserver(Observer<GridLoader, Poco::TaskFinishedNotification>(*this, &GridLoader::gridUpdated));

    for (Poco::UInt16 x = 0; x < MAX_X; x++)
        for (Poco::UInt16 y = 0; y < MAX_Y; y++)
            _isGridLoaded[x][y] = false;

    // Set LoS range
    Grid::losRange = sConfig.getIntConfig("LoSRange");
    sLog.out(Message::PRIO_TRACE, "\t[OK] LoS Range set to: %d", Grid::losRange);

    // Check for correct grid size
    ASSERT((MAP_MAX_X - MAP_MIN_X) / UNITS_PER_CELL < MAX_X)
    ASSERT((MAP_MAX_Z - MAP_MIN_Z) / UNITS_PER_CELL < MAX_Y)
}

/**
 * Class destructor, frees memory
 *
 */
GridLoader::~GridLoader()
{
    _gridManager.joinAll();
}

/**
 * Initializes the grid loader, called uppon server start
 *
 * @param server Server reference, to keep this alive
 */
void GridLoader::initialize(Server* server)
{
    _server = server;
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
void GridLoader::update(Poco::UInt64 diff)
{
    // Update all current grids, new grids shouldn't be updated now
    GridsMap safeGrids = _grids;

    for (GridsMap::const_iterator itr = safeGrids.begin(); itr != safeGrids.end(); )
    {
        Grid* grid = itr->second;
        itr++;

        // Start a task to update the grid
        _gridManager.start(new GridTask(grid, diff));
    }

    // Wait for all map updates to end
    _gridManager.wait();
}

void GridLoader::gridUpdated(Poco::TaskFinishedNotification* nf)
{
    GridTask* task = (GridTask*)nf->task();

    // If update fails, something went really wrong, delete this grid
    // If the grid has no players in it, check for nearby grids, if they are not loaded or have no players, remove it
    if (!task->getResult())
    {
        Grid* grid = task->getGrid();
        sLog.out(Message::PRIO_DEBUG, "Grid (%d, %d) has been deleted (%d %d)", grid->GetPositionX(), grid->GetPositionY(), grid->hasPlayers(), grid->isForceLoaded());
                
        _isGridLoaded[grid->GetPositionX()][grid->GetPositionY()] = false;
        _grids.erase(grid->hashCode());
        delete grid;
    }

    _gridManager.dequeue();
}

GridManager::GridManager():
    TaskManager()
{
}

void GridManager::start(Poco::Task* task)
{
    // @todo MAX Threads Configurable
    if (count() < 4)
        TaskManager::start(task);
    else
    {
        Poco::Mutex::ScopedLock lock(_mutex);
        _queue.push(task);
    }
}

void GridManager::dequeue()
{
    _mutex.lock();
    if (!_queue.empty())
    {
        Poco::Task* task = _queue.front();
        _queue.pop();
        _mutex.unlock();

        start(task);
    }
    else
        _mutex.unlock();
}

void GridManager::wait()
{
    while (count())
        Poco::Thread::sleep(1);
}

GridTask::GridTask(Grid* grid, Poco::UInt64 diff):
    Task(""),
    _grid(grid), _diff(diff)
{
}

void GridTask::runTask()
{
    _result = _grid->update(_diff);
}

bool GridTask::getResult()
{
    return _result && !(!_grid->hasPlayers() && !_grid->isForceLoaded());
}
