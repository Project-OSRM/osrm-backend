#include "engine/plugins/nearest.hpp"
#include "engine/api/nearest_api.hpp"
#include "engine/api/nearest_parameters.hpp"
#include "engine/phantom_node.hpp"
#include "engine/error.hpp"
#include "util/integer_range.hpp"

#include <cstddef>
#include <string>

#include <boost/assert.hpp>
#include <boost/numeric/conversion/cast.hpp>

namespace osrm
{
namespace engine
{
namespace plugins
{

NearestPlugin::NearestPlugin(const int max_results_) : max_results{max_results_} {}

Status NearestPlugin::HandleRequest(const RoutingAlgorithmsInterface &algorithms,
                                    const api::NearestParameters &params,
                                    util::json::Object &json_result) const
{
    auto maybe_result = HandleRequest(algorithms, params);
    if (maybe_result)
    {
        json_result = api::json::toJSON(static_cast<const api::NearestResult&>(maybe_result));
        return Status::Ok;
    }
    else
    {
        json_result = api::json::toJSON(static_cast<const engine::Error&>(maybe_result));
        return Status::Error;
    }
}

MaybeResult<api::NearestResult> NearestPlugin::HandleRequest(const RoutingAlgorithmsInterface &algorithms, const api::NearestParameters &params) const
{
    BOOST_ASSERT(params.IsValid());

    auto error = CheckAlgorithms(params, algorithms);
    if (error.code != ErrorCode::NO_ERROR)
        return error;

    const auto &facade = algorithms.GetFacade();

    if (max_results > 0 &&
        (boost::numeric_cast<std::int64_t>(params.number_of_results) > max_results))
    {
        return engine::Error {ErrorCode::TOO_BIG, "Number of results " + std::to_string(params.number_of_results) +
                         " is higher than current maximum (" + std::to_string(max_results) + ")"};
    }

    if (!CheckAllCoordinates(params.coordinates))
    {
        return engine::Error {ErrorCode::INVALID_OPTIONS, "Coordinates are invalid"};
    }

    if (params.coordinates.size() != 1)
    {
        return engine::Error {ErrorCode::INVALID_OPTIONS, "Only one input coordinate is supported"};
    }

    auto phantom_nodes = GetPhantomNodes(facade, params, params.number_of_results);

    if (phantom_nodes.front().size() == 0)
    {
        return engine::Error {ErrorCode::NO_SEGMENT, "Could not find a matching segments for coordinate"};
    }
    BOOST_ASSERT(phantom_nodes.front().size() > 0);

    api::NearestAPI nearest_api(facade, params);
    auto result = nearest_api.MakeResponse(phantom_nodes);
    return result;
}
}
}
}
