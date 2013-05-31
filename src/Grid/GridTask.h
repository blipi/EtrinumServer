#ifndef GAMESERVER_GRID_TASK_H
#define GAMESERVER_GRID_TASK_H

#include "Poco/Task.h"

class Grid;

class GridTask: public Poco::Task
{
public:
    GridTask(Grid* grid, Poco::UInt64 diff);

    void runTask();
    bool getResult();

    inline Grid* getGrid()
    {
        return _grid;
    }

private:
    Grid* _grid;
    Poco::UInt64 _diff;
    bool _result;
};

#endif