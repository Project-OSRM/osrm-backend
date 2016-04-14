//
// Created by robin on 4/13/16.
//

#ifndef ENGINE_API_ISOCHRONE_HPP
#define ENGINE_API_ISOCHRONE_HPP

#include "engine/api/base_api.hpp"
#include "engine/api/isochrone_parameters.hpp"

namespace osrm
{
namespace engine
{
namespace api
{

class IsochroneAPI final : public BaseAPI
{
  public:
    const IsochroneParameters &parameters;

    IsochroneAPI(const datafacade::BaseDataFacade &facade_, const IsochroneParameters &parameters_)
        : BaseAPI(facade_, parameters_), parameters(parameters_)
    {
    }
};
}
}
}
#endif // ENGINE_API_ISOCHRONE_HPP
