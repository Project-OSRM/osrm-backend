#ifndef GEOSPATIAL_QUERY_HPP
#define GEOSPATIAL_QUERY_HPP

#include "util/coordinate_calculation.hpp"
#include "util/typedefs.hpp"
#include "engine/phantom_node.hpp"
#include "util/bearing.hpp"
#include "util/rectangle.hpp"

#include "osrm/coordinate.hpp"

#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>

namespace osrm
{
namespace engine
{

// Implements complex queries on top of an RTree and builds PhantomNodes from it.
//
// Only holds a weak reference on the RTree!
template <typename RTreeT, typename DataFacadeT> class GeospatialQuery
{
    using EdgeData = typename RTreeT::EdgeData;
    using CoordinateList = typename RTreeT::CoordinateList;

  public:
    GeospatialQuery(RTreeT &rtree_,
                    std::shared_ptr<CoordinateList> coordinates_,
                    DataFacadeT &datafacade_)
        : rtree(rtree_), coordinates(std::move(coordinates_)), datafacade(datafacade_)
    {
    }

    std::vector<EdgeData> Search(const util::RectangleInt2D &bbox)
    {
        return rtree.SearchInBox(bbox);
    }

    // Returns nearest PhantomNodes in the given bearing range within max_distance.
    // Does not filter by small/big component!
    std::vector<PhantomNodeWithDistance>
    NearestPhantomNodesInRange(const util::Coordinate input_coordinate, const double max_distance)
    {
        auto results = rtree.Nearest(input_coordinate,
                                     [](const EdgeData &)
                                     {
                                         return std::make_pair(true, true);
                                     },
                                     [max_distance](const std::size_t, const double min_dist)
                                     {
                                         return min_dist > max_distance;
                                     });

        return MakePhantomNodes(input_coordinate, results);
    }

    // Returns nearest PhantomNodes in the given bearing range within max_distance.
    // Does not filter by small/big component!
    std::vector<PhantomNodeWithDistance>
    NearestPhantomNodesInRange(const util::Coordinate input_coordinate,
                               const double max_distance,
                               const int bearing,
                               const int bearing_range)
    {
        auto results =
            rtree.Nearest(input_coordinate,
                          [this, bearing, bearing_range, max_distance](const EdgeData &data)
                          {
                              return checkSegmentBearing(data, bearing, bearing_range);
                          },
                          [max_distance](const std::size_t, const double min_dist)
                          {
                              return min_dist > max_distance;
                          });

        return MakePhantomNodes(input_coordinate, results);
    }

    // Returns max_results nearest PhantomNodes in the given bearing range.
    // Does not filter by small/big component!
    std::vector<PhantomNodeWithDistance>
    NearestPhantomNodes(const util::Coordinate input_coordinate,
                        const unsigned max_results,
                        const int bearing,
                        const int bearing_range)
    {
        auto results = rtree.Nearest(input_coordinate,
                                     [this, bearing, bearing_range](const EdgeData &data)
                                     {
                                         return checkSegmentBearing(data, bearing, bearing_range);
                                     },
                                     [max_results](const std::size_t num_results, const double)
                                     {
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
                        const int bearing_range)
    {
        auto results = rtree.Nearest(
            input_coordinate,
            [this, bearing, bearing_range](const EdgeData &data)
            {
                return checkSegmentBearing(data, bearing, bearing_range);
            },
            [max_results, max_distance](const std::size_t num_results, const double min_dist)
            {
                return num_results >= max_results || min_dist > max_distance;
            });

        return MakePhantomNodes(input_coordinate, results);
    }

    // Returns max_results nearest PhantomNodes.
    // Does not filter by small/big component!
    std::vector<PhantomNodeWithDistance>
    NearestPhantomNodes(const util::Coordinate input_coordinate, const unsigned max_results)
    {
        auto results = rtree.Nearest(input_coordinate,
                                     [](const EdgeData &)
                                     {
                                         return std::make_pair(true, true);
                                     },
                                     [max_results](const std::size_t num_results, const double)
                                     {
                                         return num_results >= max_results;
                                     });

        return MakePhantomNodes(input_coordinate, results);
    }

    // Returns max_results nearest PhantomNodes in the given max distance.
    // Does not filter by small/big component!
    std::vector<PhantomNodeWithDistance>
    NearestPhantomNodes(const util::Coordinate input_coordinate,
                        const unsigned max_results,
                        const double max_distance)
    {
        auto results = rtree.Nearest(
            input_coordinate,
            [](const EdgeData &)
            {
                return std::make_pair(true, true);
            },
            [max_results, max_distance](const std::size_t num_results, const double min_dist)
            {
                return num_results >= max_results || min_dist > max_distance;
            });

        return MakePhantomNodes(input_coordinate, results);
    }

    // Returns the nearest phantom node. If this phantom node is not from a big component
    // a second phantom node is return that is the nearest coordinate in a big component.
    std::pair<PhantomNode, PhantomNode>
    NearestPhantomNodeWithAlternativeFromBigComponent(const util::Coordinate input_coordinate,
                                                      const double max_distance)
    {
        bool has_small_component = false;
        bool has_big_component = false;
        auto results = rtree.Nearest(
            input_coordinate,
            [&has_big_component, &has_small_component](const EdgeData &data)
            {
                auto use_segment =
                    (!has_small_component || (!has_big_component && !data.component.is_tiny));
                auto use_directions = std::make_pair(use_segment, use_segment);

                has_big_component = has_big_component || !data.component.is_tiny;
                has_small_component = has_small_component || data.component.is_tiny;

                return use_directions;
            },
            [&has_big_component, max_distance](const std::size_t num_results, const double min_dist)
            {
                return (num_results > 0 && has_big_component) || min_dist > max_distance;
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
    NearestPhantomNodeWithAlternativeFromBigComponent(const util::Coordinate input_coordinate)
    {
        bool has_small_component = false;
        bool has_big_component = false;
        auto results =
            rtree.Nearest(input_coordinate,
                          [&has_big_component, &has_small_component](const EdgeData &data)
                          {
                              auto use_segment = (!has_small_component ||
                                                  (!has_big_component && !data.component.is_tiny));
                              auto use_directions = std::make_pair(use_segment, use_segment);

                              has_big_component = has_big_component || !data.component.is_tiny;
                              has_small_component = has_small_component || data.component.is_tiny;

                              return use_directions;
                          },
                          [&has_big_component](const std::size_t num_results, const double)
                          {
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
    std::pair<PhantomNode, PhantomNode> NearestPhantomNodeWithAlternativeFromBigComponent(
        const util::Coordinate input_coordinate, const int bearing, const int bearing_range)
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
            [&has_big_component](const std::size_t num_results, const double)
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

    // Returns the nearest phantom node. If this phantom node is not from a big component
    // a second phantom node is return that is the nearest coordinate in a big component.
    std::pair<PhantomNode, PhantomNode>
    NearestPhantomNodeWithAlternativeFromBigComponent(const util::Coordinate input_coordinate,
                                                      const double max_distance,
                                                      const int bearing,
                                                      const int bearing_range)
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
            [&has_big_component, max_distance](const std::size_t num_results, const double min_dist)
            {
                return (num_results > 0 && has_big_component) || min_dist > max_distance;
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
        std::transform(results.begin(), results.end(), distance_and_phantoms.begin(),
                       [this, &input_coordinate](const EdgeData &data)
                       {
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
            util::coordinate_calculation::perpendicularDistance(
                coordinates->at(data.u), coordinates->at(data.v), input_coordinate,
                point_on_segment, ratio);

        // Find the node-based-edge that this belongs to, and directly
        // calculate the forward_weight, forward_offset, reverse_weight, reverse_offset

        int forward_offset = 0, forward_weight = 0;
        int reverse_offset = 0, reverse_weight = 0;

        if (data.forward_packed_geometry_id != SPECIAL_EDGEID)
        {
            std::vector<EdgeWeight> forward_weight_vector;
            datafacade.GetUncompressedWeights(data.forward_packed_geometry_id,
                                              forward_weight_vector);
            for (std::size_t i = 0; i < data.fwd_segment_position; i++)
            {
                forward_offset += forward_weight_vector[i];
            }
            forward_weight = forward_weight_vector[data.fwd_segment_position];
        }

        if (data.reverse_packed_geometry_id != SPECIAL_EDGEID)
        {
            std::vector<EdgeWeight> reverse_weight_vector;
            datafacade.GetUncompressedWeights(data.reverse_packed_geometry_id,
                                              reverse_weight_vector);

            BOOST_ASSERT(data.fwd_segment_position < reverse_weight_vector.size());

            for (std::size_t i = 0;
                 i < reverse_weight_vector.size() - data.fwd_segment_position - 1; i++)
            {
                reverse_offset += reverse_weight_vector[i];
            }
            reverse_weight =
                reverse_weight_vector[reverse_weight_vector.size() - data.fwd_segment_position - 1];
        }

        ratio = std::min(1.0, std::max(0.0, ratio));
        if (SPECIAL_NODEID != data.forward_edge_based_node_id)
        {
            forward_weight *= ratio;
        }
        if (SPECIAL_NODEID != data.reverse_edge_based_node_id)
        {
            reverse_weight *= 1.0 - ratio;
        }

        auto transformed =
            PhantomNodeWithDistance{PhantomNode{data, forward_weight, forward_offset,
                                                reverse_weight, reverse_offset, point_on_segment},
                                    current_perpendicular_distance};

        return transformed;
    }

    std::pair<bool, bool> checkSegmentBearing(const EdgeData &segment,
                                              const int filter_bearing,
                                              const int filter_bearing_range)
    {
        const double forward_edge_bearing = util::coordinate_calculation::bearing(
            coordinates->at(segment.u), coordinates->at(segment.v));

        const double backward_edge_bearing = (forward_edge_bearing + 180) > 360
                                                 ? (forward_edge_bearing - 180)
                                                 : (forward_edge_bearing + 180);

        const bool forward_bearing_valid =
            util::bearing::CheckInBounds(std::round(forward_edge_bearing), filter_bearing,
                                         filter_bearing_range) &&
            segment.forward_edge_based_node_id != SPECIAL_NODEID;
        const bool backward_bearing_valid =
            util::bearing::CheckInBounds(std::round(backward_edge_bearing), filter_bearing,
                                         filter_bearing_range) &&
            segment.reverse_edge_based_node_id != SPECIAL_NODEID;
        return std::make_pair(forward_bearing_valid, backward_bearing_valid);
    }

    RTreeT &rtree;
    const std::shared_ptr<CoordinateList> coordinates;
    DataFacadeT &datafacade;
};
}
}

#endif
