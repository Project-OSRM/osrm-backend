#ifndef OSRM_CUSTOMIZE_CUSTOMIZER_HPP
#define OSRM_CUSTOMIZE_CUSTOMIZER_HPP

#include "customizer/customizer_config.hpp"

namespace osrm::customizer
{

class Customizer
{
  public:
    int Run(const CustomizationConfig &config);
};

} // namespace osrm::customizer

#endif // OSRM_CUSTOMIZE_CUSTOMIZER_HPP
