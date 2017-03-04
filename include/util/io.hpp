#ifndef OSRM_INCLUDE_UTIL_IO_HPP_
#define OSRM_INCLUDE_UTIL_IO_HPP_

#include "util/log.hpp"

#include <boost/filesystem.hpp>
#include <boost/numeric/conversion/cast.hpp>

#include <cstddef>
#include <cstdint>

#include <bitset>
#include <fstream>
#include <stxxl/vector>
#include <vector>

#include "storage/io.hpp"
#include "util/fingerprint.hpp"

namespace osrm
{
namespace util
{

template <typename simple_type>
void deserializeAdjacencyArray(const std::string &filename,
                               std::vector<std::uint32_t> &offsets,
                               std::vector<simple_type> &data)
{
    storage::io::FileReader file(filename, storage::io::FileReader::HasNoFingerprint);

    file.DeserializeVector(offsets);
    file.DeserializeVector(data);

    // offsets have to match up with the size of the data
    if (offsets.empty() || (offsets.back() != boost::numeric_cast<std::uint32_t>(data.size())))
        throw util::exception(
            "Error in " + filename +
            (offsets.empty() ? "Offsets are empty" : "Offset and data size do not match") +
            SOURCE_REF);
}

} // namespace util
} // namespace osrm

#endif // OSRM_INCLUDE_UTIL_IO_HPP_
