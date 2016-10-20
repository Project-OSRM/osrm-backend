#include "extractor/scripting_environment_sol2.hpp"

#include "extractor/external_memory_node.hpp"
#include "extractor/extraction_helper_functions.hpp"
#include "extractor/extraction_node.hpp"
#include "extractor/extraction_way.hpp"
#include "extractor/internal_extractor_edge.hpp"
#include "extractor/profile_properties.hpp"
#include "extractor/raster_source.hpp"
#include "extractor/restriction_parser.hpp"
#include "util/coordinate.hpp"
#include "util/exception.hpp"
#include "util/lua_util.hpp"
#include "util/simple_logger.hpp"
#include "util/typedefs.hpp"

#include <osmium/osm.hpp>

#include <tbb/parallel_for.h>

#include <memory>
#include <sstream>

namespace sol
{
template <> struct is_container<osmium::Node> : std::false_type
{
};
template <> struct is_container<osmium::Way> : std::false_type
{
};
}

namespace osrm
{
namespace extractor
{

template <class T>
auto get_value_by_key(T const &object, const char *key) -> decltype(object.get_value_by_key(key))
{
    auto v = object.get_value_by_key(key);
    if (v && *v)
    { // non-empty string?
        return v;
    }
    else
    {
        return nullptr;
    }
}

template <class T, class D>
const char* get_value_by_key(T const &object, const char *key, D const default_value)
{
    auto v = get_value_by_key(object, key);
    if (v && *v)
    {
        return v;
    }
    else
    {
        return default_value;
    }
}

template <class T> double latToDouble(T const &object)
{
    return static_cast<double>(util::toFloating(object.lat));
}

template <class T> double lonToDouble(T const &object)
{
    return static_cast<double>(util::toFloating(object.lon));
}

auto get_nodes_for_way(const osmium::Way &way) -> decltype(way.nodes()) { return way.nodes(); }

Sol2ScriptingEnvironment::Sol2ScriptingEnvironment(const std::string &file_name)
    : file_name(file_name)
{
    util::SimpleLogger().Write() << "Using script " << file_name;
}

void Sol2ScriptingEnvironment::InitContext(Sol2ScriptingContext &context)
{
    context.state.open_libraries();

    context.state["durationIsValid"] = durationIsValid;
    context.state["parseDuration"] = parseDuration;
    context.state["trimLaneString"] = trimLaneString;
    context.state["applyAccessTokens"] = applyAccessTokens;
    context.state["canonicalizeStringList"] = canonicalizeStringList;

    context.state.new_enum("mode",
                           "inaccessible",
                           TRAVEL_MODE_INACCESSIBLE,
                           "driving",
                           TRAVEL_MODE_DRIVING,
                           "cycling",
                           TRAVEL_MODE_CYCLING,
                           "walking",
                           TRAVEL_MODE_WALKING,
                           "ferry",
                           TRAVEL_MODE_FERRY,
                           "train",
                           TRAVEL_MODE_TRAIN,
                           "pushing_bike",
                           TRAVEL_MODE_PUSHING_BIKE,
                           "steps_up",
                           TRAVEL_MODE_STEPS_UP,
                           "steps_down",
                           TRAVEL_MODE_STEPS_DOWN,
                           "river_up",
                           TRAVEL_MODE_RIVER_UP,
                           "river_down",
                           TRAVEL_MODE_RIVER_DOWN,
                           "route",
                           TRAVEL_MODE_ROUTE);

    context.state.new_enum("road_priority_class",
                           "motorway",
                           extractor::guidance::RoadPriorityClass::MOTORWAY,
                           "trunk",
                           extractor::guidance::RoadPriorityClass::TRUNK,
                           "primary",
                           extractor::guidance::RoadPriorityClass::PRIMARY,
                           "secondary",
                           extractor::guidance::RoadPriorityClass::SECONDARY,
                           "tertiary",
                           extractor::guidance::RoadPriorityClass::TERTIARY,
                           "main_residential",
                           extractor::guidance::RoadPriorityClass::MAIN_RESIDENTIAL,
                           "side_residential",
                           extractor::guidance::RoadPriorityClass::SIDE_RESIDENTIAL,
                           "link_road",
                           extractor::guidance::RoadPriorityClass::LINK_ROAD,
                           "bike_path",
                           extractor::guidance::RoadPriorityClass::BIKE_PATH,
                           "foot_path",
                           extractor::guidance::RoadPriorityClass::FOOT_PATH,
                           "connectivity",
                           extractor::guidance::RoadPriorityClass::CONNECTIVITY);

    context.state.new_usertype<SourceContainer>("sources",
                                                "load",
                                                &SourceContainer::LoadRasterSource,
                                                "query",
                                                &SourceContainer::GetRasterDataFromSource,
                                                "interpolate",
                                                &SourceContainer::GetRasterInterpolateFromSource);

    context.state.new_enum("constants", "precision", COORDINATE_PRECISION);

    context.state.new_usertype<ProfileProperties>(
        "ProfileProperties",
        "traffic_signal_penalty",
        sol::property(&ProfileProperties::GetTrafficSignalPenalty,
                      &ProfileProperties::SetTrafficSignalPenalty),
        "u_turn_penalty",
        sol::property(&ProfileProperties::GetUturnPenalty, //
                      &ProfileProperties::SetUturnPenalty),
        "max_speed_for_map_matching",
        sol::property(&ProfileProperties::GetMaxSpeedForMapMatching,
                      &ProfileProperties::SetMaxSpeedForMapMatching),
        "continue_straight_at_waypoint",
        &ProfileProperties::continue_straight_at_waypoint,
        "use_turn_restrictions",
        &ProfileProperties::use_turn_restrictions,
        "left_hand_driving",
        &ProfileProperties::left_hand_driving);

    context.state.new_usertype<std::vector<std::string>>(
        "vector",
        "Add",
        static_cast<void (std::vector<std::string>::*)(const std::string &)>(
            &std::vector<std::string>::push_back));

    context.state.new_usertype<osmium::Location>(
        "Location", "lat", &osmium::Location::lat, "lon", &osmium::Location::lon);

    context.state.new_usertype<osmium::Way>("Way",
                                            "get_value_by_key",
                                            &get_value_by_key<osmium::Way>,
                                            "id",
                                            &osmium::Way::id,
                                            "get_nodes",
                                            &get_nodes_for_way);

    context.state.new_usertype<osmium::Node>("Node",
                                             "location",
                                             &osmium::Node::location,
                                             "get_value_by_key",
                                             &get_value_by_key<osmium::Node>,
                                             "id",
                                             &osmium::Node::id);

    context.state.new_usertype<ExtractionNode>("ResultNode",
                                               "traffic_lights",
                                               &ExtractionNode::traffic_lights,
                                               "barrier",
                                               &ExtractionNode::barrier);

    context.state.new_usertype<guidance::RoadClassification>(
        "RoadClassification",
        "motorway_class",
        sol::property(&guidance::RoadClassification::IsMotorwayClass,
                      &guidance::RoadClassification::SetMotorwayFlag),
        "link_class",
        sol::property(&guidance::RoadClassification::IsLinkClass,
                      &guidance::RoadClassification::SetLinkClass),
        "may_be_ignored",
        sol::property(&guidance::RoadClassification::IsLowPriorityRoadClass,
                      &guidance::RoadClassification::SetLowPriorityFlag),
        "road_priority_class",
        sol::property(&guidance::RoadClassification::GetClass,
                      &guidance::RoadClassification::SetClass),
        "num_lanes",
        sol::property(&guidance::RoadClassification::GetNumberOfLanes,
                      &guidance::RoadClassification::SetNumberOfLanes));

    context.state.new_usertype<ExtractionWay>(
        "ResultWay",
        "forward_speed",
        &ExtractionWay::forward_speed,
        "backward_speed",
        &ExtractionWay::backward_speed,
        "name",
        sol::property(&ExtractionWay::GetName, &ExtractionWay::SetName),
        "ref",
        sol::property(&ExtractionWay::GetRef, &ExtractionWay::SetRef),
        "pronunciation",
        sol::property(&ExtractionWay::GetPronunciation, &ExtractionWay::SetPronunciation),
        "destinations",
        sol::property(&ExtractionWay::GetDestinations, &ExtractionWay::SetDestinations),
        "turn_lanes_forward",
        sol::property(&ExtractionWay::GetTurnLanesForward, &ExtractionWay::SetTurnLanesForward),
        "turn_lanes_backward",
        sol::property(&ExtractionWay::GetTurnLanesBackward, &ExtractionWay::SetTurnLanesBackward),
        "roundabout",
        &ExtractionWay::roundabout,
        "circular",
        &ExtractionWay::circular,
        "is_startpoint",
        &ExtractionWay::is_startpoint,
        "duration",
        &ExtractionWay::duration,
        "road_classification",
        &ExtractionWay::road_classification,
        "forward_mode",
        sol::property(&ExtractionWay::get_forward_mode, &ExtractionWay::set_forward_mode),
        "backward_mode",
        sol::property(&ExtractionWay::get_backward_mode, &ExtractionWay::set_backward_mode));

    context.state.new_usertype<osmium::WayNodeList>("WayNodeList");

    // Keep in mind .location is undefined since we're not using libosmium's location cache
    context.state.new_usertype<osmium::NodeRef>("NodeRef", "id", &osmium::NodeRef::ref);

    context.state.new_usertype<InternalExtractorEdge>("EdgeSource",
                                                      "source_coordinate",
                                                      &InternalExtractorEdge::source_coordinate,
                                                      "weight_data",
                                                      &InternalExtractorEdge::weight_data);

    context.state.new_usertype<InternalExtractorEdge::WeightData>(
        "WeightData", "speed", &InternalExtractorEdge::WeightData::speed);

    context.state.new_usertype<ExternalMemoryNode>("EdgeTarget",
                                                   "lon",
                                                   &lonToDouble<ExternalMemoryNode>,
                                                   "lat",
                                                   &latToDouble<ExternalMemoryNode>);

    context.state.new_usertype<util::Coordinate>(
        "Coordinate",
        "lon",
        sol::property(&lonToDouble<util::Coordinate>),
        "lat",
        sol::property(&latToDouble<util::Coordinate>));

    context.state.new_usertype<RasterDatum>(
        "RasterDatum", "datum", &RasterDatum::datum, "invalid_data", &RasterDatum::get_invalid);

    context.state["properties"] = &context.properties;
    context.state["sources"] = &context.sources;

    //
    // end of register block
    //

    util::luaAddScriptFolderToLoadPath(context.state.lua_state(), file_name.c_str());

    context.state.script_file(file_name);

    sol::function turn_function = context.state["turn_function"];
    sol::function node_function = context.state["node_function"];
    sol::function way_function = context.state["way_function"];
    sol::function segment_function = context.state["segment_function"];

    context.has_turn_penalty_function = turn_function.valid();
    context.has_node_function = node_function.valid();
    context.has_way_function = way_function.valid();
    context.has_segment_function = segment_function.valid();
}

const ProfileProperties &Sol2ScriptingEnvironment::GetProfileProperties()
{
    return GetSol2Context().properties;
}

Sol2ScriptingContext &Sol2ScriptingEnvironment::GetSol2Context()
{
    std::lock_guard<std::mutex> lock(init_mutex);
    bool initialized = false;
    auto &ref = script_contexts.local(initialized);
    if (!initialized)
    {
        ref = std::make_unique<Sol2ScriptingContext>();
        InitContext(*ref);
    }

    return *ref;
}

void Sol2ScriptingEnvironment::ProcessElements(
    const std::vector<osmium::memory::Buffer::const_iterator> &osm_elements,
    const RestrictionParser &restriction_parser,
    tbb::concurrent_vector<std::pair<std::size_t, ExtractionNode>> &resulting_nodes,
    tbb::concurrent_vector<std::pair<std::size_t, ExtractionWay>> &resulting_ways,
    tbb::concurrent_vector<boost::optional<InputRestrictionContainer>> &resulting_restrictions)
{
    // parse OSM entities in parallel, store in resulting vectors
    tbb::parallel_for(
        tbb::blocked_range<std::size_t>(0, osm_elements.size()),
        [&](const tbb::blocked_range<std::size_t> &range) {
            ExtractionNode result_node;
            ExtractionWay result_way;
            auto &local_context = this->GetSol2Context();

            for (auto x = range.begin(), end = range.end(); x != end; ++x)
            {
                const auto entity = osm_elements[x];

                switch (entity->type())
                {
                case osmium::item_type::node:
                    result_node.clear();
                    if (local_context.has_node_function)
                    {
                        local_context.processNode(static_cast<const osmium::Node &>(*entity),
                                                  result_node);
                    }
                    resulting_nodes.push_back(std::make_pair(x, std::move(result_node)));
                    break;
                case osmium::item_type::way:
                    result_way.clear();
                    if (local_context.has_way_function)
                    {
                        local_context.processWay(static_cast<const osmium::Way &>(*entity),
                                                 result_way);
                    }
                    resulting_ways.push_back(std::make_pair(x, std::move(result_way)));
                    break;
                case osmium::item_type::relation:
                    resulting_restrictions.push_back(restriction_parser.TryParse(
                        static_cast<const osmium::Relation &>(*entity)));
                    break;
                default:
                    break;
                }
            }
        });
}

std::vector<std::string> Sol2ScriptingEnvironment::GetNameSuffixList()
{
    auto &context = GetSol2Context();
    BOOST_ASSERT(context.state.lua_state() != nullptr);
    std::vector<std::string> suffixes_vector;

    sol::function get_name_suffix_list = context.state["get_name_suffix_list"];

    if (get_name_suffix_list.valid())
    {
        get_name_suffix_list(suffixes_vector);
    }

    return suffixes_vector;
}

std::vector<std::string> Sol2ScriptingEnvironment::GetRestrictions()
{
    auto &context = GetSol2Context();
    BOOST_ASSERT(context.state.lua_state() != nullptr);
    std::vector<std::string> restrictions;

    sol::function get_restrictions = context.state["get_restrictions"];

    if (get_restrictions.valid())
    {
        get_restrictions(restrictions);
    }

    return restrictions;
}

void Sol2ScriptingEnvironment::SetupSources()
{
    auto &context = GetSol2Context();
    BOOST_ASSERT(context.state.lua_state() != nullptr);

    sol::function source_function = context.state["source_function"];

    if (source_function.valid())
    {
        source_function();
    }
}

int32_t Sol2ScriptingEnvironment::GetTurnPenalty(const double angle)
{
    auto &context = GetSol2Context();

    sol::function turn_function = context.state["turn_function"];

    if (turn_function.valid())
    {
        const double penalty = turn_function(angle);

        BOOST_ASSERT(penalty < std::numeric_limits<int32_t>::max());
        BOOST_ASSERT(penalty > std::numeric_limits<int32_t>::min());

        return penalty;
    }

    return 0;
}

void Sol2ScriptingEnvironment::ProcessSegment(const osrm::util::Coordinate &source,
                                              const osrm::util::Coordinate &target,
                                              double distance,
                                              InternalExtractorEdge::WeightData &weight)
{
    auto &context = GetSol2Context();

    sol::function segment_function = context.state["segment_function"];

    if (segment_function.valid())
    {
        segment_function(source, target, distance, weight);
    }
}

void Sol2ScriptingContext::processNode(const osmium::Node &node, ExtractionNode &result)
{
    BOOST_ASSERT(state.lua_state() != nullptr);

    sol::function node_function = state["node_function"];

    node_function(node, result);
}

void Sol2ScriptingContext::processWay(const osmium::Way &way, ExtractionWay &result)
{
    BOOST_ASSERT(state.lua_state() != nullptr);

    sol::function way_function = state["way_function"];

    way_function(way, result);
}
}
}
