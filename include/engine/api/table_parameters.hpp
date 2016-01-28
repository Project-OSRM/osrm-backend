#ifndef ENGINE_API_TABLE_PARAMETERS_HPP
#define ENGINE_API_TABLE_PARAMETERS_HPP

#include "engine/api/base_parameters.hpp"

#include <vector>

namespace osrm
{
namespace engine
{
namespace api
{

struct TableParameters : public BaseParameters
{
    std::vector<bool> is_source;
    std::vector<bool> is_destination;

    bool IsValid() const;
};

}
}
}

#endif // ROUTE_PARAMETERS_HPP
