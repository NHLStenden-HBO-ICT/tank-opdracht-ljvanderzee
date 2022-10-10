#include "precomp.h"
#include "convexHull.h"

namespace Tmpl8
{
    Convexhull::Convexhull(vector<vec2>& activeTanksPositions) {
        int sizeOfVector = activeTanksPositions.size();

        //Find lowestLeftTank as a starting point.

        for (int i = 1; i < activeTanksPositions.size() - 1; i++)
        {
            if (activeTanksPositions[i].y > activeTanksPositions[0].y)
            {
                swap(activeTanksPositions[0], activeTanksPositions[i]);
                continue;
            }
            if (activeTanksPositions[i].y == activeTanksPositions[0].y && activeTanksPositions[i].x < activeTanksPositions[0].x)
            {
                swap(activeTanksPositions[0], activeTanksPositions[i]);
            }
        }

        vec2 lowestLeftTank = activeTanksPositions[0];

        heapSort(activeTanksPositions, sizeOfVector, lowestLeftTank);

        int pointer = 2;

        while (pointer <= (activeTanksPositions.size() - 1))
        {
            vec2 p0 = lowestLeftTank;
            vec2 p1 = activeTanksPositions[pointer - 1];
            vec2 p2 = activeTanksPositions[pointer];

            if (convex(p0, p1, p2) == 0)
            {
                activeTanksPositions.erase(activeTanksPositions.begin() + pointer - 1);
            }
            else
            {
                pointer++;
            }
        }

        vector<vec2> st;
        st.push_back(activeTanksPositions[0]);
        st.push_back(activeTanksPositions[1]);

        pointer = 2;

        while (pointer < (activeTanksPositions.size() - 1))
        {
            vec2 p0 = st[st.size() - 2];
            vec2 p1 = st[st.size() - 1];
            vec2 p2 = activeTanksPositions[pointer];

            if (convex(p0, p1, p2) < 0)
            {
                st.push_back(p2);
                pointer++;
            }

            else {
                // The middle point does not lie on the convex hull.
                st.pop_back();
                if (st.size() < 2) {
                    st.push_back(p2);
                    pointer++;
                }
            }
        }

        // If three collinear points are found at the end, we
      // remove the middle one.
        vec2 a = st[st.size() - 2];
        vec2 b = st[st.size() - 1];
        vec2 c = st[0];
        if (convex(a, b, c) == 0) {
            st.pop_back();
        }

        pointsOnHull = st;
    }

    Convexhull::~Convexhull()
    {
    }

    void Convexhull::heapSort(vector<vec2>& activeTanksPositions, int sizeOfVector, vec2 origin) 
    {
        //Build a maxheap
        //This for loop makes sure the leaf nodes are not checked seprately
        for (int i = (sizeOfVector / 2) - 1; i > 0; i--)
        {
            maxHeapify(activeTanksPositions, i, sizeOfVector, origin);
        }

        for (int i = (sizeOfVector - 1); i > 0; i--)
        {
            swap(activeTanksPositions[0], activeTanksPositions[i]);
            maxHeapify(activeTanksPositions, 0, i, origin);
        }
        swap(activeTanksPositions[0], activeTanksPositions[sizeOfVector - 1]);
    };
    void Convexhull::maxHeapify(vector<vec2>& tanks, int index, int sizeOfVector, vec2 origin)
    {
        int largest = index;
        int left = 2 * index + 1;
        int right = 2 * index + 2;

        if (left < sizeOfVector && compareAngles(origin, tanks[left], tanks[largest]))
        {
            largest = left;
        }

        if (right < sizeOfVector && compareAngles(origin, tanks[right], tanks[largest]))
        {
            largest = right;
        }

        if (largest != index)
        {
            swap(tanks[index], tanks[largest]);
            maxHeapify(tanks, largest, sizeOfVector, origin);
        }
    };
    bool Convexhull::compareAngles(vec2 p0, vec2 p1, vec2 p2)
    {
        int c = convex(p0, p1, p2);

        //Returns true when these three point make a counter-clockwise turn
        if (c == 0)
        {
            const int distP1 = std::abs(p1.x - p0.x) + std::abs(p1.y - p0.y);
            const int distP2 = std::abs(p2.x - p0.x) + std::abs(p2.y - p0.y);

            if (distP1 < distP2)
            {
                return true;
            }
            return false;
        }
        return (c > 0) ? true : false;
    };

    int Convexhull::convex(const vec2& p0, const vec2& p1, const vec2& p2)
    {
        long long crossProduct = (p1.x - p0.x) * (p2.y - p0.y) - (p1.y - p0.y) * (p2.x - p0.x);

        if (crossProduct == 0)
        {
            //Points or collinear
            return 0;
        }
        //returns 1 for a counter-clockwise turn and -1 for a clockwise turn
        return (crossProduct > 0) ? 1 : -1;
    };

    vector<vec2> Convexhull::getPointsOnHull() { return pointsOnHull; };
}