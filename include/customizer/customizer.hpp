#ifndef OSRM_CUSTOMIZE_CUSTOMIZER_HPP
#define OSRM_CUSTOMIZE_CUSTOMIZER_HPP

#include "customizer/customizer_config.hpp"

namespace osrm
{
namespace customize
{

class Customizer
{
  public:
    int Run(const CustomizationConfig &config);
};

} // namespace customize
} // namespace osrm

#endif // OSRM_CUSTOMIZE_CUSTOMIZER_HPP
