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

struct ParsedURL
{
    std::string service;
    unsigned version;
    std::string profile;
    std::vector<util::Coordinate> coordinates;
    std::string options;
};

}
}
}

#endif
