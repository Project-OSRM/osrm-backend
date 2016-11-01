#ifndef INTERNAL_DATAFACADE_HPP
#define INTERNAL_DATAFACADE_HPP

// implements all data storage when shared memory is _NOT_ used

#include "engine/datafacade/memory_datafacade_base.hpp"

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
 * so we can just extend from that class.  This class holds a unique_ptr
 * to the memory blocks, so they are auto-freed upon destruction.
 */
class InternalDataFacade final : public MemoryDataFacadeBase
{

  private:
    std::unique_ptr<char[]> internal_memory;
    std::unique_ptr<storage::DataLayout> internal_layout;

  public:
    explicit InternalDataFacade(const storage::StorageConfig &config)
    {
        storage::Storage storage(config);

        // Calculate the layout/size of the memory block
        internal_layout = std::make_unique<storage::DataLayout>();
        storage.LoadLayout(internal_layout.get());

        // Allocate the memory block, then load data from files into it
        internal_memory.reset(new char[internal_layout->GetSizeOfLayout()]);
        storage.LoadData(internal_layout.get(), internal_memory.get());

        // Set up the SharedDataFacade pointers
        data_layout = internal_layout.get();
        memory_block = internal_memory.get();

        // Adjust all the private m_* members to point to the right places
        LoadData();
    }
};
}
}
}

#endif // INTERNAL_DATAFACADE_HPP
