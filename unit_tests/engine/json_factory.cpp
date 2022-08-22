#include "engine/api/json_factory.hpp"
#include "guidance/turn_instruction.hpp"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(json_factory)

BOOST_AUTO_TEST_CASE(instructionTypeToString_test_size)
{
    using namespace osrm::engine::api::json::detail;
    using namespace osrm::guidance;

    BOOST_CHECK_EQUAL(instructionTypeToString(TurnType::Sliproad), "invalid");
}

BOOST_AUTO_TEST_SUITE_END()
