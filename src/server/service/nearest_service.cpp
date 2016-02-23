#include "server/service/nearest_service.hpp"
#include "server/service/utils.hpp"

#include "engine/api/nearest_parameters.hpp"
#include "server/api/parameters_parser.hpp"

#include "util/json_container.hpp"

#include <boost/format.hpp>

namespace osrm
{
namespace server
{
namespace service
{

namespace
{
std::string getWrongOptionHelp(const engine::api::NearestParameters &parameters)
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

engine::Status NearestService::RunQuery(std::vector<util::Coordinate> coordinates,
                                        std::string &options,
                                        util::json::Object &result)
{
    auto options_iterator = options.begin();
    auto parameters =
        api::parseParameters<engine::api::NearestParameters>(options_iterator, options.end());
    if (!parameters || options_iterator != options.end())
    {
        const auto position = std::distance(options.begin(), options_iterator);
        result.values["code"] = "invalid-options";
        result.values["message"] =
            "Options string malformed close to position " + std::to_string(position);
        return engine::Status::Error;
    }

    BOOST_ASSERT(parameters);
    parameters->coordinates = std::move(coordinates);

    if (!parameters->IsValid())
    {
        result.values["code"] = "invalid-options";
        result.values["message"] = getWrongOptionHelp(*parameters);
        return engine::Status::Error;
    }
    BOOST_ASSERT(parameters->IsValid());

    return BaseService::routing_machine.Nearest(*parameters, result);
    return Status::Error;
}
}
}
}
