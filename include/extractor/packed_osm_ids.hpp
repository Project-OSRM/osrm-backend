#ifndef OSRM_EXTRACTOR_PACKED_OSM_IDS_HPP
#define OSRM_EXTRACTOR_PACKED_OSM_IDS_HPP

#include "util/exception.hpp"
#include "util/packed_vector.hpp"
#include "util/typedefs.hpp"

#include <string>

namespace osrm::extractor
{
namespace detail
{
// Bit width for packed OSM node IDs. Real-world OSM node IDs already exceed
// 2^33 and grow over time, so 34 bits is no longer sufficient and would
// silently truncate IDs >= 2^34 when written into PackedVector storage.
// 40 bits supports IDs up to ~1.1 * 10^12, providing decades of headroom.
// See https://github.com/Project-OSRM/osrm-backend/issues/7069
inline constexpr std::size_t PACKED_OSM_ID_BITS = 40;

template <storage::Ownership Ownership>
using PackedOSMIDs = util::detail::PackedVector<OSMNodeID, PACKED_OSM_ID_BITS, Ownership>;
} // namespace detail

using PackedOSMIDsView = detail::PackedOSMIDs<storage::Ownership::View>;
using PackedOSMIDs = detail::PackedOSMIDs<storage::Ownership::Container>;

// Maximum OSM node ID that can be stored in PackedOSMIDs without truncation.
inline constexpr std::uint64_t MAX_PACKED_OSM_NODE_ID =
    (std::uint64_t{1} << detail::PACKED_OSM_ID_BITS) - 1;

// Throws util::exception if `id` is too large to fit into PackedOSMIDs.
// Use this at every call site that pushes OSM node IDs into PackedOSMIDs to
// fail loudly during extraction rather than silently truncating IDs.
inline void checkPackedOSMNodeIdFits(const OSMNodeID id)
{
    if (static_cast<std::uint64_t>(id) > MAX_PACKED_OSM_NODE_ID)
    {
        throw util::exception("OSM node ID " + std::to_string(static_cast<std::uint64_t>(id)) +
                              " exceeds the " + std::to_string(detail::PACKED_OSM_ID_BITS) +
                              "-bit packed storage limit (" +
                              std::to_string(MAX_PACKED_OSM_NODE_ID) +
                              "). Increase PACKED_OSM_ID_BITS in packed_osm_ids.hpp.");
    }
}
} // namespace osrm::extractor

#endif
