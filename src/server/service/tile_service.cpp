#include "server/service/tile_service.hpp"
#include "server/service/utils.hpp"

#include "server/api/parameters_parser.hpp"
#include "engine/api/tile_parameters.hpp"

#include "util/json_container.hpp"

#include <boost/format.hpp>

namespace osrm
{
namespace server
{
namespace service
{

engine::Status TileService::RunQuery(std::size_t prefix_length,
                                     std::string &query,
                                     osrm::engine::api::ResultT &result)
{
    auto query_iterator = query.begin();
    auto parameters =
        api::parseParameters<engine::api::TileParameters>(query_iterator, query.end());
    if (!parameters || query_iterator != query.end())
    {
        const auto position = std::distance(query.begin(), query_iterator);
        result = util::json::Object();
        auto &json_result = result.get<util::json::Object>();
        json_result.values["code"] = "InvalidQuery";
        json_result.values["message"] =
            "Query string malformed close to position " + std::to_string(prefix_length + position);
        return engine::Status::Error;
    }
    BOOST_ASSERT(parameters);

    if (!parameters->IsValid())
    {
        result = util::json::Object();
        auto &json_result = result.get<util::json::Object>();
        json_result.values["code"] = "InvalidOptions";
        json_result.values["message"] = "Invalid coodinates. Only zoomlevel 12+ is supported";
        return engine::Status::Error;
    }
    BOOST_ASSERT(parameters->IsValid());

    result = std::string();
    return BaseService::routing_machine.Tile(*parameters, result);
}
}
}
}
