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

#include "util/bearing.hpp"
#include "util/typedefs.hpp"

#include <boost/functional/hash.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/test_case_template.hpp>

BOOST_AUTO_TEST_SUITE(bearing)

// Verify that the bearing-bounds checking function behaves as expected
BOOST_AUTO_TEST_CASE(bearing_range_test)
{
    // Simple, non-edge-case checks
    BOOST_CHECK_EQUAL(true, bearing::CheckInBounds(45, 45, 10));
    BOOST_CHECK_EQUAL(true, bearing::CheckInBounds(35, 45, 10));
    BOOST_CHECK_EQUAL(true, bearing::CheckInBounds(55, 45, 10));

    BOOST_CHECK_EQUAL(false, bearing::CheckInBounds(34, 45, 10));
    BOOST_CHECK_EQUAL(false, bearing::CheckInBounds(56, 45, 10));

    BOOST_CHECK_EQUAL(false, bearing::CheckInBounds(34, 45, 10));
    BOOST_CHECK_EQUAL(false, bearing::CheckInBounds(56, 45, 10));

    // When angle+limit goes > 360
    BOOST_CHECK_EQUAL(true, bearing::CheckInBounds(359, 355, 10));

    // When angle-limit goes < 0
    BOOST_CHECK_EQUAL(true, bearing::CheckInBounds(359, 5, 10));
    BOOST_CHECK_EQUAL(false, bearing::CheckInBounds(354, 5, 10));
    BOOST_CHECK_EQUAL(false, bearing::CheckInBounds(16, 5, 10));


    // Checking other cases of wraparound
    BOOST_CHECK_EQUAL(true, bearing::CheckInBounds(359, -5, 10));
    BOOST_CHECK_EQUAL(false, bearing::CheckInBounds(344, -5, 10));
    BOOST_CHECK_EQUAL(false, bearing::CheckInBounds(6, -5, 10));

    BOOST_CHECK_EQUAL(true, bearing::CheckInBounds(-1, 5, 10));
    BOOST_CHECK_EQUAL(false, bearing::CheckInBounds(-6, 5, 10));

    BOOST_CHECK_EQUAL(true, bearing::CheckInBounds(-721, 5, 10));
    BOOST_CHECK_EQUAL(true, bearing::CheckInBounds(719, 5, 10));
}

BOOST_AUTO_TEST_SUITE_END()
