#include "server/service/trip_service.hpp"

#include "engine/api/trip_parameters.hpp"
#include "server/api/parameters_parser.hpp"

#include "util/json_container.hpp"

#include <boost/format.hpp>

namespace osrm
{
namespace server
{
namespace service
{

engine::Status TripService::RunQuery(std::vector<util::FixedPointCoordinate> coordinates,
                                     std::string &options,
                                     util::json::Object &result)
{
    // TODO(daniel-j-h)
    return Status::Error;
}
}
}
}
