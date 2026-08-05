#ifndef SERVER_API_JSON_PARAMETERS_PARSER_HPP
#define SERVER_API_JSON_PARAMETERS_PARSER_HPP

#include "engine/api/base_parameters.hpp"

#include <optional>
#include <string>
#include <type_traits>

namespace osrm::server::api
{

// Note: this file provides only the interface for the generic parseJSONParameters function.
// The actual implementations for each concrete parameter type live in the cpp file.
//
// Parses a JSON request body (as sent via HTTP POST) into the parameter struct for the
// requested service. On success returns the populated parameters; on failure returns
// std::nullopt and writes a human-readable reason into `error`.
//
// The accepted JSON keys mirror the URL option names documented in docs/http.md, e.g.:
//   { "coordinates": [[lon,lat], ...], "steps": true, "annotations": ["duration"] }
// `coordinates` also accepts a "polyline(...)" / "polyline6(...)" string.
template <typename ParameterT>
    requires std::is_base_of_v<engine::api::BaseParameters, ParameterT>
std::optional<ParameterT> parseJSONParameters(const std::string &json_body, std::string &error);

} // namespace osrm::server::api

#endif
