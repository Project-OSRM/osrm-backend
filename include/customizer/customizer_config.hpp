#ifndef OSRM_CUSTOMIZE_CUSTOMIZER_CONFIG_HPP
#define OSRM_CUSTOMIZE_CUSTOMIZER_CONFIG_HPP

#include <filesystem>

#include "storage/io_config.hpp"
#include "updater/updater_config.hpp"

namespace osrm::customizer
{

struct CustomizationConfig final : storage::IOConfig
{
    CustomizationConfig()
        : IOConfig({".osrm.ebg",
                    ".osrm.partition",
                    ".osrm.cells",
                    ".osrm.ebg_nodes",
                    ".osrm.properties",
                    ".osrm.enw"},
                   {},
                   {".osrm.cell_metrics", ".osrm.mldgr"}),
          requested_num_threads(0)
    {
    }

    void UseDefaultOutputNames(const std::filesystem::path &base)
    {
        IOConfig::UseDefaultOutputNames(base);
        updater_config.UseDefaultOutputNames(base);
    }

    std::filesystem::path GetOutputPath(const std::string &ext) const
    {
        // Validate the extension is configured (throws if not)
        GetPath(ext);
        if (!output_path.empty())
            return {output_path.string() + ext};
        return GetPath(ext);
    }

    std::filesystem::path output_path;
    unsigned requested_num_threads;

    updater::UpdaterConfig updater_config;
};
} // namespace osrm::customizer

#endif // OSRM_CUSTOMIZE_CUSTOMIZER_CONFIG_HPP
