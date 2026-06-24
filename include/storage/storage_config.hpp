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

#include "storage/io_config.hpp"
#include "osrm/datasets.hpp"

#include <algorithm>
#include <filesystem>
#include <istream>
#include <set>
#include <vector>

namespace osrm::storage
{

std::istream &operator>>(std::istream &in, FeatureDataset &datasets);

static bool ContainsFeatureDataset(const std::vector<storage::FeatureDataset> &feature_datasets,
                                   const storage::FeatureDataset feature_dataset)
{
    return std::find(feature_datasets.begin(), feature_datasets.end(), feature_dataset) !=
           feature_datasets.end();
}

static std::vector<storage::FeatureDataset> GetEffectiveDisabledFeatureDatasets(
    const std::vector<storage::FeatureDataset> &disabled_feature_datasets,
    const std::vector<storage::FeatureDataset> &enabled_feature_datasets)
{
    auto effective_disabled_feature_datasets = disabled_feature_datasets;
    if (!ContainsFeatureDataset(enabled_feature_datasets, FeatureDataset::ROUTE_WAY_IDS) &&
        !ContainsFeatureDataset(effective_disabled_feature_datasets, FeatureDataset::ROUTE_WAY_IDS))
    {
        effective_disabled_feature_datasets.push_back(FeatureDataset::ROUTE_WAY_IDS);
    }
    return effective_disabled_feature_datasets;
}

static std::vector<std::filesystem::path>
GetRequiredFiles(const std::vector<storage::FeatureDataset> &disabled_feature_datasets,
                 const std::vector<storage::FeatureDataset> &enabled_feature_datasets = {})
{
    const auto effective_disabled_feature_datasets =
        GetEffectiveDisabledFeatureDatasets(disabled_feature_datasets, enabled_feature_datasets);
    std::set<std::filesystem::path> required{
        ".osrm.datasource_names",
        ".osrm.ebg_nodes",
        ".osrm.edges",
        ".osrm.fileIndex",
        ".osrm.geometry",
        ".osrm.icd",
        ".osrm.maneuver_overrides",
        ".osrm.names",
        ".osrm.nbg_nodes",
        ".osrm.properties",
        ".osrm.ramIndex",
        ".osrm.timestamp",
        ".osrm.tld",
        ".osrm.tls",
        ".osrm.turn_duration_penalties",
        ".osrm.turn_weight_penalties",
    };

    for (const auto &to_disable : effective_disabled_feature_datasets)
    {
        switch (to_disable)
        {
        case FeatureDataset::ROUTE_STEPS:
            for (const auto &dataset : {".osrm.icd", ".osrm.tld", ".osrm.tls"})
            {
                required.erase(dataset);
            }
            break;
        case FeatureDataset::ROUTE_GEOMETRY:
            for (const auto &dataset :
                 {".osrm.edges", ".osrm.icd", ".osrm.names", ".osrm.tld", ".osrm.tls"})
            {
                required.erase(dataset);
            }
            break;
        case FeatureDataset::ROUTE_WAY_IDS:
            break;
        }
    }

    return std::vector<std::filesystem::path>(required.begin(), required.end());
    ;
}

/**
 * Configures OSRM's file storage paths.
 *
 * \see OSRM, EngineConfig
 */
struct StorageConfig final : IOConfig
{

    StorageConfig(const std::filesystem::path &base,
                  const std::vector<storage::FeatureDataset> &disabled_feature_datasets_ = {},
                  const std::vector<storage::FeatureDataset> &enabled_feature_datasets_ = {})
        : StorageConfig(disabled_feature_datasets_, enabled_feature_datasets_)
    {
        IOConfig::UseDefaultOutputNames(base);
    }

    StorageConfig(const std::vector<storage::FeatureDataset> &disabled_feature_datasets_ = {},
                  const std::vector<storage::FeatureDataset> &enabled_feature_datasets_ = {})
        : IOConfig(
              GetRequiredFiles(disabled_feature_datasets_, enabled_feature_datasets_),
              {".osrm.hsgr", ".osrm.cells", ".osrm.cell_metrics", ".osrm.mldgr", ".osrm.partition"},
              {}),
          disabled_feature_datasets(GetEffectiveDisabledFeatureDatasets(disabled_feature_datasets_,
                                                                        enabled_feature_datasets_))
    {
    }

    std::vector<storage::FeatureDataset> disabled_feature_datasets;
};
} // namespace osrm::storage

#endif
