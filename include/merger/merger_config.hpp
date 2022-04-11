#ifndef MERGER_CONFIG_HPP
#define MERGER_CONFIG_HPP

#include "storage/io_config.hpp"

#include <boost/filesystem/path.hpp>

#include <map>

namespace osrm
{
namespace merger
{

struct MergerConfig final : storage::IOConfig
{
    MergerConfig() noexcept
        : IOConfig(
              {
                  "",
              },
              {},
              {".osrm",
               ".osrm.restrictions",
               ".osrm.names",
               ".osrm.tls",
               ".osrm.tld",
               ".osrm.geometry",
               ".osrm.nbg_nodes",
               ".osrm.ebg_nodes",
               ".osrm.timestamp",
               ".osrm.edges",
               ".osrm.ebg",
               ".osrm.ramIndex",
               ".osrm.fileIndex",
               ".osrm.turn_duration_penalties",
               ".osrm.turn_weight_penalties",
               ".osrm.turn_penalties_index",
               ".osrm.enw",
               ".osrm.properties",
               ".osrm.icd",
               ".osrm.cnbg",
               ".osrm.cnbg_to_ebg",
               ".osrm.maneuver_overrides"})
    {
    }

    void UseDefaultOutputNames(const boost::filesystem::path &base)
    {
        IOConfig::UseDefaultOutputNames(base);
    }

    std::map<boost::filesystem::path, std::vector<boost::filesystem::path>> profile_to_input;
    boost::filesystem::path output_path;

    std::string data_version = "";

    unsigned requested_num_threads;
    unsigned small_component_size = 1000;

    bool use_metadata = false;
    bool parse_conditionals = false;
    bool use_locations_cache = true;
};

}
}

#endif
