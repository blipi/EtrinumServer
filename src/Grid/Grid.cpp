#include "Grid.h"
#include "Server.h"
#include "Object.h"
#include "Tools.h"

#ifdef SERVER_FRAMEWORK_TESTING

    #include "debugging.h"

#endif

Grid::Grid(Poco::UInt16 x, Poco::UInt16 y):
    _x(x), _y(y),
    _playersInGrid(0)
{
    _lastTick = clock();
    _objects.set_empty_key(NULL);
    _objects.set_deleted_key(std::numeric_limits<Poco::UInt64>::max());
}

/**
 * Finds an object in the Grid near to a given position
 *
 * @param it dense_hash_map element
 * @param c Position taken as center
 */
static bool findObjectsIf(std::pair<Poco::UInt64, SharedPtr<Object> > it, Vector2D c)
{
    Poco::UInt32 x = it.second->GetPosition().x;
    Poco::UInt32 y = it.second->GetPosition().y;

    return !(it.second->GetHighGUID() & HIGH_GUID_PLAYER) && (x - 20 >= c.x && x + 20 <= c.x && y - 20 >= c.y && y + 20 <= c.y);
}

/**
 * Updates the Grid and its objects
 *
 */
bool Grid::update()
{
    // Update only if it's more than 1ms since last tick
    if (clock() - _lastTick > 0)
    {
        // Delete an object if it's in the move list
        for (ObjectList::const_iterator itr = _moveList.cbegin(); itr != _moveList.cend();)
        {
            // Get the GUID and increment the iterator, it'd be safe to delete from movelist now, but we rather use clear
            Poco::UInt64 GUID = *itr;
            itr++;
            
            _objectsLock.writeLock();

            // Object MUST be in Grid
            if (_objects.find(GUID) == _objects.end())
                ASSERT(false);

            // Erase the object from this grid
            _objects.erase(GUID);
            _objectsLock.unlock();
            
            if (HIGUID(GUID) & HIGH_GUID_PLAYER)
                _playersInGrid--;
        }
        _moveList.clear();
        
        _objectsLock.readLock();
        for (ObjectMap::const_iterator itr = _objects.begin(); itr != _objects.end();)
        {
            SharedPtr<Object> object = itr->second;
            itr++;

            if (!(object->GetHighGUID() & HIGH_GUID_PLAYER))
                continue;

            // Update AI, movement, everything if there is any or we have to
            object->update(clock() - _lastTick);

            // Update near mobs
            Vector2D c(object->GetPosition().x, object->GetPosition().y);
            ObjectMap::iterator it = std::find_if(_objects.begin(), _objects.end(), std::bind2nd(std::ptr_fun(findObjectsIf), c));
            while (it != _objects.end())
            {
                // Update the object
                SharedPtr<Object> obj = it->second;
                obj->update(clock() - _lastTick);

                // Find next near object
                it = std::find_if(++it, _objects.end(), std::bind2nd(std::ptr_fun(findObjectsIf), c));
            }
        }

        _objectsLock.unlock();

        _lastTick = clock();
    }
    return true;
}


Grid::ObjectList Grid::getObjects(Poco::UInt32 highGUID)
{
    std::list<Poco::UInt64> objects;
        
    _objectsLock.readLock();
    for (ObjectMap::const_iterator itr = _objects.begin(); itr != _objects.end(); itr++)
    {
        SharedPtr<Object> object = itr->second;
        if (object->GetHighGUID() & highGUID)
            objects.push_back(itr->first);
    }
    _objectsLock.unlock();

    return objects;
}

SharedPtr<Object> Grid::getObject(Poco::UInt64 GUID)
{
    SharedPtr<Object> object = NULL;

    _objectsLock.readLock();

    ObjectMap::iterator itr = _objects.find(GUID);
    if (itr != _objects.end())
        object = itr->second;

    _objectsLock.unlock();

    return object;
}

bool Grid::addObject(SharedPtr<Object> object)
{
    _objectsLock.writeLock();

    bool inserted = _objects.insert(ObjectMapInserter(object->GetGUID(), object)).second;    
    if (inserted)
    {        
        if (object->GetHighGUID() & HIGH_GUID_PLAYER)
            _playersInGrid++;
    
        object->SetGrid(this);
    }

    _objectsLock.unlock();

    return inserted;
}

/**
 * Removes an object from the Grid
 *
 * @TODO: Most likely won't cause threading issues, but we must keep an eye to this
 */
void Grid::removeObject(Poco::UInt64 GUID)
{
    _moveList.push_back(GUID);
}

GridLoader::GridLoader():
    _server(NULL)
{
    _gridsPool = new Poco::ThreadPool(1, 1); // TODO: Multithreading (more than one grid worker thread)
    
    for (Poco::UInt16 x = 0; x < MAX_X; x++)
        for (Poco::UInt16 y = 0; y < MAX_Y; y++)
            _isGridLoaded[x][y] = false;
}

GridLoader::~GridLoader()
{
    delete _gridsPool;
}

void GridLoader::initialize(Server* server)
{
    _server = server;
    _gridsPool->start(*this);
}

