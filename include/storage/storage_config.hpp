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

#ifndef STORAGE_CONFIG_HPP
#define STORAGE_CONFIG_HPP

#include <boost/filesystem/path.hpp>

#include "storage/io_config.hpp"

namespace osrm
{
namespace storage
{

/**
 * Configures OSRM's file storage paths.
 *
 * \see OSRM, EngineConfig
 */
struct StorageConfig final : IOConfig
{
    StorageConfig(const boost::filesystem::path &base) : StorageConfig()
    {
        IOConfig::UseDefaultOutputNames(base);
    }

    StorageConfig()
        : IOConfig({".osrm.ramIndex",
                    ".osrm.fileIndex",
                    ".osrm.edges",
                    ".osrm.geometry",
                    ".osrm.turn_weight_penalties",
                    ".osrm.turn_duration_penalties",
                    ".osrm.datasource_names",
                    ".osrm.names",
                    ".osrm.timestamp",
                    ".osrm.properties",
                    ".osrm.icd",
                    ".osrm.maneuver_overrides"},
                   {".osrm.hsgr",
                    ".osrm.nbg_nodes",
                    ".osrm.ebg_nodes",
                    ".osrm.cells",
                    ".osrm.cell_metrics",
                    ".osrm.mldgr",
                    ".osrm.tld",
                    ".osrm.tls",
                    ".osrm.partition"},
                   {})
    {
    }
};
} // namespace storage
} // namespace osrm

#endif
