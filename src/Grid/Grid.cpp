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
    _lastRemoveCheck = clock();
}

bool Grid::update()
{
    // Update only if it's more than 1ms since last tick
    if (clock() - _lastTick > 0)
    {
        // TODO: Multi-threading issues here (NOTE: Only if more than 1 grid processor, not multithreading in general)
        _moveListLock.readLock();
        for (ObjectList::const_iterator itr = _moveList.cbegin(); itr != _moveList.cend(); itr++)
        {
            _objectsLock.writeLock();

            // Object MUST be in Grid
            // ASSERT(_objects.find(*itr) != _objects.end())
            if (_objects.find(*itr) == _objects.end())
            {
                _objectsLock.unlock();
                continue;
            }

            // Erase the object from this grid
            _objects.erase(*itr);
            
            if ((*itr >> 32) & HIGH_GUID_PLAYER)
                _playersInGrid--;

            _objectsLock.unlock();
        }
        _moveListLock.unlock();

        _moveListLock.writeLock();
        _moveList.clear();
        _moveListLock.unlock();
        
        _objectsLock.readLock();
        for (ObjectMap::const_iterator itr = _objects.cbegin(); itr != _objects.cend(); itr++)
        {
            SharedPtr<Object> object = itr->second;

            // Update AI if there is any or we have to
            object->update(clock() - _lastTick);

            // Update movement if we have to
            if (object->hasFlag(FLAGS_TYPE_MOVEMENT, FLAG_MOVING))
            {
                // Check if movement has finalized, in which case, remove flag
                Vector2D newPos;
                if (object->motionMaster.evaluate(clock() - _lastTick, newPos))
                {
                    #ifdef SERVER_FRAMEWORK_TESTING
                        printf("Object %s has reached destination\n", Poco::NumberFormatter::formatHex(object->GetGUID()).c_str());
                    #endif
                    
                    object->clearFlag(FLAGS_TYPE_MOVEMENT, FLAG_MOVING);
                }

                // Update grid if we have to
                Vector2D currentPos = object->GetPosition();
                if (Tools::GetCellFromPos(newPos.x) != Tools::GetCellFromPos(currentPos.x) || Tools::GetCellFromPos(newPos.y) != Tools::GetCellFromPos(currentPos.y))
                {
                    #ifdef SERVER_FRAMEWORK_TESTING
                        printf("Object %s must change grid (%d, %d) -> (%d, %d)\n", Poco::NumberFormatter::formatHex(object->GetGUID()).c_str(), Tools::GetCellFromPos(currentPos.x), Tools::GetCellFromPos(currentPos.y), Tools::GetCellFromPos(newPos.x), Tools::GetCellFromPos(newPos.y));
                    #endif

                    _moveListLock.writeLock();
                    _moveList.push_back(object->GetGUID());
                    _moveListLock.unlock();
                    
                    // Add it to GridLoader move list, we can't add it directly from here to the new Grid
                    // We would end up being deadlocked, so the GridLoader handles it
                    sGridLoader.addToMoveList(object->GetGUID());
                }

                // Relocate Object
                object->Relocate(newPos);

                // Update LoS
                object->UpdateLoS();
            }
        }

        _objectsLock.unlock();

        _lastTick = clock();
    }
    return true;
}


std::list<Poco::UInt64> Grid::getObjects()
{
    std::list<Poco::UInt64> objects;

    _objectsLock.readLock();

    for (ObjectMap::const_iterator itr = _objects.cbegin(); itr != _objects.cend(); itr++)
        objects.push_back(itr->first);
    
    _objectsLock.unlock();

    return objects;
}

bool findObjectIfGUIDMatches(Grid::ObjectMapInserter obj, Poco::UInt64 GUID)
{
    return obj.first == GUID;
}

SharedPtr<Object> Grid::getObject(Poco::UInt64 GUID)
{
    SharedPtr<Object> object = NULL;

    _objectsLock.readLock();

    ObjectMap::iterator itr = std::find_if(_objects.begin(), _objects.end(), std::bind2nd(std::ptr_fun(findObjectIfGUIDMatches), GUID));
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

void Grid::removeObject(Poco::UInt64 GUID)
{
    _moveListLock.writeLock();
    _moveList.push_back(GUID);
    _moveListLock.unlock();
}

bool Grid::mustDoRemoveCheck()
{
    if (clock() - _lastRemoveCheck > 1500)
    {
        _lastRemoveCheck = clock();
        return true;
    }

    return false;
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
        objectsList = grid->getObjects();

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
            if (grid->getNumberOfPlayers() == 0 && grid->mustDoRemoveCheck())
                removeGrid(grid);
        }
        _gridsLock.unlock();

        Poco::Thread::sleep(1);
    }
}

bool GridLoader::checkNearbyGrid(Poco::UInt16 x, Poco::UInt16 y)
{
    if (!_isGridLoaded[x][y])
        return false;

    GridsList::const_iterator itr = std::find_if(_grids.cbegin(), _grids.cend(), std::bind2nd(std::ptr_fun(findGridIfPositionMatches), Vector2D(x, y)));
    Grid* grid = *itr;

    return grid->getNumberOfPlayers() != 0;
}

void GridLoader::removeGrid(Grid* grid)
{
#if defined(SERVER_FRAMEWORK_TESTING)
    printf("Removing Grid (%d, %d)\n", grid->GetPositionX(), grid->GetPositionY());
#endif

    _removeList.push_back(grid);
}
