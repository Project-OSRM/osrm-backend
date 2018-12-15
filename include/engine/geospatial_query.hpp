#ifndef GEOSPATIAL_QUERY_HPP
#define GEOSPATIAL_QUERY_HPP

#include "engine/approach.hpp"
#include "engine/phantom_node.hpp"
#include "util/bearing.hpp"
#include "util/coordinate_calculation.hpp"
#include "util/rectangle.hpp"
#include "util/typedefs.hpp"
#include "util/web_mercator.hpp"

#include "osrm/coordinate.hpp"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <memory>
#include <vector>

namespace osrm
{
namespace engine
{

inline std::pair<bool, bool> boolPairAnd(const std::pair<bool, bool> &A,
                                         const std::pair<bool, bool> &B)
{
    return std::make_pair(A.first && B.first, A.second && B.second);
}

// Implements complex queries on top of an RTree and builds PhantomNodes from it.
//
// Only holds a weak reference on the RTree and coordinates!
template <typename RTreeT, typename DataFacadeT> class GeospatialQuery
{
    using EdgeData = typename RTreeT::EdgeData;
    using CoordinateList = typename RTreeT::CoordinateList;
    using CandidateSegment = typename RTreeT::CandidateSegment;

  public:
    GeospatialQuery(RTreeT &rtree_, const CoordinateList &coordinates_, DataFacadeT &datafacade_)
        : rtree(rtree_), coordinates(coordinates_), datafacade(datafacade_)
    {
    }

    std::vector<EdgeData> Search(const util::RectangleInt2D &bbox)
    {
        return rtree.SearchInBox(bbox);
    }

    // Returns nearest PhantomNodes in the given bearing range within max_distance.
    // Does not filter by small/big component!
    std::vector<PhantomNodeWithDistance>
    NearestPhantomNodesInRange(const util::Coordinate input_coordinate,
                               const double max_distance,
                               const Approach approach,
                               const bool use_all_edges) const
    {
        auto results = rtree.Nearest(
            input_coordinate,
            [this, approach, &input_coordinate, use_all_edges](const CandidateSegment &segment) {
                return boolPairAnd(
                    boolPairAnd(HasValidEdge(segment, use_all_edges), CheckSegmentExclude(segment)),
                    CheckApproach(input_coordinate, segment, approach));
            },
            [this, max_distance, input_coordinate](const std::size_t,
                                                   const CandidateSegment &segment) {
                return CheckSegmentDistance(input_coordinate, segment, max_distance);
            });

        return MakePhantomNodes(input_coordinate, results);
    }

    // Returns nearest PhantomNodes in the given bearing range within max_distance.
    // Does not filter by small/big component!
    std::vector<PhantomNodeWithDistance>
    NearestPhantomNodesInRange(const util::Coordinate input_coordinate,
                               const double max_distance,
                               const int bearing,
                               const int bearing_range,
                               const Approach approach,
                               const bool use_all_edges) const
    {
        auto results = rtree.Nearest(
            input_coordinate,
            [this, approach, &input_coordinate, bearing, bearing_range, use_all_edges](
                const CandidateSegment &segment) {
                auto use_direction =
                    boolPairAnd(CheckSegmentBearing(segment, bearing, bearing_range),
                                boolPairAnd(HasValidEdge(segment, use_all_edges),
                                            CheckSegmentExclude(segment)));
                use_direction =
                    boolPairAnd(use_direction, CheckApproach(input_coordinate, segment, approach));
                return use_direction;
            },
            [this, max_distance, input_coordinate](const std::size_t,
                                                   const CandidateSegment &segment) {
                return CheckSegmentDistance(input_coordinate, segment, max_distance);
            });

        return MakePhantomNodes(input_coordinate, results);
    }

    // Returns max_results nearest PhantomNodes in the given bearing range.
    // Does not filter by small/big component!
    std::vector<PhantomNodeWithDistance>
    NearestPhantomNodes(const util::Coordinate input_coordinate,
                        const unsigned max_results,
                        const int bearing,
                        const int bearing_range,
                        const Approach approach) const
    {
        auto results = rtree.Nearest(
            input_coordinate,
            [this, approach, &input_coordinate, bearing, bearing_range](
                const CandidateSegment &segment) {
                auto use_direction =
                    boolPairAnd(CheckSegmentBearing(segment, bearing, bearing_range),
                                boolPairAnd(HasValidEdge(segment), CheckSegmentExclude(segment)));
                return boolPairAnd(use_direction,
                                   CheckApproach(input_coordinate, segment, approach));
            },
            [max_results](const std::size_t num_results, const CandidateSegment &) {
                return num_results >= max_results;
            });

        return MakePhantomNodes(input_coordinate, results);
    }

    // Returns max_results nearest PhantomNodes in the given bearing range within the maximum
    // distance.
    // Does not filter by small/big component!
    std::vector<PhantomNodeWithDistance>
    NearestPhantomNodes(const util::Coordinate input_coordinate,
                        const unsigned max_results,
                        const double max_distance,
                        const int bearing,
                        const int bearing_range,
                        const Approach approach) const
    {
        auto results = rtree.Nearest(
            input_coordinate,
            [this, approach, &input_coordinate, bearing, bearing_range](
                const CandidateSegment &segment) {
                auto use_direction =
                    boolPairAnd(CheckSegmentBearing(segment, bearing, bearing_range),
                                boolPairAnd(HasValidEdge(segment), CheckSegmentExclude(segment)));
                return boolPairAnd(use_direction,
                                   CheckApproach(input_coordinate, segment, approach));
            },
            [this, max_distance, max_results, input_coordinate](const std::size_t num_results,
                                                                const CandidateSegment &segment) {
                return num_results >= max_results ||
                       CheckSegmentDistance(input_coordinate, segment, max_distance);
            });

        return MakePhantomNodes(input_coordinate, results);
    }

    // Returns max_results nearest PhantomNodes.
    // Does not filter by small/big component!
    std::vector<PhantomNodeWithDistance>
    NearestPhantomNodes(const util::Coordinate input_coordinate,
                        const unsigned max_results,
                        const Approach approach) const
    {
        auto results = rtree.Nearest(
            input_coordinate,
            [this, approach, &input_coordinate](const CandidateSegment &segment) {
                return boolPairAnd(boolPairAnd(HasValidEdge(segment), CheckSegmentExclude(segment)),
                                   CheckApproach(input_coordinate, segment, approach));
            },
            [max_results](const std::size_t num_results, const CandidateSegment &) {
                return num_results >= max_results;
            });

        return MakePhantomNodes(input_coordinate, results);
    }

    // Returns max_results nearest PhantomNodes in the given max distance.
    // Does not filter by small/big component!
    std::vector<PhantomNodeWithDistance>
    NearestPhantomNodes(const util::Coordinate input_coordinate,
                        const unsigned max_results,
                        const double max_distance,
                        const Approach approach) const
    {
        auto results = rtree.Nearest(
            input_coordinate,
            [this, approach, &input_coordinate](const CandidateSegment &segment) {
                return boolPairAnd(boolPairAnd(HasValidEdge(segment), CheckSegmentExclude(segment)),
                                   CheckApproach(input_coordinate, segment, approach));
            },
            [this, max_distance, max_results, input_coordinate](const std::size_t num_results,
                                                                const CandidateSegment &segment) {
                return num_results >= max_results ||
                       CheckSegmentDistance(input_coordinate, segment, max_distance);
            });

        return MakePhantomNodes(input_coordinate, results);
    }

    // Returns the nearest phantom node. If this phantom node is not from a big component
    // a second phantom node is return that is the nearest coordinate in a big component.
    std::pair<PhantomNode, PhantomNode>
    NearestPhantomNodeWithAlternativeFromBigComponent(const util::Coordinate input_coordinate,
                                                      const double max_distance,
                                                      const Approach approach) const
    {
        bool has_small_component = false;
        bool has_big_component = false;
        auto results = rtree.Nearest(
            input_coordinate,
            [this, approach, &input_coordinate, &has_big_component, &has_small_component](
                const CandidateSegment &segment) {
                auto use_segment =
                    (!has_small_component || (!has_big_component && !IsTinyComponent(segment)));
                auto use_directions = std::make_pair(use_segment, use_segment);
                const auto valid_edges = HasValidEdge(segment);
                const auto admissible_segments = CheckSegmentExclude(segment);
                use_directions = boolPairAnd(use_directions, admissible_segments);
                use_directions = boolPairAnd(use_directions, valid_edges);
                use_directions =
                    boolPairAnd(use_directions, CheckApproach(input_coordinate, segment, approach));

                if (use_directions.first || use_directions.second)
                {
                    has_big_component = has_big_component || !IsTinyComponent(segment);
                    has_small_component = has_small_component || IsTinyComponent(segment);
                }

                return use_directions;
            },
            [this, &has_big_component, max_distance, input_coordinate](
                const std::size_t num_results, const CandidateSegment &segment) {
                return (num_results > 0 && has_big_component) ||
                       CheckSegmentDistance(input_coordinate, segment, max_distance);
            });

        if (results.size() == 0)
        {
            return std::make_pair(PhantomNode{}, PhantomNode{});
        }

        BOOST_ASSERT(results.size() == 1 || results.size() == 2);
        return std::make_pair(MakePhantomNode(input_coordinate, results.front()).phantom_node,
                              MakePhantomNode(input_coordinate, results.back()).phantom_node);
    }

    // Returns the nearest phantom node. If this phantom node is not from a big component
    // a second phantom node is return that is the nearest coordinate in a big component.
    std::pair<PhantomNode, PhantomNode>
    NearestPhantomNodeWithAlternativeFromBigComponent(const util::Coordinate input_coordinate,
                                                      const Approach approach) const
    {
        bool has_small_component = false;
        bool has_big_component = false;
        auto results = rtree.Nearest(
            input_coordinate,
            [this, approach, &input_coordinate, &has_big_component, &has_small_component](
                const CandidateSegment &segment) {
                auto use_segment =
                    (!has_small_component || (!has_big_component && !IsTinyComponent(segment)));
                auto use_directions = std::make_pair(use_segment, use_segment);

                const auto valid_edges = HasValidEdge(segment);
                const auto admissible_segments = CheckSegmentExclude(segment);
                use_directions = boolPairAnd(use_directions, admissible_segments);
                use_directions = boolPairAnd(use_directions, valid_edges);
                use_directions =
                    boolPairAnd(use_directions, CheckApproach(input_coordinate, segment, approach));

                if (use_directions.first || use_directions.second)
                {
                    has_big_component = has_big_component || !IsTinyComponent(segment);
                    has_small_component = has_small_component || IsTinyComponent(segment);
                }

                return use_directions;
            },
            [&has_big_component](const std::size_t num_results, const CandidateSegment &) {
                return num_results > 0 && has_big_component;
            });

        if (results.size() == 0)
        {
            return std::make_pair(PhantomNode{}, PhantomNode{});
        }

        BOOST_ASSERT(results.size() == 1 || results.size() == 2);
        return std::make_pair(MakePhantomNode(input_coordinate, results.front()).phantom_node,
                              MakePhantomNode(input_coordinate, results.back()).phantom_node);
    }

    // Returns the nearest phantom node. If this phantom node is not from a big component
    // a second phantom node is return that is the nearest coordinate in a big component.
    std::pair<PhantomNode, PhantomNode>
    NearestPhantomNodeWithAlternativeFromBigComponent(const util::Coordinate input_coordinate,
                                                      const int bearing,
                                                      const int bearing_range,
                                                      const Approach approach) const
    {
        bool has_small_component = false;
        bool has_big_component = false;
        auto results = rtree.Nearest(
            input_coordinate,
            [this,
             approach,
             &input_coordinate,
             bearing,
             bearing_range,
             &has_big_component,
             &has_small_component](const CandidateSegment &segment) {
                auto use_segment =
                    (!has_small_component || (!has_big_component && !IsTinyComponent(segment)));
                auto use_directions = std::make_pair(use_segment, use_segment);
                const auto admissible_segments = CheckSegmentExclude(segment);
                use_directions = boolPairAnd(use_directions, HasValidEdge(segment));

                if (use_segment)
                {
                    use_directions =
                        boolPairAnd(CheckSegmentBearing(segment, bearing, bearing_range),
                                    HasValidEdge(segment));
                    use_directions = boolPairAnd(use_directions, admissible_segments);
                    use_directions = boolPairAnd(
                        use_directions, CheckApproach(input_coordinate, segment, approach));

                    if (use_directions.first || use_directions.second)
                    {
                        has_big_component = has_big_component || !IsTinyComponent(segment);
                        has_small_component = has_small_component || IsTinyComponent(segment);
                    }
                }

                return use_directions;
            },
            [&has_big_component](const std::size_t num_results, const CandidateSegment &) {
                return num_results > 0 && has_big_component;
            });

        if (results.size() == 0)
        {
            return std::make_pair(PhantomNode{}, PhantomNode{});
        }

        BOOST_ASSERT(results.size() > 0);
        return std::make_pair(MakePhantomNode(input_coordinate, results.front()).phantom_node,
                              MakePhantomNode(input_coordinate, results.back()).phantom_node);
    }

    // Returns the nearest phantom node. If this phantom node is not from a big component
    // a second phantom node is return that is the nearest coordinate in a big component.
    std::pair<PhantomNode, PhantomNode>
    NearestPhantomNodeWithAlternativeFromBigComponent(const util::Coordinate input_coordinate,
                                                      const double max_distance,
                                                      const int bearing,
                                                      const int bearing_range,
                                                      const Approach approach) const
    {
        bool has_small_component = false;
        bool has_big_component = false;
        auto results = rtree.Nearest(
            input_coordinate,
            [this,
             approach,
             &input_coordinate,
             bearing,
             bearing_range,
             &has_big_component,
             &has_small_component](const CandidateSegment &segment) {
                auto use_segment =
                    (!has_small_component || (!has_big_component && !IsTinyComponent(segment)));
                auto use_directions = std::make_pair(use_segment, use_segment);
                const auto admissible_segments = CheckSegmentExclude(segment);
                use_directions = boolPairAnd(use_directions, HasValidEdge(segment));

                if (use_segment)
                {
                    use_directions =
                        boolPairAnd(CheckSegmentBearing(segment, bearing, bearing_range),
                                    HasValidEdge(segment));
                    use_directions = boolPairAnd(use_directions, admissible_segments);
                    use_directions = boolPairAnd(
                        use_directions, CheckApproach(input_coordinate, segment, approach));

                    if (use_directions.first || use_directions.second)
                    {
                        has_big_component = has_big_component || !IsTinyComponent(segment);
                        has_small_component = has_small_component || IsTinyComponent(segment);
                    }
                }

                return use_directions;
            },
            [this, &has_big_component, max_distance, input_coordinate](
                const std::size_t num_results, const CandidateSegment &segment) {
                return (num_results > 0 && has_big_component) ||
                       CheckSegmentDistance(input_coordinate, segment, max_distance);
            });

        if (results.size() == 0)
        {
            return std::make_pair(PhantomNode{}, PhantomNode{});
        }

        BOOST_ASSERT(results.size() > 0);
        return std::make_pair(MakePhantomNode(input_coordinate, results.front()).phantom_node,
                              MakePhantomNode(input_coordinate, results.back()).phantom_node);
    }

  private:
    std::vector<PhantomNodeWithDistance>
    MakePhantomNodes(const util::Coordinate input_coordinate,
                     const std::vector<EdgeData> &results) const
    {
        std::vector<PhantomNodeWithDistance> distance_and_phantoms(results.size());
        std::transform(results.begin(),
                       results.end(),
                       distance_and_phantoms.begin(),
                       [this, &input_coordinate](const EdgeData &data) {
                           return MakePhantomNode(input_coordinate, data);
                       });
        return distance_and_phantoms;
    }

    PhantomNodeWithDistance MakePhantomNode(const util::Coordinate input_coordinate,
                                            const EdgeData &data) const
    {
        util::Coordinate point_on_segment;
        double ratio;
        const auto current_perpendicular_distance =
            util::coordinate_calculation::perpendicularDistance(coordinates[data.u],
                                                                coordinates[data.v],
                                                                input_coordinate,
                                                                point_on_segment,
                                                                ratio);

        // Find the node-based-edge that this belongs to, and directly
        // calculate the forward_weight, forward_offset, reverse_weight, reverse_offset

        BOOST_ASSERT(data.forward_segment_id.enabled || data.reverse_segment_id.enabled);
        BOOST_ASSERT(!data.reverse_segment_id.enabled ||
                     datafacade.GetGeometryIndex(data.forward_segment_id.id).id ==
                         datafacade.GetGeometryIndex(data.reverse_segment_id.id).id);
        const auto geometry_id = datafacade.GetGeometryIndex(data.forward_segment_id.id).id;
        const auto component_id = datafacade.GetComponentID(data.forward_segment_id.id);

        const auto forward_weights = datafacade.GetUncompressedForwardWeights(geometry_id);
        const auto reverse_weights = datafacade.GetUncompressedReverseWeights(geometry_id);

        const auto forward_durations = datafacade.GetUncompressedForwardDurations(geometry_id);
        const auto reverse_durations = datafacade.GetUncompressedReverseDurations(geometry_id);

        const auto forward_geometry = datafacade.GetUncompressedForwardGeometry(geometry_id);

        const auto forward_weight_offset =
            std::accumulate(forward_weights.begin(),
                            forward_weights.begin() + data.fwd_segment_position,
                            EdgeWeight{0});

        const auto forward_duration_offset =
            std::accumulate(forward_durations.begin(),
                            forward_durations.begin() + data.fwd_segment_position,
                            EdgeDuration{0});

        EdgeDistance forward_distance_offset = 0;
        // Sum up the distance from the start to the fwd_segment_position
        for (auto current = forward_geometry.begin();
             current < forward_geometry.begin() + data.fwd_segment_position;
             ++current)
        {
            forward_distance_offset += util::coordinate_calculation::fccApproximateDistance(
                datafacade.GetCoordinateOfNode(*current),
                datafacade.GetCoordinateOfNode(*std::next(current)));
        }

        BOOST_ASSERT(data.fwd_segment_position <
                     std::distance(forward_durations.begin(), forward_durations.end()));

        EdgeWeight forward_weight = forward_weights[data.fwd_segment_position];
        EdgeDuration forward_duration = forward_durations[data.fwd_segment_position];
        EdgeDistance forward_distance = util::coordinate_calculation::fccApproximateDistance(
            datafacade.GetCoordinateOfNode(forward_geometry(data.fwd_segment_position)),
            point_on_segment);

        const auto reverse_weight_offset =
            std::accumulate(reverse_weights.begin(),
                            reverse_weights.end() - data.fwd_segment_position - 1,
                            EdgeWeight{0});

        const auto reverse_duration_offset =
            std::accumulate(reverse_durations.begin(),
                            reverse_durations.end() - data.fwd_segment_position - 1,
                            EdgeDuration{0});

        EdgeDistance reverse_distance_offset = 0;
        // Sum up the distance from just after the fwd_segment_position to the end
        for (auto current = forward_geometry.begin() + data.fwd_segment_position + 1;
             current != std::prev(forward_geometry.end());
             ++current)
        {
            reverse_distance_offset += util::coordinate_calculation::fccApproximateDistance(
                datafacade.GetCoordinateOfNode(*current),
                datafacade.GetCoordinateOfNode(*std::next(current)));
        }

        EdgeWeight reverse_weight =
            reverse_weights[reverse_weights.size() - data.fwd_segment_position - 1];
        EdgeDuration reverse_duration =
            reverse_durations[reverse_durations.size() - data.fwd_segment_position - 1];
        EdgeDistance reverse_distance = util::coordinate_calculation::fccApproximateDistance(
            point_on_segment,
            datafacade.GetCoordinateOfNode(forward_geometry(data.fwd_segment_position + 1)));

        ratio = std::min(1.0, std::max(0.0, ratio));
        if (data.forward_segment_id.id != SPECIAL_SEGMENTID)
        {
            forward_weight = static_cast<EdgeWeight>(forward_weight * ratio);
            forward_duration = static_cast<EdgeDuration>(forward_duration * ratio);
        }
        if (data.reverse_segment_id.id != SPECIAL_SEGMENTID)
        {
            reverse_weight -= static_cast<EdgeWeight>(reverse_weight * ratio);
            reverse_duration -= static_cast<EdgeDuration>(reverse_duration * ratio);
        }

        // check phantom node segments validity
        auto areSegmentsValid = [](auto first, auto last) -> bool {
            return std::find(first, last, INVALID_SEGMENT_WEIGHT) == last;
        };
        bool is_forward_valid_source =
            areSegmentsValid(forward_weights.begin(), forward_weights.end());
        bool is_forward_valid_target = areSegmentsValid(
            forward_weights.begin(), forward_weights.begin() + data.fwd_segment_position + 1);
        bool is_reverse_valid_source =
            areSegmentsValid(reverse_weights.begin(), reverse_weights.end());
        bool is_reverse_valid_target = areSegmentsValid(
            reverse_weights.begin(), reverse_weights.end() - data.fwd_segment_position);

        auto transformed = PhantomNodeWithDistance{
            PhantomNode{data,
                        component_id,
                        forward_weight,
                        reverse_weight,
                        forward_weight_offset,
                        reverse_weight_offset,
                        forward_distance,
                        reverse_distance,
                        forward_distance_offset,
                        reverse_distance_offset,
                        forward_duration,
                        reverse_duration,
                        forward_duration_offset,
                        reverse_duration_offset,
                        is_forward_valid_source,
                        is_forward_valid_target,
                        is_reverse_valid_source,
                        is_reverse_valid_target,
                        point_on_segment,
                        input_coordinate,
                        static_cast<unsigned short>(util::coordinate_calculation::bearing(
                            coordinates[data.u], coordinates[data.v]))},
            current_perpendicular_distance};

        return transformed;
    }

    bool CheckSegmentDistance(const Coordinate input_coordinate,
                              const CandidateSegment &segment,
                              const double max_distance) const
    {
        BOOST_ASSERT(segment.data.forward_segment_id.id != SPECIAL_SEGMENTID ||
                     !segment.data.forward_segment_id.enabled);
        BOOST_ASSERT(segment.data.reverse_segment_id.id != SPECIAL_SEGMENTID ||
                     !segment.data.reverse_segment_id.enabled);

        Coordinate wsg84_coordinate =
            util::web_mercator::toWGS84(segment.fixed_projected_coordinate);

        return util::coordinate_calculation::haversineDistance(input_coordinate, wsg84_coordinate) >
               max_distance;
    }

    std::pair<bool, bool> CheckSegmentExclude(const CandidateSegment &segment) const
    {
        std::pair<bool, bool> valid = {true, true};

        if (segment.data.forward_segment_id.enabled &&
            datafacade.ExcludeNode(segment.data.forward_segment_id.id))
        {
            valid.first = false;
        }

        if (segment.data.reverse_segment_id.enabled &&
            datafacade.ExcludeNode(segment.data.reverse_segment_id.id))
        {
            valid.second = false;
        }

        return valid;
    }

    std::pair<bool, bool> CheckSegmentBearing(const CandidateSegment &segment,
                                              const int filter_bearing,
                                              const int filter_bearing_range) const
    {
        BOOST_ASSERT(segment.data.forward_segment_id.id != SPECIAL_SEGMENTID ||
                     !segment.data.forward_segment_id.enabled);
        BOOST_ASSERT(segment.data.reverse_segment_id.id != SPECIAL_SEGMENTID ||
                     !segment.data.reverse_segment_id.enabled);

        const double forward_edge_bearing = util::coordinate_calculation::bearing(
            coordinates[segment.data.u], coordinates[segment.data.v]);

        const double backward_edge_bearing = (forward_edge_bearing + 180) > 360
                                                 ? (forward_edge_bearing - 180)
                                                 : (forward_edge_bearing + 180);

        const bool forward_bearing_valid =
            util::bearing::CheckInBounds(
                std::round(forward_edge_bearing), filter_bearing, filter_bearing_range) &&
            segment.data.forward_segment_id.enabled;
        const bool backward_bearing_valid =
            util::bearing::CheckInBounds(
                std::round(backward_edge_bearing), filter_bearing, filter_bearing_range) &&
            segment.data.reverse_segment_id.enabled;
        return std::make_pair(forward_bearing_valid, backward_bearing_valid);
    }

    /**
     * Checks to see if the edge weights are valid.  We might have an edge,
     * but a traffic update might set the speed to 0 (weight == INVALID_SEGMENT_WEIGHT).
     * which means that this edge is not currently traversible.  If this is the case,
     * then we shouldn't snap to this edge.
     */
    std::pair<bool, bool> HasValidEdge(const CandidateSegment &segment,
                                       const bool use_all_edges = false) const
    {

        bool forward_edge_valid = false;
        bool reverse_edge_valid = false;

        const auto &data = segment.data;
        BOOST_ASSERT(data.forward_segment_id.enabled);
        BOOST_ASSERT(data.forward_segment_id.id != SPECIAL_NODEID);
        const auto geometry_id = datafacade.GetGeometryIndex(data.forward_segment_id.id).id;

        const auto forward_weights = datafacade.GetUncompressedForwardWeights(geometry_id);
        if (forward_weights[data.fwd_segment_position] != INVALID_SEGMENT_WEIGHT)
        {
            forward_edge_valid = data.forward_segment_id.enabled;
        }

        const auto reverse_weights = datafacade.GetUncompressedReverseWeights(geometry_id);
        if (reverse_weights[reverse_weights.size() - data.fwd_segment_position - 1] !=
            INVALID_SEGMENT_WEIGHT)
        {
            reverse_edge_valid = data.reverse_segment_id.enabled;
        }

        forward_edge_valid = forward_edge_valid && (data.is_startpoint || use_all_edges);
        reverse_edge_valid = reverse_edge_valid && (data.is_startpoint || use_all_edges);

        return std::make_pair(forward_edge_valid, reverse_edge_valid);
    }

    bool IsTinyComponent(const CandidateSegment &segment) const
    {
        const auto &data = segment.data;
        BOOST_ASSERT(data.forward_segment_id.enabled);
        BOOST_ASSERT(data.forward_segment_id.id != SPECIAL_NODEID);
        return datafacade.GetComponentID(data.forward_segment_id.id).is_tiny;
    }

    std::pair<bool, bool> CheckApproach(const util::Coordinate &input_coordinate,
                                        const CandidateSegment &segment,
                                        const Approach approach) const
    {
        bool isOnewaySegment =
            !(segment.data.forward_segment_id.enabled && segment.data.reverse_segment_id.enabled);
        if (!isOnewaySegment && approach == Approach::CURB)
        {
            // Check the counter clockwise
            //
            //                  input_coordinate
            //                       |
            //                       |
            // segment.data.u ---------------- segment.data.v

            bool input_coordinate_is_at_right = !util::coordinate_calculation::isCCW(
                coordinates[segment.data.u], coordinates[segment.data.v], input_coordinate);

            if (datafacade.IsLeftHandDriving(segment.data.forward_segment_id.id))
                input_coordinate_is_at_right = !input_coordinate_is_at_right;

            return std::make_pair(input_coordinate_is_at_right, (!input_coordinate_is_at_right));
        }
        return std::make_pair(true, true);
    }

    const RTreeT &rtree;
    const CoordinateList &coordinates;
    DataFacadeT &datafacade;
};
}
}

#endif
