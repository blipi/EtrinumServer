#include "GridTask.h"
#include "Grid.h"

GridTask::GridTask(Grid* grid, Poco::UInt64 diff):
    Task(""),
    _grid(grid), _diff(diff),
    _result(true)
{
}

void GridTask::runTask()
{
    if (_grid->hasPlayers())
        _result = _grid->update(_diff);
}

bool GridTask::getResult()
{
    return _result && !(!_grid->hasPlayers() && !_grid->isForceLoaded());
}
