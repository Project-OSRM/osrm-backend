#ifndef OSRM_STORAGE_COMPACT_HPP
#define OSRM_STORAGE_COMPACT_HPP

#include <filesystem>

namespace osrm::storage
{
// The output's parent directory must not exist. On failure, only that newly
// created directory is removed. The input is never written to.
void compactDataset(const std::filesystem::path &input, const std::filesystem::path &output);
} // namespace osrm::storage

#endif
