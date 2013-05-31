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
}

/**
 * Updates the Grid and its objects
 *
 * @return false if the grid must be deleted, true otherwise
 */
bool Grid::update(Poco::UInt64 diff)
{
    // Update all objects
    for (ObjectMap::iterator itr = _objects.begin(); itr != _objects.end(); )
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
        bool updateResult = object->update(objectDiff);

        // Visit near objects as to update LoS
        GuidsSet objects;
        visit(object, objects);

        // Update other grids near objects
        for (GridsList::iterator nGrid = nearGrids.begin(); nGrid != nearGrids.end(); nGrid++)
            (*nGrid)->visit(object, objects);

        // Object the object LoS if it's a player or a creature
        if (Character* character = object->ToCharacter())
            character->UpdateLoS(objects);

        if (!updateResult)
        {
            sGridLoader.addObject(object);
            removeObject(object->GetGUID());

            if (!hasPlayers())
                forceLoad();
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
