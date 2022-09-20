#include "precomp.h"
#include "convexHull.h"

namespace Tmpl8
{
    //convexHull::convexHull(vector<vec2>& activeTanksPositions)
    //{
    //    int sizeOfVector = activeTanksPositions.size() - 1;

    //    //Find lowestLeftTank as a starting point.
    //    vec2 lowestLeftTank = activeTanksPositions[0];
    //    
    //    for (int i = 1; i < activeTanksPositions.size() - 1; i++)
    //    {
    //        if (activeTanksPositions[i].y < lowestLeftTank.y)
    //        {
    //            lowestLeftTank = activeTanksPositions[i];
    //            continue;
    //        }
    //        if (activeTanksPositions[i].y == lowestLeftTank.y && activeTanksPositions[i].x < lowestLeftTank.x)
    //        {
    //            lowestLeftTank = activeTanksPositions[i];
    //        }
    //    }

    //    swap(lowestLeftTank, activeTanksPositions[0]);

    //    //swap(activeTanks[0], activeTanks[1]);

    //};

    ////This function assumes that the input vector at index 0 contains the reference point.
    //void convexHull::heapSort(vector<vec2>& activeTanksPositions, int sizeOfVector)
    //{
    //    //Build a minheap
    //    //This for loop makes sure the leaf nodes are not checked seprately
    //    for (int i = 1; i > (sizeOfVector / 2) - 1; i++)
    //    {

    //    }
    //};

    //void convexHull::minHeapify(vector<vec2>& tanks, int index, int sizeOfVector, vec2 origin)
    //{
    //    int smallest = index;
    //    int left = 2 * index + 1;
    //    int right = 2 * index + 2;

    //    if (left < sizeOfVector && !compareAngles(origin, tanks[left], tanks[right]))
    //    {
    //        smallest = left;
    //    }

    //    if (right < sizeOfVector && !compareAngles(origin, tanks[right], tanks[left]))
    //    {
    //        smallest = right;   
    //    }


    //    //Compare angles for all three points in relation to the referencepoint

    //};

    //bool compareAngles(vec2 p0, vec2 p1, vec2 p2)
    //{
    //    int c = convex(p0, p1, p2);

    //    (c > 0) ? true : false;
    //};

    ////Returns 0 if the points are collinear, 1 for counter-clockwise turn and -1 for a clockwise turn
    //int convex(const vec2& p0, const vec2& p1, const vec2& p2)
    //{
    //    long long crossProduct = (p1.x - p0.x) * (p2.y - p0.y) - (p1.y - p0.y) * (p2.x - p0.x);

    //    if (crossProduct == 0)
    //    {
    //        return 0;
    //    }
    //    return (crossProduct > 0) ? 1 : -1;
    //};

}