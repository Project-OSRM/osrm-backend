#ifndef SERVER_API_ROUTE_PARAMETERS_PARSER_HPP
#define SERVER_API_ROUTE_PARAMETERS_PARSER_HPP

#include "engine/api/route_parameters.hpp"
#include "engine/api/table_parameters.hpp"

namespace osrm
{
namespace server
{
namespace api
{

// Starts parsing and iter and modifies it until iter == end or parsing failed
template<typename ParameterT>
boost::optional<ParameterT> parseParameters(std::string::iterator& iter, std::string::iterator end);

// copy on purpose because we need mutability
template<typename ParameterT>
inline boost::optional<ParameterT> parseParameters(std::string options_string)
{
    auto iter = options_string.begin();
    return parseParameters<ParameterT>(iter, options_string.end());
}

template<>
boost::optional<engine::api::RouteParameters> parseParameters(std::string::iterator& iter, std::string::iterator end);
template<>
boost::optional<engine::api::TableParameters> parseParameters(std::string::iterator& iter, std::string::iterator end);

}
}
}

#endif
