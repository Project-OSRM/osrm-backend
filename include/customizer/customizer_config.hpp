#ifndef OSRM_CUSTOMIZE_CUSTOMIZER_CONFIG_HPP
#define OSRM_CUSTOMIZE_CUSTOMIZER_CONFIG_HPP

#include <boost/filesystem/path.hpp>

#include <array>
#include <string>

#include "storage/io_config.hpp"
#include "updater/updater_config.hpp"

namespace osrm
{
namespace customizer
{

struct CustomizationConfig final : storage::IOConfig
{
    CustomizationConfig() : requested_num_threads(0) {}

    void UseDefaultOutputNames()
    {
        IOConfig::UseDefaultOutputNames();
        updater_config.osrm_path = osrm_path;
        updater_config.UseDefaultOutputNames();
    }

    unsigned requested_num_threads;

    updater::UpdaterConfig updater_config;
};
}
}

#endif // OSRM_CUSTOMIZE_CUSTOMIZER_CONFIG_HPP
