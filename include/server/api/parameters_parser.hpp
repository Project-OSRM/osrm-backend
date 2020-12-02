#ifndef SERVER_API_ROUTE_PARAMETERS_PARSER_HPP
#define SERVER_API_ROUTE_PARAMETERS_PARSER_HPP

#include "engine/api/base_parameters.hpp"
#include "engine/api/tile_parameters.hpp"

#include <boost/optional/optional.hpp>

#include <type_traits>

namespace osrm
{
namespace server
{
namespace api
{

// Note: this file provides only the interface for the generic parseParameters function.
// The actual implementations for each concrete parameter type live in the cpp file.

namespace detail
{
template <typename T>
using is_parameter_t =
    std::integral_constant<bool,
                           std::is_base_of<engine::api::BaseParameters, T>::value ||
                               std::is_same<engine::api::TileParameters, T>::value>;
} // namespace detail

// Starts parsing and iter and modifies it until iter == end or parsing failed
template <typename ParameterT,
          typename std::enable_if<detail::is_parameter_t<ParameterT>::value, int>::type = 0>
boost::optional<ParameterT> parseParameters(std::string::iterator &iter,
                                            const std::string::iterator end);

// Copy on purpose because we need mutability
template <typename ParameterT,
          typename std::enable_if<detail::is_parameter_t<ParameterT>::value, int>::type = 0>
boost::optional<ParameterT> parseParameters(std::string options_string)
{
    auto first = options_string.begin();
    const auto last = options_string.end();
    return parseParameters<ParameterT>(first, last);
}

} // namespace api
} // namespace server
} // namespace osrm

#endif
