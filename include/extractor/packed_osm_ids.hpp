#ifndef OSRM_EXTRACTOR_PACKED_OSM_IDS_HPP
#define OSRM_EXTRACTOR_PACKED_OSM_IDS_HPP

#include "util/packed_vector.hpp"
#include "util/typedefs.hpp"

namespace osrm::extractor
{
namespace detail
{
template <storage::Ownership Ownership>
using PackedOSMIDs = util::detail::PackedVector<OSMNodeID, 34, Ownership>;
} // namespace detail

using PackedOSMIDsView = detail::PackedOSMIDs<storage::Ownership::View>;
using PackedOSMIDs = detail::PackedOSMIDs<storage::Ownership::Container>;
} // namespace osrm::extractor

#endif
