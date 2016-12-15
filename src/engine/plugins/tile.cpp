#include "engine/plugins/tile.hpp"
#include "engine/edge_unpacker.hpp"
#include "engine/plugins/plugin_base.hpp"

#include "util/coordinate_calculation.hpp"
#include "util/string_view.hpp"
#include "util/vector_tile.hpp"
#include "util/web_mercator.hpp"

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/multi/geometries/multi_linestring.hpp>

#include <protozero/pbf_writer.hpp>
#include <protozero/varint.hpp>

#include <algorithm>
#include <numeric>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <cmath>
#include <cstdint>

namespace osrm
{
namespace engine
{
namespace plugins
{
namespace
{
// TODO: Port all this encoding logic to https://github.com/mapbox/vector-tile, which wasn't
// available when this code was originally written.

// Simple container class for WGS84 coordinates
template <typename T> struct Point final
{
    Point(T _x, T _y) : x(_x), y(_y) {}

    const T x;
    const T y;
};

// Simple container to hold a bounding box
struct BBox final
{
    BBox(const double _minx, const double _miny, const double _maxx, const double _maxy)
        : minx(_minx), miny(_miny), maxx(_maxx), maxy(_maxy)
    {
    }

    double width() const { return maxx - minx; }
    double height() const { return maxy - miny; }

    const double minx;
    const double miny;
    const double maxx;
    const double maxy;
};

// Simple container for integer coordinates (i.e. pixel coords)
struct point_type_i final
{
    point_type_i(std::int64_t _x, std::int64_t _y) : x(_x), y(_y) {}

    const std::int64_t x;
    const std::int64_t y;
};

// Used to accumulate all the information we want in the tile about
// a turn.
struct TurnData final
{
    TurnData(const util::Coordinate coordinate_,
             const std::size_t _in,
             const std::size_t _out,
             const std::size_t _weight)
        : coordinate(std::move(coordinate_)), in_angle_offset(_in), turn_angle_offset(_out),
          weight_offset(_weight)
    {
    }