bool GridLoader::checkAndLoad(Poco::UInt16 x, Poco::UInt16 y)
{
    if (_isGridLoaded[x][y])
        return true;

    Grid* grid = new Grid(x, y);

    _gridsLock.writeLock();
    _grids.push_back(grid);
    _isGridLoaded[x][y] = true;
    _gridsLock.unlock();

    #ifdef SERVER_FRAMEWORK_TESTING
        printf("Grid (%d, %d) has been created\n", x, y);
    #endif

    return true;
}

bool findGridIfPositionMatches(Grid* grid, Vector2D position)
{
    return (grid->GetPositionX() == position.x && grid->GetPositionY() == position.y);
}

 Grid* GridLoader::GetGrid(Poco::UInt16 x, Poco::UInt16 y)
 {
    _gridsLock.readLock();
    GridsList::const_iterator itr = std::find_if(_grids.cbegin(), _grids.cend(), std::bind2nd(std::ptr_fun(findGridIfPositionMatches), Vector2D(x, y)));
    if (itr == _grids.end())
    {
        _gridsLock.unlock();
        return NULL;
    }
    Grid* grid = *itr;
    _gridsLock.unlock();

    return grid;
 }

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

Grid* GridLoader::addObject(SharedPtr<Object> object)
{
    Vector2D pos = object->GetPosition();
    return addObjectTo(Tools::GetCellFromPos(pos.x), Tools::GetCellFromPos(pos.y), object);
}


bool GridLoader::removeObject(Object* object)
{
    Poco::UInt16 x = Tools::GetCellFromPos(object->GetPosition().x);
    Poco::UInt16 y = Tools::GetCellFromPos(object->GetPosition().y);

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

std::list<Poco::UInt64> GridLoader::ObjectsInGrid(Object* object)
{
    Poco::UInt16 x = Tools::GetCellFromPos(object->GetPosition().x);
    Poco::UInt16 y = Tools::GetCellFromPos(object->GetPosition().y);
    std::list<Poco::UInt64> objectsList;
    
    if (!_isGridLoaded[x][y])
        return objectsList;

    if (Grid* grid = GetGrid(x, y))
        objectsList = grid->getObjects(HIGH_GUID_PLAYER | HIGH_GUID_MONSTER | HIGH_GUID_ITEM);

    return objectsList;
}

std::list<Poco::UInt64> GridLoader::ObjectsInGridNear(Object* object, float distance)
{
    std::list<Poco::UInt64> objects_grid = ObjectsInGrid(object);
    std::list<Poco::UInt64> objects_near;

    for (std::list<Poco::UInt64>::iterator itr = objects_grid.begin(); itr != objects_grid.end(); itr++)
    {
        SharedPtr<Object> object_near = sServer->GetObject(*itr);
        if (object_near.isNull())
            continue;

        Vector2D org = object->GetPosition();
        Vector2D dest = object_near->GetPosition();
        if (pow(org.x - dest.x, 2) + pow(org.y - dest.y, 2) <= pow(distance, 2))
			objects_near.push_back(*itr);
    }

    return objects_near;
}


/**
* Can't be used from outside the GridLoader or the Grid classes as it is NOT thread safe
*
* @param GUID Object GUID, including HIGH_GUID
*/
void GridLoader::addToMoveList(Poco::UInt64 GUID)
{
    _moveList.push_back(GUID);
}

void GridLoader::run_impl()
{
    while (sServer->isRunning())
    {
        // TODO: Multi-threading
        // Objects moved from a Grid to another are added here to the new Grid, otherwise we would run into deadlocks
        for (Grid::ObjectList::const_iterator itr = _moveList.cbegin(); itr != _moveList.cend(); itr++)
        {
            SharedPtr<Object> object = _server->GetObject(*itr);
            if (!object.isNull())
                addObject(object);
        }
        _moveList.clear();

        // Remove Grids where there are no players (save RAM and CPU, less ticks)
        _gridsLock.writeLock();
        for (GridsList::iterator itr = _removeList.begin(); itr != _removeList.end(); itr++)
        {
            Grid* grid = *itr;

            _grids.remove(grid);
            _isGridLoaded[grid->GetPositionX()][grid->GetPositionY()] = false;
            delete grid;
        }
        _gridsLock.unlock();
        _removeList.clear();

        // Update all Grids
        _gridsLock.readLock();
        for (GridsList::const_iterator itr = _grids.cbegin(); itr != _grids.cend(); itr++)
        {
            Grid* grid = *itr;

            // If update fails, something went really wrong, delete this grid
            if (!grid->update())
                removeGrid(grid);

            // If the grid has no players in it, check for nearby grids, if they are not loaded or have no players, remove it
            if (!grid->hasPlayers())
                removeGrid(grid);
        }
        _gridsLock.unlock();

        Poco::Thread::sleep(1);
    }
}

void GridLoader::removeGrid(Grid* grid)
{
#if defined(SERVER_FRAMEWORK_TESTING)
    printf("Removing Grid (%d, %d)\n", grid->GetPositionX(), grid->GetPositionY());
#endif

    _removeList.push_back(grid);
}
