#ifndef SERVER_URL_PARSER_HPP
#define SERVER_URL_PARSER_HPP

#include "server/api/parsed_url.hpp"

#include <boost/optional.hpp>

#include <string>

namespace osrm
{
namespace server
{
namespace api
{

// Starts parsing and iter and modifies it until iter == end or parsing failed
boost::optional<ParsedURL> parseURL(std::string::iterator &iter, std::string::iterator end);
// copy on purpose because we need mutability
inline boost::optional<ParsedURL> parseURL(std::string url_string)
{
    auto iter = url_string.begin();
    return parseURL(iter, url_string.end());
}
}
}
}

#endif
