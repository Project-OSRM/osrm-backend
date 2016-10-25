#ifndef INTERNAL_DATAFACADE_HPP
#define INTERNAL_DATAFACADE_HPP

// implements all data storage when shared memory is _NOT_ used

#include "engine/datafacade/datafacade_base.hpp"

#include "extractor/guidance/turn_instruction.hpp"
#include "util/guidance/bearing_class.hpp"
#include "util/guidance/entry_class.hpp"

#include "extractor/compressed_edge_container.hpp"
#include "extractor/original_edge_data.hpp"
#include "extractor/profile_properties.hpp"
#include "extractor/query_node.hpp"
#include "storage/io.hpp"
#include "storage/storage_config.hpp"
#include "engine/geospatial_query.hpp"
#include "util/graph_loader.hpp"
#include "util/guidance/turn_bearing.hpp"
#include "util/guidance/turn_lanes.hpp"
#include "util/io.hpp"
#include "util/packed_vector.hpp"
#include "util/range_table.hpp"
#include "util/rectangle.hpp"
#include "util/shared_memory_vector_wrapper.hpp"
#include "util/simple_logger.hpp"
#include "util/static_graph.hpp"
#include "util/static_rtree.hpp"
#include "util/typedefs.hpp"

#include "osrm/coordinate.hpp"

#include <cstddef>
#include <cstdlib>

#include <algorithm>
#include <fstream>
#include <ios>
#include <limits>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <boost/assert.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/thread/tss.hpp>

#include "storage/storage.hpp"
#include "storage/shared_datatype.hpp"

namespace osrm
{
namespace engine
{
namespace datafacade
{

/**
 * This datafacade uses a process-local memory block to load
 * data into.  The logic is otherwise identical to the SharedMemoryFacade,
 * so we can just extend from that class.
 */
class InternalDataFacade final : public SharedDataFacade
{

  private:
    std::unique_ptr<char> internal_memory;
    std::unique_ptr<storage::DataLayout> internal_layout;

  public:
    explicit InternalDataFacade(const storage::StorageConfig &config)
    {
        storage::Storage storage(config);
        internal_layout = std::make_unique<storage::DataLayout>();
        data_layout = internal_layout.get();

        // Calculate the total memory size and offsets for each structure
        storage.LoadLayout(data_layout);
        // Allocate the large memory block
        internal_memory = std::make_unique<char>(internal_layout->GetSizeOfLayout());
        shared_memory = internal_memory.get();
        // Load all the datafiles into that block
        storage.LoadData(data_layout, shared_memory);

        // Datafacade is now ready for use.
    }
};
}
}
}

#endif // INTERNAL_DATAFACADE_HPP
