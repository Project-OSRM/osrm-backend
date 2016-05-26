#include "extractor/scripting_environment.hpp"

#include "extractor/external_memory_node.hpp"
#include "extractor/extraction_helper_functions.hpp"
#include "extractor/extraction_node.hpp"
#include "extractor/extraction_way.hpp"
#include "extractor/internal_extractor_edge.hpp"
#include "extractor/profile_properties.hpp"
#include "extractor/raster_source.hpp"
#include "util/exception.hpp"
#include "util/lua_util.hpp"
#include "util/make_unique.hpp"
#include "util/simple_logger.hpp"
#include "util/typedefs.hpp"

#include <luabind/iterator_policy.hpp>
#include <luabind/operator.hpp>
#include <luabind/tag_function.hpp>

#include <osmium/osm.hpp>

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

ScriptingEnvironment::ScriptingEnvironment(const std::string &file_name) : file_name(file_name)
{
    util::SimpleLogger().Write() << "Using script " << file_name;
}

void ScriptingEnvironment::InitContext(ScriptingEnvironment::Context &context)
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
             .property("u_turn_penalty",
                       &ProfileProperties::GetUturnPenalty,
                       &ProfileProperties::SetUturnPenalty)
             .def_readwrite("use_turn_restrictions", &ProfileProperties::use_turn_restrictions)
             .def_readwrite("continue_straight_at_waypoint",
                            &ProfileProperties::continue_straight_at_waypoint),

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

         luabind::class_<ExtractionWay>("ResultWay")
             // .def(luabind::constructor<>())
             .def_readwrite("forward_speed", &ExtractionWay::forward_speed)
             .def_readwrite("backward_speed", &ExtractionWay::backward_speed)
             .def_readwrite("name", &ExtractionWay::name)
             .def_readwrite("pronunciation", &ExtractionWay::pronunciation)
             .def_readwrite("roundabout", &ExtractionWay::roundabout)
             .def_readwrite("is_access_restricted", &ExtractionWay::is_access_restricted)
             .def_readwrite("is_startpoint", &ExtractionWay::is_startpoint)
             .def_readwrite("duration", &ExtractionWay::duration)
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
         luabind::class_<InternalExtractorEdge>("EdgeSource")
             .def_readonly("source_coordinate", &InternalExtractorEdge::source_coordinate)
             .def_readwrite("weight_data", &InternalExtractorEdge::weight_data),
         luabind::class_<InternalExtractorEdge::WeightData>("WeightData")
             .def_readwrite("speed", &InternalExtractorEdge::WeightData::speed),
         luabind::class_<ExternalMemoryNode>("EdgeTarget")
             .property("lon", &lonToDouble<ExternalMemoryNode>)
             .property("lat", &latToDouble<ExternalMemoryNode>),
         luabind::class_<util::Coordinate>("Coordinate")
             .property("lon", &lonToDouble<util::Coordinate>)
             .property("lat", &latToDouble<util::Coordinate>),
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
}

ScriptingEnvironment::Context &ScriptingEnvironment::GetContex()
{
    std::lock_guard<std::mutex> lock(init_mutex);
    bool initialized = false;
    auto &ref = script_contexts.local(initialized);
    if (!initialized)
    {
        ref = util::make_unique<Context>();
        InitContext(*ref);
    }
    luabind::set_pcall_callback(&luaErrorCallback);

    return *ref;
}
}
}
