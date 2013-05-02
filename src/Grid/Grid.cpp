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
        for (ObjectList::const_iterator itr = _moveList.begin(); itr != _moveList.end(); )
        {
            // Get the GUID and increment the iterator, it'd be safe to delete from movelist now, but we rather use clear
            Poco::UInt64 GUID = *itr;
            itr++;
            
            // Object MUST be in Grid
            if (_objects.find(GUID) == _objects.end())
                ASSERT(false);

            // Erase the object from this grid
            _objects.erase(GUID);
            
            if (HIGUID(GUID) & HIGH_GUID_PLAYER)
                _playersInGrid--;
        }
        _moveList.clear();
        
        for (ObjectMap::const_iterator itr = _objects.begin(); itr != _objects.end(); )
        {
            SharedPtr<Object> object = itr->second;
            ++itr;

            if (!(object->GetHighGUID() & HIGH_GUID_PLAYER))
                continue;

            // Update AI, movement, everything if there is any or we have to
            // If update returns false, that means the object is no longer in this grid!
            if (!object->update(clock() - _lastTick))
            {
                _objects.erase(object->GetGUID());
                _playersInGrid--;
            }

            //@todo: Check proximity to another Grid and load it if we have to!
            /*
                We could add a parameter, such as _forcedLoadTime = clock()
                which gets updated at each this->update().

                At GridLoader run method:
                If clock() - nearGrid->_forcedLoadTime > xxx and !nearGrid->hasPlayers() Then
                    RemoveGrid

                std::list<Grid*> nearGrids = xxx; // Up to 3 if we are in a corner!!
            */

            // Update near mobs
            Vector2D c(object->GetPosition().x, object->GetPosition().y);
            ObjectMap::iterator it = std::find_if(_objects.begin(), _objects.end(), std::bind2nd(std::ptr_fun(findObjectsIf), c));
            while (it != _objects.end())
            {
                // Update the object, if it fails, it means it is in a new grid
                SharedPtr<Object> obj = it->second;
                if (!obj->update(clock() - _lastTick))
                    _objects.erase(obj->GetGUID());

                // Find next near object
                it = std::find_if(++it, _objects.end(), std::bind2nd(std::ptr_fun(findObjectsIf), c));
            }

            //@todo: Update objects in near grids
            /*
                Foreach nearGrids Do
                    Foreach FindNearObjects Do
                        nearObject->update
            */
        }

        _lastTick = clock();
    }
    return true;
}


Grid::ObjectList Grid::getObjects(Poco::UInt32 highGUID)
{
    std::list<Poco::UInt64> objects;

    for (ObjectMap::const_iterator itr = _objects.begin(); itr != _objects.end(); )
    {
        Poco::UInt64 GUID = itr->first;
        itr++;

        if (GUID & highGUID)
            objects.push_back(GUID);
    }

    return objects;
}

SharedPtr<Object> Grid::getObject(Poco::UInt64 GUID)
{
    SharedPtr<Object> object = NULL;
    
    ObjectMap::iterator itr = _objects.find(GUID);
    if (itr != _objects.end())
        object = itr->second;
    
    return object;
}

bool Grid::addObject(SharedPtr<Object> object)
{
    bool inserted = _objects.insert(ObjectMapInserter(object->GetGUID(), object)).second;    
    if (inserted)
    {        
        if (object->GetHighGUID() & HIGH_GUID_PLAYER)
            _playersInGrid++;
    
        object->SetGrid(this);
    }

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
    
    _grids.set_empty_key(NULL);
    _grids.set_deleted_key(std::numeric_limits<Poco::UInt32>::max());

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
    _grids.insert(GridInserter(grid->hashCode(), grid));
    _isGridLoaded[x][y] = true;

    #ifdef SERVER_FRAMEWORK_TESTING
        printf("Grid (%d, %d) has been created\n", x, y);
    #endif

    return true;
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

Grid* GridLoader::GetGridOrLoad(Poco::UInt16 x, Poco::UInt16 y)
{
    if (checkAndLoad(x, y))
        return GetGrid(x, y);
    return NULL;
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
            if (!grid->update() || !grid->hasPlayers())
            {
                _grids.erase(grid->hashCode());
                _isGridLoaded[grid->GetPositionX()][grid->GetPositionY()] = false;
                delete grid;
            }
        }

        Poco::Thread::sleep(1);
    }
}
