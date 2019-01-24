#include "engine/plugins/match.hpp"
#include "engine/plugins/plugin_base.hpp"

#include "engine/api/match_api.hpp"
#include "engine/api/match_parameters.hpp"
#include "engine/api/match_parameters_tidy.hpp"
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
#include <set>
#include <string>
#include <vector>

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
            if (turn_angle <= 45.0 || turn_angle >= 315.0)
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

Status MatchPlugin::HandleRequest(const RoutingAlgorithmsInterface &algorithms,
                                  const api::MatchParameters &parameters,
                                  util::json::Object &json_result) const
{
    if (!algorithms.HasMapMatching())
    {
        return Error("NotImplemented",
                     "Map matching is not implemented for the chosen search algorithm.",
                     json_result);
    }

    if (!CheckAlgorithms(parameters, algorithms, json_result))
        return Status::Error;

    const auto &facade = algorithms.GetFacade();

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

    if (max_radius_map_matching > 0 && std::any_of(parameters.radiuses.begin(),
                                                   parameters.radiuses.end(),
                                                   [&](const auto &radius) {
                                                       if (!radius)
                                                           return false;
                                                       return *radius > max_radius_map_matching;
                                                   }))
    {
        return Error("TooBig", "Radius search size is too large for map matching.", json_result);
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

    SubMatchingList sub_matchings;
    api::tidy::Result tidied;
    if (parameters.tidy)
    {
        // Transparently tidy match parameters, do map matching on tidied parameters.
        // Then use the mapping to restore the original <-> tidied relationship.
        tidied = api::tidy::tidy(parameters);
    }
    else
    {
        tidied = api::tidy::keep_all(parameters);
    }

    // Error: first and last points should be waypoints
    if (!parameters.waypoints.empty() &&
        (tidied.parameters.waypoints[0] != 0 ||
         tidied.parameters.waypoints.back() != (tidied.parameters.coordinates.size() - 1)))
    {
        return Error("InvalidValue",
                     "First and last coordinates must be specified as waypoints.",
                     json_result);
    }

    // assuming radius is the standard deviation of a normal distribution
    // that models GPS noise (in this model), x3 should give us the correct
    // search radius with > 99% confidence
    std::vector<double> search_radiuses;
    if (tidied.parameters.radiuses.empty())
    {
        search_radiuses.resize(tidied.parameters.coordinates.size(),
                               routing_algorithms::DEFAULT_GPS_PRECISION * RADIUS_MULTIPLIER);
    }
    else
    {
        search_radiuses.resize(tidied.parameters.coordinates.size());
        std::transform(tidied.parameters.radiuses.begin(),
                       tidied.parameters.radiuses.end(),
                       search_radiuses.begin(),
                       [](const boost::optional<double> &maybe_radius) {
                           if (maybe_radius)
                           {
                               return *maybe_radius * RADIUS_MULTIPLIER;
                           }
                           else
                           {
                               return routing_algorithms::DEFAULT_GPS_PRECISION * RADIUS_MULTIPLIER;
                           }

                       });
    }

    auto candidates_lists =
        GetPhantomNodesInRange(facade, tidied.parameters, search_radiuses, true);

    filterCandidates(tidied.parameters.coordinates, candidates_lists);
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
    sub_matchings =
        algorithms.MapMatching(candidates_lists,
                               tidied.parameters.coordinates,
                               tidied.parameters.timestamps,
                               tidied.parameters.radiuses,
                               parameters.gaps == api::MatchParameters::GapsType::Split);

    if (sub_matchings.size() == 0)
    {
        return Error("NoMatch", "Could not match the trace.", json_result);
    }

    // trace was split, we don't support the waypoints parameter across multiple match objects
    if (sub_matchings.size() > 1 && !parameters.waypoints.empty())
    {
        return Error("NoMatch", "Could not match the trace with the given waypoints.", json_result);
    }

    // Error: Check if user-supplied waypoints can be found in the resulting matches
    if (!parameters.waypoints.empty())
    {
        std::set<std::size_t> tidied_waypoints(tidied.parameters.waypoints.begin(),
                                               tidied.parameters.waypoints.end());
        for (const auto &sm : sub_matchings)
        {
            std::for_each(sm.indices.begin(),
                          sm.indices.end(),
                          [&tidied_waypoints](const auto index) { tidied_waypoints.erase(index); });
        }
        if (!tidied_waypoints.empty())
        {
            return Error(
                "NoMatch", "Requested waypoint parameter could not be matched.", json_result);
        }
    }
    // we haven't errored yet, only allow leg collapsing if it was originally requested
    BOOST_ASSERT(parameters.waypoints.empty() || sub_matchings.size() == 1);
    const auto collapse_legs = !parameters.waypoints.empty();

    // each sub_route will correspond to a MatchObject
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
        // force uturns to be on
        // we split the phantom nodes anyway and only have bi-directional phantom nodes for
        // possible uturns
        sub_routes[index] =
            algorithms.ShortestPathSearch(sub_routes[index].segment_end_coordinates, {false});
        BOOST_ASSERT(sub_routes[index].shortest_path_weight != INVALID_EDGE_WEIGHT);
        if (collapse_legs)
        {
            std::vector<bool> waypoint_legs;
            waypoint_legs.reserve(sub_matchings[index].indices.size());
            for (unsigned i = 0, j = 0; i < sub_matchings[index].indices.size(); ++i)
            {
                auto current_wp = tidied.parameters.waypoints[j];
                if (current_wp == sub_matchings[index].indices[i])
                {
                    waypoint_legs.push_back(true);
                    ++j;
                }
                else
                {
                    waypoint_legs.push_back(false);
                }
            }
            sub_routes[index] = CollapseInternalRouteResult(sub_routes[index], waypoint_legs);
        }
    }

    api::MatchAPI match_api{facade, parameters, tidied};
    match_api.MakeResponse(sub_matchings, sub_routes, json_result);

    return Status::Ok;
}
}
}
}
