#ifndef GEOSPATIAL_QUERY_HPP
#define GEOSPATIAL_QUERY_HPP

#include "coordinate_calculation.hpp"
#include "../typedefs.h"
#include "../data_structures/phantom_node.hpp"
#include "../util/bearing.hpp"

#include <osrm/coordinate.hpp>

#include <vector>
#include <memory>
#include <algorithm>

// Implements complex queries on top of an RTree and builds PhantomNodes from it.
//
// Only holds a weak reference on the RTree!
template <typename RTreeT> class GeospatialQuery
{
    using EdgeData = typename RTreeT::EdgeData;
    using CoordinateList = typename RTreeT::CoordinateList;

  public:
    GeospatialQuery(RTreeT &rtree_, std::shared_ptr<CoordinateList> coordinates_)
        : rtree(rtree_), coordinates(coordinates_)
    {
    }

    // Returns nearest PhantomNodes in the given bearing range within max_distance.
    // Does not filter by small/big component!
    std::vector<PhantomNodeWithDistance>
    NearestPhantomNodesInRange(const FixedPointCoordinate &input_coordinate,
                               const float max_distance,
                               const int bearing = 0,
                               const int bearing_range = 180)
    {
        auto results =
            rtree.Nearest(input_coordinate,
                          [this, bearing, bearing_range, max_distance](const EdgeData &data)
                          {
                              return checkSegmentBearing(data, bearing, bearing_range);
                          },
                          [max_distance](const std::size_t, const float min_dist)
                          {
                              return min_dist > max_distance;
                          });

        return MakePhantomNodes(input_coordinate, results);
    }

    // Returns max_results nearest PhantomNodes in the given bearing range.
    // Does not filter by small/big component!
    std::vector<PhantomNodeWithDistance>
    NearestPhantomNodes(const FixedPointCoordinate &input_coordinate,
                        const unsigned max_results,
                        const int bearing = 0,
                        const int bearing_range = 180)
    {
        auto results = rtree.Nearest(input_coordinate,
                                     [this, bearing, bearing_range](const EdgeData &data)
                                     {
                                         return checkSegmentBearing(data, bearing, bearing_range);
                                     },
                                     [max_results](const std::size_t num_results, const float)
                                     {
                                         return num_results >= max_results;
                                     });

        return MakePhantomNodes(input_coordinate, results);
    }

    // Returns the nearest phantom node. If this phantom node is not from a big component
    // a second phantom node is return that is the nearest coordinate in a big component.
    std::pair<PhantomNode, PhantomNode>
    NearestPhantomNodeWithAlternativeFromBigComponent(const FixedPointCoordinate &input_coordinate,
                                                      const int bearing = 0,
                                                      const int bearing_range = 180)
    {
        bool has_small_component = false;
        bool has_big_component = false;
        auto results = rtree.Nearest(
            input_coordinate,
            [this, bearing, bearing_range, &has_big_component,
             &has_small_component](const EdgeData &data)
            {
                auto use_segment =
                    (!has_small_component || (!has_big_component && !data.component.is_tiny));
                auto use_directions = std::make_pair(use_segment, use_segment);

                if (use_segment)
                {
                    use_directions = checkSegmentBearing(data, bearing, bearing_range);
                    if (use_directions.first || use_directions.second)
                    {
                        has_big_component = has_big_component || !data.component.is_tiny;
                        has_small_component = has_small_component || data.component.is_tiny;
                    }
                }

                return use_directions;
            },
            [&has_big_component](const std::size_t num_results, const float)
            {
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

  private:
    std::vector<PhantomNodeWithDistance>
    MakePhantomNodes(const FixedPointCoordinate &input_coordinate,
                     const std::vector<EdgeData> &results) const
    {
        std::vector<PhantomNodeWithDistance> distance_and_phantoms(results.size());
        std::transform(results.begin(), results.end(), distance_and_phantoms.begin(),
                       [this, &input_coordinate](const EdgeData &data)
                       {
                           return MakePhantomNode(input_coordinate, data);
                       });
        return distance_and_phantoms;
    }

    PhantomNodeWithDistance MakePhantomNode(const FixedPointCoordinate &input_coordinate,
                                                   const EdgeData &data) const
    {
        FixedPointCoordinate point_on_segment;
        float ratio;
        const auto current_perpendicular_distance = coordinate_calculation::perpendicular_distance(
            coordinates->at(data.u), coordinates->at(data.v), input_coordinate, point_on_segment,
            ratio);

        auto transformed =
            PhantomNodeWithDistance { PhantomNode{data, point_on_segment}, current_perpendicular_distance };

        ratio = std::min(1.f, std::max(0.f, ratio));

        if (SPECIAL_NODEID != transformed.phantom_node.forward_node_id)
        {
            transformed.phantom_node.forward_weight *= ratio;
        }
        if (SPECIAL_NODEID != transformed.phantom_node.reverse_node_id)
        {
            transformed.phantom_node.reverse_weight *= 1.f - ratio;
        }
        return transformed;
    }

    std::pair<bool, bool> checkSegmentBearing(const EdgeData &segment,
                                              const float filter_bearing,
                                              const float filter_bearing_range)
    {
        const float forward_edge_bearing =
            coordinate_calculation::bearing(coordinates->at(segment.u), coordinates->at(segment.v));

        const float backward_edge_bearing = (forward_edge_bearing + 180) > 360
                                                ? (forward_edge_bearing - 180)
                                                : (forward_edge_bearing + 180);

        const bool forward_bearing_valid =
            bearing::CheckInBounds(forward_edge_bearing, filter_bearing, filter_bearing_range) &&
            segment.forward_edge_based_node_id != SPECIAL_NODEID;
        const bool backward_bearing_valid =
            bearing::CheckInBounds(backward_edge_bearing, filter_bearing, filter_bearing_range) &&
            segment.reverse_edge_based_node_id != SPECIAL_NODEID;
        return std::make_pair(forward_bearing_valid, backward_bearing_valid);
    }

    RTreeT &rtree;
    const std::shared_ptr<CoordinateList> coordinates;
};

#endif
