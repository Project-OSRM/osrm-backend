#ifndef SERVER_API_PARSED_URL_HPP
#define SERVER_API_PARSED_URL_HPP

#include "util/coordinate.hpp"

#include <boost/fusion/include/adapt_struct.hpp>

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
};

} // api
} // server
} // osrm

BOOST_FUSION_ADAPT_STRUCT(osrm::server::api::ParsedURL, service, version, profile, query)

#endif
