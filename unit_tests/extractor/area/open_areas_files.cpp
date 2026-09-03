#include "extractor/area_routing_data.hpp"
#include "extractor/edge_based_node_segment.hpp"
#include "extractor/files.hpp"

#include "../../common/temporary_file.hpp"

#include <boost/test/unit_test.hpp>

#include <cstdint>
#include <vector>

BOOST_AUTO_TEST_SUITE(open_areas_files_test)

using namespace osrm;
using namespace osrm::extractor;

// The engine reads what extraction wrote, so the round trip has to be exact -- including
// the ring lengths, which are what separate an area's outer boundary from its obstacles.
BOOST_AUTO_TEST_CASE(open_areas_round_trip)
{
    TemporaryFile file;

    std::vector<AreaPolygonSegment> areas(2);
    areas[0].u = 0;
    areas[0].v = 1;
    areas[0].vertices_offset = 0;
    areas[0].num_vertices = 8; // a square with a square hole
    areas[0].rings_offset = 0;
    areas[0].num_rings = 2;
    areas[0].walking_speed = 1.4;

    areas[1].u = 2;
    areas[1].v = 3;
    areas[1].vertices_offset = 8;
    areas[1].num_vertices = 3; // a triangle, no obstacles
    areas[1].rings_offset = 2;
    areas[1].num_rings = 1;
    areas[1].walking_speed = 1.25;

    std::vector<util::Coordinate> bbox_corners;
    bbox_corners.reserve(2 * areas.size());
    for (std::size_t i = 0; i < 2 * areas.size(); ++i)
        bbox_corners.emplace_back(util::FloatLongitude{1.0 + i}, util::FloatLatitude{2.0 + i});

    std::vector<util::Coordinate> vertices;
    vertices.reserve(11);
    for (int i = 0; i < 11; ++i)
        vertices.emplace_back(util::FloatLongitude{1.0 + i * 0.001},
                              util::FloatLatitude{2.0 - i * 0.001});

    const std::vector<std::uint32_t> ring_lengths{4, 4, 3};

    // Two segments stand on vertex 0, one on vertex 3, none on the rest.  The offsets run
    // one longer than the vertices, so the last vertex's range has something to end at.
    std::vector<EdgeBasedNodeSegment> vertex_segments{
        EdgeBasedNodeSegment{SegmentID{7, true}, SegmentID{8, true}, 100, 101, 0, true},
        EdgeBasedNodeSegment{SegmentID{9, true}, SegmentID{10, true}, 100, 102, 1, true},
        EdgeBasedNodeSegment{SegmentID{11, true}, SegmentID{12, false}, 103, 100, 2, false}};
    std::vector<std::uint32_t> vertex_segment_offsets{0, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3};
    // vertex 0 sees 1 and 3, vertex 1 sees 0, vertex 3 sees 0; nothing else sees anything
    std::vector<std::uint32_t> visibility_targets{1, 3, 0, 0};
    std::vector<std::uint32_t> visibility_offsets{0, 2, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4};

    files::writeOpenAreas(file.path,
                          areas,
                          bbox_corners,
                          vertices,
                          ring_lengths,
                          vertex_segments,
                          vertex_segment_offsets,
                          visibility_targets,
                          visibility_offsets);

    std::vector<AreaPolygonSegment> read_areas;
    std::vector<util::Coordinate> read_bbox_corners;
    std::vector<util::Coordinate> read_vertices;
    std::vector<std::uint32_t> read_ring_lengths;
    std::vector<EdgeBasedNodeSegment> read_vertex_segments;
    std::vector<std::uint32_t> read_vertex_segment_offsets;
    std::vector<std::uint32_t> read_visibility_targets;
    std::vector<std::uint32_t> read_visibility_offsets;
    files::readOpenAreas(file.path,
                         read_areas,
                         read_bbox_corners,
                         read_vertices,
                         read_ring_lengths,
                         read_vertex_segments,
                         read_vertex_segment_offsets,
                         read_visibility_targets,
                         read_visibility_offsets);
    BOOST_CHECK(read_visibility_targets == visibility_targets);
    BOOST_CHECK(read_visibility_offsets == visibility_offsets);

    BOOST_REQUIRE_EQUAL(read_vertex_segments.size(), vertex_segments.size());
    for (std::size_t i = 0; i < vertex_segments.size(); ++i)
    {
        BOOST_CHECK_EQUAL(read_vertex_segments[i].forward_segment_id.id,
                          vertex_segments[i].forward_segment_id.id);
        BOOST_CHECK_EQUAL(read_vertex_segments[i].forward_segment_id.enabled,
                          vertex_segments[i].forward_segment_id.enabled);
        BOOST_CHECK_EQUAL(read_vertex_segments[i].reverse_segment_id.id,
                          vertex_segments[i].reverse_segment_id.id);
        BOOST_CHECK_EQUAL(read_vertex_segments[i].reverse_segment_id.enabled,
                          vertex_segments[i].reverse_segment_id.enabled);
        BOOST_CHECK_EQUAL(read_vertex_segments[i].u, vertex_segments[i].u);
        BOOST_CHECK_EQUAL(read_vertex_segments[i].v, vertex_segments[i].v);
        BOOST_CHECK_EQUAL(read_vertex_segments[i].fwd_segment_position,
                          vertex_segments[i].fwd_segment_position);
        BOOST_CHECK_EQUAL(read_vertex_segments[i].is_startpoint, vertex_segments[i].is_startpoint);
    }
    BOOST_CHECK(read_vertex_segment_offsets == vertex_segment_offsets);

    BOOST_REQUIRE_EQUAL(read_areas.size(), areas.size());
    for (std::size_t i = 0; i < areas.size(); ++i)
    {
        BOOST_CHECK_EQUAL(read_areas[i].u, areas[i].u);
        BOOST_CHECK_EQUAL(read_areas[i].v, areas[i].v);
        BOOST_CHECK_EQUAL(read_areas[i].vertices_offset, areas[i].vertices_offset);
        BOOST_CHECK_EQUAL(read_areas[i].num_vertices, areas[i].num_vertices);
        BOOST_CHECK_EQUAL(read_areas[i].rings_offset, areas[i].rings_offset);
        BOOST_CHECK_EQUAL(read_areas[i].num_rings, areas[i].num_rings);
        BOOST_CHECK_CLOSE(read_areas[i].walking_speed, areas[i].walking_speed, 0.0001);
    }

    const auto same_coordinates =
        [](const std::vector<util::Coordinate> &read, const std::vector<util::Coordinate> &written)
    {
        BOOST_REQUIRE_EQUAL(read.size(), written.size());
        for (std::size_t i = 0; i < written.size(); ++i)
        {
            BOOST_CHECK_EQUAL(static_cast<std::int32_t>(read[i].lon),
                              static_cast<std::int32_t>(written[i].lon));
            BOOST_CHECK_EQUAL(static_cast<std::int32_t>(read[i].lat),
                              static_cast<std::int32_t>(written[i].lat));
        }
    };
    same_coordinates(read_bbox_corners, bbox_corners);
    same_coordinates(read_vertices, vertices);

    BOOST_CHECK_EQUAL_COLLECTIONS(read_ring_lengths.begin(),
                                  read_ring_lengths.end(),
                                  ring_lengths.begin(),
                                  ring_lengths.end());
    // the ring lengths must account for exactly the vertices each area claims
    for (const auto &area : read_areas)
    {
        std::uint32_t total = 0;
        for (std::uint32_t r = 0; r < area.num_rings; ++r)
            total += read_ring_lengths[area.rings_offset + r];
        BOOST_CHECK_EQUAL(total, area.num_vertices);
    }
}

BOOST_AUTO_TEST_SUITE_END()
