#include "extractor/scripting_environment_lua.hpp"

#include "extractor/driving_side_index.hpp"
#include "extractor/extraction_turn.hpp"
#include "extractor/location_dependent_data.hpp"
#include "util/node_based_graph.hpp"

#include <boost/test/unit_test.hpp>

#include <vector>

BOOST_AUTO_TEST_SUITE(lua_scripting_memory)

BOOST_AUTO_TEST_CASE(respects_the_profiles_garbage_collector_state)
{
    using osrm::extractor::DrivingSideIndex;
    using osrm::extractor::LocationDependentData;
    using osrm::extractor::LuaScriptingContext;

    const LocationDependentData locations({});
    const DrivingSideIndex driving_side(DrivingSideIndex::Mode::Off);
    LuaScriptingContext context(locations, driving_side);
    context.state.stop_gc();
    context.state.script(R"(
local scratch = {}
for i = 1, 2097152 do
    scratch[i] = i
end
)");
    const auto before = context.state.memory_used();
    BOOST_REQUIRE_GT(before, 16 * 1024 * 1024);

    context.collectUnusedMemory();
    BOOST_CHECK_EQUAL(context.state.memory_used(), before);
    BOOST_CHECK(!context.state.is_gc_on());

    context.state.restart_gc();
    context.collectUnusedMemory();
    BOOST_CHECK_LT(context.state.memory_used(), before);
    BOOST_CHECK_EQUAL(context.live_heap_bytes, context.state.memory_used());
}

BOOST_AUTO_TEST_CASE(keeps_turn_results_after_the_periodic_heap_check)
{
    using osrm::extractor::ExtractionTurn;
    using osrm::extractor::ExtractionTurnLeg;
    using osrm::extractor::Sol2ScriptingEnvironment;
    using osrm::util::NodeBasedEdgeData;

    Sol2ScriptingEnvironment environment(OSRM_PROFILES_DIR "/testbot.lua", {});
    NodeBasedEdgeData edge;
    edge.duration = EdgeDuration{10};
    edge.distance = EdgeDistance{10};
    const std::vector<ExtractionTurnLeg> other_roads;
    const ExtractionTurn input(0, 1, 2, edge, edge, other_roads, other_roads, true);
    auto expected = input;
    environment.ProcessTurn(expected);
    BOOST_REQUIRE_GT(expected.duration, 0.0);

    auto result = input;
    for (unsigned i = 0; i < 65536; ++i)
    {
        result.duration = input.duration;
        result.weight = input.weight;
        environment.ProcessTurn(result);
    }
    BOOST_CHECK_EQUAL(result.duration, expected.duration);
    BOOST_CHECK_EQUAL(result.weight, expected.weight);
}

BOOST_AUTO_TEST_SUITE_END()
