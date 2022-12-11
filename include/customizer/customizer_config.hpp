#ifndef OSRM_CUSTOMIZE_CUSTOMIZER_CONFIG_HPP
#define OSRM_CUSTOMIZE_CUSTOMIZER_CONFIG_HPP

#include "storage/io_config.hpp"
#include "updater/updater_config.hpp"

#include <array>
#include <filesystem>
#include <string>

namespace osrm
{
namespace customizer
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

    unsigned requested_num_threads;

    updater::UpdaterConfig updater_config;
};
} // namespace customizer
} // namespace osrm

#endif // OSRM_CUSTOMIZE_CUSTOMIZER_CONFIG_HPP
