#include "osrm/customizer.hpp"
#include "customizer/customizer.hpp"
#include "osrm/customizer_config.hpp"

namespace osrm
{

// Pimpl-like facade

void customize(const CustomizationConfig &config) { customizer::Customizer().Run(config); }

} // ns osrm
