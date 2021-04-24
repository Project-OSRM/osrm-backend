#ifndef OSRM_EXTRACTOR_PACKED_OSM_IDS_HPP
#define OSRM_EXTRACTOR_PACKED_OSM_IDS_HPP

#include "util/packed_vector.hpp"
#include "util/typedefs.hpp"

namespace osrm
{
namespace extractor
{
namespace detail
{
template <storage::Ownership Ownership>
using PackedOSMIDs = util::detail::PackedVector<OSMNodeID, 34, Ownership>;
}

using PackedOSMIDsView = detail::PackedOSMIDs<storage::Ownership::View>;
using PackedOSMIDs = detail::PackedOSMIDs<storage::Ownership::Container>;
} // namespace extractor
} // namespace osrm

#endif
