#ifndef MATCH_HPP
#define MATCH_HPP

#include "engine/plugins/plugin_base.hpp"

#include "engine/map_matching/bayes_classifier.hpp"
#include "engine/object_encoder.hpp"
#include "engine/search_engine.hpp"
#include "engine/guidance/textual_route_annotation.hpp"
#include "engine/guidance/segment_list.hpp"
#include "engine/api_response_generator.hpp"
#include "engine/routing_algorithms/map_matching.hpp"
#include "util/coordinate_calculation.hpp"
#include "util/integer_range.hpp"
#include "util/json/logger.hpp"
#include "util/json/util.hpp"
#include "util/string_util.hpp"

#include <cstdlib>

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

namespace osrm
{
namespace engine
{
namespace plugins
{

template <class DataFacadeT> class MapMatchingPlugin : public BasePlugin
{
    std::shared_ptr<SearchEngine<DataFacadeT>> search_engine_ptr;

    using SubMatching = routing_algorithms::SubMatching;
    using SubMatchingList = routing_algorithms::SubMatchingList;
    using CandidateLists = routing_algorithms::CandidateLists;
    using ClassifierT = map_matching::BayesClassifier<map_matching::LaplaceDistribution,
                                                      map_matching::LaplaceDistribution,
                                                      double>;
    using TraceClassification = ClassifierT::ClassificationT;

  public:
    MapMatchingPlugin(DataFacadeT *facade, const int max_locations_map_matching)
        : descriptor_string("match"), facade(facade),
          max_locations_map_matching(max_locations_map_matching),
          // the values where derived from fitting a laplace distribution
          // to the values of manually classified traces
          classifier(map_matching::LaplaceDistribution(0.005986, 0.016646),
                     map_matching::LaplaceDistribution(0.054385, 0.458432),
                     0.696774) // valid apriori probability
    {
        search_engine_ptr = std::make_shared<SearchEngine<DataFacadeT>>(facade);
    }

    virtual ~MapMatchingPlugin() {}

    const std::string GetDescriptor() const final override { return descriptor_string; }

    TraceClassification
    classify(const float trace_length, const float matched_length, const int removed_points) const
    {
        (void)removed_points; // unused

        const double distance_feature = -std::log(trace_length) + std::log(matched_length);

        // matched to the same point
        if (!std::isfinite(distance_feature))
        {
            return std::make_pair(ClassifierT::ClassLabel::NEGATIVE, 1.0);
        }

        const auto label_with_confidence = classifier.classify(distance_feature);

        return label_with_confidence;
    }

    CandidateLists getCandidates(
        const std::vector<util::FixedPointCoordinate> &input_coords,
        const std::vector<std::pair<const int, const boost::optional<int>>> &input_bearings,
        const double gps_precision,
        std::vector<double> &sub_trace_lengths)
    {
        CandidateLists candidates_lists;

        // assuming gps_precision is the standard deviation of a normal distribution that
        // models GPS noise (in this model), this should give us the correct search radius
        // with > 99% confidence
        double query_radius = 3 * gps_precision;
        double last_distance =
            util::coordinate_calculation::haversineDistance(input_coords[0], input_coords[1]);

        sub_trace_lengths.resize(input_coords.size());
        sub_trace_lengths[0] = 0;
        for (const auto current_coordinate : util::irange<std::size_t>(0, input_coords.size()))
        {
            bool allow_uturn = false;
            if (0 < current_coordinate)
            {
                last_distance = util::coordinate_calculation::haversineDistance(
                    input_coords[current_coordinate - 1], input_coords[current_coordinate]);

                sub_trace_lengths[current_coordinate] +=
                    sub_trace_lengths[current_coordinate - 1] + last_distance;
            }

            if (input_coords.size() - 1 > current_coordinate && 0 < current_coordinate)
            {
                double turn_angle = util::coordinate_calculation::computeAngle(
                    input_coords[current_coordinate - 1], input_coords[current_coordinate],
                    input_coords[current_coordinate + 1]);

                // sharp turns indicate a possible uturn
                if (turn_angle <= 90.0 || turn_angle >= 270.0)
                {
                    allow_uturn = true;
                }
            }

            // Use bearing values if supplied, otherwise fallback to 0,180 defaults
            auto bearing = input_bearings.size() > 0 ? input_bearings[current_coordinate].first : 0;
            auto range = input_bearings.size() > 0
                             ? (input_bearings[current_coordinate].second
                                    ? *input_bearings[current_coordinate].second
                                    : 10)
                             : 180;
            auto candidates = facade->NearestPhantomNodesInRange(input_coords[current_coordinate],
                                                                 query_radius, bearing, range);

            if (candidates.size() == 0)
            {
                break;
            }

            // sort by forward id, then by reverse id and then by distance
            std::sort(
                candidates.begin(), candidates.end(),
                [](const PhantomNodeWithDistance &lhs, const PhantomNodeWithDistance &rhs)
                {
                    return lhs.phantom_node.forward_node_id < rhs.phantom_node.forward_node_id ||
                           (lhs.phantom_node.forward_node_id == rhs.phantom_node.forward_node_id &&
                            (lhs.phantom_node.reverse_node_id < rhs.phantom_node.reverse_node_id ||
                             (lhs.phantom_node.reverse_node_id ==
                                  rhs.phantom_node.reverse_node_id &&
                              lhs.distance < rhs.distance)));
                });

            auto new_end = std::unique(
                candidates.begin(), candidates.end(),
                [](const PhantomNodeWithDistance &lhs, const PhantomNodeWithDistance &rhs)
                {
                    return lhs.phantom_node.forward_node_id == rhs.phantom_node.forward_node_id &&
                           lhs.phantom_node.reverse_node_id == rhs.phantom_node.reverse_node_id;
                });
            candidates.resize(new_end - candidates.begin());

            if (!allow_uturn)
            {
                const auto compact_size = candidates.size();
                for (const auto i : util::irange<std::size_t>(0, compact_size))
                {
                    // Split edge if it is bidirectional and append reverse direction to end of list
                    if (candidates[i].phantom_node.forward_node_id != SPECIAL_NODEID &&
                        candidates[i].phantom_node.reverse_node_id != SPECIAL_NODEID)
                    {
                        PhantomNode reverse_node(candidates[i].phantom_node);
                        reverse_node.forward_node_id = SPECIAL_NODEID;
                        candidates.push_back(
                            PhantomNodeWithDistance{reverse_node, candidates[i].distance});

                        candidates[i].phantom_node.reverse_node_id = SPECIAL_NODEID;
                    }
                }
            }

            // sort by distance to make pruning effective
            std::sort(candidates.begin(), candidates.end(),
                      [](const PhantomNodeWithDistance &lhs, const PhantomNodeWithDistance &rhs)
                      {
                          return lhs.distance < rhs.distance;
                      });

            candidates_lists.push_back(std::move(candidates));
        }

        return candidates_lists;
    }

    util::json::Object submatchingToJSON(const SubMatching &sub,
                                         const RouteParameters &route_parameters,
                                         const InternalRouteResult &raw_route)
    {
        util::json::Object subtrace;

        if (route_parameters.classify)
        {
            subtrace.values["confidence"] = sub.confidence;
        }

        auto response_generator = MakeApiResponseGenerator(facade);

        subtrace.values["hint_data"] = response_generator.BuildHintData(raw_route);

        if (route_parameters.geometry || route_parameters.print_instructions)
        {
            using SegmentList = guidance::SegmentList<DataFacadeT>;
            // Passing false to extract_alternative extracts the route.
            const constexpr bool EXTRACT_ROUTE = false;
            // by passing false to segment_list, we skip the douglas peucker simplification
            // and mark all segments as necessary within the generation process
            const constexpr bool NO_ROUTE_SIMPLIFICATION = false;
            SegmentList segment_list(raw_route, EXTRACT_ROUTE, route_parameters.zoom_level,
                                     NO_ROUTE_SIMPLIFICATION, facade);

            if (route_parameters.geometry)
            {
                subtrace.values["geometry"] =
                    response_generator.GetGeometry(route_parameters.compression, segment_list);
            }

            if (route_parameters.print_instructions)
            {
                subtrace.values["instructions"] =
                    guidance::AnnotateRoute<DataFacadeT>(segment_list.Get(), facade);
            }

            util::json::Object json_route_summary;
            json_route_summary.values["total_distance"] = segment_list.GetDistance();
            json_route_summary.values["total_time"] = segment_list.GetDuration();
            subtrace.values["route_summary"] = json_route_summary;
        }

        subtrace.values["indices"] = util::json::make_array(sub.indices);

        util::json::Array points;
        for (const auto &node : sub.nodes)
        {
            points.values.emplace_back(
                util::json::make_array(node.location.lat / COORDINATE_PRECISION,
                                       node.location.lon / COORDINATE_PRECISION));
        }
        subtrace.values["matched_points"] = points;

        util::json::Array names;
        for (const auto &node : sub.nodes)
        {
            names.values.emplace_back(facade->get_name_for_id(node.name_id));
        }
        subtrace.values["matched_names"] = names;

        return subtrace;
    }

    Status HandleRequest(const RouteParameters &route_parameters,
                         util::json::Object &json_result) final override
    {
        // enforce maximum number of locations for performance reasons
        if (max_locations_map_matching > 0 &&
            static_cast<int>(route_parameters.coordinates.size()) > max_locations_map_matching)
        {
            json_result.values["status_message"] = "Too many coodindates";
            return Status::Error;
        }

        // check number of parameters
        if (!check_all_coordinates(route_parameters.coordinates))
        {
            json_result.values["status_message"] = "Invalid coordinates";
            return Status::Error;
        }

        std::vector<double> sub_trace_lengths;
        const auto &input_coords = route_parameters.coordinates;
        const auto &input_timestamps = route_parameters.timestamps;
        const auto &input_bearings = route_parameters.bearings;
        if (input_timestamps.size() > 0 && input_coords.size() != input_timestamps.size())
        {
            json_result.values["status_message"] =
                "Number of timestamps does not match number of coordinates";
            return Status::Error;
        }

        if (input_bearings.size() > 0 && input_coords.size() != input_bearings.size())
        {
            json_result.values["status_message"] =
                "Number of bearings does not match number of coordinates";
            return Status::Error;
        }

        // at least two coordinates are needed for map matching
        if (static_cast<int>(input_coords.size()) < 2)
        {
            json_result.values["status_message"] = "At least two coordinates needed";
            return Status::Error;
        }

        const auto candidates_lists = getCandidates(
            input_coords, input_bearings, route_parameters.gps_precision, sub_trace_lengths);
        if (candidates_lists.size() != input_coords.size())
        {
            BOOST_ASSERT(candidates_lists.size() < input_coords.size());
            json_result.values["status_message"] =
                std::string("Could not find a matching segment for coordinate ") +
                std::to_string(candidates_lists.size());
            return Status::NoSegment;
        }

        // setup logging if enabled
        if (util::json::Logger::get())
            util::json::Logger::get()->initialize("matching");

        // call the actual map matching
        SubMatchingList sub_matchings;
        search_engine_ptr->map_matching(candidates_lists, input_coords, input_timestamps,
                                        route_parameters.matching_beta,
                                        route_parameters.gps_precision, sub_matchings);

        util::json::Array matchings;
        for (auto &sub : sub_matchings)
        {
            // classify result
            if (route_parameters.classify)
            {
                double trace_length =
                    sub_trace_lengths[sub.indices.back()] - sub_trace_lengths[sub.indices.front()];
                TraceClassification classification =
                    classify(trace_length, sub.length,
                             (sub.indices.back() - sub.indices.front() + 1) - sub.nodes.size());
                if (classification.first == ClassifierT::ClassLabel::POSITIVE)
                {
                    sub.confidence = classification.second;
                }
                else
                {
                    sub.confidence = 1 - classification.second;
                }
            }

            BOOST_ASSERT(sub.nodes.size() > 1);

            // FIXME we only run this to obtain the geometry
            // The clean way would be to get this directly from the map matching plugin
            InternalRouteResult raw_route;
            PhantomNodes current_phantom_node_pair;
            for (unsigned i = 0; i < sub.nodes.size() - 1; ++i)
            {
                current_phantom_node_pair.source_phantom = sub.nodes[i];
                current_phantom_node_pair.target_phantom = sub.nodes[i + 1];
                BOOST_ASSERT(current_phantom_node_pair.source_phantom.IsValid());
                BOOST_ASSERT(current_phantom_node_pair.target_phantom.IsValid());
                raw_route.segment_end_coordinates.emplace_back(current_phantom_node_pair);
            }
            search_engine_ptr->shortest_path(
                raw_route.segment_end_coordinates,
                std::vector<bool>(raw_route.segment_end_coordinates.size() + 1, true), raw_route);

            BOOST_ASSERT(raw_route.shortest_path_length != INVALID_EDGE_WEIGHT);

            matchings.values.emplace_back(submatchingToJSON(sub, route_parameters, raw_route));
        }

        if (util::json::Logger::get())
            util::json::Logger::get()->render("matching", json_result);
        json_result.values["matchings"] = matchings;

        if (sub_matchings.empty())
        {
            json_result.values["status_message"] = "Cannot find matchings";
            return Status::EmptyResult;
        }

        json_result.values["status_message"] = "Found matchings";
        return Status::Ok;
    }

  private:
    std::string descriptor_string;
    DataFacadeT *facade;
    int max_locations_map_matching;
    ClassifierT classifier;
};
}
}
}

#endif // MATCH_HPP
