#ifndef OSRM_FWD_HPP
#define OSRM_FWD_HPP

// OSRM API forward declarations for usage in interfaces. Exposes forward declarations for:
// osrm::util::json::Object, osrm::engine::api::XParameters

namespace osrm
{

namespace util
{
namespace json
{
struct Object;
} // ns json
} // ns util

namespace engine
{
namespace api
{
struct RouteParameters;
struct TableParameters;
struct NearestParameters;
struct TripParameters;
struct MatchParameters;
} // ns api

class Engine;
struct EngineConfig;
} // ns engine
} // ns osrm

#endif
