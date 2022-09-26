#pragma once

namespace Tmpl8
{
	class Convexhull
	{
	public:
		Convexhull(vector<vec2>& activeTanksPositions);

		~Convexhull();

		void heapSort(vector<vec2>& activeTanksPositions, int sizeOfVector, vec2 origin);
		void maxHeapify(vector<vec2>& tanks, int index, int sizeOfVector, vec2 origin);
		bool compareAngles(vec2 p0, vec2 p1, vec2 p2);
		int convex(const vec2& p0, const vec2& p1, const vec2& p2);
		vector<vec2> getPointsOnHull();
	private:
		vector<vec2> pointsOnHull;
	};
}

