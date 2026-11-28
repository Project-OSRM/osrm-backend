#include <osrm/customizer.hpp>
#include <osrm/customizer_config.hpp>

void checkInstalledCustomizationConfig()
{
    osrm::CustomizationConfig config;
    config.UseDefaultOutputNames("data.osrm");
    config.updater_config.segment_speed_lookup_paths = {"traffic.csv"};
    config.requested_num_threads = 1;
    osrm::customize(config);
}
