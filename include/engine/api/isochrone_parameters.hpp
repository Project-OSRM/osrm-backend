//
// Created by robin on 4/13/16.
//

#ifndef ENGINE_API_ISOCHRONE_PARAMETERS_HPP
#define ENGINE_API_ISOCHRONE_PARAMETERS_HPP

#include "engine/api/base_parameters.hpp"

namespace osrm
{
namespace engine
{
namespace api
{

struct IsochroneParameters : public BaseParameters
{
    unsigned number_of_results = 1;
    unsigned distance = 1000;

    bool IsValid() const {
        return BaseParameters::IsValid() && number_of_results >= 1;
    }
};
}
}
}
#endif // ENGINE_API_ISOCHRONE_PARAMETERS_HPP
