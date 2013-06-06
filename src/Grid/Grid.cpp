#include "Grid.h"
#include "GridLoader.h"
#include "Sector.h"

#include "debugging.h"
#include "Creature.h"
#include "Log.h"
#include "Object.h"
#include "Packet.h"
#include "Player.h"
#include "Server.h"
#include "Tools.h"

Poco::UInt8 Grid::LOSRange;
Poco::UInt8 Grid::AggroRange;
Poco::UInt32 Grid::GridRemove;

/**
 * Initializes a Grid object
 *
 */
Grid::Grid(Poco::UInt16 x, Poco::UInt16 y):
    _x(x), _y(y),
    _playersCount(0)
{
    forceLoad();
}

Grid::~Grid()
{
    // Delete all sectors
    for (TypeSectorsMap::iterator itr = _sectors.begin(); itr != _sectors.end(); )
    {
        Sector* sector = itr->second;
        ++itr;

        _sectors.erase(sector->hashCode());
        delete sector;
    }
}

/**
 * Updates the Grid and its objects
 *
 * @return false if the grid must be deleted, true otherwise
 */
bool Grid::update(Poco::UInt64 diff)
{
    Poco::Mutex::ScopedLock lock(_mutex);
    
    // Iterate a safe list
    TypeSectorsMap sectors = _sectors;
    TypeHashList eraseSectors;
    for (TypeSectorsMap::iterator itr = sectors.begin(); itr != sectors.end(); )
    {
        Sector* sector = itr->second;
        ++itr;
        
        // If updates fails, there are no objects, delete sector
        if (!sector->update(diff))
            eraseSectors.push_back(sector->hashCode());
    }

    for (TypeHashList::iterator itr = eraseSectors.begin(); itr != eraseSectors.end();)
    {
        Poco::UInt16 hash = *itr;
        ++itr;

        delete _sectors[hash];
        _sectors.erase(hash);
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
    if (gridX < LOSRange && GetPositionX() > 1)
    {
        Grid* grid = sGridLoader.GetGridOrLoad(GetPositionX() - 1, GetPositionY());
        grid->forceLoad();
        nearGrids.push_back(grid);

        // Near at left top corner
        if (gridY < LOSRange && GetPositionY() > 1)
        {
            Grid* grid = sGridLoader.GetGridOrLoad(GetPositionX() - 1, GetPositionY() - 1);
            grid->forceLoad();
            nearGrids.push_back(grid);
        }

        // Near at left bottom corner
        if (gridY > UNITS_PER_CELL - LOSRange && GetPositionY() < 0xFF)
        {
            Grid* grid = sGridLoader.GetGridOrLoad(GetPositionX() - 1, GetPositionY() + 1);
            grid->forceLoad();
            nearGrids.push_back(grid);
        }
    }

    // Near at top
    if (gridY < LOSRange && GetPositionY() > 1)
    {
        Grid* grid = sGridLoader.GetGridOrLoad(GetPositionX(), GetPositionY() - 1);
        grid->forceLoad();
        nearGrids.push_back(grid);
    }

    // Near at right
    if (gridX > UNITS_PER_CELL - LOSRange && GetPositionX() < 0xFF)
    {
        Grid* grid = sGridLoader.GetGridOrLoad(GetPositionX() + 1, GetPositionY());
        grid->forceLoad();
        nearGrids.push_back(grid);

        // Near at right top corner
        if (gridY < LOSRange && GetPositionY() > 1)
        {
            Grid* grid = sGridLoader.GetGridOrLoad(GetPositionX() + 1, GetPositionY() - 1);
            grid->forceLoad();
            nearGrids.push_back(grid);
        }

        // Near at right bottom corner
        if (gridY > UNITS_PER_CELL - LOSRange && GetPositionY() < 0xFF)
        {
            Grid* grid = sGridLoader.GetGridOrLoad(GetPositionX() + 1, GetPositionY() + 1);
            grid->forceLoad();
            nearGrids.push_back(grid);
        }
    }

    // Near at bottom
    if (gridY > UNITS_PER_CELL - LOSRange && GetPositionY() < 0xFF)
    {
        Grid* grid = sGridLoader.GetGridOrLoad(GetPositionX(), GetPositionY() + 1);
        grid->forceLoad();
        nearGrids.push_back(grid);
    }

    return nearGrids;
}

Sector* Grid::getOrLoadSector(Poco::UInt16 hash)
{
    Poco::Mutex::ScopedLock lock(_mutex);
    return getOrLoadSector_i(hash);
}

Sector* Grid::getOrLoadSector_i(Poco::UInt16 hash)
{
    TypeSectorsMap::iterator itr = _sectors.find(hash);
    if (itr != _sectors.end())
        return itr->second;

    Sector* sector = new Sector(hash, this);
    _sectors.insert(rde::make_pair(hash, sector));
    return sector;
}

/**
 * Adds an object to the grid
 *
 * @param object Object to be added
 * @return true if the object has been added
 */
bool Grid::addObject(SharedPtr<Object> object)
{
    if (getOrLoadSector(object->GetPosition().sector)->add(object))
    {
        object->SetGrid(this);
        return true;
    }

    return false;
}

/**
 * Removes an object from the Grid
 *
 */
void Grid::removeObject(SharedPtr<Object> object)
{
    getOrLoadSector(object->GetPosition().sector)->remove(object);
}


/**
 * Forces a Grid to remain loaded (this happens when a players gets near to a grid!)
 *
 */
void Grid::forceLoad()   
{
    _forceLoad.update();
}

/**
 * Checks whether a grid has been force loaded or not
 *
 * @return true if it's force loaded
 */
bool Grid::isForceLoaded()
{
    return (_forceLoad.elapsed() / 1000) < GridRemove;
}