    const util::Coordinate coordinate;
    const std::size_t in_angle_offset;
    const std::size_t turn_angle_offset;
    const std::size_t weight_offset;
};

using FixedPoint = Point<std::int32_t>;
using FloatPoint = Point<double>;

using FixedLine = std::vector<FixedPoint>;
using FloatLine = std::vector<FloatPoint>;

constexpr const static int MIN_ZOOM_FOR_TURNS = 15;

// We use boost::geometry to clip lines/points that are outside or cross the boundary
// of the tile we're rendering.  We need these types defined to use boosts clipping
// logic
typedef boost::geometry::model::point<double, 2, boost::geometry::cs::cartesian> point_t;
typedef boost::geometry::model::linestring<point_t> linestring_t;
typedef boost::geometry::model::box<point_t> box_t;
typedef boost::geometry::model::multi_linestring<linestring_t> multi_linestring_t;
const static box_t clip_box(point_t(-util::vector_tile::BUFFER, -util::vector_tile::BUFFER),
                            point_t(util::vector_tile::EXTENT + util::vector_tile::BUFFER,
                                    util::vector_tile::EXTENT + util::vector_tile::BUFFER));

// from mapnik-vector-tile
// Encodes a linestring using protobuf zigzag encoding
inline bool encodeLinestring(const FixedLine &line,
                             protozero::packed_field_uint32 &geometry,
                             std::int32_t &start_x,
                             std::int32_t &start_y)
{
    const std::size_t line_size = line.size();
    if (line_size < 2)
    {
        return false;
    }

    const unsigned lineto_count = static_cast<const unsigned>(line_size) - 1;

    auto pt = line.begin();
    const constexpr int MOVETO_COMMAND = 9;
    geometry.add_element(MOVETO_COMMAND); // move_to | (1 << 3)
    geometry.add_element(protozero::encode_zigzag32(pt->x - start_x));
    geometry.add_element(protozero::encode_zigzag32(pt->y - start_y));
    start_x = pt->x;
    start_y = pt->y;
    // This means LINETO repeated N times
    // See: https://github.com/mapbox/vector-tile-spec/tree/master/2.1#example-command-integers
    geometry.add_element((lineto_count << 3u) | 2u);
    // Now that we've issued the LINETO REPEAT N command, we append
    // N coordinate pairs immediately after the command.
    for (++pt; pt != line.end(); ++pt)
    {
        const std::int32_t dx = pt->x - start_x;
        const std::int32_t dy = pt->y - start_y;
        geometry.add_element(protozero::encode_zigzag32(dx));
        geometry.add_element(protozero::encode_zigzag32(dy));
        start_x = pt->x;
        start_y = pt->y;
    }
    return true;
}

// from mapnik-vctor-tile
// Encodes a point
inline void encodePoint(const FixedPoint &pt, protozero::packed_field_uint32 &geometry)
{
    const constexpr int MOVETO_COMMAND = 9;
    geometry.add_element(MOVETO_COMMAND);
    const std::int32_t dx = pt.x;
    const std::int32_t dy = pt.y;
    // Manual zigzag encoding.
    geometry.add_element(protozero::encode_zigzag32(dx));
    geometry.add_element(protozero::encode_zigzag32(dy));
}

/**
 * Returnx the x1,y1,x2,y2 pixel coordinates of a line in a given
 * tile.
 *
 * @param start the first coordinate of the line
 * @param target the last coordinate of the line
 * @param tile_bbox the boundaries of the tile, in mercator coordinates
 * @return a FixedLine with coordinates relative to the tile_bbox.
 */
FixedLine coordinatesToTileLine(const util::Coordinate start,
                                const util::Coordinate target,
                                const BBox &tile_bbox)
{
    FloatLine geo_line;
    geo_line.emplace_back(static_cast<double>(util::toFloating(start.lon)),
                          static_cast<double>(util::toFloating(start.lat)));
    geo_line.emplace_back(static_cast<double>(util::toFloating(target.lon)),
                          static_cast<double>(util::toFloating(target.lat)));

    linestring_t unclipped_line;

    for (auto const &pt : geo_line)
    {
        double px_merc = pt.x * util::web_mercator::DEGREE_TO_PX;
        double py_merc = util::web_mercator::latToY(util::FloatLatitude{pt.y}) *
                         util::web_mercator::DEGREE_TO_PX;
        // convert lon/lat to tile coordinates
        const auto px = std::round(
            ((px_merc - tile_bbox.minx) * util::web_mercator::TILE_SIZE / tile_bbox.width()) *
            util::vector_tile::EXTENT / util::web_mercator::TILE_SIZE);
        const auto py = std::round(
            ((tile_bbox.maxy - py_merc) * util::web_mercator::TILE_SIZE / tile_bbox.height()) *
            util::vector_tile::EXTENT / util::web_mercator::TILE_SIZE);

        boost::geometry::append(unclipped_line, point_t(px, py));
    }

    multi_linestring_t clipped_line;

    boost::geometry::intersection(clip_box, unclipped_line, clipped_line);

    FixedLine tile_line;

    // b::g::intersection might return a line with one point if the
    // original line was very short and coords were dupes
    if (!clipped_line.empty() && clipped_line[0].size() == 2)
    {
        if (clipped_line[0].size() == 2)
        {
            for (const auto &p : clipped_line[0])
            {
                tile_line.emplace_back(p.get<0>(), p.get<1>());
            }
        }
    }

    return tile_line;
}

/**
 * Converts lon/lat into coordinates inside a Mercator projection tile (x/y pixel values)
 *
 * @param point the lon/lat you want the tile coords for
 * @param tile_bbox the mercator boundaries of the tile
 * @return a point (x,y) on the tile defined by tile_bbox
 */
FixedPoint coordinatesToTilePoint(const util::Coordinate point, const BBox &tile_bbox)
{
    const FloatPoint geo_point{static_cast<double>(util::toFloating(point.lon)),
                               static_cast<double>(util::toFloating(point.lat))};

    const double px_merc = geo_point.x * util::web_mercator::DEGREE_TO_PX;
    const double py_merc = util::web_mercator::latToY(util::FloatLatitude{geo_point.y}) *
                           util::web_mercator::DEGREE_TO_PX;

    const auto px = static_cast<std::int32_t>(std::round(
        ((px_merc - tile_bbox.minx) * util::web_mercator::TILE_SIZE / tile_bbox.width()) *
        util::vector_tile::EXTENT / util::web_mercator::TILE_SIZE));
    const auto py = static_cast<std::int32_t>(std::round(
        ((tile_bbox.maxy - py_merc) * util::web_mercator::TILE_SIZE / tile_bbox.height()) *
        util::vector_tile::EXTENT / util::web_mercator::TILE_SIZE));

    return FixedPoint{px, py};
}

} // namespace

Status TilePlugin::HandleRequest(const std::shared_ptr<const datafacade::BaseDataFacade> facade,
                                 const api::TileParameters &parameters,
                                 std::string &pbf_buffer) const
{
    BOOST_ASSERT(parameters.IsValid());

    double min_lon, min_lat, max_lon, max_lat;

    // Convert the z,x,y mercator tile coordinates into WGS84 lon/lat values
    //
    util::web_mercator::xyzToWGS84(parameters.x,
                                   parameters.y,
                                   parameters.z,
                                   min_lon,
                                   min_lat,
                                   max_lon,
                                   max_lat,
                                   util::web_mercator::TILE_SIZE * 0.10);

    util::Coordinate southwest{util::FloatLongitude{min_lon}, util::FloatLatitude{min_lat}};
    util::Coordinate northeast{util::FloatLongitude{max_lon}, util::FloatLatitude{max_lat}};

    // Fetch all the segments that are in our bounding box.
    // This hits the OSRM StaticRTree
    const auto edges = facade->GetEdgesInBox(southwest, northeast);

    // Vector tiles encode properties as references to a common lookup table.
    // When we add a property to a "feature", we actually attach the index of the value
    // rather than the value itself.  Thus, we need to keep a list of the unique
    // values we need, and we add this list to the tile as a lookup table.  This
    // vector holds all the actual used values, the feature refernce offsets in
    // this vector.
    // for integer values
    std::vector<int> used_line_ints;
    // While constructing the tile, we keep track of which integers we have in our table
    // and their offsets, so multiple features can re-use the same values
    std::unordered_map<int, std::size_t> line_int_offsets;

    // Same idea for street names - one lookup table for names for all features
    std::vector<util::StringView> names;
    std::unordered_map<util::StringView, std::size_t> name_offsets;

    // And again for integer values used by points.
    std::vector<int> used_point_ints;
    std::unordered_map<int, std::size_t> point_int_offsets;

    // And again for float values used by points
    std::vector<float> used_point_floats;
    std::unordered_map<float, std::size_t> point_float_offsets;

    std::uint8_t max_datasource_id = 0;

    // This is where we accumulate information on turns
    std::vector<TurnData> all_turn_data;

    // Helper function for adding a new value to the line_ints lookup table.  Returns
    // the index of the value in the table, adding the value if it doesn't already
    // exist
    const auto use_line_value = [&used_line_ints, &line_int_offsets](const int value) {
        const auto found = line_int_offsets.find(value);

        if (found == line_int_offsets.end())
        {
            used_line_ints.push_back(value);
            line_int_offsets[value] = used_line_ints.size() - 1;
        }

        return;
    };

    // Same again
    const auto use_point_int_value = [&used_point_ints, &point_int_offsets](const int value) {
        const auto found = point_int_offsets.find(value);
        std::size_t offset;

        if (found == point_int_offsets.end())
        {
            used_point_ints.push_back(value);
            offset = used_point_ints.size() - 1;
            point_int_offsets[value] = offset;
        }
        else
        {
            offset = found->second;
        }

        return offset;
    };

    // And a third time, should probably template this....
    const auto use_point_float_value = [&used_point_floats,
                                        &point_float_offsets](const float value) {
        const auto found = point_float_offsets.find(value);
        std::size_t offset;

        if (found == point_float_offsets.end())
        {
            used_point_floats.push_back(value);
            offset = used_point_floats.size() - 1;
            point_float_offsets[value] = offset;
        }
        else
        {
            offset = found->second;
        }

        return offset;
    };

    // In order to ensure consistent tile encoding, we need to process
    // all edges in the same order.  Differences in OSX/Linux/Windows
    // sorting methods mean that GetEdgesInBox doesn't return the same
    // ordered array on all platforms.
    // GetEdgesInBox is marked `const`, so we can't sort the array itself,
    // instead we create an array of indexes and sort that instead.
    std::vector<std::size_t> sorted_edge_indexes(edges.size(), 0);
    std::iota(sorted_edge_indexes.begin(), sorted_edge_indexes.end(), 0); // fill with 1,2,3,...N

    // Now, sort that array based on the edges list, using the u/v node IDs
    // as the sort condition
    std::sort(sorted_edge_indexes.begin(),
              sorted_edge_indexes.end(),
              [&edges](const std::size_t &left, const std::size_t &right) -> bool {
                  return (edges[left].u != edges[right].u) ? edges[left].u < edges[right].u
                                                           : edges[left].v < edges[right].v;
              });
    // From here on, we'll iterate over the sorted_edge_indexes instead of `edges` directly.
    // Note, that we do this because `

    // If we're zooming into 16 or higher, include turn data.  Why?  Because turns make the map
    // really cramped, so we don't bother including the data for tiles that span a large area.
    if (parameters.z >= MIN_ZOOM_FOR_TURNS)
    {
        // Struct to hold info on all the EdgeBasedNodes that are visible in our tile
        // When we create these, we insure that (source, target) and packed_geometry_id
        // are all pointed in the same direction.
        struct EdgeBasedNodeInfo
        {
            bool is_geometry_forward; // Is the geometry forward or reverse?
            unsigned packed_geometry_id;
        };
        // Lookup table for edge-based-nodes
        std::unordered_map<NodeID, EdgeBasedNodeInfo> edge_based_node_info;

        struct SegmentData
        {
            NodeID target_node;
            EdgeID edge_based_node_id;
        };

        std::unordered_map<NodeID, std::vector<SegmentData>> directed_graph;
        // Reserve enough space for unique edge-based-nodes on every edge.
        // Only a tile with all unique edges will use this much, but
        // it saves us a bunch of re-allocations during iteration.
        directed_graph.reserve(edges.size() * 2);

        // Build an adjacency list for all the road segments visible in
        // the tile
        for (const auto &edge_index : sorted_edge_indexes)
        {
            const auto &edge = edges[edge_index];
            if (edge.forward_segment_id.enabled)
            {
                // operator[] will construct an empty vector at [edge.u] if there is no value.
                directed_graph[edge.u].push_back({edge.v, edge.forward_segment_id.id});
                if (edge_based_node_info.count(edge.forward_segment_id.id) == 0)
                {
                    edge_based_node_info[edge.forward_segment_id.id] = {true,
                                                                        edge.packed_geometry_id};
                }
                else
                {
                    BOOST_ASSERT(
                        edge_based_node_info[edge.forward_segment_id.id].is_geometry_forward ==
                        true);
                    BOOST_ASSERT(
                        edge_based_node_info[edge.forward_segment_id.id].packed_geometry_id ==
                        edge.packed_geometry_id);
                }
            }
            if (edge.reverse_segment_id.enabled)
            {
                directed_graph[edge.v].push_back({edge.u, edge.reverse_segment_id.id});
                if (edge_based_node_info.count(edge.reverse_segment_id.id) == 0)
                {
                    edge_based_node_info[edge.reverse_segment_id.id] = {false,
                                                                        edge.packed_geometry_id};
                }
                else
                {
                    BOOST_ASSERT(
                        edge_based_node_info[edge.reverse_segment_id.id].is_geometry_forward ==
                        false);
                    BOOST_ASSERT(
                        edge_based_node_info[edge.reverse_segment_id.id].packed_geometry_id ==
                        edge.packed_geometry_id);
                }
            }
        }

        // Given a turn:
        //     u---v
        //         |
        //         w
        //  uv is the "approach"
        //  vw is the "exit"
        std::vector<contractor::QueryEdge::EdgeData> unpacked_shortcut;
        std::vector<EdgeWeight> approach_weight_vector;

        // Make sure we traverse the startnodes in a consistent order
        // to ensure identical PBF encoding on all platforms.
        std::vector<NodeID> sorted_startnodes;
        sorted_startnodes.reserve(directed_graph.size());
        for (const auto &startnode : directed_graph)
            sorted_startnodes.push_back(startnode.first);
        std::sort(sorted_startnodes.begin(), sorted_startnodes.end());

        // Look at every node in the directed graph we created
        for (const auto &startnode : sorted_startnodes)
        {
            const auto &nodedata = directed_graph[startnode];
            // For all the outgoing edges from the node
            for (const auto &approachedge : nodedata)
            {
                // If the target of this edge doesn't exist in our directed
                // graph, it's probably outside the tile, so we can skip it
                if (directed_graph.count(approachedge.target_node) == 0)
                    continue;

                // For each of the outgoing edges from our target coordinate
                for (const auto &exit_edge : directed_graph[approachedge.target_node])
                {
                    // If the next edge has the same edge_based_node_id, then it's
                    // not a turn, so skip it
                    if (approachedge.edge_based_node_id == exit_edge.edge_based_node_id)
                        continue;

                    // Skip u-turns
                    if (startnode == exit_edge.target_node)
                        continue;

                    // Find the connection between our source road and the target node
                    // Since we only want to find direct edges, we cannot check shortcut edges here.
                    // Otherwise we might find a forward edge even though a shorter backward edge
                    // exists (due to oneways).
                    //
                    // a > - > - > - b
                    // |             |
                    // |------ c ----|
                    //
                    // would offer a backward edge at `b` to `a` (due to the oneway from a to b)
                    // but could also offer a shortcut (b-c-a) from `b` to `a` which is longer.
                    EdgeID smaller_edge_id =
                        facade->FindSmallestEdge(approachedge.edge_based_node_id,
                                                 exit_edge.edge_based_node_id,
                                                 [](const contractor::QueryEdge::EdgeData &data) {
                                                     return data.forward && !data.shortcut;
                                                 });

                    // Depending on how the graph is constructed, we might have to look for
                    // a backwards edge instead.  They're equivalent, just one is available for
                    // a forward routing search, and one is used for the backwards dijkstra
                    // steps.  Their weight should be the same, we can use either one.
                    // If we didn't find a forward edge, try for a backward one
                    if (SPECIAL_EDGEID == smaller_edge_id)
                    {
                        smaller_edge_id = facade->FindSmallestEdge(
                            exit_edge.edge_based_node_id,
                            approachedge.edge_based_node_id,
                            [](const contractor::QueryEdge::EdgeData &data) {
                                return data.backward && !data.shortcut;
                            });
                    }

                    // If no edge was found, it means that there's no connection between these
                    // nodes, due to oneways or turn restrictions. Given the edge-based-nodes
                    // that we're examining here, we *should* only find directly-connected
                    // edges, not shortcuts
                    if (smaller_edge_id != SPECIAL_EDGEID)
                    {
                        const auto &data = facade->GetEdgeData(smaller_edge_id);
                        BOOST_ASSERT_MSG(!data.shortcut, "Connecting edge must not be a shortcut");

                        // Now, calculate the sum of the weight of all the segments.
                        if (edge_based_node_info[approachedge.edge_based_node_id]
                                .is_geometry_forward)
                        {
                            approach_weight_vector = facade->GetUncompressedForwardWeights(
                                edge_based_node_info[approachedge.edge_based_node_id]
                                    .packed_geometry_id);
                        }
                        else
                        {
                            approach_weight_vector = facade->GetUncompressedReverseWeights(
                                edge_based_node_info[approachedge.edge_based_node_id]
                                    .packed_geometry_id);
                        }
                        const auto sum_node_weight = std::accumulate(approach_weight_vector.begin(),
                                                                     approach_weight_vector.end(),
                                                                     EdgeWeight{0});

                        // The edge.weight is the whole edge weight, which includes the turn
                        // cost.
                        // The turn cost is the edge.weight minus the sum of the individual road
                        // segment weights.  This might not be 100% accurate, because some
                        // intersections include stop signs, traffic signals and other
                        // penalties, but at this stage, we can't divide those out, so we just
                        // treat the whole lot as the "turn cost" that we'll stick on the map.
                        const auto turn_cost = data.weight - sum_node_weight;

                        // Find the three nodes that make up the turn movement)
                        const auto node_from = startnode;
                        const auto node_via = approachedge.target_node;
                        const auto node_to = exit_edge.target_node;

                        const auto coord_from = facade->GetCoordinateOfNode(node_from);
                        const auto coord_via = facade->GetCoordinateOfNode(node_via);
                        const auto coord_to = facade->GetCoordinateOfNode(node_to);

                        // Calculate the bearing that we approach the intersection at
                        const auto angle_in = static_cast<int>(
                            util::coordinate_calculation::bearing(coord_from, coord_via));

                        // Add the angle to the values table for the vector tile, and get the
                        // index
                        // of that value in the table
                        const auto angle_in_index = use_point_int_value(angle_in);

                        // Calculate the bearing leading away from the intersection
                        const auto exit_bearing = static_cast<int>(
                            util::coordinate_calculation::bearing(coord_via, coord_to));

                        // Figure out the angle of the turn
                        auto turn_angle = exit_bearing - angle_in;
                        while (turn_angle > 180)
                        {
                            turn_angle -= 360;
                        }
                        while (turn_angle < -180)
                        {
                            turn_angle += 360;
                        }

                        // Add the turn angle value to the value lookup table for the vector
                        // tile.
                        const auto turn_angle_index = use_point_int_value(turn_angle);
                        // And, same for the actual turn cost value - it goes in the lookup
                        // table,
                        // not directly on the feature itself.
                        const auto turn_cost_index = use_point_float_value(
                            turn_cost / 10.0); // Note conversion to float here

                        // Save everything we need to later add all the points to the tile.
                        // We need the coordinate of the intersection, the angle in, the turn
                        // angle and the turn cost.
                        all_turn_data.emplace_back(
                            coord_via, angle_in_index, turn_angle_index, turn_cost_index);
                    }
                }
            }
        }
    }

    // Vector tiles encode feature properties as indexes into a lookup table.  So, we need
    // to "pre-loop" over all the edges to create the lookup tables.  Once we have those, we
    // can then encode the features, and we'll know the indexes that feature properties
    // need to refer to.
    for (const auto &edge_index : sorted_edge_indexes)
    {
        const auto &edge = edges[edge_index];

        const auto forward_datasource_vector =
            facade->GetUncompressedForwardDatasources(edge.packed_geometry_id);
        const auto reverse_datasource_vector =
            facade->GetUncompressedReverseDatasources(edge.packed_geometry_id);

        BOOST_ASSERT(edge.fwd_segment_position < forward_datasource_vector.size());
        const auto forward_datasource = forward_datasource_vector[edge.fwd_segment_position];
        BOOST_ASSERT(edge.fwd_segment_position < reverse_datasource_vector.size());
        const auto reverse_datasource = reverse_datasource_vector[reverse_datasource_vector.size() -
                                                                  edge.fwd_segment_position - 1];

        // Keep track of the highest datasource seen so that we don't write unnecessary
        // data to the layer attribute values
        max_datasource_id = std::max(max_datasource_id, forward_datasource);
        max_datasource_id = std::max(max_datasource_id, reverse_datasource);
    }

    // Convert tile coordinates into mercator coordinates
    double min_mercator_lon, min_mercator_lat, max_mercator_lon, max_mercator_lat;
    util::web_mercator::xyzToMercator(parameters.x,
                                      parameters.y,
                                      parameters.z,
                                      min_mercator_lon,
                                      min_mercator_lat,
                                      max_mercator_lon,
                                      max_mercator_lat);
    const BBox tile_bbox{min_mercator_lon, min_mercator_lat, max_mercator_lon, max_mercator_lat};

    // Protobuf serializes blocks when objects go out of scope, hence
    // the extra scoping below.
    protozero::pbf_writer tile_writer{pbf_buffer};
    {
        {
            // Add a layer object to the PBF stream.  3=='layer' from the vector tile spec
            // (2.1)
            protozero::pbf_writer line_layer_writer(tile_writer, util::vector_tile::LAYER_TAG);
            // TODO: don't write a layer if there are no features

            line_layer_writer.add_uint32(util::vector_tile::VERSION_TAG, 2); // version
            // Field 1 is the "layer name" field, it's a string
            line_layer_writer.add_string(util::vector_tile::NAME_TAG, "speeds"); // name
            // Field 5 is the tile extent.  It's a uint32 and should be set to 4096
            // for normal vector tiles.
            line_layer_writer.add_uint32(util::vector_tile::EXTENT_TAG,
                                         util::vector_tile::EXTENT); // extent

            // Because we need to know the indexes into the vector tile lookup table,
            // we need to do an initial pass over the data and create the complete
            // index of used values.
            for (const auto &edge_index : sorted_edge_indexes)
            {
                const auto &edge = edges[edge_index];
                const auto forward_weight_vector =
                    facade->GetUncompressedForwardWeights(edge.packed_geometry_id);
                const auto reverse_weight_vector =
                    facade->GetUncompressedReverseWeights(edge.packed_geometry_id);
                const auto forward_weight = forward_weight_vector[edge.fwd_segment_position];
                const auto reverse_weight = reverse_weight_vector[reverse_weight_vector.size() -
                                                                  edge.fwd_segment_position - 1];
                use_line_value(reverse_weight);
                use_line_value(forward_weight);
            }

            // Begin the layer features block
            {
                // Each feature gets a unique id, starting at 1
                unsigned id = 1;
                for (const auto &edge_index : sorted_edge_indexes)
                {
                    const auto &edge = edges[edge_index];
                    // Get coordinates for start/end nodes of segment (NodeIDs u and v)
                    const auto a = facade->GetCoordinateOfNode(edge.u);
                    const auto b = facade->GetCoordinateOfNode(edge.v);
                    // Calculate the length in meters
                    const double length =
                        osrm::util::coordinate_calculation::haversineDistance(a, b);

                    const auto forward_weight_vector =
                        facade->GetUncompressedForwardWeights(edge.packed_geometry_id);
                    const auto reverse_weight_vector =
                        facade->GetUncompressedReverseWeights(edge.packed_geometry_id);
                    const auto forward_datasource_vector =
                        facade->GetUncompressedForwardDatasources(edge.packed_geometry_id);
                    const auto reverse_datasource_vector =
                        facade->GetUncompressedReverseDatasources(edge.packed_geometry_id);
                    const auto forward_weight = forward_weight_vector[edge.fwd_segment_position];
                    const auto reverse_weight =
                        reverse_weight_vector[reverse_weight_vector.size() -
                                              edge.fwd_segment_position - 1];
                    const auto forward_datasource =
                        forward_datasource_vector[edge.fwd_segment_position];
                    const auto reverse_datasource =
                        reverse_datasource_vector[reverse_datasource_vector.size() -
                                                  edge.fwd_segment_position - 1];

                    auto name = facade->GetNameForID(edge.name_id);

                    const auto name_offset = [&name, &names, &name_offsets]() {
                        auto iter = name_offsets.find(name);
                        if (iter == name_offsets.end())
                        {
                            auto offset = names.size();
                            name_offsets[name] = offset;
                            names.push_back(name);
                            return offset;
                        }
                        return iter->second;
                    }();

                    const auto encode_tile_line = [&line_layer_writer,
                                                   &edge,
                                                   &id,
                                                   &max_datasource_id,
                                                   &used_line_ints](const FixedLine &tile_line,
                                                                    const std::uint32_t speed_kmh,
                                                                    const std::size_t duration,
                                                                    const DatasourceID datasource,
                                                                    const std::size_t name_idx,
                                                                    std::int32_t &start_x,
                                                                    std::int32_t &start_y) {
                        // Here, we save the two attributes for our feature: the speed and
                        // the is_small boolean.  We only serve up speeds from 0-139, so all we
                        // do is save the first
                        protozero::pbf_writer feature_writer(line_layer_writer,
                                                             util::vector_tile::FEATURE_TAG);
                        // Field 3 is the "geometry type" field.  Value 2 is "line"
                        feature_writer.add_enum(
                            util::vector_tile::GEOMETRY_TAG,
                            util::vector_tile::GEOMETRY_TYPE_LINE); // geometry type
                        // Field 1 for the feature is the "id" field.
                        feature_writer.add_uint64(util::vector_tile::ID_TAG, id++); // id
                        {
                            // When adding attributes to a feature, we have to write
                            // pairs of numbers.  The first value is the index in the
                            // keys array (written later), and the second value is the
                            // index into the "values" array (also written later).  We're
                            // not writing the actual speed or bool value here, we're saving
                            // an index into the "values" array.  This means many features
                            // can share the same value data, leading to smaller tiles.
                            protozero::packed_field_uint32 field(
                                feature_writer, util::vector_tile::FEATURE_ATTRIBUTES_TAG);

                            field.add_element(0); // "speed" tag key offset
                            field.add_element(
                                std::min(speed_kmh, 127u)); // save the speed value, capped at 127
                            field.add_element(1);           // "is_small" tag key offset
                            field.add_element(128 +
                                              (edge.component.is_tiny ? 0 : 1)); // is_small feature
                            field.add_element(2);                // "datasource" tag key offset
                            field.add_element(130 + datasource); // datasource value offset
                            field.add_element(3);                // "duration" tag key offset
                            field.add_element(130 + max_datasource_id + 1 +
                                              duration); // duration value offset
                            field.add_element(4);        // "name" tag key offset

                            field.add_element(130 + max_datasource_id + 1 + used_line_ints.size() +
                                              name_idx); // name value offset
                        }
                        {

                            // Encode the geometry for the feature
                            protozero::packed_field_uint32 geometry(
                                feature_writer, util::vector_tile::FEATURE_GEOMETRIES_TAG);
                            encodeLinestring(tile_line, geometry, start_x, start_y);
                        }
                    };

                    // If this is a valid forward edge, go ahead and add it to the tile
                    if (forward_weight != 0 && edge.forward_segment_id.enabled)
                    {
                        std::int32_t start_x = 0;
                        std::int32_t start_y = 0;

                        // Calculate the speed for this line
                        std::uint32_t speed_kmh =
                            static_cast<std::uint32_t>(round(length / forward_weight * 10 * 3.6));

                        auto tile_line = coordinatesToTileLine(a, b, tile_bbox);
                        if (!tile_line.empty())
                        {
                            encode_tile_line(tile_line,
                                             speed_kmh,
                                             line_int_offsets[forward_weight],
                                             forward_datasource,
                                             name_offset,
                                             start_x,
                                             start_y);
                        }
                    }

                    // Repeat the above for the coordinates reversed and using the `reverse`
                    // properties
                    if (reverse_weight != 0 && edge.reverse_segment_id.enabled)
                    {
                        std::int32_t start_x = 0;
                        std::int32_t start_y = 0;

                        // Calculate the speed for this line
                        std::uint32_t speed_kmh =
                            static_cast<std::uint32_t>(round(length / reverse_weight * 10 * 3.6));

                        auto tile_line = coordinatesToTileLine(b, a, tile_bbox);
                        if (!tile_line.empty())
                        {
                            encode_tile_line(tile_line,
                                             speed_kmh,
                                             line_int_offsets[reverse_weight],
                                             reverse_datasource,
                                             name_offset,
                                             start_x,
                                             start_y);
                        }
                    }
                }
            }

            // Field id 3 is the "keys" attribute
            // We need two "key" fields, these are referred to with 0 and 1 (their array
            // indexes) earlier
            line_layer_writer.add_string(util::vector_tile::KEY_TAG, "speed");
            line_layer_writer.add_string(util::vector_tile::KEY_TAG, "is_small");
            line_layer_writer.add_string(util::vector_tile::KEY_TAG, "datasource");
            line_layer_writer.add_string(util::vector_tile::KEY_TAG, "duration");
            line_layer_writer.add_string(util::vector_tile::KEY_TAG, "name");

            // Now, we write out the possible speed value arrays and possible is_tiny
            // values.  Field type 4 is the "values" field.  It's a variable type field,
            // so requires a two-step write (create the field, then write its value)
            for (std::size_t i = 0; i < 128; i++)
            {
                // Writing field type 4 == variant type
                protozero::pbf_writer values_writer(line_layer_writer,
                                                    util::vector_tile::VARIANT_TAG);
                // Attribute value 5 == uint64 type
                values_writer.add_uint64(util::vector_tile::VARIANT_TYPE_UINT64, i);
            }
            {
                protozero::pbf_writer values_writer(line_layer_writer,
                                                    util::vector_tile::VARIANT_TAG);
                // Attribute value 7 == bool type
                values_writer.add_bool(util::vector_tile::VARIANT_TYPE_BOOL, true);
            }
            {
                protozero::pbf_writer values_writer(line_layer_writer,
                                                    util::vector_tile::VARIANT_TAG);
                // Attribute value 7 == bool type
                values_writer.add_bool(util::vector_tile::VARIANT_TYPE_BOOL, false);
            }
            for (std::size_t i = 0; i <= max_datasource_id; i++)
            {
                // Writing field type 4 == variant type
                protozero::pbf_writer values_writer(line_layer_writer,
                                                    util::vector_tile::VARIANT_TAG);
                // Attribute value 1 == string type
                values_writer.add_string(util::vector_tile::VARIANT_TYPE_STRING,
                                         facade->GetDatasourceName(i).to_string());
            }
            for (auto value : used_line_ints)
            {
                // Writing field type 4 == variant type
                protozero::pbf_writer values_writer(line_layer_writer,
                                                    util::vector_tile::VARIANT_TAG);
                // Attribute value 2 == float type
                // Durations come out of OSRM in integer deciseconds, so we convert them
                // to seconds with a simple /10 for display
                values_writer.add_double(util::vector_tile::VARIANT_TYPE_DOUBLE, value / 10.);
            }

            for (const auto &name : names)
            {
                // Writing field type 4 == variant type
                protozero::pbf_writer values_writer(line_layer_writer,
                                                    util::vector_tile::VARIANT_TAG);
                // Attribute value 1 == string type
                values_writer.add_string(
                    util::vector_tile::VARIANT_TYPE_STRING, name.data(), name.size());
            }
        }

        // Only add the turn layer to the tile if it has some features (we sometimes won't
        // for tiles of z<16, and tiles that don't show any intersections)
        if (!all_turn_data.empty())
        {
            // Now write the points layer for turn penalty data:
            // Add a layer object to the PBF stream.  3=='layer' from the vector tile spec
            // (2.1)
            protozero::pbf_writer point_layer_writer(tile_writer, util::vector_tile::LAYER_TAG);
            point_layer_writer.add_uint32(util::vector_tile::VERSION_TAG, 2);    // version
            point_layer_writer.add_string(util::vector_tile::NAME_TAG, "turns"); // name
            point_layer_writer.add_uint32(util::vector_tile::EXTENT_TAG,
                                          util::vector_tile::EXTENT); // extent

            // Begin writing the set of point features
            {
                // Start each features with an ID starting at 1
                int id = 1;

                // Helper function to encode a new point feature on a vector tile.
                const auto encode_tile_point = [&point_layer_writer, &used_point_ints, &id](
                    const FixedPoint &tile_point, const TurnData &point_turn_data) {
                    protozero::pbf_writer feature_writer(point_layer_writer,
                                                         util::vector_tile::FEATURE_TAG);
                    // Field 3 is the "geometry type" field.  Value 1 is "point"
                    feature_writer.add_enum(
                        util::vector_tile::GEOMETRY_TAG,
                        util::vector_tile::GEOMETRY_TYPE_POINT);                // geometry type
                    feature_writer.add_uint64(util::vector_tile::ID_TAG, id++); // id
                    {
                        // Write out the 3 properties we want on the feature.  These
                        // refer to indexes in the properties lookup table, which we
                        // add to the tile after we add all features.
                        protozero::packed_field_uint32 field(
                            feature_writer, util::vector_tile::FEATURE_ATTRIBUTES_TAG);
                        field.add_element(0); // "bearing_in" tag key offset
                        field.add_element(point_turn_data.in_angle_offset);
                        field.add_element(1); // "turn_angle" tag key offset
                        field.add_element(point_turn_data.turn_angle_offset);
                        field.add_element(2); // "cost" tag key offset
                        field.add_element(used_point_ints.size() + point_turn_data.weight_offset);
                    }
                    {
                        // Add the geometry as the last field in this feature
                        protozero::packed_field_uint32 geometry(
                            feature_writer, util::vector_tile::FEATURE_GEOMETRIES_TAG);
                        encodePoint(tile_point, geometry);
                    }
                };

                // Loop over all the turns we found and add them as features to the layer
                for (const auto &turndata : all_turn_data)
                {
                    const auto tile_point = coordinatesToTilePoint(turndata.coordinate, tile_bbox);
                    if (!boost::geometry::within(point_t(tile_point.x, tile_point.y), clip_box))
                    {
                        continue;
                    }
                    encode_tile_point(tile_point, turndata);
                }
            }

            // Add the names of the three attributes we added to all the turn penalty
            // features previously.  The indexes used there refer to these keys.
            point_layer_writer.add_string(util::vector_tile::KEY_TAG, "bearing_in");
            point_layer_writer.add_string(util::vector_tile::KEY_TAG, "turn_angle");
            point_layer_writer.add_string(util::vector_tile::KEY_TAG, "cost");

            // Now, save the lists of integers and floats that our features refer to.
            for (const auto &value : used_point_ints)
            {
                protozero::pbf_writer values_writer(point_layer_writer,
                                                    util::vector_tile::VARIANT_TAG);
                values_writer.add_sint64(util::vector_tile::VARIANT_TYPE_SINT64, value);
            }
            for (const auto &value : used_point_floats)
            {
                protozero::pbf_writer values_writer(point_layer_writer,
                                                    util::vector_tile::VARIANT_TAG);
                values_writer.add_float(util::vector_tile::VARIANT_TYPE_FLOAT, value);
            }
        }
    }
    // protozero serializes data during object destructors, so once the scope closes,
    // our result buffer will have all the tile data encoded into it.

    return Status::Ok;
}
}
}
}
