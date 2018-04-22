#ifndef OSRM_PARTITIONER_CONFIG_HPP
#define OSRM_PARTITIONER_CONFIG_HPP

#include <boost/filesystem/path.hpp>

#include <array>
#include <string>

#include "storage/io_config.hpp"

namespace osrm
{
namespace partitioner
{

struct PartitionerConfig final : storage::IOConfig
{
    PartitionerConfig()
        : IOConfig({".osrm", ".osrm.fileIndex", ".osrm.ebg_nodes", ".osrm.enw"},
                   {".osrm.hsgr", ".osrm.cnbg"},
                   {".osrm.ebg",
                    ".osrm.cnbg",
                    ".osrm.cnbg_to_ebg",
                    ".osrm.nbg_nodes",
                    ".osrm.partition",
                    ".osrm.cells",
                    ".osrm.maneuver_overrides"}),
          requested_num_threads(0), balance(1.2), boundary_factor(0.25), num_optimizing_cuts(10),
          small_component_size(1000),
          max_cell_sizes({128, 128 * 32, 128 * 32 * 16, 128 * 32 * 16 * 32})
    {
    }

    void UseDefaultOutputNames(const boost::filesystem::path &base)
    {
        IOConfig::UseDefaultOutputNames(base);
    }

    unsigned requested_num_threads;

    double balance;
    double boundary_factor;
    std::size_t num_optimizing_cuts;
    std::size_t small_component_size;
    std::vector<std::size_t> max_cell_sizes;
};
}
}

#endif // OSRM_PARTITIONER_CONFIG_HPP
