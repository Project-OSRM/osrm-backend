#include "merger/merger.hpp"
#include "merger/merger_config.hpp"
#include "osrm/merger.hpp"

namespace osrm
{
    void merge(const merger::MergerConfig merger_config)
    {
        merger::Merger(merger_config).run();
    }
}
