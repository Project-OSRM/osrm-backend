#include "server/service/match_service.hpp"

#include "engine/api/match_parameters.hpp"
#include "server/api/parameters_parser.hpp"

#include "util/json_container.hpp"

#include <boost/format.hpp>

namespace osrm
{
namespace server
{
namespace service
{

engine::Status MatchService::RunQuery(std::vector<util::FixedPointCoordinate> coordinates,
                                      std::string &options,
                                      util::json::Object &result)
{
    // TODO(daniel-j-h)
    return Status::Error;
}
}
}
}
