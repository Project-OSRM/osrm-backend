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

#include <osrm/Coordinate.h>

#include "DouglasPeucker.h"
#include "../DataStructures/SegmentInformation.h"
#include "../Util/SimpleLogger.h"

#include <boost/assert.hpp>

#include <cmath>

#include <limits>

DouglasPeucker::DouglasPeucker()
    : douglas_peucker_thresholds({2621440, // z0
                                  1310720, // z1
                                  655360,  // z2
                                  327680,  // z3
                                  163840,  // z4
                                  81920,   // z5
                                  40960,   // z6
                                  20480,   // z7
                                  9600,    // z8
                                  4800,    // z9
                                  2800,    // z10
                                  900,     // z11
                                  600,     // z12
                                  275,     // z13
                                  160,     // z14
                                  60,      // z15
                                  8,       // z16
                                  6,       // z17
                                  4        // z18
      })
{
}

void DouglasPeucker::Run(std::vector<SegmentInformation> &input_geometry, const unsigned zoom_level)
{
    input_geometry.front().necessary = true;
    input_geometry.back().necessary = true;

    BOOST_ASSERT_MSG(!input_geometry.empty(), "geometry invalid");
    if (input_geometry.size() < 2)
    {
        return;
    }

    {
        BOOST_ASSERT_MSG(zoom_level < 19, "unsupported zoom level");
        unsigned left_border = 0;
        unsigned right_border = 1;
        // Sweep over array and identify those ranges that need to be checked
        do
        {
            if (input_geometry[right_border].necessary)
            {
                BOOST_ASSERT(input_geometry[left_border].necessary);
                BOOST_ASSERT(input_geometry[right_border].necessary);
                recursion_stack.emplace(left_border, right_border);
                left_border = right_border;
            }
            ++right_border;
        } while (right_border < input_geometry.size());
    }
    while (!recursion_stack.empty())
    {
        // pop next element
        const GeometryRange pair = recursion_stack.top();
        recursion_stack.pop();
        BOOST_ASSERT_MSG(input_geometry[pair.first].necessary, "left border mus be necessary");
        BOOST_ASSERT_MSG(input_geometry[pair.second].necessary, "right border must be necessary");
        BOOST_ASSERT_MSG(pair.second < input_geometry.size(), "right border outside of geometry");
        BOOST_ASSERT_MSG(pair.first < pair.second, "left border on the wrong side");
        int max_int_distance = 0;

        unsigned farthest_element_index = pair.second;
        // find index idx of element with max_distance
        for (unsigned i = pair.first + 1; i < pair.second; ++i)
        {
            const int int_dist = FixedPointCoordinate::OrderedPerpendicularDistanceApproximation(
                input_geometry[i].location,
                input_geometry[pair.first].location,
                input_geometry[pair.second].location);

            if (int_dist > max_int_distance && int_dist > douglas_peucker_thresholds[zoom_level])
            {
                farthest_element_index = i;
                max_int_distance = int_dist;
            }
        }
        if (max_int_distance > douglas_peucker_thresholds[zoom_level])
        {
            //  mark idx as necessary
            input_geometry[farthest_element_index].necessary = true;
            if (1 < (farthest_element_index - pair.first))
            {
                recursion_stack.emplace(pair.first, farthest_element_index);
            }
            if (1 < (pair.second - farthest_element_index))
            {
                recursion_stack.emplace(farthest_element_index, pair.second);
            }
        }
    }
}
