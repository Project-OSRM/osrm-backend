#ifndef OSRM_STORAGE_COMPACT_HPP
#define OSRM_STORAGE_COMPACT_HPP

#include <filesystem>

namespace osrm::storage
{
// Validate paths before a preprocessing stage writes any files. MLD graph and
// metric inputs may use a different base after customization with --output.
void validateCompactDatasetPaths(const std::filesystem::path &input,
                                 const std::filesystem::path &output,
                                 const std::filesystem::path &mld_input = {});

// The output's parent directory must not exist. On failure, only that newly
// created directory is removed. The input is never written to.
void compactDataset(const std::filesystem::path &input,
                    const std::filesystem::path &output,
                    const std::filesystem::path &mld_input = {});

void writeQueryData(const std::filesystem::path &input);
} // namespace osrm::storage

#endif
