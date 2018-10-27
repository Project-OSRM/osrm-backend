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

#ifndef ENGINE_CONFIG_HPP
#define ENGINE_CONFIG_HPP

#include "storage/storage_config.hpp"

#include <boost/filesystem/path.hpp>

#include <string>

namespace osrm
{

namespace engine
{

/**
 * Configures an OSRM instance.
 *
 * You can customize the storage OSRM uses for auxiliary files specifying a storage config.
 *
 * You can further set service constraints.
 * These are the maximum number of allowed locations (-1 for unlimited) for the services:
 *  - Trip
 *  - Route
 *  - Table
 *  - Match
 *  - Nearest
 *
 * In addition, shared memory can be used for datasets loaded with osrm-datastore.
 *
 * You can chose between three algorithms:
 *  - Algorithm::CH
 *      Contraction Hierarchies, extremely fast queries but slow pre-processing. The default right
 * now.
 *  - Algorithm::CoreCH
 *      Deprecated, to be removed in v6.0
 *      Contraction Hierachies with partial contraction for faster pre-processing but slower
 * queries.
 *  - Algorithm::MLD
 *      Multi Level Dijkstra, moderately fast in both pre-processing and query.
 *
 * \see OSRM, StorageConfig
 */
struct EngineConfig final
{
    bool IsValid() const;

    enum class Algorithm
    {
        CH,
        CoreCH, // Deprecated, will be removed in v6.0
        MLD
    };

    storage::StorageConfig storage_config;
    int max_locations_trip = -1;
    int max_locations_viaroute = -1;
    int max_locations_distance_table = -1;
    int max_locations_map_matching = -1;
    double max_radius_map_matching = -1.0;
    int max_results_nearest = -1;
    int max_alternatives = 3; // set an arbitrary upper bound; can be adjusted by user
    bool use_shared_memory = true;
    boost::filesystem::path memory_file;
    bool use_mmap = true;
    Algorithm algorithm = Algorithm::CH;
    std::string verbosity;
    std::string dataset_name;
};
}
}

#endif // SERVER_CONFIG_HPP
