#pragma once

#include "engine/engine_config.hpp"
#include "osrm/datasets.hpp"

#include <CLI/CLI.hpp>

#include <algorithm>
#include <cctype>
#include <map>
#include <string>
#include <vector>

namespace osrm::util::cli
{

inline auto algorithm_transform()
{
    return CLI::CheckedTransformer(
        std::map<std::string, engine::EngineConfig::Algorithm>{
            {"CH", engine::EngineConfig::Algorithm::CH},
            {"MLD", engine::EngineConfig::Algorithm::MLD},
        },
        CLI::ignore_case);
}

// Returns a callback for use with `app.add_option_function<std::string>(...)`.
// We use a function-style binding (rather than `add_option` with a vector and
// a CheckedTransformer) because CLI11's vector options are greedy across argv
// tokens — they would consume the positional base path as a feature-dataset
// value and reject it. A function-bound option takes exactly one token per
// invocation; repeat the flag for multiple values.
inline auto disable_feature_dataset_handler(std::vector<storage::FeatureDataset> &target)
{
    return [&target](const std::string &name)
    {
        std::string lower;
        std::transform(name.begin(),
                       name.end(),
                       std::back_inserter(lower),
                       [](unsigned char c) { return std::tolower(c); });
        if (lower == "route_steps")
        {
            target.push_back(storage::FeatureDataset::ROUTE_STEPS);
        }
        else if (lower == "route_geometry")
        {
            target.push_back(storage::FeatureDataset::ROUTE_GEOMETRY);
        }
        else
        {
            throw CLI::ValidationError("--disable-feature-dataset",
                                       "Invalid value '" + name +
                                           "'. Options: ROUTE_STEPS, ROUTE_GEOMETRY");
        }
    };
}

// Rewrites "unlimited" to "-1.0" before CLI11's numeric conversion runs.
// Apply via `->transform()`, not `->check()` — `check` runs after parsing
// and cannot mutate the input string.
inline auto unlimited_double()
{
    return CLI::Validator(
        [](std::string &input)
        {
            if (input == "unlimited")
                input = "-1.0";
            return std::string{};
        },
        "",
        "UNLIMITED_OR_DOUBLE");
}

} // namespace osrm::util::cli
