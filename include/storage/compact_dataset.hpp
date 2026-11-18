#ifndef OSRM_STORAGE_COMPACT_DATASET_HPP
#define OSRM_STORAGE_COMPACT_DATASET_HPP

#include "storage/tar.hpp"

#include <filesystem>
#include <random>
#include <string>
#include <utility>

namespace osrm::storage
{
inline constexpr std::uint64_t COMPACT_DATASET_VERSION = 2;
inline constexpr auto COMPACT_VERSION_ENTRY = "/common/compact_format/version";
inline constexpr auto COMPACT_INCOMPLETE_FILE = ".osrm-compact-incomplete";
inline constexpr auto MLD_GENERATION_ENTRY = "/mld/query_generation";
inline constexpr auto CH_GENERATION_ENTRY = "/ch/query_generation";

inline std::uint64_t readQueryGeneration(const std::filesystem::path &path,
                                         const std::string &entry)
{
    if (!std::filesystem::exists(path))
        return 0;
    tar::FileReader reader(path, tar::FileReader::VerifyFingerprint);
    if (!reader.HasEntry(entry))
        return 0;
    if (!reader.HasEntry(entry + ".meta") || reader.ReadElementCount64(entry) != 1)
        throw util::exception("Invalid query generation metadata: " + path.string());
    std::uint64_t generation;
    reader.ReadInto(entry, generation);
    return generation;
}

inline std::uint64_t nextQueryGeneration(const std::filesystem::path &path,
                                         const std::string &entry)
{
    const auto previous = readQueryGeneration(path, entry);
    std::random_device random;
    std::uint64_t generation = 0;
    do
    {
        generation = (std::uint64_t(random()) << 32) | std::uint64_t(random());
    } while (generation == 0 || generation == previous);
    return generation;
}

inline bool hasQueryData(const std::filesystem::path &base_path)
{
    const auto path = base_path.string() + ".osrm.query";
    if (!std::filesystem::exists(path))
        return false;
    if (readQueryGeneration(path, COMPACT_VERSION_ENTRY) != COMPACT_DATASET_VERSION)
        throw util::exception("Unsupported compact query archive version: " + path);
    const auto generation = readQueryGeneration(path, MLD_GENERATION_ENTRY);
    const auto source_generation =
        readQueryGeneration(base_path.string() + ".osrm.mldgr", MLD_GENERATION_ENTRY);
    if (generation == 0 || generation != source_generation)
        return false;
    const auto ch_generation = readQueryGeneration(path, CH_GENERATION_ENTRY);
    const auto source_ch_generation =
        readQueryGeneration(base_path.string() + ".osrm.hsgr", CH_GENERATION_ENTRY);
    if (ch_generation != source_ch_generation ||
        (source_ch_generation == 0 && std::filesystem::exists(base_path.string() + ".osrm.hsgr")))
        return false;
    tar::FileReader query(path, tar::FileReader::VerifyFingerprint);
    for (const auto &[suffix, entry] : {std::pair{".osrm.mldgr", "/mld/connectivity_checksum"},
                                        std::pair{".osrm.hsgr", "/ch/connectivity_checksum"}})
    {
        const auto source_path = base_path.string() + suffix;
        if (!std::filesystem::exists(source_path))
            continue;
        tar::FileReader source(source_path, tar::FileReader::VerifyFingerprint);
        std::uint32_t checksum, source_checksum;
        query.ReadInto(entry, checksum);
        source.ReadInto(entry, source_checksum);
        if (checksum != source_checksum)
            return false;
    }
    return true;
}

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
        throw util::exception("Compact datasets are query-only. Update the original prepared "
                              "dataset, then run osrm-compact into a new directory.");
}
} // namespace osrm::storage

#endif
