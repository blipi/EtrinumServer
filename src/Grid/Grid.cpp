#include "Grid.h"
#include "GridLoader.h"

#include "debugging.h"
#include "Creature.h"
#include "Log.h"
#include "Object.h"
#include "Player.h"
#include "Tools.h"

Poco::UInt8 Grid::losRange;
Poco::UInt32 Grid::gridRemove;




bool Sector::update(Poco::UInt64 diff)
{
    Poco::Mutex::ScopedLock lock(_mutex);

    // Update all objects
    for (TypeObjectsMap::iterator itr = _objects.begin(); itr != _objects.end(); )
    {
        SharedPtr<Object> object = itr->second;
        ++itr;

        // For non players, update only if a player is near
        if (!(object->GetHighGUID() & HIGH_GUID_PLAYER))
            if (!object->hasNearPlayers())
                continue;

        // Get last update time (in case the object switches grid)
        Poco::UInt64 objectDiff = object->getLastUpdate();
        if (objectDiff > diff)
            objectDiff = diff;

        // Update AI, movement, everything if there is any or we have to
        // If update returns false, that means the object is no longer in this grid!
        Poco::UInt16 prevSector = Tools::GetSector(object->GetPosition(), Grid::losRange);
        bool updateResult = object->update(objectDiff);
        
        if (Character* character = object->ToCharacter())
            character->UpdateLoS(_objects);

        // Change Grid if we have to
        if (!updateResult)
        {
            sGridLoader.addObject(object); // Add to the new Grid
            remove_i(object->GetGUID()); // Delete from the Sector (and Grid)
        }
        else if (object->hasFlag(FLAGS_TYPE_MOVEMENT, FLAG_MOVING))
        {
            Poco::UInt16 actSector = Tools::GetSector(object->GetPosition(), Grid::losRange);

            // Have we changed sector?
            if (prevSector != actSector)
            {
                _grid->getOrLoadSector_i(actSector)->add(object); // Add us to the new sector
                remove_i(object->GetGUID()); // Remove from this sector
            }
        }
        
    }

    return !_objects.empty();
}

bool Sector::add(SharedPtr<Object> object)
{
    Poco::Mutex::ScopedLock lock(_mutex);
    
    if (_objects.insert(rde::make_pair(object->GetGUID(), object)).second)
    {
        if (object->GetHighGUID() & HIGH_GUID_PLAYER)
            _grid->onPlayerAdded();

        return true;
    }

    return false;
}

void Sector::remove(Poco::UInt64 GUID)
{
    Poco::Mutex::ScopedLock lock(_mutex);
    remove_i(GUID);
}

void Sector::remove_i(Poco::UInt64 GUID)
{
    _objects.erase(GUID);

    if (HIGUID(GUID) & HIGH_GUID_PLAYER)
        _grid->onPlayerErased();
}

Poco::UInt16 Sector::hashCode()
{
    return _hash;
}

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
        itr++;

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

    for (TypeSectorsMap::iterator itr = _sectors.begin(); itr != _sectors.end(); )
    {
        Sector* sector = itr->second;
        itr++;
        
        // If updates fails, there are no objects, delete sector
        if (!sector->update(diff))
        {
            _sectors.erase(sector->hashCode());
            delete sector;
        }
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
    Poco::UInt16 hash = Tools::GetSector(object->GetPosition(), losRange);
    if (getOrLoadSector(hash)->add(object))
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
    Poco::UInt16 hash = Tools::GetSector(object->GetPosition(), losRange);
    getOrLoadSector(hash)->remove(object->GetGUID());
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
    return (_forceLoad.elapsed() / 1000) < 5000;
}
