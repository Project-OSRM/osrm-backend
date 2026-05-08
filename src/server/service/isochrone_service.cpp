#include "server/service/isochrone_service.hpp"
#include "server/service/utils.hpp"

#include "server/api/parameters_parser.hpp"
#include "engine/api/isochrone_parameters.hpp"

#include "util/json_container.hpp"

namespace osrm
{
namespace server
{
namespace service
{

engine::Status
IsochroneService::RunQuery(std::size_t prefix_length, std::string &query, osrm::engine::api::ResultT &result)
{
    auto query_iterator = query.begin();
    auto parameters =
        api::parseParameters<engine::api::IsochroneParameters>(query_iterator, query.end());
    if (!parameters || query_iterator != query.end())
    {
        const auto position = std::distance(query.begin(), query_iterator);
        result = util::json::Object();
        auto &json_result = std::get<util::json::Object>(result);
        json_result.values["code"] = "InvalidQuery";
        json_result.values["message"] =
            "Query string malformed close to position " + std::to_string(prefix_length + position);
        return engine::Status::Error;
    }
    BOOST_ASSERT(parameters);

    if (!parameters->IsValid())
    {
        result = util::json::Object();
        auto &json_result = std::get<util::json::Object>(result);
        json_result.values["code"] = "InvalidOptions";
        json_result.values["message"] = "Invalid coodinates.  Range must be >= 1";
        return engine::Status::Error;
    }
    BOOST_ASSERT(parameters->IsValid());

    result = std::string();
    auto &string_result = std::get<std::string>(result);
    return BaseService::routing_machine.Isochrone(*parameters, string_result);
}
}
}
}
