#include "osrm/contractor.hpp"
#include "contractor/contractor.hpp"
#include "contractor/contractor_config.hpp"

namespace osrm
{

// Pimpl-like facade

void contract(const contractor::ContractorConfig &config) { contractor::Contractor(config).Run(); }

} // namespace osrm
