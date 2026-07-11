#include "extractor/extraction_node.hpp"
#include "extractor/extraction_relation.hpp"
#include "extractor/extraction_way.hpp"
#include "extractor/maneuver_override.hpp"
#include "extractor/maneuver_override_relation_parser.hpp"
#include "extractor/restriction.hpp"
#include "extractor/restriction_parser.hpp"
#include "extractor/scripting_environment_lua.hpp"

#include <boost/test/unit_test.hpp>

#include <osmium/builder/osm_object_builder.hpp>
#include <osmium/memory/buffer.hpp>

#include <initializer_list>
#include <utility>
#include <vector>

BOOST_AUTO_TEST_SUITE(car_profile_width_penalty)

namespace
{

osrm::extractor::ExtractionWay
process_way(std::initializer_list<std::pair<const char *, const char *>> tags)
{
    osmium::memory::Buffer buffer{1024, osmium::memory::Buffer::auto_grow::yes};
    {
        osmium::builder::WayBuilder way_builder{buffer};
        way_builder.set_id(1);
        way_builder.add_node_refs({osmium::NodeRef{1}, osmium::NodeRef{2}});
        way_builder.add_tags(tags);
    }
    buffer.commit();

    osrm::extractor::Sol2ScriptingEnvironment scripting_environment(OSRM_PROFILES_DIR "/car.lua",
                                                                    {});
    osrm::extractor::RestrictionParser restriction_parser(
        false, false, scripting_environment.GetRestrictions());
    osrm::extractor::ManeuverOverrideRelationParser maneuver_override_parser;
    osrm::extractor::ScriptingResults results;
    results.osmium_buffer = std::make_shared<osmium::memory::Buffer>(std::move(buffer));

    scripting_environment.ProcessElements(
        results, restriction_parser, maneuver_override_parser);

    BOOST_REQUIRE_EQUAL(results.resulting_ways.size(), 1);
    return results.resulting_ways.front().second;
}

double forward_rate_for_width(const char *width)
{
    return process_way({{"highway", "primary"}, {"width", width}}).forward_rate;
}

} // namespace

BOOST_AUTO_TEST_CASE(width_penalty_uses_graduated_steps)
{
    const double baseline_rate = forward_rate_for_width("10");
    BOOST_CHECK_CLOSE(forward_rate_for_width("5"), baseline_rate * 0.8, 0.0001);
    BOOST_CHECK_CLOSE(forward_rate_for_width("5.5"), baseline_rate * 0.8, 0.0001);
    BOOST_CHECK_CLOSE(forward_rate_for_width("4.5 m"), baseline_rate * 0.75, 0.0001);
    BOOST_CHECK_CLOSE(forward_rate_for_width("3.5"), baseline_rate * 0.6, 0.0001);
    BOOST_CHECK_CLOSE(forward_rate_for_width("3"), baseline_rate * 0.5, 0.0001);
}

BOOST_AUTO_TEST_CASE(width_penalty_supports_imperial_width_values)
{
    BOOST_CHECK_CLOSE(forward_rate_for_width("9'10\""), forward_rate_for_width("3"), 0.0001);
    BOOST_CHECK_CLOSE(forward_rate_for_width("18'"), forward_rate_for_width("5.5"), 0.0001);
    BOOST_CHECK_CLOSE(forward_rate_for_width("20'"), forward_rate_for_width("10"), 0.0001);
}

BOOST_AUTO_TEST_CASE(width_penalty_ignores_malformed_width_values)
{
    const double baseline_rate = forward_rate_for_width("10");
    BOOST_CHECK_CLOSE(forward_rate_for_width("abc"), baseline_rate, 0.0001);
    BOOST_CHECK_CLOSE(forward_rate_for_width("n/a"), baseline_rate, 0.0001);
    BOOST_CHECK_CLOSE(forward_rate_for_width("unknown"), baseline_rate, 0.0001);
}

BOOST_AUTO_TEST_SUITE_END()
