#ifndef OSRM_STORAGE_COMPACT_DATASET_HPP
#define OSRM_STORAGE_COMPACT_DATASET_HPP

#include "storage/tar.hpp"

#include <filesystem>
#include <string>

namespace osrm::storage
{
inline constexpr std::uint64_t SPATIAL_PACKED_DATASET_VERSION = 3;
inline constexpr auto COMPACT_DATASET_VERSION = SPATIAL_PACKED_DATASET_VERSION;
inline constexpr auto COMPACT_VERSION_ENTRY = "/common/compact_format/version";
inline constexpr auto COMPACT_INCOMPLETE_FILE = ".osrm-compact-incomplete";

inline void invalidateQueryData(const std::filesystem::path &base_path)
{
    if (!base_path.empty())
        std::filesystem::remove(base_path.string() + ".osrm.query");
}

// base_path follows IOConfig: it does not include the .osrm extension.
inline bool isCompactDataset(const std::filesystem::path &base_path)
{
    if (std::filesystem::exists(base_path.parent_path() / COMPACT_INCOMPLETE_FILE))
        throw util::exception("Dataset compaction is incomplete; prepare a new output directory.");
    const auto properties = base_path.string() + ".osrm.properties";
    if (!std::filesystem::exists(properties))
        return false;
    tar::FileReader reader(properties, tar::FileReader::VerifyFingerprint);
    const auto has_version = reader.HasEntry(COMPACT_VERSION_ENTRY);
    const auto has_count = reader.HasEntry(std::string(COMPACT_VERSION_ENTRY) + ".meta");
    if (!has_version && !has_count)
        return false;
    if (!has_version || !has_count || reader.ReadElementCount64(COMPACT_VERSION_ENTRY) != 1)
        throw util::exception("Invalid compact dataset version metadata");
    std::uint64_t version;
    reader.ReadInto(COMPACT_VERSION_ENTRY, version);
    if (version == 0 || version > COMPACT_DATASET_VERSION)
        throw util::exception("Unsupported compact dataset version " + std::to_string(version) +
                              "; rebuild the query dataset with this OSRM version.");
    return true;
}

inline void requireMutableDataset(const std::filesystem::path &base_path)
{
    if (isCompactDataset(base_path))
        throw util::exception("Query-only datasets cannot be modified. Run the preprocessing "
                              "tools on the source dataset.");
}
} // namespace osrm::storage

#endif
