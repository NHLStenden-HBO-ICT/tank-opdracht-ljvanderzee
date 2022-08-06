#include "precomp.h"
#include "grid.h"

namespace Tmpl8
{
	Grid::Grid(int width, int height, int cellSize) :
		width(width),
		height(height),
		cellSize(cellSize) 
	{
		//Casting to a float and using ceil to round up and make there are enough cells
		xCells = ceil((float)width / cellSize);
		yCells = ceil((float)height / cellSize);

		//Resize the cells vector to the total number of cells on the grid
		cells.resize(xCells * yCells);
	}

	Grid::~Grid()
	{
	}

	void Grid::addTank(Tank* tank)
	{
		Cell* cell = getCell(tank->position);
		cell->tanks.push_back(tank);
		tank->currentCell = cell;
		tank->cellIndex = cell->tanks.size() - 1;
	};

	//Adds the tank to the defined cell
	void Grid::addTank(Tank* tank, Cell* cell)
	{
		cell->tanks.push_back(tank);
		tank->currentCell = cell;
		tank->cellIndex = cell->tanks.size() - 1;
	};

	Cell* Grid::getCell(int x, int y) 
	{
		if (x < 0) x = 0;
		if (x >= xCells) x = xCells;
		if (y < 0) y = 0;
		if (y >= yCells) y = yCells;

		//This call lets us use the cells vector as a 2d vector
		return &cells[y * xCells + x];
	}

	Cell* Grid::getCell(vec2 position)
	{
		int cellX = (int)(position.x / cellSize);
		int cellY = (int)(position.y / cellSize);

		return getCell(cellX, cellY);
	};

	void Grid::removeTankFromCell(Tank* tank)
	{
		vector<Tank*>& tanks = tank->currentCell->tanks;
		//Swap the current tank with the last tank in the vector so it is in the last position
		tanks[tank->cellIndex] = tanks.back();
		//Remove last tank from the vector
		tanks.pop_back();
		//Update the cellIndex for the tank that got swapped from the last position
		if (tank->cellIndex < tanks.size())
		{
			tanks[tank->cellIndex]->cellIndex = tank->cellIndex;
		}
		//Reset the index of the tank being removed to -1
		tank->cellIndex = -1;
		tank->currentCell = nullptr;
	};
}