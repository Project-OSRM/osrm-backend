/*

Copyright (c) 2016, Project OSRM contributors
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

#ifndef CONTRACTOR_OPTIONS_HPP
#define CONTRACTOR_OPTIONS_HPP

#include "storage/io_config.hpp"
#include "updater/updater_config.hpp"

#include <boost/filesystem/path.hpp>

#include <string>

namespace osrm
{
namespace contractor
{

struct ContractorConfig final : storage::IOConfig
{
    ContractorConfig() : requested_num_threads(0) {}

    ContractorConfig(const boost::filesystem::path &base) : requested_num_threads(0),
      IOConfig({".osrm", }, {}, {".osrm.level", ".osrm.core", ".osrm.hsgr", ".osrm.enw"})
    {
    }

    // Infer the output names from the path of the .osrm file
    void UseDefaultOutputNames()
    {
        IOConfig::UseDefaultOutputNames();
        updater_config.osrm_path = osrm_path;
        updater_config.UseDefaultOutputNames();
    }

    // TODO override IsValid to also check updater_config validity
    // TODO remove direct access to osrm_path to allow passing osrm_path to underlying configs

    updater::UpdaterConfig updater_config;

    bool use_cached_priority;

    unsigned requested_num_threads;

    // A percentage of vertices that will be contracted for the hierarchy.
    // Offers a trade-off between preprocessing and query time.
    // The remaining vertices form the core of the hierarchy
    //(e.g. 0.8 contracts 80 percent of the hierarchy, leaving a core of 20%)
    double core_factor;
};
}
}

#endif // EXTRACTOR_OPTIONS_HPP
