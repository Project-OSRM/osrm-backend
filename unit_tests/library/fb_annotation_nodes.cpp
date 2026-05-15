#include <boost/test/unit_test.hpp>

#include <cstdint>
#include <vector>

#include "engine/api/flatbuffers/fbresult_generated.h"
#include "extractor/packed_osm_ids.hpp"

#include <flatbuffers/flatbuffers.h>

BOOST_AUTO_TEST_CASE(fb_annotation_nodes_64bit_roundtrip)
{
    using namespace osrm;
    using namespace osrm::engine::api;

    flatbuffers::FlatBufferBuilder builder;

    const std::vector<std::uint64_t> inputs = {
        0ull,
        1ull,
        (1ull << 33),                                // 2^33
        (1ull << 34),                                // 2^34, the old truncation boundary
        (1ull << 34) + 1,                            // just above 2^34
        20'000'000'000ull,                           // realistic projected OSM node ID
        (1ull << 35),                                // 2^35
        extractor::MAX_PACKED_OSM_NODE_ID - 1,       // just below the new limit
        extractor::MAX_PACKED_OSM_NODE_ID,           // exactly at the new limit
    };

    // Create flatbuffers vector of uint64_t nodes
    auto nodes_vector = builder.CreateVector(inputs);

    // Build an Annotation with the nodes vector
    fbresult::AnnotationBuilder annotation_builder(builder);
    annotation_builder.add_nodes(nodes_vector);
    auto annotation_off = annotation_builder.Finish();

    // Create a Leg that contains the annotation
    fbresult::LegBuilder leg_builder(builder);
    leg_builder.add_annotations(annotation_off);
    auto leg_off = leg_builder.Finish();

    // Create a RouteObject that contains the leg
    std::vector<flatbuffers::Offset<fbresult::Leg>> legs_vec = {leg_off};
    auto legs_vector = builder.CreateVector(legs_vec);

    fbresult::RouteObjectBuilder route_builder(builder);
    route_builder.add_legs(legs_vector);
    auto route_off = route_builder.Finish();

    // Create FBResult root with the route
    std::vector<flatbuffers::Offset<fbresult::RouteObject>> routes_vec = {route_off};
    auto routes_vector = builder.CreateVector(routes_vec);

    fbresult::FBResultBuilder result_builder(builder);
    result_builder.add_routes(routes_vector);
    auto result_off = result_builder.Finish();
    builder.Finish(result_off);

    // Read back and verify
    auto fb = fbresult::GetFBResult(builder.GetBufferPointer());
    BOOST_REQUIRE(fb);
    auto routes = fb->routes();
    BOOST_REQUIRE(routes);
    auto route = routes->Get(0);
    BOOST_REQUIRE(route);
    auto legs = route->legs();
    BOOST_REQUIRE(legs);
    auto leg = legs->Get(0);
    BOOST_REQUIRE(leg);
    auto annotation = leg->annotations();
    BOOST_REQUIRE(annotation);
    auto nodes = annotation->nodes();
    BOOST_REQUIRE(nodes);

    BOOST_CHECK_EQUAL(nodes->size(), inputs.size());
    for (std::size_t i = 0; i < inputs.size(); ++i)
    {
        BOOST_CHECK_EQUAL(static_cast<std::uint64_t>(nodes->Get(i)), inputs[i]);
    }
}
