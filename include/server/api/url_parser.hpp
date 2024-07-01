#ifndef SERVER_URL_PARSER_HPP
#define SERVER_URL_PARSER_HPP

#include "server/api/parsed_url.hpp"

#include <optional>

#include <string>

namespace osrm::server::api
{

// Starts parsing and iter and modifies it until iter == end or parsing failed
std::optional<ParsedURL> parseURL(std::string::iterator &iter, const std::string::iterator end);

inline std::optional<ParsedURL> parseURL(std::string url_string)
{
    auto iter = url_string.begin();
    return parseURL(iter, url_string.end());
}
} // namespace osrm::server::api

#endif
