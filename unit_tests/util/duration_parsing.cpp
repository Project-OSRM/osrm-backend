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

#include "../../extractor/extraction_helper_functions.hpp"

#include <boost/test/unit_test.hpp>
#include <boost/test/test_case_template.hpp>

BOOST_AUTO_TEST_SUITE(durations_are_valid)

BOOST_AUTO_TEST_CASE(all_necessary_test)
{
    BOOST_CHECK_EQUAL(durationIsValid("00:01"), true);
    BOOST_CHECK_EQUAL(durationIsValid("00:01:01"), true);
    BOOST_CHECK_EQUAL(durationIsValid("PT15M"), true);
}

BOOST_AUTO_TEST_CASE(common_durations_get_translated)
{
    BOOST_CHECK_EQUAL(parseDuration("00:01"), 60);
    BOOST_CHECK_EQUAL(parseDuration("00:01:01"), 61);
    BOOST_CHECK_EQUAL(parseDuration("01:01"), 3660);

    // check all combinations of iso duration tokens
    BOOST_CHECK_EQUAL(parseDuration("PT1M1S"), 61);
    BOOST_CHECK_EQUAL(parseDuration("PT1H1S"), 3601);
    BOOST_CHECK_EQUAL(parseDuration("PT15M"), 900);
    BOOST_CHECK_EQUAL(parseDuration("PT15S"), 15);
    BOOST_CHECK_EQUAL(parseDuration("PT15H"), 54000);
    BOOST_CHECK_EQUAL(parseDuration("PT1H15M"), 4500);
    BOOST_CHECK_EQUAL(parseDuration("PT1H15M1S"), 4501);
}

BOOST_AUTO_TEST_CASE(iso_8601_durations_case_insensitive)
{
    BOOST_CHECK_EQUAL(parseDuration("PT15m"), 900);
    BOOST_CHECK_EQUAL(parseDuration("PT1h15m"), 4500);
}

BOOST_AUTO_TEST_SUITE_END()
