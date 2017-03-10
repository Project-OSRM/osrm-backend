#ifndef OSRM_CUSTOMIZE_CUSTOMIZER_HPP
#define OSRM_CUSTOMIZE_CUSTOMIZER_HPP

#include "customizer/customizer_config.hpp"

namespace osrm
{
namespace customizer
{

class Customizer
{
  public:
    int Run(const CustomizationConfig &config);
};

} // namespace customizer
} // namespace osrm

#endif // OSRM_CUSTOMIZE_CUSTOMIZER_HPP
