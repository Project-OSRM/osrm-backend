//
// Created by robin on 4/14/16.
//

#include "server/service/isochrone_service.hpp"
#include "server/service/utils.hpp"

#include "engine/api/isochrone_parameters.hpp"
#include "server/api/parameters_parser.hpp"

#include "util/json_container.hpp"

#include <boost/format.hpp>
#include <boost/optional.hpp>

namespace osrm
{
namespace server
{
namespace service
{

namespace
{
std::string getWrongOptionHelp(const engine::api::IsochroneParameters &parameters)
{
    std::string help;

    const auto coord_size = parameters.coordinates.size();

    const bool param_size_mismatch = constrainParamSize(PARAMETER_SIZE_MISMATCH_MSG, "hints",
                                                        parameters.hints, coord_size, help) ||
                                     constrainParamSize(PARAMETER_SIZE_MISMATCH_MSG, "bearings",
                                                        parameters.bearings, coord_size, help) ||
                                     constrainParamSize(PARAMETER_SIZE_MISMATCH_MSG, "radiuses",
                                                        parameters.radiuses, coord_size, help);

    if (!param_size_mismatch && parameters.coordinates.size() < 2)
    {
        help = "Number of coordinates needs to be at least two.";
    }

    return help;
}
} // anon. ns

engine::Status IsochroneService::RunQuery(std::string &query, ResultT &result)
{
    result = util::json::Object();
    auto &json_result = result.get<util::json::Object>();

    auto query_iterator = query.begin();
    auto parameters = api::parseParameters<engine::api::IsochroneParameters>(query_iterator, query.end());
    if (!parameters || query_iterator != query.end())
    {
        const auto position = std::distance(query.begin(), query_iterator);
        json_result.values["code"] = "InvalidQuery";
        json_result.values["message"] =
            "Query string malformed close to position " + std::to_string(position);
        return engine::Status::Error;
    }
    BOOST_ASSERT(parameters);

    if (!parameters->IsValid())
    {
        json_result.values["code"] = "InvalidOptions";
        json_result.values["message"] = getWrongOptionHelp(*parameters);
        return engine::Status::Error;
    }
    BOOST_ASSERT(parameters->IsValid());

    return BaseService::routing_machine.Isochrone(*parameters, json_result);
}
}
}
}
