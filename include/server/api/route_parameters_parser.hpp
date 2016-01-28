#ifndef SERVER_API_ROUTE_PARAMETERS_PARSER_HPP
#define SERVER_API_ROUTE_PARAMETERS_PARSER_HPP

#include "engine/api/route_parameters.hpp"
#include "server/api/route_parameters_parser.hpp"

namespace osrm
{
namespace server
{
namespace api
{

// Starts parsing and iter and modifies it until iter == end or parsing failed
boost::optional<engine::api::RouteParameters> parseRouteParameters(std::string::iterator& iter, std::string::iterator end);

// copy on purpose because we need mutability
inline boost::optional<engine::api::RouteParameters> parseRouteParameters(std::string options_string)
{
    auto iter = options_string.begin();
    return parseRouteParameters(iter, options_string.end());
}

}
}
}

#endif
