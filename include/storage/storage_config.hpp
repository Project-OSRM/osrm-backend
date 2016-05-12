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

#ifndef STORAGE_CONFIG_HPP
#define STORAGE_CONFIG_HPP

#include <boost/filesystem/path.hpp>

namespace osrm
{
namespace storage
{

/**
 * Configures OSRM's file storage paths.
 *
 * \see OSRM, EngineConfig
 */
struct StorageConfig final
{
    StorageConfig() = default;

    /**
     * Constructs a storage configuration setting paths based on a base path.
     *
     * \param base The base path (e.g. france.pbf.osrm) to derive auxiliary file suffixes from.
     */
    StorageConfig(const boost::filesystem::path &base);
    bool IsValid() const;

    boost::filesystem::path ram_index_path;
    boost::filesystem::path file_index_path;
    boost::filesystem::path hsgr_data_path;
    boost::filesystem::path nodes_data_path;
    boost::filesystem::path edges_data_path;
    boost::filesystem::path core_data_path;
    boost::filesystem::path geometries_path;
    boost::filesystem::path timestamp_path;
    boost::filesystem::path turn_weight_penalties_path;
    boost::filesystem::path turn_duration_penalties_path;
    boost::filesystem::path datasource_names_path;
    boost::filesystem::path datasource_indexes_path;
    boost::filesystem::path names_data_path;
    boost::filesystem::path properties_path;
    boost::filesystem::path intersection_class_path;
    boost::filesystem::path turn_lane_data_path;
    boost::filesystem::path turn_lane_description_path;
};
}
}

#endif
