#pragma once

namespace Tmpl8 
{
	struct Cell
	{
		vector<Tank*> tanks;
	};

	class Grid
	{
		friend class Game;
	public:
		Grid(int width, int height, int cellSize);

		~Grid();

		void addTank(Tank* tank);
		void addTank(Tank* tank, Cell* cell);

		Cell* getCell(int x, int y);
		//Gets cell based on tank position
		Cell* getCell(vec2 position);

		void removeTankFromCell(Tank* tank);

	private:
		//The vectors contained in this vector contain instances of type Tank
		vector<Cell> cells;
		int cellSize;
		int width;
		int height;
		int xCells;
		int yCells;
	};

}