#ifndef SERVER_API_PARSED_URL_HPP
#define SERVER_API_PARSED_URL_HPP

#include "util/coordinate.hpp"

#include <string>
#include <vector>

namespace osrm
{
namespace server
{
namespace api
{

struct ParsedURL final
{
    std::string service;
    unsigned version;
    std::string profile;
    std::string query;
    std::size_t prefix_length;
};

} // namespace api
} // namespace server
} // namespace osrm

#endif
