#include "extractor/extractor_callbacks.hpp"
#include "extractor/external_memory_node.hpp"
#include "extractor/extraction_containers.hpp"
#include "extractor/extraction_node.hpp"
#include "extractor/extraction_way.hpp"
#include "extractor/guidance/road_classification.hpp"
#include "extractor/profile_properties.hpp"
#include "extractor/restriction.hpp"

#include "util/for_each_pair.hpp"
#include "util/guidance/turn_lanes.hpp"
#include "util/log.hpp"

#include <boost/numeric/conversion/cast.hpp>
#include <boost/optional/optional.hpp>
#include <boost/tokenizer.hpp>

#include <osmium/osm.hpp>

#include "osrm/coordinate.hpp"

#include <cstring>
#include <iterator>
#include <limits>
#include <string>
#include <vector>

namespace osrm
{
namespace extractor
{

using TurnLaneDescription = guidance::TurnLaneDescription;
namespace TurnLaneType = guidance::TurnLaneType;

ExtractorCallbacks::ExtractorCallbacks(ExtractionContainers &extraction_containers_,
                                       const ProfileProperties &properties)
    : external_memory(extraction_containers_), fallback_to_duration(properties.fallback_to_duration)
{
    // we reserved 0, 1, 2, 3 for the empty case
    string_map[MapKey("", "", "", "")] = 0;
    lane_description_map[TurnLaneDescription()] = 0;
}

/**
 * Takes the node position from osmium and the filtered properties from the lua
 * profile and saves them to external memory.
 *
 * warning: caller needs to take care of synchronization!
 */
void ExtractorCallbacks::ProcessNode(const osmium::Node &input_node,
                                     const ExtractionNode &result_node)
{
    external_memory.all_nodes_list.push_back(
        {util::toFixed(util::FloatLongitude{input_node.location().lon()}),
         util::toFixed(util::FloatLatitude{input_node.location().lat()}),
         OSMNodeID{static_cast<std::uint64_t>(input_node.id())},
         result_node.barrier,
         result_node.traffic_lights});
}

void ExtractorCallbacks::ProcessRestriction(
    const boost::optional<InputRestrictionContainer> &restriction)
{
    if (restriction)
    {
        external_memory.restrictions_list.push_back(restriction.get());
        // util::Log() << "from: " << restriction.get().restriction.from.node <<
        //                           ",via: " << restriction.get().restriction.via.node <<
        //                           ", to: " << restriction.get().restriction.to.node <<
        //                           ", only: " << (restriction.get().restriction.flags.is_only ?
        //                           "y" : "n");
    }
}
/**
 * Takes the geometry contained in the ```input_way``` and the tags computed
 * by the lua profile inside ```parsed_way``` and computes all edge segments.
 *
 * Depending on the forward/backwards weights the edges are split into forward
 * and backward edges.
 *
 * warning: caller needs to take care of synchronization!
 */
void ExtractorCallbacks::ProcessWay(const osmium::Way &input_way, const ExtractionWay &parsed_way)
{
    if ((parsed_way.forward_travel_mode == TRAVEL_MODE_INACCESSIBLE ||
         parsed_way.forward_speed <= 0) &&
        (parsed_way.backward_travel_mode == TRAVEL_MODE_INACCESSIBLE ||
         parsed_way.backward_speed <= 0) &&
        parsed_way.duration <= 0)
    { // Only true if the way is assigned a valid speed/duration
        return;
    }

    if (!fallback_to_duration && (parsed_way.forward_travel_mode == TRAVEL_MODE_INACCESSIBLE ||
                                  parsed_way.forward_rate <= 0) &&
        (parsed_way.backward_travel_mode == TRAVEL_MODE_INACCESSIBLE ||
         parsed_way.backward_rate <= 0) &&
        parsed_way.weight <= 0)
    { // Only true if the way is assigned a valid rate/weight and there is no duration fallback
        return;
    }

    const auto &nodes = input_way.nodes();
    if (nodes.size() <= 1)
    { // safe-guard against broken data
        return;
    }

    if (std::numeric_limits<decltype(input_way.id())>::max() == input_way.id())
    {
        util::Log(logDEBUG) << "found bogus way with id: " << input_way.id() << " of size "
                            << nodes.size();
        return;
    }

    InternalExtractorEdge::DurationData forward_duration_data;
    InternalExtractorEdge::DurationData backward_duration_data;
    InternalExtractorEdge::WeightData forward_weight_data;
    InternalExtractorEdge::WeightData backward_weight_data;

    const auto toValueByEdgeOrByMeter = [&nodes](const double by_way, const double by_meter) {
        using Value = detail::ByEdgeOrByMeterValue;
        // get value by weight per edge
        if (by_way >= 0)
        {
            // FIXME We divide by the number of edges here, but should rather consider
            // the length of each segment. We would either have to compute the length
            // of the whole way here (we can't: no node coordinates) or push that back
            // to the container and keep a reference to the way.
            const std::size_t num_edges = (nodes.size() - 1);
            return Value(Value::by_edge, by_way / num_edges);
        }
        else
        {
            // get value by deriving weight from speed per edge
            return Value(Value::by_meter, by_meter);
        }
    };

    if (parsed_way.forward_travel_mode != TRAVEL_MODE_INACCESSIBLE)
    {
        BOOST_ASSERT(parsed_way.duration > 0 || parsed_way.forward_speed > 0);
        forward_duration_data =
            toValueByEdgeOrByMeter(parsed_way.duration, parsed_way.forward_speed / 3.6);
        // fallback to duration as weight
        if (fallback_to_duration)
        {
            forward_weight_data = forward_duration_data;
        }
        else
        {
            BOOST_ASSERT(parsed_way.weight > 0 || parsed_way.forward_rate > 0);
            forward_weight_data =
                toValueByEdgeOrByMeter(parsed_way.weight, parsed_way.forward_rate);
        }
    }
    if (parsed_way.backward_travel_mode != TRAVEL_MODE_INACCESSIBLE)
    {
        BOOST_ASSERT(parsed_way.duration > 0 || parsed_way.backward_speed > 0);
        backward_duration_data =
            toValueByEdgeOrByMeter(parsed_way.duration, parsed_way.backward_speed / 3.6);
        // fallback to duration as weight
        if (fallback_to_duration)
        {
            backward_weight_data = backward_duration_data;
        }
        else
        {
            BOOST_ASSERT(parsed_way.weight > 0 || parsed_way.backward_rate > 0);
            backward_weight_data =
                toValueByEdgeOrByMeter(parsed_way.weight, parsed_way.backward_rate);
        }
    }

    const auto laneStringToDescription = [](const std::string &lane_string) -> TurnLaneDescription {
        if (lane_string.empty())
            return {};

        TurnLaneDescription lane_description;

        typedef boost::tokenizer<boost::char_separator<char>> tokenizer;
        boost::char_separator<char> sep("|", "", boost::keep_empty_tokens);
        boost::char_separator<char> inner_sep(";", "");
        tokenizer tokens(lane_string, sep);

        const constexpr std::size_t num_osm_tags = 11;
        const constexpr char *osm_lane_strings[num_osm_tags] = {"none",
                                                                "through",
                                                                "sharp_left",
                                                                "left",
                                                                "slight_left",
                                                                "slight_right",
                                                                "right",
                                                                "sharp_right",
                                                                "reverse",
                                                                "merge_to_left",
                                                                "merge_to_right"};

        const constexpr TurnLaneType::Mask masks_by_osm_string[num_osm_tags + 1] = {
            TurnLaneType::none,
            TurnLaneType::straight,
            TurnLaneType::sharp_left,
            TurnLaneType::left,
            TurnLaneType::slight_left,
            TurnLaneType::slight_right,
            TurnLaneType::right,
            TurnLaneType::sharp_right,
            TurnLaneType::uturn,
            TurnLaneType::merge_to_left,
            TurnLaneType::merge_to_right,
            TurnLaneType::empty}; // fallback, if string not found

        for (auto iter = tokens.begin(); iter != tokens.end(); ++iter)
        {
            tokenizer inner_tokens(*iter, inner_sep);
            guidance::TurnLaneType::Mask lane_mask = inner_tokens.begin() == inner_tokens.end()
                                                         ? TurnLaneType::none
                                                         : TurnLaneType::empty;
            for (auto token_itr = inner_tokens.begin(); token_itr != inner_tokens.end();
                 ++token_itr)
            {
                auto position =
                    std::find(osm_lane_strings, osm_lane_strings + num_osm_tags, *token_itr);
                const auto translated_mask =
                    masks_by_osm_string[std::distance(osm_lane_strings, position)];
                if (translated_mask == TurnLaneType::empty)
                {
                    // if we have unsupported tags, don't handle them
                    util::Log(logDEBUG) << "Unsupported lane tag found: \"" << *token_itr << "\"";
                    return {};
                }

                // In case of multiple times the same lane indicators withn a lane, as in
                // "left;left|.."  or-ing the masks generates a single "left" enum.
                // Which is fine since this is data issue and we can't represent it anyway.
                lane_mask |= translated_mask;
            }
            // add the lane to the description
            lane_description.push_back(lane_mask);
        }
        return lane_description;
    };

    // convert the lane description into an ID and, if necessary, remember the description in the
    // description_map
    const auto requestId = [&](const std::string &lane_string) {
        if (lane_string.empty())
            return INVALID_LANE_DESCRIPTIONID;
        TurnLaneDescription lane_description = laneStringToDescription(std::move(lane_string));

        const auto lane_description_itr = lane_description_map.find(lane_description);
        if (lane_description_itr == lane_description_map.end())
        {
            const LaneDescriptionID new_id =
                boost::numeric_cast<LaneDescriptionID>(lane_description_map.size());
            lane_description_map[lane_description] = new_id;
            return new_id;
        }
        else
        {
            return lane_description_itr->second;
        }
    };

    // Deduplicates street names, refs, destinations, pronunciation based on the string_map.
    // In case we do not already store the key, inserts (key, id) tuple and return id.
    // Otherwise fetches the id based on the name and returns it without insertion.
    const auto turn_lane_id_forward = requestId(parsed_way.turn_lanes_forward);
    const auto turn_lane_id_backward = requestId(parsed_way.turn_lanes_backward);

    const auto road_classification = parsed_way.road_classification;

    // Get the unique identifier for the street name, destination, and ref
    const auto name_iterator = string_map.find(
        MapKey(parsed_way.name, parsed_way.destinations, parsed_way.ref, parsed_way.pronunciation));
    NameID name_id = EMPTY_NAMEID;
    if (string_map.end() == name_iterator)
    {
        // name_offsets has a sentinel element with the total name data size
        // take the sentinels index as the name id of the new name data pack
        // (name [name_id], destination [+1], pronunciation [+2], ref [+3])
        name_id = external_memory.name_offsets.size() - 1;

        std::copy(parsed_way.name.begin(),
                  parsed_way.name.end(),
                  std::back_inserter(external_memory.name_char_data));
        external_memory.name_offsets.push_back(external_memory.name_char_data.size());

        std::copy(parsed_way.destinations.begin(),
                  parsed_way.destinations.end(),
                  std::back_inserter(external_memory.name_char_data));
        external_memory.name_offsets.push_back(external_memory.name_char_data.size());

        std::copy(parsed_way.pronunciation.begin(),
                  parsed_way.pronunciation.end(),
                  std::back_inserter(external_memory.name_char_data));
        external_memory.name_offsets.push_back(external_memory.name_char_data.size());

        std::copy(parsed_way.ref.begin(),
                  parsed_way.ref.end(),
                  std::back_inserter(external_memory.name_char_data));
        external_memory.name_offsets.push_back(external_memory.name_char_data.size());

        auto k = MapKey{
            parsed_way.name, parsed_way.destinations, parsed_way.ref, parsed_way.pronunciation};
        auto v = MapVal{name_id};
        string_map.emplace(std::move(k), std::move(v));
    }
    else
    {
        name_id = name_iterator->second;
    }

    const bool in_forward_direction =
        (parsed_way.forward_speed > 0 || parsed_way.forward_rate > 0 || parsed_way.duration > 0 ||
         parsed_way.weight > 0) &&
        (parsed_way.forward_travel_mode != TRAVEL_MODE_INACCESSIBLE);

    const bool in_backward_direction =
        (parsed_way.backward_speed > 0 || parsed_way.backward_rate > 0 || parsed_way.duration > 0 ||
         parsed_way.weight > 0) &&
        (parsed_way.backward_travel_mode != TRAVEL_MODE_INACCESSIBLE);

    // split an edge into two edges if forwards/backwards behavior differ
    const bool split_edge = in_forward_direction && in_backward_direction &&
                            ((parsed_way.forward_rate != parsed_way.backward_rate) ||
                             (parsed_way.forward_speed != parsed_way.backward_speed) ||
                             (parsed_way.forward_travel_mode != parsed_way.backward_travel_mode) ||
                             (turn_lane_id_forward != turn_lane_id_backward));

    if (in_forward_direction)
    {
        util::for_each_pair(
            nodes.cbegin(),
            nodes.cend(),
            [&](const osmium::NodeRef &first_node, const osmium::NodeRef &last_node) {
                external_memory.all_edges_list.push_back(
                    InternalExtractorEdge(OSMNodeID{static_cast<std::uint64_t>(first_node.ref())},
                                          OSMNodeID{static_cast<std::uint64_t>(last_node.ref())},
                                          name_id,
                                          forward_weight_data,
                                          forward_duration_data,
                                          true,
                                          in_backward_direction && !split_edge,
                                          parsed_way.roundabout,
                                          parsed_way.circular,
                                          parsed_way.is_startpoint,
                                          parsed_way.forward_restricted,
                                          split_edge,
                                          parsed_way.forward_travel_mode,
                                          turn_lane_id_forward,
                                          road_classification,
                                          {}));
            });
    }

    if (in_backward_direction || split_edge)
    {
        util::for_each_pair(
            nodes.cbegin(),
            nodes.cend(),
            [&](const osmium::NodeRef &first_node, const osmium::NodeRef &last_node) {
                external_memory.all_edges_list.push_back(
                    InternalExtractorEdge(OSMNodeID{static_cast<std::uint64_t>(first_node.ref())},
                                          OSMNodeID{static_cast<std::uint64_t>(last_node.ref())},
                                          name_id,
                                          backward_weight_data,
                                          backward_duration_data,
                                          false,
                                          true,
                                          parsed_way.roundabout,
                                          parsed_way.circular,
                                          parsed_way.is_startpoint,
                                          parsed_way.backward_restricted,
                                          split_edge,
                                          parsed_way.backward_travel_mode,
                                          turn_lane_id_backward,
                                          road_classification,
                                          {}));
            });
    }

    std::transform(nodes.begin(),
                   nodes.end(),
                   std::back_inserter(external_memory.used_node_id_list),
                   [](const osmium::NodeRef &ref) {
                       return OSMNodeID{static_cast<std::uint64_t>(ref.ref())};
                   });

    external_memory.way_start_end_id_list.push_back(
        {OSMWayID{static_cast<std::uint32_t>(input_way.id())},
         OSMNodeID{static_cast<std::uint64_t>(nodes[0].ref())},
         OSMNodeID{static_cast<std::uint64_t>(nodes[1].ref())},
         OSMNodeID{static_cast<std::uint64_t>(nodes[nodes.size() - 2].ref())},
         OSMNodeID{static_cast<std::uint64_t>(nodes.back().ref())}});
}

guidance::LaneDescriptionMap &&ExtractorCallbacks::moveOutLaneDescriptionMap()
{
    return std::move(lane_description_map);
}
} // namespace extractor
} // namespace osrm
