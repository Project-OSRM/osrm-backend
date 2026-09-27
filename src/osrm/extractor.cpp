#include "osrm/extractor.hpp"
#include "extractor/driving_side_index.hpp"
#include "extractor/extractor.hpp"
#include "extractor/extractor_config.hpp"
#include "extractor/scripting_environment_lua.hpp"

namespace osrm
{

// Pimpl-like facade

void extract(const extractor::ExtractorConfig &config)
{
    // The tool validates the name; anything else reaching here is a library
    // caller that set it by hand, and off is the conservative reading.
    const auto driving_side_mode = extractor::DrivingSideIndex::ParseMode(config.driving_side_index)
                                       .value_or(extractor::DrivingSideIndex::Mode::Off);

    extractor::Sol2ScriptingEnvironment scripting_environment(
        config.profile_path.string(), config.location_dependent_data_paths, driving_side_mode);
    extractor::Extractor(config).run(scripting_environment);
}

} // namespace osrm
