#include "engine/plugins/match.hpp"
#include "engine/plugins/plugin_base.hpp"

#include "engine/api/match_api.hpp"
#include "engine/api/match_parameters.hpp"
#include "engine/map_matching/bayes_classifier.hpp"
#include "engine/map_matching/sub_matching.hpp"
#include "util/coordinate_calculation.hpp"
#include "util/integer_range.hpp"
#include "util/json_util.hpp"
#include "util/string_util.hpp"

#include <cstdlib>

#include <algorithm>
#include <functional>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

static double search_radius_for_gps_radius(double gps_radius)
{
    // For a given GPS radius, determine the radius we need to search for candidate street segments
    // to have a 99.9% chance of finding the correct segment.
    // For more detail, see the analysis at https://github.com/Project-OSRM/osrm-backend/pull/3184
    return std::min(gps_radius * 3.5 + 45, 200.0);
}

namespace osrm
{
namespace engine
{
namespace plugins
{

// Filters PhantomNodes to obtain a set of viable candiates
void filterCandidates(const std::vector<util::Coordinate> &coordinates,
                      MatchPlugin::CandidateLists &candidates_lists)
{
    for (const auto current_coordinate : util::irange<std::size_t>(0, coordinates.size()))
    {
        bool allow_uturn = false;

        if (coordinates.size() - 1 > current_coordinate && 0 < current_coordinate)
        {
            double turn_angle =
                util::coordinate_calculation::computeAngle(coordinates[current_coordinate - 1],
                                                           coordinates[current_coordinate],
                                                           coordinates[current_coordinate + 1]);

            // sharp turns indicate a possible uturn
            if (turn_angle <= 90.0 || turn_angle >= 270.0)
            {
                allow_uturn = true;
            }
        }

        auto &candidates = candidates_lists[current_coordinate];
        if (candidates.empty())
        {
            continue;
        }

        // sort by forward id, then by reverse id and then by distance
        std::sort(candidates.begin(),
                  candidates.end(),
                  [](const PhantomNodeWithDistance &lhs, const PhantomNodeWithDistance &rhs) {
                      return lhs.phantom_node.forward_segment_id.id <
                                 rhs.phantom_node.forward_segment_id.id ||
                             (lhs.phantom_node.forward_segment_id.id ==
                                  rhs.phantom_node.forward_segment_id.id &&
                              (lhs.phantom_node.reverse_segment_id.id <
                                   rhs.phantom_node.reverse_segment_id.id ||
                               (lhs.phantom_node.reverse_segment_id.id ==
                                    rhs.phantom_node.reverse_segment_id.id &&
                                lhs.distance < rhs.distance)));
                  });

        auto new_end =
            std::unique(candidates.begin(),
                        candidates.end(),
                        [](const PhantomNodeWithDistance &lhs, const PhantomNodeWithDistance &rhs) {
                            return lhs.phantom_node.forward_segment_id.id ==
                                       rhs.phantom_node.forward_segment_id.id &&
                                   lhs.phantom_node.reverse_segment_id.id ==
                                       rhs.phantom_node.reverse_segment_id.id;
                        });
        candidates.resize(new_end - candidates.begin());

        if (!allow_uturn)
        {
            const auto compact_size = candidates.size();
            for (const auto i : util::irange<std::size_t>(0, compact_size))
            {
                // Split edge if it is bidirectional and append reverse direction to end of list
                if (candidates[i].phantom_node.forward_segment_id.enabled &&
                    candidates[i].phantom_node.reverse_segment_id.enabled)
                {
                    PhantomNode reverse_node(candidates[i].phantom_node);
                    reverse_node.forward_segment_id.enabled = false;
                    candidates.push_back(
                        PhantomNodeWithDistance{reverse_node, candidates[i].distance});

                    candidates[i].phantom_node.reverse_segment_id.enabled = false;
                }
            }
        }

        // sort by distance to make pruning effective
        std::sort(candidates.begin(),
                  candidates.end(),
                  [](const PhantomNodeWithDistance &lhs, const PhantomNodeWithDistance &rhs) {
                      return lhs.distance < rhs.distance;
                  });
    }
}

Status MatchPlugin::HandleRequest(const std::shared_ptr<datafacade::BaseDataFacade> facade,
                                  const api::MatchParameters &parameters,
                                  util::json::Object &json_result) const
{
    BOOST_ASSERT(parameters.IsValid());

    // enforce maximum number of locations for performance reasons
    if (max_locations_map_matching > 0 &&
        static_cast<int>(parameters.coordinates.size()) > max_locations_map_matching)
    {
        return Error("TooBig", "Too many trace coordinates", json_result);
    }

    if (!CheckAllCoordinates(parameters.coordinates))
    {
        return Error("InvalidValue", "Invalid coordinate value.", json_result);
    }

    // Check for same or increasing timestamps. Impl. note: Incontrast to `sort(first,
    // last, less_equal)` checking `greater` in reverse meets irreflexive requirements.
    const auto time_increases_monotonically = std::is_sorted(
        parameters.timestamps.rbegin(), parameters.timestamps.rend(), std::greater<>{});

    if (!time_increases_monotonically)
    {
        return Error(
            "InvalidValue", "Timestamps need to be monotonically increasing.", json_result);
    }

    // assuming radius is the standard deviation of a normal distribution
    // that models GPS noise (in this model), x3 should give us the correct
    // search radius with > 99% confidence
    std::vector<double> search_radiuses;
    if (parameters.radiuses.empty())
    {
        search_radiuses.resize(parameters.coordinates.size(),
                               DEFAULT_GPS_PRECISION * RADIUS_MULTIPLIER);
    }
    else
    {
        search_radiuses.resize(parameters.coordinates.size());
        std::transform(parameters.radiuses.begin(),
                       parameters.radiuses.end(),
                       search_radiuses.begin(),
                       [&](const boost::optional<double> &maybe_radius) {
                           double gps_radius = maybe_radius ? *maybe_radius : DEFAULT_GPS_PRECISION;
                           return search_radius_for_gps_radius(gps_radius);
                       });
    }

    auto candidates_lists = GetPhantomNodesInRange(*facade, parameters, search_radiuses);

    filterCandidates(parameters.coordinates, candidates_lists);
    if (std::all_of(candidates_lists.begin(),
                    candidates_lists.end(),
                    [](const std::vector<PhantomNodeWithDistance> &candidates) {
                        return candidates.empty();
                    }))
    {
        return Error("NoSegment",
                     std::string("Could not find a matching segment for any coordinate."),
                     json_result);
    }

    // call the actual map matching
    SubMatchingList sub_matchings = map_matching(*facade,
                                                 candidates_lists,
                                                 parameters.coordinates,
                                                 parameters.timestamps,
                                                 parameters.radiuses);

    if (sub_matchings.size() == 0)
    {
        return Error("NoMatch", "Could not match the trace.", json_result);
    }

    std::vector<InternalRouteResult> sub_routes(sub_matchings.size());
    for (auto index : util::irange<std::size_t>(0UL, sub_matchings.size()))
    {
        BOOST_ASSERT(sub_matchings[index].nodes.size() > 1);

        // FIXME we only run this to obtain the geometry
        // The clean way would be to get this directly from the map matching plugin
        PhantomNodes current_phantom_node_pair;
        for (unsigned i = 0; i < sub_matchings[index].nodes.size() - 1; ++i)
        {
            current_phantom_node_pair.source_phantom = sub_matchings[index].nodes[i];
            current_phantom_node_pair.target_phantom = sub_matchings[index].nodes[i + 1];
            BOOST_ASSERT(current_phantom_node_pair.source_phantom.IsValid());
            BOOST_ASSERT(current_phantom_node_pair.target_phantom.IsValid());
            sub_routes[index].segment_end_coordinates.emplace_back(current_phantom_node_pair);
        }
        // force uturns to be on, since we split the phantom nodes anyway and only have
        // bi-directional
        // phantom nodes for possible uturns
        shortest_path(
            *facade, sub_routes[index].segment_end_coordinates, {false}, sub_routes[index]);
        BOOST_ASSERT(sub_routes[index].shortest_path_length != INVALID_EDGE_WEIGHT);
    }

    api::MatchAPI match_api{*facade, parameters};
    match_api.MakeResponse(sub_matchings, sub_routes, json_result);

    return Status::Ok;
}
}
}
}
