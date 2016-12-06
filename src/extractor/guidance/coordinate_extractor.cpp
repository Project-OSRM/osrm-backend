#include "extractor/guidance/coordinate_extractor.hpp"
#include "extractor/guidance/constants.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <tuple>
#include <utility>

#include "util/bearing.hpp"
#include "util/coordinate_calculation.hpp"

using osrm::util::angularDeviation;

namespace osrm
{
namespace extractor
{
namespace guidance
{

namespace
{
// to use the corrected coordinate, we require it to be at least a bit further down the
// road than the offset coordinate. We postulate a minimum Distance of 2 Meters
const constexpr double DESIRED_COORDINATE_DIFFERENCE = 2.0;
// the default distance we lookahead on a road. This distance prevents small mapping
// errors to impact the turn angles.
const constexpr double LOOKAHEAD_DISTANCE_WITHOUT_LANES = 10.0;
// The standard with of a interstate highway is 3.7 meters. Local roads have
// smaller widths, ranging from 2.5 to 3.25 meters. As a compromise, we use
// the 3.25 here for our angle calculations
const constexpr double FAR_LOOKAHEAD_DISTANCE = 40.0;

// The count of lanes assumed when no lanes are present. Since most roads will have lanes for both
// directions or a lane count specified, we use 2. Overestimating only makes our calculations safer,
// so we are fine for 1-lane ways. larger than 2 lanes should usually be specified in the data.
const constexpr std::uint16_t ASSUMED_LANE_COUNT = 2;

// When looking at lane offsets, motorway exits often are modelled not at a 90 degree angle but at
// some slight turn. To correctly detect these offsets, we need to allow for a bit more than just
// the lane width as an offset
double GetOffsetCorrectionFactor(const RoadClassification &road_classification)
{
    if (road_classification.IsMotorwayClass() || road_classification.IsRampClass())
        return 2.5;
    switch (road_classification.GetClass())
    {
    case RoadPriorityClass::TRUNK:
        return 2.0;
    case RoadPriorityClass::PRIMARY:
        return 1.5;
    default:
        return 1.0;
    };
}
}

CoordinateExtractor::CoordinateExtractor(
    const util::NodeBasedDynamicGraph &node_based_graph,
    const extractor::CompressedEdgeContainer &compressed_geometries,
    const std::vector<extractor::QueryNode> &node_coordinates)
    : node_based_graph(node_based_graph), compressed_geometries(compressed_geometries),
      node_coordinates(node_coordinates)
{
}

util::Coordinate
CoordinateExtractor::GetCoordinateAlongRoad(const NodeID intersection_node,
                                            const EdgeID turn_edge,
                                            const bool traversed_in_reverse,
                                            const NodeID to_node,
                                            const std::uint8_t intersection_lanes) const
{
    // we first extract all coordinates from the road
    auto coordinates =
        GetCoordinatesAlongRoad(intersection_node, turn_edge, traversed_in_reverse, to_node);

    return ExtractRepresentativeCoordinate(intersection_node,
                                           turn_edge,
                                           traversed_in_reverse,
                                           to_node,
                                           intersection_lanes,
                                           std::move(coordinates));
}

util::Coordinate CoordinateExtractor::ExtractRepresentativeCoordinate(
    const NodeID intersection_node,
    const EdgeID turn_edge,
    const bool traversed_in_reverse,
    const NodeID to_node,
    const std::uint8_t intersection_lanes,
    std::vector<util::Coordinate> coordinates) const
{
    const auto is_valid_result = [&](const util::Coordinate coordinate) {
        return util::Coordinate(traversed_in_reverse
                                    ? node_coordinates[to_node]
                                    : node_coordinates[intersection_node]) != coordinate;
    };
    // this is only used for debug purposes in assertions. We don't want warnings about it
    (void)is_valid_result;

    // the lane count might not always be set. We need to assume a positive number, though. Here we
    // select the number of lanes to operate on
    const auto considered_lanes =
        GetOffsetCorrectionFactor(node_based_graph.GetEdgeData(turn_edge).road_classification) *
        ((intersection_lanes == 0) ? ASSUMED_LANE_COUNT : intersection_lanes);

    // Fallback. These roads are small broken self-loops that shouldn't be in the data at all
    if (intersection_node == to_node)
        return coordinates[1];

    /* if we are looking at a straight line, we don't care where exactly the coordinate
     * is. Simply return the final coordinate. Turn angles/turn vectors are the same no matter which
     * coordinate we look at.
     */
    if (coordinates.size() <= 2)
    {
        // Here we can't check for validity, due to possible dead-ends with repeated coordinates
        // BOOST_ASSERT(is_valid_result(coordinates.back()));
        return coordinates.back();
    }

    // due to repeated coordinates / smaller offset errors we skip over the very first parts of the
    // coordinate set to add a small level of fault tolerance
    const constexpr double skipping_inaccuracies_distance = 2;

    // fallback, mostly necessary for dead ends
    if (intersection_node == to_node)
    {
        const auto result = ExtractCoordinateAtLength(skipping_inaccuracies_distance, coordinates);
        BOOST_ASSERT(is_valid_result(result));
        return result;
    }

    // If this reduction leaves us with only two coordinates, the turns/angles are represented in a
    // valid way. Only curved roads and other difficult scenarios will require multiple coordinates.
    if (coordinates.size() == 2)
        return coordinates.back();

    const auto &turn_edge_data = node_based_graph.GetEdgeData(turn_edge);

    // roundabouts, check early to avoid other costly checks
    if (turn_edge_data.roundabout || turn_edge_data.circular)
    {
        const auto result = ExtractCoordinateAtLength(skipping_inaccuracies_distance, coordinates);
        BOOST_ASSERT(is_valid_result(result));
        return result;
    }

    const util::Coordinate turn_coordinate =
        node_coordinates[traversed_in_reverse ? to_node : intersection_node];

    // Low priority roads are usually modelled very strangely. The roads are so small, though, that
    // our basic heuristic looking at the road should be fine.
    if (turn_edge_data.road_classification.IsLowPriorityRoadClass())
    {
        // Look ahead a tiny bit. Low priority road classes can be modelled fairly distinct in the
        // very first part of the road. It's less accurate than searching for offsets but the models
        // contained in OSM are just to strange to capture fully. Using the fallback here we try to
        // do the best of what we can.
        coordinates =
            TrimCoordinatesToLength(std::move(coordinates), LOOKAHEAD_DISTANCE_WITHOUT_LANES);
        if (coordinates.size() > 2 &&
            util::coordinate_calculation::haversineDistance(turn_coordinate, coordinates[1]) <
                ASSUMED_LANE_WIDTH)
        {
            const auto result =
                GetCorrectedCoordinate(turn_coordinate, coordinates[1], coordinates.back());
            BOOST_ASSERT(is_valid_result(result));
            return result;
        }
        else
        {
            BOOST_ASSERT(is_valid_result(coordinates.back()));
            return coordinates.back();
        }
    }

    const auto first_distance =
        util::coordinate_calculation::haversineDistance(coordinates[0], coordinates[1]);

    /* if the very first coordinate along the road is reasonably far away from the road, we assume
     * the coordinate to correctly represent the turn. This could probably be improved using
     * information on the very first turn angle (requires knowledge about previous road) and the
     * respective lane widths.
     */
    const bool first_coordinate_is_far_away = [&first_distance, considered_lanes]() {
        const auto required_distance =
            considered_lanes * 0.5 * ASSUMED_LANE_WIDTH + LOOKAHEAD_DISTANCE_WITHOUT_LANES;
        return first_distance > required_distance;
    }();

    if (first_coordinate_is_far_away)
    {
        BOOST_ASSERT(is_valid_result(coordinates[1]));
        return coordinates[1];
    }

    // now, after the simple checks have succeeded make our further computations simpler
    const auto lookahead_distance =
        FAR_LOOKAHEAD_DISTANCE + considered_lanes * ASSUMED_LANE_WIDTH * 0.5;

    /*
     * The coordinates along the road are in different distances from the source. If only very few
     * coordinates are close to the intersection, It might just be we simply looked to far down the
     * road. We can decide to weight coordinates differently based on their distance from the
     * intersection.
     * In addition, changes very close to an intersection indicate graphical representation of the
     * intersection over perceived turn angles.
     *
     * a -
     *    \
     *     -------------------- b
     *
     * Here the initial angle close to a might simply be due to OSM-Ways being located in the middle
     * of the actual roads. If a road splits in two, the ways for the separate direction can be
     * modeled very far apart with a steep angle at the split, even though the roads actually don't
     * take a turn. The distance between the coordinates can be an indicator for these small changes
     *
     * Luckily, these segment distances are a byproduct of trimming
     */
    auto segment_distances = PrepareLengthCache(coordinates, lookahead_distance);
    coordinates =
        TrimCoordinatesToLength(std::move(coordinates), lookahead_distance, segment_distances);
    segment_distances.back() = std::min(segment_distances.back(), lookahead_distance);
    BOOST_ASSERT(segment_distances.size() == coordinates.size());

    const auto total_distance =
        std::accumulate(segment_distances.begin(), segment_distances.end(), 0.);

    // if we are now left with two, well than we don't have to worry, or the segment is very small
    if (coordinates.size() == 2 || total_distance <= skipping_inaccuracies_distance)
    {
        BOOST_ASSERT(is_valid_result(coordinates.back()));
        return coordinates.back();
    }

    const double max_deviation_from_straight = GetMaxDeviation(
        coordinates.begin(), coordinates.end(), coordinates.front(), coordinates.back());

    // if the deviation from a straight line is small, we can savely use the coordinate. We use half
    // a lane as heuristic to determine if the road is straight enough.
    if (max_deviation_from_straight < 0.5 * ASSUMED_LANE_WIDTH)
    {
        // At loops in traffic circles, we can have small deviations as well (if the circle is tiny)
        // As a back-up, we have to check for this case
        if (coordinates.front() == coordinates.back())
        {
            const auto result =
                ExtractCoordinateAtLength(skipping_inaccuracies_distance, coordinates);
            BOOST_ASSERT(is_valid_result(result));
            return result;
        }
        else
        {
            BOOST_ASSERT(is_valid_result(coordinates.back()));
            return coordinates.back();
        }
    }

    /*
     * if a road turns barely in the beginning, it is similar to the first coordinate being
     * sufficiently far ahead.
     * possible negative:
     * http://www.openstreetmap.org/search?query=52.514503%2013.32252#map=19/52.51450/13.32252
     */
    const auto straight_distance_and_index = [&]() {
        auto straight_distance = segment_distances[1];

        std::size_t index;
        for (index = 2; index < coordinates.size(); ++index)
        {
            // check the deviation from a straight line
            if (GetMaxDeviation(coordinates.begin(),
                                coordinates.begin() + index,
                                coordinates.front(),
                                *(coordinates.begin() + index)) < 0.25 * ASSUMED_LANE_WIDTH)
                straight_distance += segment_distances[index];
            else
                break;
        }
        return std::make_pair(index - 1, straight_distance);
    }();
    const auto straight_distance = straight_distance_and_index.second;
    const auto straight_index = straight_distance_and_index.first;

    const bool starts_of_without_turn = [&]() {
        return straight_distance >=
               considered_lanes * 0.5 * ASSUMED_LANE_WIDTH + LOOKAHEAD_DISTANCE_WITHOUT_LANES;
    }();

    if (starts_of_without_turn)
    {
        // skip over repeated coordinates
        const auto result = ExtractCoordinateAtLength(5, coordinates, segment_distances);
        BOOST_ASSERT(is_valid_result(result));
        return result;
    }

    // compute the regression vector based on the sum of least squares
    const auto regression_line = RegressionLine(coordinates);

    /*
     * If we can find a line that represents the full set of coordinates within a certain range in
     * relation to ASSUMED_LANE_WIDTH, we use the regression line to express the turn angle.
     * This yields a transformation similar to:
     *
     *         c   d                       d
     *    b              ->             c
     *                               b
     * a                          a
     */
    const double max_deviation_from_regression = GetMaxDeviation(
        coordinates.begin(), coordinates.end(), regression_line.first, regression_line.second);

    if (max_deviation_from_regression < 0.35 * ASSUMED_LANE_WIDTH)
    {
        // We use the locations on the regression line to offset the regression line onto the
        // intersection.
        const auto coord_between_front =
            util::coordinate_calculation::projectPointOnSegment(
                regression_line.first, regression_line.second, coordinates.front())
                .second;
        const auto coord_between_back =
            util::coordinate_calculation::projectPointOnSegment(
                regression_line.first, regression_line.second, coordinates.back())
                .second;
        const auto result =
            GetCorrectedCoordinate(turn_coordinate, coord_between_front, coord_between_back);
        BOOST_ASSERT(is_valid_result(result));
        return result;
    }

    // We check offsets before curves to avoid detecting lane offsets due to large roads as curves
    // (think major highway here). If the road is wide, it can have quite a few coordinates in the
    // beginning.
    if (IsDirectOffset(coordinates,
                       straight_index,
                       straight_distance,
                       total_distance,
                       segment_distances,
                       considered_lanes))
    {
        // could be too agressive? Depend on lanes to check how far we want to go out?
        // compare
        // http://www.openstreetmap.org/search?query=52.411243%2013.363575#map=19/52.41124/13.36357
        const auto offset_index = std::max<decltype(straight_index)>(1, straight_index);
        const auto result = GetCorrectedCoordinate(
            turn_coordinate, coordinates[offset_index], coordinates[offset_index + 1]);

        BOOST_ASSERT(is_valid_result(result));
        return result;
    }

    if (IsCurve(coordinates,
                segment_distances,
                total_distance,
                considered_lanes * 0.5 * ASSUMED_LANE_WIDTH,
                turn_edge_data))
    {
        if (total_distance <= skipping_inaccuracies_distance)
            return coordinates.back();
        /*
         * In curves we now have to distinguish between larger curves and tiny curves modelling the
         * actual turn in the beginnig.
         *
         * We distinguish between turns that simply model the initial way of getting onto the
         * destination lanes and the ones that performa a larger turn.
         */
        coordinates = TrimCoordinatesToLength(
            std::move(coordinates), 2 * skipping_inaccuracies_distance, segment_distances);
        BOOST_ASSERT(coordinates.size() >= 2);
        segment_distances.resize(coordinates.size());
        segment_distances.back() = util::coordinate_calculation::haversineDistance(
            *(coordinates.end() - 2), coordinates.back());
        const auto vector_head = coordinates.back();
        coordinates = TrimCoordinatesToLength(
            std::move(coordinates), skipping_inaccuracies_distance, segment_distances);
        BOOST_ASSERT(coordinates.size() >= 2);
        const auto result =
            GetCorrectedCoordinate(turn_coordinate, coordinates.back(), vector_head);
        BOOST_ASSERT(is_valid_result(result));
        return result;
    }

    {
        // skip over the first coordinates, in specific the assumed lane count. We add a small
        // safety factor, to not overshoot on the regression
        const auto trimming_length = 0.8 * (considered_lanes * ASSUMED_LANE_WIDTH);
        const auto trimmed_coordinates =
            TrimCoordinatesByLengthFront(coordinates, 0.8 * trimming_length);
        if (trimmed_coordinates.size() >= 2 && (total_distance >= trimming_length + 2))
        {
            // get the regression line
            const auto regression_line_trimmed = RegressionLine(trimmed_coordinates);

            // and compute the maximum deviation from it
            const auto max_deviation_from_trimmed_regression =
                GetMaxDeviation(trimmed_coordinates.begin(),
                                trimmed_coordinates.end(),
                                regression_line_trimmed.first,
                                regression_line_trimmed.second);

            if (max_deviation_from_trimmed_regression < 0.5 * ASSUMED_LANE_WIDTH)
            {
                const auto result = GetCorrectedCoordinate(
                    turn_coordinate, regression_line_trimmed.first, regression_line_trimmed.second);
                BOOST_ASSERT(is_valid_result(result));
                return result;
            }
        }
    }

    // We use the locations on the regression line to offset the regression line onto the
    // intersection.
    const auto result =
        ExtractCoordinateAtLength(LOOKAHEAD_DISTANCE_WITHOUT_LANES, coordinates, segment_distances);
    BOOST_ASSERT(is_valid_result(result));
    return result;
}

util::Coordinate
CoordinateExtractor::ExtractCoordinateAtLength(const double distance,
                                               const std::vector<util::Coordinate> &coordinates,
                                               const std::vector<double> &length_cache) const
{
    BOOST_ASSERT(length_cache.size() == coordinates.size());
    BOOST_ASSERT(!coordinates.empty());

    double accumulated_distance = 0.;
    auto length_cache_itr = length_cache.begin() + 1;
    // find the end of the segment containing the coordinate which is at least distance away
    const auto find_coordinate_at_distance = [distance, &accumulated_distance, &length_cache_itr](
        const util::Coordinate /*coordinate*/) mutable {
        const auto result = (accumulated_distance + *length_cache_itr) >= distance;
        if (!result)
        {
            accumulated_distance += *length_cache_itr;
            ++length_cache_itr;
        }
        return result;
    };

    // find the beginning fo the segment (begin here and above for the length cache need to match
    // up!)
    const auto coordinate_after =
        std::find_if(coordinates.begin() + 1, coordinates.end(), find_coordinate_at_distance);

    if (coordinate_after == coordinates.end())
        return coordinates.back();

    const auto interpolation_factor =
        ComputeInterpolationFactor(distance - accumulated_distance, 0, *length_cache_itr);

    return util::coordinate_calculation::interpolateLinear(
        interpolation_factor, *std::prev(coordinate_after), *coordinate_after);
}

util::Coordinate CoordinateExtractor::ExtractCoordinateAtLength(
    const double distance, const std::vector<util::Coordinate> &coordinates) const
{
    BOOST_ASSERT(!coordinates.empty());
    double accumulated_distance = 0.;
    // checks (via its state) for an accumulated distance
    const auto coordinate_at_distance =
        [ distance, &accumulated_distance, last_coordinate = coordinates.front() ](
            const util::Coordinate coordinate) mutable
    {
        const double segment_distance =
            util::coordinate_calculation::haversineDistance(last_coordinate, coordinate);
        const auto result = (accumulated_distance + segment_distance) >= distance;
        if (!result)
        {
            accumulated_distance += segment_distance;
            last_coordinate = coordinate;
        }

        return result;
    };

    // find the begin of the segment containing the coordinate
    const auto coordinate_after =
        std::find_if(coordinates.begin() + 1, coordinates.end(), coordinate_at_distance);

    if (coordinate_after == coordinates.end())
        return coordinates.back();

    const auto interpolation_factor =
        ComputeInterpolationFactor(distance - accumulated_distance,
                                   0,
                                   util::coordinate_calculation::haversineDistance(
                                       *std::prev(coordinate_after), *coordinate_after));

    return util::coordinate_calculation::interpolateLinear(
        interpolation_factor, *std::prev(coordinate_after), *coordinate_after);
}

util::Coordinate CoordinateExtractor::GetCoordinateCloseToTurn(const NodeID from_node,
                                                               const EdgeID turn_edge,
                                                               const bool traversed_in_reverse,
                                                               const NodeID to_node) const
{
    const auto end_node = traversed_in_reverse ? from_node : to_node;
    const auto start_node = traversed_in_reverse ? to_node : from_node;
    if (!compressed_geometries.HasEntryForID(turn_edge))
        return node_coordinates[end_node];
    else
    {
        const auto &geometry = compressed_geometries.GetBucketReference(turn_edge);

        // the compressed edges contain node ids, we transfer them to coordinates accessing the
        // node_coordinates array
        const auto compressedGeometryToCoordinate =
            [this](const CompressedEdgeContainer::OnewayCompressedEdge &compressed_edge) {
                return node_coordinates[compressed_edge.node_id];
            };

        // return the first coordinate that is reasonably far away from the start node
        const util::Coordinate start_coordinate = node_coordinates[start_node];

        // OSM data has a tendency to include repeated nodes with identical coordinates. To skip
        // over these, we search for the first coordinate along the path that is at least a meter
        // away from the first entry
        const auto far_enough_away = [start_coordinate, compressedGeometryToCoordinate](
            const CompressedEdgeContainer::OnewayCompressedEdge &compressed_edge) {
            return util::coordinate_calculation::haversineDistance(
                       compressedGeometryToCoordinate(compressed_edge), start_coordinate) > 1;
        };

        // find the first coordinate, that is at least unequal to the begin of the edge
        if (traversed_in_reverse)
        {
            const auto far_enough =
                std::find_if(geometry.rbegin(), geometry.rend(), far_enough_away);
            return (far_enough != geometry.rend()) ? compressedGeometryToCoordinate(*far_enough)
                                                   : node_coordinates[end_node];
        }
        else
        {
            const auto far_enough = std::find_if(geometry.begin(), geometry.end(), far_enough_away);
            return (far_enough != geometry.end()) ? compressedGeometryToCoordinate(*far_enough)
                                                  : node_coordinates[end_node];
        }
    }
}

std::vector<util::Coordinate>
CoordinateExtractor::GetForwardCoordinatesAlongRoad(const NodeID from, const EdgeID turn_edge) const
{
    return GetCoordinatesAlongRoad(from, turn_edge, false, node_based_graph.GetTarget(turn_edge));
}

std::vector<util::Coordinate>
CoordinateExtractor::GetCoordinatesAlongRoad(const NodeID intersection_node,
                                             const EdgeID turn_edge,
                                             const bool traversed_in_reverse,
                                             const NodeID to_node) const
{
    if (!compressed_geometries.HasEntryForID(turn_edge))
    {
        if (traversed_in_reverse)
            return {{node_coordinates[to_node]}, {node_coordinates[intersection_node]}};
        else
            return {{node_coordinates[intersection_node]}, {node_coordinates[to_node]}};
    }
    else
    {
        // extracts the geometry in coordinates from the compressed edge container
        std::vector<util::Coordinate> result;
        const auto &geometry = compressed_geometries.GetBucketReference(turn_edge);
        result.reserve(geometry.size() + 2);

        // the compressed edges contain node ids, we transfer them to coordinates accessing the
        // node_coordinates array
        const auto compressedGeometryToCoordinate =
            [this](const CompressedEdgeContainer::OnewayCompressedEdge &compressed_edge)
            -> util::Coordinate { return node_coordinates[compressed_edge.node_id]; };

        // add the coordinates to the result in either normal or reversed order, based on
        // traversed_in_reverse
        if (traversed_in_reverse)
        {
            std::transform(geometry.rbegin(),
                           geometry.rend(),
                           std::back_inserter(result),
                           compressedGeometryToCoordinate);
            BOOST_ASSERT(intersection_node < node_coordinates.size());
            result.push_back(node_coordinates[intersection_node]);
        }
        else
        {
            BOOST_ASSERT(intersection_node < node_coordinates.size());
            result.push_back(node_coordinates[intersection_node]);
            std::transform(geometry.begin(),
                           geometry.end(),
                           std::back_inserter(result),
                           compressedGeometryToCoordinate);
        }
        // filter duplicated coordinates
        auto end = std::unique(result.begin(), result.end());
        result.erase(end, result.end());
        return result;
    }
}

double
CoordinateExtractor::GetMaxDeviation(std::vector<util::Coordinate>::const_iterator range_begin,
                                     const std::vector<util::Coordinate>::const_iterator &range_end,
                                     const util::Coordinate straight_begin,
                                     const util::Coordinate straight_end) const
{
    // compute the deviation of a single coordinate from a straight line
    auto get_single_deviation = [&](const util::Coordinate coordinate) {
        // find the projected coordinate
        auto coord_between = util::coordinate_calculation::projectPointOnSegment(
                                 straight_begin, straight_end, coordinate)
                                 .second;
        // and calculate the distance between the intermediate coordinate and the coordinate
        // on the osrm-way
        return util::coordinate_calculation::haversineDistance(coord_between, coordinate);
    };

    // note: we don't accumulate here but rather compute the maximum. The functor passed here is not
    // summing up anything.
    return std::accumulate(
        range_begin, range_end, 0.0, [&](const double current, const util::Coordinate coordinate) {
            return std::max(current, get_single_deviation(coordinate));
        });
}

bool CoordinateExtractor::IsCurve(const std::vector<util::Coordinate> &coordinates,
                                  const std::vector<double> &segment_distances,
                                  const double segment_length,
                                  const double considered_lane_width,
                                  const util::NodeBasedEdgeData &edge_data) const
{
    BOOST_ASSERT(coordinates.size() > 2);

    // by default, we treat roundabout as curves
    if (edge_data.roundabout)
        return true;

    // TODO we might have to fix this to better compensate for errors due to repeated coordinates
    const bool takes_an_actual_turn = [&coordinates]() {
        const auto begin_bearing =
            util::coordinate_calculation::bearing(coordinates[0], coordinates[1]);
        const auto end_bearing = util::coordinate_calculation::bearing(
            coordinates[coordinates.size() - 2], coordinates[coordinates.size() - 1]);

        const auto total_angle = util::angularDeviation(begin_bearing, end_bearing);
        return total_angle > 0.5 * NARROW_TURN_ANGLE;
    }();

    if (!takes_an_actual_turn)
        return false;

    const auto get_deviation = [](const util::Coordinate line_start,
                                  const util::Coordinate line_end,
                                  const util::Coordinate point) {
        // find the projected coordinate
        auto coord_between =
            util::coordinate_calculation::projectPointOnSegment(line_start, line_end, point).second;
        // and calculate the distance between the intermediate coordinate and the coordinate
        return util::coordinate_calculation::haversineDistance(coord_between, point);
    };

    // a curve needs to be on one side of the coordinate array
    const bool all_same_side = [&]() {
        if (coordinates.size() <= 3)
            return true;

        const bool ccw = util::coordinate_calculation::isCCW(
            coordinates.front(), coordinates.back(), coordinates[1]);

        return std::all_of(
            coordinates.begin() + 2, coordinates.end() - 1, [&](const util::Coordinate coordinate) {
                const bool compare_ccw = util::coordinate_calculation::isCCW(
                    coordinates.front(), coordinates.back(), coordinate);
                return ccw == compare_ccw;
            });
    }();

    if (!all_same_side)
        return false;

    // check if the deviation is a sequence that increases up to a maximum deviation and decreses
    // after, following what we would expect from a modelled curve
    bool has_up_down_deviation = false;
    std::size_t maximum_deviation_index = 0;
    double maximum_deviation = 0;

    std::tie(has_up_down_deviation, maximum_deviation_index, maximum_deviation) =
        [&coordinates, get_deviation]() -> std::tuple<bool, std::size_t, double> {
        const auto increasing = [&](const util::Coordinate lhs, const util::Coordinate rhs) {
            return get_deviation(coordinates.front(), coordinates.back(), lhs) <
                   get_deviation(coordinates.front(), coordinates.back(), rhs);
        };

        const auto decreasing = [&](const util::Coordinate lhs, const util::Coordinate rhs) {
            return get_deviation(coordinates.front(), coordinates.back(), lhs) >
                   get_deviation(coordinates.front(), coordinates.back(), rhs);
        };

        if (coordinates.size() < 3)
            return std::make_tuple(true, 0, 0.);

        if (coordinates.size() == 3)
            return std::make_tuple(
                true, 1, get_deviation(coordinates.front(), coordinates.back(), coordinates[1]));

        const auto one_past_maximum_iter =
            std::is_sorted_until(coordinates.begin() + 1, coordinates.end(), increasing);

        if (one_past_maximum_iter == coordinates.end())
            return std::make_tuple(true, coordinates.size() - 1, 0.);
        else if (std::is_sorted(one_past_maximum_iter, coordinates.end(), decreasing))
            return std::make_tuple(true,
                                   std::distance(coordinates.begin(), one_past_maximum_iter) - 1,
                                   get_deviation(coordinates.front(),
                                                 coordinates.back(),
                                                 *(one_past_maximum_iter - 1)));
        else
            return std::make_tuple(false, 0, 0.);
    }();

    // a curve has increasing deviation from its front/back vertices to a certain point and after it
    // only decreases
    if (!has_up_down_deviation)
        return false;

    // if the maximum deviation is at a quarter of the total curve, we are probably looking at a
    // normal turn
    const auto distance_to_max_deviation = std::accumulate(
        segment_distances.begin(), segment_distances.begin() + maximum_deviation_index + 1, 0.);

    if ((distance_to_max_deviation <= 0.35 * segment_length ||
         maximum_deviation < std::max(0.3 * considered_lane_width, 0.5 * ASSUMED_LANE_WIDTH)) &&
        segment_length > LOOKAHEAD_DISTANCE_WITHOUT_LANES)
        return false;

    BOOST_ASSERT(coordinates.size() >= 3);
    // Compute all turn angles along the road
    const auto turn_angles = [coordinates]() {
        std::vector<double> turn_angles;
        turn_angles.reserve(coordinates.size() - 2);
        for (std::size_t index = 0; index + 2 < coordinates.size(); ++index)
        {
            turn_angles.push_back(util::coordinate_calculation::computeAngle(
                coordinates[index], coordinates[index + 1], coordinates[index + 2]));
        }
        return turn_angles;
    }();

    const bool curve_is_valid = [&turn_angles,
                                 &segment_distances,
                                 &segment_length,
                                 &considered_lane_width]() {
        // internal state for our lamdae
        bool last_was_straight = false;
        // a turn angle represents two segments between three coordinates. We initialize the
        // distance with the very first segment length (in-segment) of the first turn-angle
        double straight_distance = std::max(0., segment_distances[1] - considered_lane_width);
        auto distance_itr = segment_distances.begin() + 1;

        // every call to the lamda requires a call to the distances. They need to be aligned
        BOOST_ASSERT(segment_distances.size() == turn_angles.size() + 2);

        const auto detect_invalid_curve = [&](const double previous_angle,
                                              const double current_angle) {
            const auto both_actually_turn =
                (util::angularDeviation(previous_angle, STRAIGHT_ANGLE) > FUZZY_ANGLE_DIFFERENCE) &&
                (util::angularDeviation(current_angle, STRAIGHT_ANGLE) > FUZZY_ANGLE_DIFFERENCE);
            // they cannot be straight, since they differ at least by FUZZY_ANGLE_DIFFERENCE
            const auto turn_direction_switches =
                (previous_angle > STRAIGHT_ANGLE) == (current_angle < STRAIGHT_ANGLE);

            // a turn that switches direction mid-curve is not a valid curve
            if (both_actually_turn && turn_direction_switches)
                return true;

            const bool is_straight = util::angularDeviation(current_angle, STRAIGHT_ANGLE) < 5;
            ++distance_itr;
            if (is_straight)
            {
                // since the angle is straight, we augment it by the second part of the segment
                straight_distance += *distance_itr;
                if (last_was_straight && straight_distance > 0.3 * segment_length)
                {
                    return true;
                }
            } // if a segment on its own is long enough, thats fair game as well
            else if (straight_distance > 0.3 * segment_length)
                return true;
            else
            {
                // we reset the last distance, starting with the next in-segment again
                straight_distance = *distance_itr;
            }
            last_was_straight = is_straight;
            return false;
        };

        const auto end_of_straight_segment =
            std::adjacent_find(turn_angles.begin(), turn_angles.end(), detect_invalid_curve);

        // No curve should have a very long straight segment
        return end_of_straight_segment == turn_angles.end();
    }();

    return (segment_length > 2 * considered_lane_width && curve_is_valid);
}

bool CoordinateExtractor::IsDirectOffset(const std::vector<util::Coordinate> &coordinates,
                                         const std::size_t straight_index,
                                         const double straight_distance,
                                         const double segment_length,
                                         const std::vector<double> &segment_distances,
                                         const std::uint8_t considered_lanes) const
{
    // check if a given length is with half a lane of the assumed lane offset
    const auto IsCloseToLaneDistance = [considered_lanes](const double width) {
        // a road usually is connected to the middle of the lanes. So the lane-offset has to
        // consider half to road
        const auto lane_offset = 0.5 * considered_lanes * ASSUMED_LANE_WIDTH;
        return width - lane_offset < ASSUMED_LANE_WIDTH; // less or going over at most a small bit
    };

    // Check whether the very first coordinate is simply an offset. This is the case if the initial
    // vertex is close to the turn and the remaining coordinates are nearly straight.
    const auto offset_index = std::max<decltype(straight_index)>(1, straight_index);

    // we need at least a single coordinate
    if (offset_index + 1 >= coordinates.size())
        return false;

    BOOST_ASSERT(segment_distances.size() == coordinates.size());
    // the straight part has to be around the lane distance
    if (!IsCloseToLaneDistance(std::accumulate(
            segment_distances.begin(), segment_distances.begin() + offset_index + 1, 0.)))
        return false;

    // the segment itself cannot be short
    if (segment_length < 0.4 * FAR_LOOKAHEAD_DISTANCE)
        return false;

    // if the remaining segment is short, we don't consider it an offset
    if ((segment_length - std::max(straight_distance, segment_distances[1])) < 0.1 * segment_length)
        return false;

    // when we compare too long a distance, we run into problems due to turning roads. Here we
    // compute which index to consider when checking if the remaining road remains straight
    const auto segment_offset_past_thirty_meters =
        std::find_if(segment_distances.begin() + offset_index,
                     segment_distances.end(),
                     [accumulated_distance = 0.](const auto value) mutable {
                         accumulated_distance += value;
                         return value >= 30;
                     });

    // transform the found offset in the segment distances into the appropriate part in the
    // coordinates array
    const auto deviation_compare_end =
        coordinates.begin() +
        std::min<std::size_t>(
            coordinates.size(), // don't go over
            std::distance(segment_distances.begin(), segment_offset_past_thirty_meters) + 1);

    // finally, we cannot be far off from a straight line for the remaining coordinates
    return 0.5 * ASSUMED_LANE_WIDTH > GetMaxDeviation(coordinates.begin() + offset_index,
                                                      deviation_compare_end,
                                                      coordinates[offset_index],
                                                      *(deviation_compare_end - 1));
}

std::vector<double>
CoordinateExtractor::PrepareLengthCache(const std::vector<util::Coordinate> &coordinates,
                                        const double limit) const
{
    BOOST_ASSERT(!coordinates.empty());
    BOOST_ASSERT(limit >= 0);
    std::vector<double> segment_distances;
    segment_distances.reserve(coordinates.size());
    segment_distances.push_back(0);
    // sentinel
    std::find_if(std::next(std::begin(coordinates)), std::end(coordinates), [
        last_coordinate = coordinates.front(),
        limit,
        &segment_distances,
        accumulated_distance = 0.
    ](const util::Coordinate current_coordinate) mutable {
        const auto distance =
            util::coordinate_calculation::haversineDistance(last_coordinate, current_coordinate);
        accumulated_distance += distance;
        last_coordinate = current_coordinate;
        segment_distances.push_back(distance);
        return accumulated_distance >= limit;
    });
    return segment_distances;
}

std::vector<util::Coordinate>
CoordinateExtractor::TrimCoordinatesToLength(std::vector<util::Coordinate> coordinates,
                                             const double desired_length,
                                             const std::vector<double> &length_cache) const
{
    BOOST_ASSERT(coordinates.size() >= 2);
    BOOST_ASSERT(desired_length >= 0);

    double distance_to_current_coordinate = 0;
    std::size_t coordinate_index = 0;

    const auto compute_length =
        [&coordinate_index, &distance_to_current_coordinate, &coordinates]() {
            const auto new_distance =
                distance_to_current_coordinate +
                util::coordinate_calculation::haversineDistance(coordinates[coordinate_index - 1],
                                                                coordinates[coordinate_index]);
            return new_distance;
        };

    const auto read_length_from_cache = [&length_cache, &coordinate_index]() {
        return length_cache[coordinate_index];
    };

    bool use_cache = !length_cache.empty();

    if (use_cache && length_cache.back() < desired_length && coordinates.size() >= 2)
    {
        if (coordinates.size() > length_cache.size())
            coordinates.erase(coordinates.begin() + length_cache.size(), coordinates.end());

        const auto distance_between_last_coordinates =
            util::coordinate_calculation::haversineDistance(*(coordinates.end() - 2),
                                                            *(coordinates.end() - 1));

        if (distance_between_last_coordinates > 0)
        {
            const auto interpolation_factor = ComputeInterpolationFactor(
                length_cache.back(), 0, distance_between_last_coordinates);

            coordinates.back() = util::coordinate_calculation::interpolateLinear(
                interpolation_factor, *(coordinates.end() - 2), coordinates.back());
        }
        return coordinates;
    }
    else
    {
        BOOST_ASSERT(!use_cache || length_cache.back() >= desired_length);
        for (coordinate_index = 1; coordinate_index < coordinates.size(); ++coordinate_index)
        {
            // get the length to the next candidate, given that we can or cannot have a length cache
            const auto distance_to_next_coordinate =
                use_cache ? read_length_from_cache() : compute_length();

            // if we reached the number of coordinates, we can stop here
            if (distance_to_next_coordinate >= desired_length)
            {
                coordinates.resize(coordinate_index + 1);
                coordinates.back() = util::coordinate_calculation::interpolateLinear(
                    ComputeInterpolationFactor(desired_length,
                                               distance_to_current_coordinate,
                                               distance_to_next_coordinate),
                    coordinates[coordinate_index - 1],
                    coordinates[coordinate_index]);
                break;
            }

            // remember the accumulated distance
            distance_to_current_coordinate = distance_to_next_coordinate;
        }
        BOOST_ASSERT(!coordinates.empty());
        return coordinates;
    }
}

util::Coordinate
CoordinateExtractor::GetCorrectedCoordinate(const util::Coordinate fixpoint,
                                            const util::Coordinate vector_base,
                                            const util::Coordinate vector_head) const
{
    // if the coordinates are close together, we were not able to look far ahead, so
    // we can use the end-coordinate
    if (util::coordinate_calculation::haversineDistance(vector_base, vector_head) <
        DESIRED_COORDINATE_DIFFERENCE)
        return vector_head;
    else
    {
        /* to correct for the initial offset, we move the lookahead coordinate close
         * to the original road. We do so by subtracting the difference between the
         * turn coordinate and the offset coordinate from the lookahead coordinge:
         *
         * a ------ b ------ c
         *          |
         *          d
         *             \
         *                \
         *                   e
         *
         * is converted to:
         *
         * a ------ b ------ c
         *             \
         *                \
         *                   e
         *
         * for turn node `b`, vector_base `d` and vector_head `e`
         */
        const auto offset_percentage = 90;
        const auto corrected_lon =
            vector_head.lon -
            util::FixedLongitude{offset_percentage *
                                 static_cast<int>(vector_base.lon - fixpoint.lon) / 100};
        const auto corrected_lat =
            vector_head.lat -
            util::FixedLatitude{offset_percentage *
                                static_cast<int>(vector_base.lat - fixpoint.lat) / 100};

        return util::Coordinate(corrected_lon, corrected_lat);
    }
}

std::vector<util::Coordinate>
CoordinateExtractor::SampleCoordinates(const std::vector<util::Coordinate> &coordinates,
                                       const double max_sample_length,
                                       const double rate) const
{
    BOOST_ASSERT(rate > 0 && coordinates.size() >= 2);

    // the return value
    std::vector<util::Coordinate> sampled_coordinates;
    sampled_coordinates.reserve(ceil(max_sample_length / rate) + 2);

    // the very first coordinate is always part of the sample
    sampled_coordinates.push_back(coordinates.front());

    double carry_length = 0., total_length = 0.;
    // interpolate coordinates as long as we are not past the desired length
    const auto add_samples_until_length_limit = [&](const util::Coordinate previous_coordinate,
                                                    const util::Coordinate current_coordinate) {
        // pretend to have found an element and stop the sampling
        if (total_length > max_sample_length)
            return true;

        const auto distance_between = util::coordinate_calculation::haversineDistance(
            previous_coordinate, current_coordinate);

        if (carry_length + distance_between >= rate)
        {
            // within the current segment, there is at least a single coordinate that we want to
            // sample. We extract all coordinates that are on our sampling intervals and update our
            // local sampling item to reflect the travelled distance
            const auto base_sampling = rate - carry_length;

            // the number of samples in the interval is equal to the length of the interval (+ the
            // already traversed part from the previous segment) divided by the sampling rate
            BOOST_ASSERT(max_sample_length > total_length);
            const std::size_t num_samples = std::floor(
                (std::min(max_sample_length - total_length, distance_between) + carry_length) /
                rate);

            for (std::size_t sample_value = 0; sample_value < num_samples; ++sample_value)
            {
                const auto interpolation_factor = ComputeInterpolationFactor(
                    base_sampling + sample_value * rate, 0, distance_between);
                auto sampled_coordinate = util::coordinate_calculation::interpolateLinear(
                    interpolation_factor, previous_coordinate, current_coordinate);
                sampled_coordinates.emplace_back(sampled_coordinate);
            }

            // current length needs to reflect how much is missing to the next sample. Here we can
            // ignore max sample range, because if we reached it, the loop is done anyhow
            carry_length = (distance_between + carry_length) - (num_samples * rate);
        }
        else
        {
            // do the necessary bookkeeping and continue
            carry_length += distance_between;
        }
        // the total length travelled is always updated by the full distance
        total_length += distance_between;

        return false;
    };

    // misuse of adjacent_find. Loop over coordinates, until a total sample length is reached
    std::adjacent_find(coordinates.begin(), coordinates.end(), add_samples_until_length_limit);

    return sampled_coordinates;
}

double CoordinateExtractor::ComputeInterpolationFactor(const double desired_distance,
                                                       const double distance_to_first,
                                                       const double distance_to_second) const
{
    BOOST_ASSERT(distance_to_first < desired_distance);
    double segment_length = distance_to_second - distance_to_first;
    BOOST_ASSERT(segment_length > 0);
    BOOST_ASSERT(distance_to_second >= desired_distance);
    double missing_distance = desired_distance - distance_to_first;
    return std::max(0., std::min(missing_distance / segment_length, 1.0));
}

std::vector<util::Coordinate>
CoordinateExtractor::TrimCoordinatesByLengthFront(std::vector<util::Coordinate> coordinates,
                                                  const double desired_length) const
{
    BOOST_ASSERT(desired_length >= 0);
    double distance_to_index = 0;
    std::size_t index = 0;
    for (std::size_t next_index = 1; next_index < coordinates.size(); ++next_index)
    {
        const double next_distance =
            distance_to_index + util::coordinate_calculation::haversineDistance(
                                    coordinates[index], coordinates[next_index]);
        if (next_distance >= desired_length)
        {
            const auto factor =
                ComputeInterpolationFactor(desired_length, distance_to_index, next_distance);
            auto interpolated_coordinate = util::coordinate_calculation::interpolateLinear(
                factor, coordinates[index], coordinates[next_index]);
            if (index > 0)
                coordinates.erase(coordinates.begin(), coordinates.begin() + index);
            coordinates.front() = interpolated_coordinate;
            return coordinates;
        }

        distance_to_index = next_distance;
        index = next_index;
    }

    // the coordinates in total are too short in length for the desired length
    // this part is only reached when we don't return from within the above loop
    coordinates.clear();
    return coordinates;
}

std::pair<util::Coordinate, util::Coordinate>
CoordinateExtractor::RegressionLine(const std::vector<util::Coordinate> &coordinates) const
{
    // create a sample of all coordinates to improve the quality of our regression vector
    // (less dependent on modelling of the data in OSM)
    const auto sampled_coordinates = SampleCoordinates(coordinates, FAR_LOOKAHEAD_DISTANCE, 1);

    BOOST_ASSERT(!coordinates.empty());
    if (sampled_coordinates.size() < 2) // less than 1 meter in length
        return {coordinates.front(), coordinates.back()};

    // compute the regression vector based on the sum of least squares
    const auto regression_line = util::coordinate_calculation::leastSquareRegression(
        sampled_coordinates.begin(), sampled_coordinates.end());
    const auto coord_between_front =
        util::coordinate_calculation::projectPointOnSegment(
            regression_line.first, regression_line.second, coordinates.front())
            .second;
    const auto coord_between_back =
        util::coordinate_calculation::projectPointOnSegment(
            regression_line.first, regression_line.second, coordinates.back())
            .second;

    return {coord_between_front, coord_between_back};
}

} // namespace guidance
} // namespace extractor
} // namespace osrm
