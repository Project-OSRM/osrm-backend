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

#include "../DataStructures/Coordinate.h"

#include <boost/assert.hpp>

#include <cmath>

#include <limits>
#include <stack>
#include <vector>

/*This class object computes the bitvector of indicating generalized input points
 * according to the (Ramer-)Douglas-Peucker algorithm.
 *
 * Input is vector of pairs. Each pair consists of the point information and a bit
 * indicating if the points is present in the generalization.
 * Note: points may also be pre-selected*/

//These thresholds are more or less heuristically chosen.
//                                                  0          1         2         3         4          5         6         7       8       9       10     11       12     13    14   15  16  17   18
static double DouglasPeuckerThresholds[19] = { 32000000., 16240000., 80240000., 40240000., 20000000., 10000000., 500000., 240000., 120000., 60000., 30000., 19000., 5000., 2000., 200, 16,  6, 3. , 3. };

template<class PointT>
class DouglasPeucker {
private:
    typedef std::pair<std::size_t, std::size_t> PairOfPoints;
    //Stack to simulate the recursion
    std::stack<PairOfPoints > recursion_stack;

    /**
     * This distance computation does integer arithmetic only and is about twice as fast as
     * the other distance function. It is an approximation only, but works more or less ok.
     */
    template<class CoordT>
    inline int fastDistance(const CoordT& point, const CoordT& segA, const CoordT& segB) const {
        const int p2x = (segB.lon - segA.lat);
        const int p2y = (segB.lon - segA.lat);
        const int something = p2x*p2x + p2y*p2y;
        int u = ( 0 == something  ? 0 : ((point.lon - segA.lon) * p2x + (point.lat - segA.lat) * p2y) / something);

        if (u > 1) {
            u = 1;
        } else if (u < 0) {
            u = 0;
        }

        const int x = segA.lon + u * p2x;
        const int y = segA.lat + u * p2y;

        const int dx = x - point.lon;
        const int dy = y - point.lat;

        const int dist = (dx*dx + dy*dy);

        return dist;
    }


public:
    void Run(std::vector<PointT> & input_geometry, const unsigned zoom_level) {
        {
            BOOST_ASSERT_MSG(zoom_level < 19, "unsupported zoom level");
            BOOST_ASSERT_MSG(1 < input_geometry.size(), "geometry invalid");
            std::size_t left_border = 0;
            std::size_t right_border = 1;
            //Sweep linerarily over array and identify those ranges that need to be checked
            do {
                BOOST_ASSERT_MSG(
                    input_geometry[left_border].necessary,
                    "left border must be necessary"
                );
                BOOST_ASSERT_MSG(
                    input_geometry.back().necessary,
                    "right border must be necessary"
                );

                if(input_geometry[right_border].necessary) {
                    recursion_stack.push(std::make_pair(left_border, right_border));
                    left_border = right_border;
                }
                ++right_border;
            } while( right_border < input_geometry.size());
        }
        while(!recursion_stack.empty()) {
            //pop next element
            const PairOfPoints pair = recursion_stack.top();
            recursion_stack.pop();
            BOOST_ASSERT_MSG(
                input_geometry[pair.first].necessary,
                "left border mus be necessary"
            );
            BOOST_ASSERT_MSG(
                input_geometry[pair.second].necessary,
                "right border must be necessary"
            );
            BOOST_ASSERT_MSG(
                pair.second < input_geometry.size(),
                "right border outside of geometry"
            );
            BOOST_ASSERT_MSG(
                pair.first < pair.second,
                "left border on the wrong side"
            );
            int max_distance = INT_MIN;

            std::size_t farthest_element_index = pair.second;
            //find index idx of element with max_distance
            for(std::size_t i = pair.first+1; i < pair.second; ++i){
                const int temp_dist = fastDistance(
                                        input_geometry[i].location,
                                        input_geometry[pair.first].location,
                                        input_geometry[pair.second].location
                                    );
                const double distance = std::fabs(temp_dist);
                if(
                    distance > DouglasPeuckerThresholds[zoom_level] &&
                    distance > max_distance
                ) {
                    farthest_element_index = i;
                    max_distance = distance;
                }
            }
            if (max_distance > DouglasPeuckerThresholds[zoom_level]) {
                //  mark idx as necessary
                input_geometry[farthest_element_index].necessary = true;
                if (1 < (farthest_element_index - pair.first) ) {
                    recursion_stack.push(
                        std::make_pair(pair.first, farthest_element_index)
                    );
                }
                if (1 < (pair.second - farthest_element_index) ) {
                    recursion_stack.push(
                        std::make_pair(farthest_element_index, pair.second)
                    );
                }
            }
        }
    }
};

#endif /* DOUGLASPEUCKER_H_ */
