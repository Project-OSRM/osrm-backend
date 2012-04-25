/*
    open source routing machine
    Copyright (C) Dennis Luxen, others 2010

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU AFFERO General Public License as published by
the Free Software Foundation; either version 3 of the License, or
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
or see http://www.gnu.org/licenses/agpl.txt.
 */

#ifndef DOUGLASPEUCKER_H_
#define DOUGLASPEUCKER_H_

#include <cfloat>
#include <stack>

#include "../DataStructures/ExtractorStructs.h"

/*This class object computes the bitvector of indicating generalized input points
 * according to the (Ramer-)Douglas-Peucker algorithm.
 *
 * Input is vector of pairs. Each pair consists of the point information and a bit
 * indicating if the points is present in the generalization.
 * Note: points may also be pre-selected*/

//These thresholds are more or less heuristically chosen.
static double DouglasPeuckerThresholds[19] = { 10240000., 5120000., 2560000., 1280000., 640000., 320000., 160000., 80000., 40000., 20000., 10000., 5000., 2400., 1200., 200, 16, 6, 3., 1. };

template<class PointT>
class DouglasPeucker {
private:
    typedef std::pair<std::size_t, std::size_t> PairOfPoints;
    //Stack to simulate the recursion
    std::stack<PairOfPoints > recursionStack;

    double ComputeDistanceOfPointToLine(const _Coordinate& inputPoint, const _Coordinate& source, const _Coordinate& target) {
        double r;
        const double x = (double)inputPoint.lat;
        const double y = (double)inputPoint.lon;
        const double a = (double)source.lat;
        const double b = (double)source.lon;
        const double c = (double)target.lat;
        const double d = (double)target.lon;
        double p,q,mX,nY;
        if(c != a) {
            const double m = (d-b)/(c-a); // slope
            // Projection of (x,y) on line joining (a,b) and (c,d)
            p = ((x + (m*y)) + (m*m*a - m*b))/(1 + m*m);
            q = b + m*(p - a);
        } else {
            p = c;
            q = y;
        }
        nY = (d*p - c*q)/(a*d - b*c);
        mX = (p - nY*a)/c;// These values are actually n/m+n and m/m+n , we neednot calculate the values of m an n as we are just interested in the ratio
        r = mX;
        if(r<=0){
            return ((b - y)*(b - y) + (a - x)*(a - x));
        }
        else if(r >= 1){
            return ((d - y)*(d - y) + (c - x)*(c - x));

        }
        // point lies in between
        return (p-x)*(p-x) + (q-y)*(q-y);
    }

public:
    void Run(std::vector<PointT> & inputVector, const unsigned zoomLevel) {
        {
            assert(zoomLevel < 19);
            assert(1 < inputVector.size());
            std::size_t leftBorderOfRange = 0;
            std::size_t rightBorderOfRange = 1;
            //Sweep linerarily over array and identify those ranges that need to be checked
//            recursionStack.hint(inputVector.size());
            do {
                assert(inputVector[leftBorderOfRange].necessary);
                assert(inputVector[inputVector.size()-1].necessary);

                if(inputVector[rightBorderOfRange].necessary) {
                    recursionStack.push(std::make_pair(leftBorderOfRange, rightBorderOfRange));
                    leftBorderOfRange = rightBorderOfRange;
                }
                ++rightBorderOfRange;
            } while( rightBorderOfRange < inputVector.size());
        }
        while(!recursionStack.empty()) {
            //pop next element
            const PairOfPoints pair = recursionStack.top();
            recursionStack.pop();
            assert(inputVector[pair.first].necessary);
            assert(inputVector[pair.second].necessary);
            assert(pair.second < inputVector.size());
            assert(pair.first < pair.second);
            double maxDistance = -DBL_MAX;
            std::size_t indexOfFarthestElement = pair.second;
            //find index idx of element with maxDistance
            for(std::size_t i = pair.first+1; i < pair.second; ++i){
                double distance = std::fabs(ComputeDistanceOfPointToLine(inputVector[i].location, inputVector[pair.first].location, inputVector[pair.second].location));
                if(distance > DouglasPeuckerThresholds[zoomLevel] && distance > maxDistance) {
                    indexOfFarthestElement = i;
                    maxDistance = distance;
                }
            }
            if (maxDistance > DouglasPeuckerThresholds[zoomLevel]) {
                //  mark idx as necessary
                inputVector[indexOfFarthestElement].necessary = true;
                if (1 < indexOfFarthestElement - pair.first) {
                    recursionStack.push(std::make_pair(pair.first, indexOfFarthestElement) );
                }
                if (1 < pair.second - indexOfFarthestElement)
                    recursionStack.push(std::make_pair(indexOfFarthestElement, pair.second) );
            }
        }
    }
};

#endif /* DOUGLASPEUCKER_H_ */
