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
#include "../DataStructures/SegmentInformation.h"
#include "../Util/MercatorUtil.h"

#include <limits>

//These thresholds are more or less heuristically chosen.
static double DouglasPeuckerThresholds[19] = {
      262144., //z0
      131072., //z1
       65536., //z2
       32768., //z3
       16384., //z4
        8192., //z5
        4096., //z6
        2048., //z7
         960., //z8
         480., //z9
         240., //z10
          90., //z11
          50., //z12
          25., //z13
          15., //z14
           5., //z15
            .65, //z16
            .5, //z17
            .35  //z18
};

/**
 * Yuck! Code duplication. This function is also in EgdeBasedNode.h
 */
double DouglasPeucker::ComputeDistance(
    const FixedPointCoordinate& point,
    const FixedPointCoordinate& segA,
    const FixedPointCoordinate& segB
) const {
    const double x = lat2y(point.lat/COORDINATE_PRECISION);
    const double y = point.lon/COORDINATE_PRECISION;
    const double a = lat2y(segA.lat/COORDINATE_PRECISION);
    const double b = segA.lon/COORDINATE_PRECISION;
    const double c = lat2y(segB.lat/COORDINATE_PRECISION);
    const double d = segB.lon/COORDINATE_PRECISION;
    double p,q,nY;
    if( std::abs(a-c) > std::numeric_limits<double>::epsilon() ){
        const double m = (d-b)/(c-a); // slope
        // Projection of (x,y) on line joining (a,b) and (c,d)
        p = ((x + (m*y)) + (m*m*a - m*b))/(1. + m*m);
        q = b + m*(p - a);
    } else {
        p = c;
        q = y;
    }
    nY = (d*p - c*q)/(a*d - b*c);

    //discretize the result to coordinate precision. it's a hack!
    if( std::abs(nY) < (1./COORDINATE_PRECISION) ) {
        nY = 0.;
    }

    double r = (p - nY*a)/c;
    if( std::isnan(r) ) {
        r = ((segB.lat == point.lat) && (segB.lon == point.lon)) ? 1. : 0.;
    } else if( std::abs(r) <= std::numeric_limits<double>::epsilon() ) {
        r = 0.;
    } else if( std::abs(r-1.) <= std::numeric_limits<double>::epsilon() ) {
        r = 1.;
    }
    FixedPointCoordinate nearest_location;
    BOOST_ASSERT( !std::isnan(r) );
    if( r <= 0. ){
        nearest_location.lat = segA.lat;
        nearest_location.lon = segA.lon;
    } else if( r >= 1. ){
        nearest_location.lat = segB.lat;
        nearest_location.lon = segB.lon;
    } else { // point lies in between
        nearest_location.lat = y2lat(p)*COORDINATE_PRECISION;
        nearest_location.lon = q*COORDINATE_PRECISION;
    }
    BOOST_ASSERT( nearest_location.isValid() );
    const double approximated_distance = FixedPointCoordinate::ApproximateEuclideanDistance(
        point,
        nearest_location
    );
    BOOST_ASSERT( 0. <= approximated_distance );
    return approximated_distance;
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
        //Sweep over array and identify those ranges that need to be checked
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
    while( !recursion_stack.empty() ) {
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
        double max_distance = std::numeric_limits<double>::min();

        std::size_t farthest_element_index = pair.second;
        //find index idx of element with max_distance
        for(std::size_t i = pair.first+1; i < pair.second; ++i){
            const int temp_dist = ComputeDistance(
                                    input_geometry[i].location,
                                    input_geometry[pair.first].location,
                                    input_geometry[pair.second].location
                                );
            const double distance = std::abs(temp_dist);
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
