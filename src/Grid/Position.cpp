#include "Position.h"
#include "Grid.h"

#include <cmath>

Vector2D::Vector2D()
{
}

Vector2D::Vector2D(float x, float z)
{
    this->x = x;
    this->z = z;
}

void Vector2D::Relocate(Vector2D vector2D)
{
    *this = vector2D;
}

float Vector2D::Distance(Vector2D to)
{
    return std::sqrt(std::pow(x - to.x, 2) + std::pow(z - to.z, 2));
}

void Vector2D::Process()
{
    gridX = Tools::GetXCellFromPos(x);
    gridY = Tools::GetYCellFromPos(z);

    _inCellX = Tools::GetPositionInXCell(gridX, x);
    _inCellY = Tools::GetPositionInYCell(gridX, z);

    sector = Tools::GetSector(_inCellX, _inCellY, Grid::LOSRange / 2);
}
