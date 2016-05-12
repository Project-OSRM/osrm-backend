#include "extractor/scripting_environment_lua.hpp"

#include "extractor/external_memory_node.hpp"
#include "extractor/extraction_helper_functions.hpp"
#include "extractor/extraction_node.hpp"
#include "extractor/extraction_turn.hpp"
#include "extractor/extraction_segment.hpp"
#include "extractor/extraction_way.hpp"
#include "extractor/internal_extractor_edge.hpp"
#include "extractor/profile_properties.hpp"
#include "extractor/raster_source.hpp"
#include "extractor/restriction_parser.hpp"
#include "util/exception.hpp"
#include "util/lua_util.hpp"
#include "util/simple_logger.hpp"
#include "util/typedefs.hpp"

#include <luabind/iterator_policy.hpp>
#include <luabind/operator.hpp>
#include <luabind/tag_function.hpp>

#include <osmium/osm.hpp>

#include <tbb/parallel_for.h>

#include <memory>
#include <sstream>

namespace osrm
{
namespace extractor
{
namespace
{
// wrapper method as luabind doesn't automatically overload funcs w/ default parameters
template <class T>
auto get_value_by_key(T const &object, const char *key) -> decltype(object.get_value_by_key(key))
{
    return object.get_value_by_key(key, "");
}

template <class T> double latToDouble(T const &object)
{
    return static_cast<double>(util::toFloating(object.lat));
}

template <class T> double lonToDouble(T const &object)
{
    return static_cast<double>(util::toFloating(object.lon));
}

// Luabind does not like memr funs: instead of casting to the function's signature (mem fun ptr) we
// simply wrap it
auto get_nodes_for_way(const osmium::Way &way) -> decltype(way.nodes()) { return way.nodes(); }

// Error handler
int luaErrorCallback(lua_State *state)
{
    std::string error_msg = lua_tostring(state, -1);
    std::ostringstream error_stream;
    error_stream << error_msg;
    throw util::exception("ERROR occurred in profile script:\n" + error_stream.str());
}
}

LuaScriptingEnvironment::LuaScriptingEnvironment(const std::string &file_name)
    : file_name(file_name)
{
    util::SimpleLogger().Write() << "Using script " << file_name;
}

void LuaScriptingEnvironment::InitContext(LuaScriptingContext &context)
{
    typedef double (osmium::Location::*location_member_ptr_type)() const;

    luabind::open(context.state);
    // open utility libraries string library;
    luaL_openlibs(context.state);

    util::luaAddScriptFolderToLoadPath(context.state, file_name.c_str());

    // Add our function to the state's global scope
    luabind::module(context.state)
        [luabind::def("durationIsValid", durationIsValid),
         luabind::def("parseDuration", parseDuration),
         luabind::def("trimLaneString", trimLaneString),
         luabind::def("applyAccessTokens", applyAccessTokens),
         luabind::def("canonicalizeStringList", canonicalizeStringList),
         luabind::class_<TravelMode>("mode").enum_(
             "enums")[luabind::value("inaccessible", TRAVEL_MODE_INACCESSIBLE),
                      luabind::value("driving", TRAVEL_MODE_DRIVING),
                      luabind::value("cycling", TRAVEL_MODE_CYCLING),
                      luabind::value("walking", TRAVEL_MODE_WALKING),
                      luabind::value("ferry", TRAVEL_MODE_FERRY),
                      luabind::value("train", TRAVEL_MODE_TRAIN),
                      luabind::value("pushing_bike", TRAVEL_MODE_PUSHING_BIKE),
                      luabind::value("steps_up", TRAVEL_MODE_STEPS_UP),
                      luabind::value("steps_down", TRAVEL_MODE_STEPS_DOWN),
                      luabind::value("river_up", TRAVEL_MODE_RIVER_UP),
                      luabind::value("river_down", TRAVEL_MODE_RIVER_DOWN),
                      luabind::value("route", TRAVEL_MODE_ROUTE)],

         luabind::class_<extractor::guidance::RoadPriorityClass::Enum>("road_priority_class")
             .enum_("enums")
                 [luabind::value("motorway", extractor::guidance::RoadPriorityClass::MOTORWAY),
                  luabind::value("trunk", extractor::guidance::RoadPriorityClass::TRUNK),
                  luabind::value("primary", extractor::guidance::RoadPriorityClass::PRIMARY),
                  luabind::value("secondary", extractor::guidance::RoadPriorityClass::SECONDARY),
                  luabind::value("tertiary", extractor::guidance::RoadPriorityClass::TERTIARY),
                  luabind::value("main_residential",
                                 extractor::guidance::RoadPriorityClass::MAIN_RESIDENTIAL),
                  luabind::value("side_residential",
                                 extractor::guidance::RoadPriorityClass::SIDE_RESIDENTIAL),
                  luabind::value("link_road", extractor::guidance::RoadPriorityClass::LINK_ROAD),
                  luabind::value("bike_path", extractor::guidance::RoadPriorityClass::BIKE_PATH),
                  luabind::value("foot_path", extractor::guidance::RoadPriorityClass::FOOT_PATH),
                  luabind::value("connectivity",
                                 extractor::guidance::RoadPriorityClass::CONNECTIVITY)],

         luabind::class_<SourceContainer>("sources")
             .def(luabind::constructor<>())
             .def("load", &SourceContainer::LoadRasterSource)
             .def("query", &SourceContainer::GetRasterDataFromSource)
             .def("interpolate", &SourceContainer::GetRasterInterpolateFromSource),
         luabind::class_<const float>("constants")
             .enum_("enums")[luabind::value("precision", COORDINATE_PRECISION)],

         luabind::class_<ProfileProperties>("ProfileProperties")
             .def(luabind::constructor<>())
             .property("traffic_signal_penalty",
                       &ProfileProperties::GetTrafficSignalPenalty,
                       &ProfileProperties::SetTrafficSignalPenalty)
             .property("max_speed_for_map_matching",
                       &ProfileProperties::GetMaxSpeedForMapMatching,
                       &ProfileProperties::SetMaxSpeedForMapMatching)
             .property("weight_name",
                       &ProfileProperties::GetWeightName,
                       &ProfileProperties::SetWeightName)
             .def_readwrite("use_turn_restrictions", &ProfileProperties::use_turn_restrictions)
             .def_readwrite("continue_straight_at_waypoint",
                            &ProfileProperties::continue_straight_at_waypoint)
             .def_readwrite("left_hand_driving", &ProfileProperties::left_hand_driving),

         luabind::class_<std::vector<std::string>>("vector").def(
             "Add",
             static_cast<void (std::vector<std::string>::*)(const std::string &)>(
                 &std::vector<std::string>::push_back)),

         luabind::class_<osmium::Location>("Location")
             .def<location_member_ptr_type>("lat", &osmium::Location::lat)
             .def<location_member_ptr_type>("lon", &osmium::Location::lon),

         luabind::class_<osmium::Node>("Node")
             // .def<node_member_ptr_type>("tags", &osmium::Node::tags)
             .def("location", &osmium::Node::location)
             .def("get_value_by_key", &osmium::Node::get_value_by_key)
             .def("get_value_by_key", &get_value_by_key<osmium::Node>)
             .def("id", &osmium::Node::id),

         luabind::class_<ExtractionNode>("ResultNode")
             .def_readwrite("traffic_lights", &ExtractionNode::traffic_lights)
             .def_readwrite("barrier", &ExtractionNode::barrier),

         // road classification to be set in profile
         luabind::class_<guidance::RoadClassification>("RoadClassification")
             .property("motorway_class",
                       &guidance::RoadClassification::IsMotorwayClass,
                       &guidance::RoadClassification::SetMotorwayFlag)
             .property("link_class",
                       &guidance::RoadClassification::IsLinkClass,
                       &guidance::RoadClassification::SetLinkClass)
             .property("may_be_ignored",
                       &guidance::RoadClassification::IsLowPriorityRoadClass,
                       &guidance::RoadClassification::SetLowPriorityFlag)
             .property("road_priority_class",
                       &guidance::RoadClassification::GetClass,
                       &guidance::RoadClassification::SetClass)
             .property("num_lanes",
                       &guidance::RoadClassification::GetNumberOfLanes,
                       &guidance::RoadClassification::SetNumberOfLanes),

         luabind::class_<ExtractionWay>("ResultWay")
             // .def(luabind::constructor<>())
             .def_readwrite("forward_speed", &ExtractionWay::forward_speed)
             .def_readwrite("backward_speed", &ExtractionWay::backward_speed)
             .def_readwrite("forward_weight_per_meter", &ExtractionWay::forward_weight_per_meter)
             .def_readwrite("backward_weight_per_meter", &ExtractionWay::backward_weight_per_meter)
             .def_readwrite("name", &ExtractionWay::name)
             .def_readwrite("ref", &ExtractionWay::ref)
             .def_readwrite("pronunciation", &ExtractionWay::pronunciation)
             .def_readwrite("destinations", &ExtractionWay::destinations)
             .def_readwrite("roundabout", &ExtractionWay::roundabout)
             .def_readwrite("is_access_restricted", &ExtractionWay::is_access_restricted)
             .def_readwrite("is_startpoint", &ExtractionWay::is_startpoint)
             .def_readwrite("duration", &ExtractionWay::duration)
             .def_readwrite("turn_lanes_forward", &ExtractionWay::turn_lanes_forward)
             .def_readwrite("turn_lanes_backward", &ExtractionWay::turn_lanes_backward)
             .def_readwrite("road_classification", &ExtractionWay::road_classification)
             .def_readwrite("weight", &ExtractionWay::weight)
             .property(
                 "forward_mode", &ExtractionWay::get_forward_mode, &ExtractionWay::set_forward_mode)
             .property("backward_mode",
                       &ExtractionWay::get_backward_mode,
                       &ExtractionWay::set_backward_mode),
         luabind::class_<osmium::WayNodeList>("WayNodeList").def(luabind::constructor<>()),
         luabind::class_<osmium::NodeRef>("NodeRef")
             .def(luabind::constructor<>())
             // Dear ambitious reader: registering .location() as in:
             // .def("location", +[](const osmium::NodeRef& nref){ return nref.location(); })
             // will crash at runtime, since we're not (yet?) using libosnmium's
             // NodeLocationsForWays cache
             .def("id", &osmium::NodeRef::ref),
         luabind::class_<osmium::Way>("Way")
             .def("get_value_by_key", &osmium::Way::get_value_by_key)
             .def("get_value_by_key", &get_value_by_key<osmium::Way>)
             .def("id", &osmium::Way::id)
             .def("get_nodes", get_nodes_for_way, luabind::return_stl_iterator),
         luabind::class_<util::Coordinate>("Coordinate")
             .property("lon", &lonToDouble<util::Coordinate>)
             .property("lat", &latToDouble<util::Coordinate>),
         luabind::class_<ExtractionSegment>("Segment")
             .def_readonly("source", &ExtractionSegment::source)
             .def_readonly("target", &ExtractionSegment::target)
             .def_readonly("distance", &ExtractionSegment::distance)
             .def_readwrite("duration", &ExtractionSegment::duration)
             .def_readwrite("weight", &ExtractionSegment::weight),
         luabind::class_<ExtractionTurn>("Turn")
             .def_readonly("angle", &ExtractionTurn::angle)
             .def_readonly("turn_type", &ExtractionTurn::turn_type)
             .def_readonly("direction_modifier", &ExtractionTurn::direction_modifier)
             .def_readwrite("weight", &ExtractionTurn::weight)
             .def_readwrite("duration", &ExtractionTurn::duration),
         luabind::class_<guidance::TurnType::Enum>("turn_type").enum_("enums")[
             luabind::value("invalid", extractor::guidance::TurnType::Invalid),
             luabind::value("new_name", extractor::guidance::TurnType::NewName),
             luabind::value("continue", extractor::guidance::TurnType::Continue),
             luabind::value("turn", extractor::guidance::TurnType::Turn),
             luabind::value("merge", extractor::guidance::TurnType::Merge),
             luabind::value("on_ramp", extractor::guidance::TurnType::OnRamp),
             luabind::value("off_ramp", extractor::guidance::TurnType::OffRamp),
             luabind::value("fork", extractor::guidance::TurnType::Fork),
             luabind::value("end_of_road", extractor::guidance::TurnType::EndOfRoad),
             luabind::value("notification", extractor::guidance::TurnType::Notification),
             luabind::value("enter_roundabout", extractor::guidance::TurnType::EnterRoundabout),
             luabind::value("enter_and_exit_roundabout", extractor::guidance::TurnType::EnterAndExitRoundabout),
             luabind::value("enter_rotary", extractor::guidance::TurnType::EnterRotary),
             luabind::value("enter_and_exit_rotary", extractor::guidance::TurnType::EnterAndExitRotary),
             luabind::value("enter_roundabout_intersection", extractor::guidance::TurnType::EnterRoundaboutIntersection),
             luabind::value("enter_and_exit_roundabout_intersection", extractor::guidance::TurnType::EnterAndExitRoundaboutIntersection),
             luabind::value("use_lane", extractor::guidance::TurnType::UseLane),
             luabind::value("no_turn", extractor::guidance::TurnType::NoTurn),
             luabind::value("suppressed", extractor::guidance::TurnType::Suppressed),
             luabind::value("enter_roundabout_at_exit", extractor::guidance::TurnType::EnterRoundaboutAtExit),
             luabind::value("exit_roundabout", extractor::guidance::TurnType::ExitRoundabout),
             luabind::value("enter_rotary_at_exit", extractor::guidance::TurnType::EnterRotaryAtExit),
             luabind::value("exit_rotary", extractor::guidance::TurnType::ExitRotary),
             luabind::value("enter_roundabout_intersection_at_exit", extractor::guidance::TurnType::EnterRoundaboutIntersectionAtExit),
             luabind::value("exit_roundabout_intersection", extractor::guidance::TurnType::ExitRoundaboutIntersection),
             luabind::value("stay_on_roundabout", extractor::guidance::TurnType::StayOnRoundabout),
             luabind::value("max_turn_type", extractor::guidance::TurnType::MaxTurnType)
         ],
         luabind::class_<guidance::DirectionModifier::Enum>("direction_modifier").enum_("enums")[
             luabind::value("uturn", extractor::guidance::DirectionModifier::UTurn),
             luabind::value("sharp_right", extractor::guidance::DirectionModifier::SharpRight),
             luabind::value("right", extractor::guidance::DirectionModifier::Right),
             luabind::value("slight_right", extractor::guidance::DirectionModifier::SlightRight),
             luabind::value("left", extractor::guidance::DirectionModifier::Left),
             luabind::value("sharp_left", extractor::guidance::DirectionModifier::SharpLeft),
             luabind::value("max_direction_modifier", extractor::guidance::DirectionModifier::MaxDirectionModifier)
         ],
         luabind::class_<RasterDatum>("RasterDatum")
             .def_readonly("datum", &RasterDatum::datum)
             .def("invalid_data", &RasterDatum::get_invalid)];

    luabind::globals(context.state)["properties"] = &context.properties;
    luabind::globals(context.state)["sources"] = &context.sources;

    if (0 != luaL_dofile(context.state, file_name.c_str()))
    {
        luabind::object error_msg(luabind::from_stack(context.state, -1));
        std::ostringstream error_stream;
        error_stream << error_msg;
        throw util::exception("ERROR occurred in profile script:\n" + error_stream.str());
    }

    context.has_turn_penalty_function = util::luaFunctionExists(context.state, "turn_function");
    context.has_node_function = util::luaFunctionExists(context.state, "node_function");
    context.has_way_function = util::luaFunctionExists(context.state, "way_function");
    context.has_segment_function = util::luaFunctionExists(context.state, "segment_function");
}

const ProfileProperties &LuaScriptingEnvironment::GetProfileProperties()
{
    return GetLuaContext().properties;
}

LuaScriptingContext &LuaScriptingEnvironment::GetLuaContext()
{
    std::lock_guard<std::mutex> lock(init_mutex);
    bool initialized = false;
    auto &ref = script_contexts.local(initialized);
    if (!initialized)
    {
        ref = std::make_unique<LuaScriptingContext>();
        InitContext(*ref);
    }
    luabind::set_pcall_callback(&luaErrorCallback);

    return *ref;
}

void LuaScriptingEnvironment::ProcessElements(
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
            auto &local_context = this->GetLuaContext();

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

std::vector<std::string> LuaScriptingEnvironment::GetNameSuffixList()
{
    auto &context = GetLuaContext();
    BOOST_ASSERT(context.state != nullptr);
    if (!util::luaFunctionExists(context.state, "get_name_suffix_list"))
        return {};

    std::vector<std::string> suffixes_vector;
    try
    {
        // call lua profile to compute turn penalty
        luabind::call_function<void>(
            context.state, "get_name_suffix_list", boost::ref(suffixes_vector));
    }
    catch (const luabind::error &er)
    {
        util::SimpleLogger().Write(logWARNING) << er.what();
    }

    return suffixes_vector;
}

std::vector<std::string> LuaScriptingEnvironment::GetRestrictions()
{
    auto &context = GetLuaContext();
    BOOST_ASSERT(context.state != nullptr);
    std::vector<std::string> restrictions;
    if (util::luaFunctionExists(context.state, "get_restrictions"))
    {
        // get list of turn restriction exceptions
        luabind::call_function<void>(context.state, "get_restrictions", boost::ref(restrictions));
    }
    return restrictions;
}

void LuaScriptingEnvironment::SetupSources()
{
    auto &context = GetLuaContext();
    BOOST_ASSERT(context.state != nullptr);
    if (util::luaFunctionExists(context.state, "source_function"))
    {
        luabind::call_function<void>(context.state, "source_function");
    }
}

void LuaScriptingEnvironment::ProcessTurn(ExtractionTurn &turn)
{
    auto &context = GetLuaContext();
    if (context.has_turn_penalty_function)
    {
        BOOST_ASSERT(context.state != nullptr);
        try
        {
            luabind::call_function<void>(context.state, "turn_function", boost::ref(turn));
        }
        catch (const luabind::error &er)
        {
            util::SimpleLogger().Write(logWARNING) << er.what();
        }
    }
}

void LuaScriptingEnvironment::ProcessSegment(ExtractionSegment &segment)
{
    auto &context = GetLuaContext();

    if (context.has_segment_function)
    {
        BOOST_ASSERT(context.state != nullptr);
        luabind::call_function<void>(context.state,
                                       "segment_function",
                                       boost::ref(segment));
    }

    return;
}

void LuaScriptingContext::processNode(const osmium::Node &node, ExtractionNode &result)
{
    BOOST_ASSERT(state != nullptr);
    luabind::call_function<void>(state, "node_function", boost::cref(node), boost::ref(result));
}

void LuaScriptingContext::processWay(const osmium::Way &way, ExtractionWay &result)
{
    BOOST_ASSERT(state != nullptr);
    luabind::call_function<void>(state, "way_function", boost::cref(way), boost::ref(result));
}
}
}
