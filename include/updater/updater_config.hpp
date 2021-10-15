/*

Copyright (c) 2017, Project OSRM contributors
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list
of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef OSRM_UPDATER_UPDATER_CONFIG_HPP
#define OSRM_UPDATER_UPDATER_CONFIG_HPP

#include <boost/filesystem/path.hpp>

#include <chrono>
#include <string>

#include "storage/io_config.hpp"
#include "storage/storage_config.hpp"

namespace osrm
{
namespace updater
{

struct UpdaterConfig final : storage::IOConfig
{
    UpdaterConfig()
        : IOConfig({".osrm.ebg",
                    ".osrm.turn_weight_penalties",
                    ".osrm.turn_duration_penalties",
                    ".osrm.turn_penalties_index",
                    ".osrm.nbg_nodes",
                    ".osrm.ebg_nodes",
                    ".osrm.geometry",
                    ".osrm.fileIndex",
                    ".osrm.properties",
                    ".osrm.restrictions",
                    ".osrm.enw"},
                   {},
                   {".osrm.datasource_names"}),
          valid_now(0)
    {
    }

    void UseDefaultOutputNames(const boost::filesystem::path &base)
    {
        IOConfig::UseDefaultOutputNames(base);
    }

    double log_edge_updates_factor;
    std::time_t valid_now;

    std::vector<std::string> segment_speed_lookup_paths;
    std::vector<std::string> turn_penalty_lookup_paths;
    std::string tz_file_path;
};
} // namespace updater
} // namespace osrm

#endif // EXTRACTOR_OPTIONS_HPP
