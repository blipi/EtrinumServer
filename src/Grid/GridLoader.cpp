#include "GridLoader.h"
#include "Grid.h"
#include "GridManager.h"
#include "GridTask.h"

#include "debugging.h"
#include "Log.h"
#include "Object.h"
#include "Server.h"
#include "ServerConfig.h"
#include "Tools.h"

#include "Poco/Observer.h"
#include "Poco/Timestamp.h"
#include "Poco/Task.h"

using Poco::Observer;
using Poco::Timestamp;

/**
 * Initializes the grid loader, which manages and handles all grids
 *
 */
GridLoader::GridLoader()
{
    // Create the GridManager and add a grid finished update callback
    _gridManager = new GridManager(sConfig.getDefaultInt("MapThreads", 1));
    _gridManager->addObserver(Observer<GridLoader, Poco::TaskFinishedNotification>(*this, &GridLoader::gridUpdated));

    for (Poco::UInt16 x = 0; x < MAX_X; ++x)
        for (Poco::UInt16 y = 0; y < MAX_Y; ++y)
            _isGridLoaded[x][y] = false;

    // Set LoS range
    Grid::losRange = sConfig.getDefaultInt("LoSRange", 35);
    Grid::gridRemove = sConfig.getDefaultInt("GridRemove", 15000);
    sLog.out(Message::PRIO_TRACE, "\t[OK] LoS Range set to: %d", Grid::gridRemove);
    sLog.out(Message::PRIO_TRACE, "\t[OK] Grid Remove interval set to: %d", Grid::losRange);
    sLog.out(Message::PRIO_TRACE, "\t[OK] Map threads set to: %d", _gridManager->getMaxThreads());

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
    _gridManager->joinAll();
    delete _gridManager;
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
    ASSERT(x > 0 && x <= MAX_X)
    ASSERT(y > 0 && y <= MAX_Y)

    if (_isGridLoaded[x][y])
        return true;

    Grid* grid = new Grid(x, y);
    bool inserted = _grids.insert(rde::make_pair(grid->hashCode(), grid)).second;
    _isGridLoaded[x][y] = inserted;

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
        grid->removeObject(object);
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
    // Do NOT, never, iterate the list, iterate a safe copy!
    GridsMap grids = _grids;
    
    _gridManager->queue(grids.size());
    for (GridsMap::const_iterator itr = grids.begin(); itr != grids.end(); )
    {
        Grid* grid = itr->second;
        ++itr;

        // Start a task to update the grid
        _gridManager->start(new GridTask(grid, diff));
    }

    // Wait for all map updates to end
    _gridManager->wait();

    for (GridsSet::iterator itr = _remove.begin(); itr != _remove.end(); )
    {
        Grid* grid = *itr;
        ++itr;
        sLog.out(Message::PRIO_DEBUG, "Grid (%d, %d) has been deleted (%d %d)", grid->GetPositionX(), grid->GetPositionY(), grid->hasPlayers(), grid->isForceLoaded());

        _isGridLoaded[grid->GetPositionX()][grid->GetPositionY()] = false;
        _grids.erase(grid->hashCode());
        delete grid;
    }
    _remove.clear();
}

void GridLoader::gridUpdated(Poco::TaskFinishedNotification* nf)
{
    GridTask* task = (GridTask*)nf->task();
    nf->release();

    // If update fails, something went really wrong, delete this grid
    // If the grid has no players in it, check for nearby grids, if they are not loaded or have no players, remove it
    if (!task->getResult())
    {
        Grid* grid = task->getGrid();
        _remove.insert(grid);
    }

    _gridManager->dequeue();
}
