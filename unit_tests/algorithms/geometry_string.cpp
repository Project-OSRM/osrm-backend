/*

Copyright (c) 2015, Project OSRM contributors
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

#include <boost/test/floating_point_comparison.hpp>
#include <boost/test/unit_test.hpp>

#include "../../algorithms/polyline_compressor.hpp"
#include "../../data_structures/coordinate_calculation.hpp"

#include <osrm/coordinate.hpp>

#include <cmath>
#include <vector>

BOOST_AUTO_TEST_CASE(geometry_string)
{
    // Polyline string for the 5 coordinates
    const std::string polyline = "_gjaR_gjaR_pR_ibE_pR_ibE_pR_ibE_pR_ibE";
    PolylineCompressor pc;
    std::vector<FixedPointCoordinate> coords = pc.decode_string(polyline);

    // Test coordinates; these would be the coordinates we give the loc parameter,
    // e.g. loc=10.00,10.0&loc=10.01,10.1...
    FixedPointCoordinate coord1(10.00 * COORDINATE_PRECISION, 10.0 * COORDINATE_PRECISION);
    FixedPointCoordinate coord2(10.01 * COORDINATE_PRECISION, 10.1 * COORDINATE_PRECISION);
    FixedPointCoordinate coord3(10.02 * COORDINATE_PRECISION, 10.2 * COORDINATE_PRECISION);
    FixedPointCoordinate coord4(10.03 * COORDINATE_PRECISION, 10.3 * COORDINATE_PRECISION);
    FixedPointCoordinate coord5(10.04 * COORDINATE_PRECISION, 10.4 * COORDINATE_PRECISION);
    
    // Put the test coordinates into the vector for comparison
    std::vector<FixedPointCoordinate> cmp_coords;
    cmp_coords.emplace_back(coord1);
    cmp_coords.emplace_back(coord2);
    cmp_coords.emplace_back(coord3);
    cmp_coords.emplace_back(coord4);
    cmp_coords.emplace_back(coord5);

    BOOST_CHECK_EQUAL(cmp_coords.size(), coords.size());

    for(unsigned i = 0; i < cmp_coords.size(); ++i)
    {
	const double cmp1_lat = coords.at(i).lat;
	const double cmp2_lat = cmp_coords.at(i).lat;
        BOOST_CHECK_CLOSE(cmp1_lat, cmp2_lat, 0.0001);
	
	const double cmp1_lon = coords.at(i).lon;
	const double cmp2_lon = cmp_coords.at(i).lon;
        BOOST_CHECK_CLOSE(cmp1_lon, cmp2_lon, 0.0001);
    }
}
