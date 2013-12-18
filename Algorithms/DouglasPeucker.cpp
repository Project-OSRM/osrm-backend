/*

Copyright (c) 2013, Project OSRM, Dennis Luxen, others
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list
of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "DouglasPeucker.h"

//These thresholds are more or less heuristically chosen.
static double DouglasPeuckerThresholds[19] = {
    32000000., //z0
    16240000., //z1
     8240000., //z2
     4240000., //z3
     2000000., //z4
     1000000., //z5
      500000., //z6
      240000., //z7
      120000., //z8
       60000., //z9
       30000., //z10
       19000., //z11
        5000., //z12
        2000., //z13
         200., //z14
          16., //z15
           6., //z16
           3., //z17
           3.  //z18
};

/**
 * This distance computation does integer arithmetic only and is about twice as
 * fast as the other distance function. It is an approximation only, but works
 * more or less ok.
 */
int DouglasPeucker::fastDistance(
    const FixedPointCoordinate& point,
    const FixedPointCoordinate& segA,
    const FixedPointCoordinate& segB
) const {
    const int p2x = (segB.lon - segA.lon);
    const int p2y = (segB.lat - segA.lat);
    const int something = p2x*p2x + p2y*p2y;
    double u = ( 0 == something  ? 0 : ((point.lon - segA.lon) * p2x + (point.lat - segA.lat) * p2y) / something);

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

void DouglasPeucker::Run(
    std::vector<SegmentInformation> & input_geometry,
    const unsigned zoom_level
) {
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
