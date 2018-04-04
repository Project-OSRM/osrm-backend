#include "storage/storage.hpp"

#include "storage/io.hpp"
#include "storage/shared_datatype.hpp"
#include "storage/shared_memory.hpp"
#include "storage/shared_memory_ownership.hpp"
#include "storage/shared_monitor.hpp"
#include "storage/view_factory.hpp"

#include "contractor/files.hpp"
#include "customizer/files.hpp"
#include "extractor/files.hpp"
#include "guidance/files.hpp"
#include "partitioner/files.hpp"

#include "util/exception.hpp"
#include "util/exception_utils.hpp"
#include "util/fingerprint.hpp"
#include "util/log.hpp"

#ifdef __linux__
#include <sys/mman.h>
#endif

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/interprocess/sync/file_lock.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

#include <cstdint>

#include <fstream>
#include <iostream>
#include <iterator>
#include <new>
#include <string>

namespace osrm
{
namespace storage
{
namespace
{
inline void readBlocks(const boost::filesystem::path &path, DataLayout &layout)
{
    tar::FileReader reader(path, tar::FileReader::VerifyFingerprint);

    std::vector<tar::FileReader::FileEntry> entries;
    reader.List(std::back_inserter(entries));

    for (const auto &entry : entries)
    {
        const auto name_end = entry.name.rfind(".meta");
        if (name_end == std::string::npos)
        {
            auto number_of_elements = reader.ReadElementCount64(entry.name);
            layout.SetBlock(entry.name, Block{number_of_elements, entry.size});
        }
    }
}
}

using Monitor = SharedMonitor<SharedRegionRegister>;

Storage::Storage(StorageConfig config_) : config(std::move(config_)) {}

int Storage::Run(int max_wait, const std::string &dataset_name)
{
    BOOST_ASSERT_MSG(config.IsValid(), "Invalid storage config");

    util::LogPolicy::GetInstance().Unmute();

    boost::filesystem::path lock_path =
        boost::filesystem::temp_directory_path() / "osrm-datastore.lock";
    if (!boost::filesystem::exists(lock_path))
    {
        boost::filesystem::ofstream ofs(lock_path);
    }

    boost::interprocess::file_lock file_lock(lock_path.string().c_str());
    boost::interprocess::scoped_lock<boost::interprocess::file_lock> datastore_lock(
        file_lock, boost::interprocess::defer_lock);

    if (!datastore_lock.try_lock())
    {
        util::UnbufferedLog(logWARNING) << "Data update in progress, waiting until it finishes... ";
        datastore_lock.lock();
        util::UnbufferedLog(logWARNING) << "ok.";
    }

#ifdef __linux__
    // try to disable swapping on Linux
    const bool lock_flags = MCL_CURRENT | MCL_FUTURE;
    if (-1 == mlockall(lock_flags))
    {
        util::Log(logWARNING) << "Could not request RAM lock";
    }
#endif

    // Get the next region ID and time stamp without locking shared barriers.
    // Because of datastore_lock the only write operation can occur sequentially later.
    Monitor monitor(SharedRegionRegister{});
    auto &shared_register = monitor.data();

    // This is safe because we have an exclusive lock for all osrm-datastore processes.
    auto shm_key = shared_register.ReserveKey();

    // ensure that the shared memory region we want to write to is really removed
    // this is only needef for failure recovery because we actually wait for all clients
    // to detach at the end of the function
    if (storage::SharedMemory::RegionExists(shm_key))
    {
        util::Log(logWARNING) << "Old shared memory region " << shm_key << " still exists.";
        util::UnbufferedLog() << "Retrying removal... ";
        storage::SharedMemory::Remove(shm_key);
        util::UnbufferedLog() << "ok.";
    }

    util::Log() << "Loading data into " << shm_key;

    // Populate a memory layout into stack memory
    DataLayout layout;
    PopulateLayout(layout);

    io::BufferWriter writer;
    serialization::write(writer, layout);
    auto encoded_layout = writer.GetBuffer();

    // Allocate shared memory block
    auto regions_size = encoded_layout.size() + layout.GetSizeOfLayout();
    util::Log() << "Data layout has a size of " << encoded_layout.size() << " bytes";
    util::Log() << "Allocating shared memory of " << regions_size << " bytes";
    auto data_memory = makeSharedMemory(shm_key, regions_size);

    // Copy memory layout to shared memory and populate data
    char *shared_memory_ptr = static_cast<char *>(data_memory->Ptr());
    std::copy_n(encoded_layout.data(), encoded_layout.size(), shared_memory_ptr);
    PopulateData(layout, shared_memory_ptr + encoded_layout.size());

    std::uint32_t next_timestamp = 0;
    std::uint8_t in_use_key = SharedRegionRegister::MAX_SHM_KEYS;

    { // Lock for write access shared region mutex
        boost::interprocess::scoped_lock<Monitor::mutex_type> lock(monitor.get_mutex(),
                                                                   boost::interprocess::defer_lock);

        if (max_wait >= 0)
        {
            if (!lock.timed_lock(boost::posix_time::microsec_clock::universal_time() +
                                 boost::posix_time::seconds(max_wait)))
            {
                util::Log(logERROR) << "Could not aquire current region lock after " << max_wait
                                    << " seconds. Data update failed.";
                SharedMemory::Remove(shm_key);
                return EXIT_FAILURE;
            }
        }
        else
        {
            lock.lock();
        }

        auto region_id = shared_register.Find(dataset_name + "/data");
        if (region_id == SharedRegionRegister::INVALID_REGION_ID)
        {
            region_id = shared_register.Register(dataset_name + "/data", shm_key);
        }
        else
        {
            auto &shared_region = shared_register.GetRegion(region_id);
            next_timestamp = shared_region.timestamp + 1;
            in_use_key = shared_region.shm_key;
            shared_region.shm_key = shm_key;
            shared_region.timestamp = next_timestamp;
        }
    }

    util::Log() << "All data loaded. Notify all client about new data in " << shm_key
                << " with timestamp " << next_timestamp;
    monitor.notify_all();

    // SHMCTL(2): Mark the segment to be destroyed. The segment will actually be destroyed
    // only after the last process detaches it.
    if (in_use_key != SharedRegionRegister::MAX_SHM_KEYS &&
        storage::SharedMemory::RegionExists(in_use_key))
    {
        util::UnbufferedLog() << "Marking old shared memory region " << in_use_key
                              << " for removal... ";

        // aquire a handle for the old shared memory region before we mark it for deletion
        // we will need this to wait for all users to detach
        auto in_use_shared_memory = makeSharedMemory(in_use_key);

        storage::SharedMemory::Remove(in_use_key);
        util::UnbufferedLog() << "ok.";

        util::UnbufferedLog() << "Waiting for clients to detach... ";
        in_use_shared_memory->WaitForDetach();
        util::UnbufferedLog() << " ok.";

        shared_register.ReleaseKey(in_use_key);
    }

    util::Log() << "All clients switched.";

    return EXIT_SUCCESS;
}

/**
 * This function examines all our data files and figures out how much
 * memory needs to be allocated, and the position of each data structure
 * in that big block.  It updates the fields in the DataLayout parameter.
 */
void Storage::PopulateLayout(DataLayout &layout)
{
    {
        auto absolute_file_index_path =
            boost::filesystem::absolute(config.GetPath(".osrm.fileIndex"));

        layout.SetBlock("/common/rtree/file_index_path",
                        make_block<char>(absolute_file_index_path.string().length() + 1));
    }

    constexpr bool REQUIRED = true;
    constexpr bool OPTIONAL = false;
    std::vector<std::pair<bool, boost::filesystem::path>> tar_files = {
        {OPTIONAL, config.GetPath(".osrm.mldgr")},
        {OPTIONAL, config.GetPath(".osrm.cells")},
        {OPTIONAL, config.GetPath(".osrm.partition")},
        {OPTIONAL, config.GetPath(".osrm.cell_metrics")},
        {OPTIONAL, config.GetPath(".osrm.hsgr")},
        {REQUIRED, config.GetPath(".osrm.icd")},
        {REQUIRED, config.GetPath(".osrm.properties")},
        {REQUIRED, config.GetPath(".osrm.nbg_nodes")},
        {REQUIRED, config.GetPath(".osrm.datasource_names")},
        {REQUIRED, config.GetPath(".osrm.geometry")},
        {REQUIRED, config.GetPath(".osrm.ebg_nodes")},
        {REQUIRED, config.GetPath(".osrm.tls")},
        {REQUIRED, config.GetPath(".osrm.tld")},
        {REQUIRED, config.GetPath(".osrm.maneuver_overrides")},
        {REQUIRED, config.GetPath(".osrm.turn_weight_penalties")},
        {REQUIRED, config.GetPath(".osrm.turn_duration_penalties")},
        {REQUIRED, config.GetPath(".osrm.edges")},
        {REQUIRED, config.GetPath(".osrm.names")},
        {REQUIRED, config.GetPath(".osrm.ramIndex")},
    };

    for (const auto &file : tar_files)
    {
        if (boost::filesystem::exists(file.second))
        {
            readBlocks(file.second, layout);
        }
        else
        {
            if (file.first == REQUIRED)
            {
                throw util::exception("Could not find required filed: " +
                                      std::get<1>(file).string());
            }
        }
    }
}

void Storage::PopulateData(const DataLayout &layout, char *memory_ptr)
{
    BOOST_ASSERT(memory_ptr != nullptr);

    // Connectivity matrix checksum
    std::uint32_t turns_connectivity_checksum = 0;

    // read actual data into shared memory object //

    // store the filename of the on-disk portion of the RTree
    {
        const auto file_index_path_ptr =
            layout.GetBlockPtr<char>(memory_ptr, "/common/rtree/file_index_path");
        // make sure we have 0 ending
        std::fill(file_index_path_ptr,
                  file_index_path_ptr + layout.GetBlockSize("/common/rtree/file_index_path"),
                  0);
        const auto absolute_file_index_path =
            boost::filesystem::absolute(config.GetPath(".osrm.fileIndex")).string();
        BOOST_ASSERT(static_cast<std::size_t>(layout.GetBlockSize(
                         "/common/rtree/file_index_path")) >= absolute_file_index_path.size());
        std::copy(
            absolute_file_index_path.begin(), absolute_file_index_path.end(), file_index_path_ptr);
    }

    // Name data
    {
        auto name_table = make_name_table_view(memory_ptr, layout, "/common/names");
        extractor::files::readNames(config.GetPath(".osrm.names"), name_table);
    }

    // Turn lane data
    {
        auto turn_lane_data = make_lane_data_view(memory_ptr, layout, "/common/turn_lanes");
        extractor::files::readTurnLaneData(config.GetPath(".osrm.tld"), turn_lane_data);
    }

    // Turn lane descriptions
    {
        auto views = make_turn_lane_description_views(memory_ptr, layout, "/common/turn_lanes");
        extractor::files::readTurnLaneDescriptions(
            config.GetPath(".osrm.tls"), std::get<0>(views), std::get<1>(views));
    }

    // Load edge-based nodes data
    {
        auto node_data = make_ebn_data_view(memory_ptr, layout, "/common/ebg_node_data");
        extractor::files::readNodeData(config.GetPath(".osrm.ebg_nodes"), node_data);
    }

    // Load original edge data
    {
        auto turn_data = make_turn_data_view(memory_ptr, layout, "/common/turn_data");

        auto connectivity_checksum_ptr =
            layout.GetBlockPtr<std::uint32_t>(memory_ptr, "/common/connectivity_checksum");

        guidance::files::readTurnData(
            config.GetPath(".osrm.edges"), turn_data, *connectivity_checksum_ptr);

        turns_connectivity_checksum = *connectivity_checksum_ptr;
    }

    // load compressed geometry
    {
        auto segment_data = make_segment_data_view(memory_ptr, layout, "/common/segment_data");
        extractor::files::readSegmentData(config.GetPath(".osrm.geometry"), segment_data);
    }

    {
        const auto datasources_names_ptr =
            layout.GetBlockPtr<extractor::Datasources>(memory_ptr, "/common/data_sources_names");
        extractor::files::readDatasources(config.GetPath(".osrm.datasource_names"),
                                          *datasources_names_ptr);
    }

    // Loading list of coordinates
    {
        auto views = make_nbn_data_view(memory_ptr, layout, "/common/nbn_data");
        extractor::files::readNodes(
            config.GetPath(".osrm.nbg_nodes"), std::get<0>(views), std::get<1>(views));
    }

    // load turn weight penalties
    {
        auto turn_duration_penalties =
            make_turn_weight_view(memory_ptr, layout, "/common/turn_penalty");
        extractor::files::readTurnWeightPenalty(config.GetPath(".osrm.turn_weight_penalties"),
                                                turn_duration_penalties);
    }

    // load turn duration penalties
    {
        auto turn_duration_penalties =
            make_turn_duration_view(memory_ptr, layout, "/common/turn_penalty");
        extractor::files::readTurnDurationPenalty(config.GetPath(".osrm.turn_duration_penalties"),
                                                  turn_duration_penalties);
    }

    // store search tree portion of rtree
    {
        auto rtree = make_search_tree_view(memory_ptr, layout, "/common/rtree");
        extractor::files::readRamIndex(config.GetPath(".osrm.ramIndex"), rtree);
    }

    // FIXME we only need to get the weight name
    std::string metric_name;
    // load profile properties
    {
        const auto profile_properties_ptr =
            layout.GetBlockPtr<extractor::ProfileProperties>(memory_ptr, "/common/properties");
        extractor::files::readProfileProperties(config.GetPath(".osrm.properties"),
                                                *profile_properties_ptr);

        metric_name = profile_properties_ptr->GetWeightName();
    }

    // Load intersection data
    {
        auto intersection_bearings_view =
            make_intersection_bearings_view(memory_ptr, layout, "/common/intersection_bearings");
        auto entry_classes = make_entry_classes_view(memory_ptr, layout, "/common/entry_classes");
        extractor::files::readIntersections(
            config.GetPath(".osrm.icd"), intersection_bearings_view, entry_classes);
    }

    if (boost::filesystem::exists(config.GetPath(".osrm.hsgr")))
    {
        const std::string metric_prefix = "/ch/metrics/" + metric_name;
        auto contracted_metric = make_contracted_metric_view(memory_ptr, layout, metric_prefix);
        std::unordered_map<std::string, contractor::ContractedMetricView> metrics = {
            {metric_name, std::move(contracted_metric)}};

        std::uint32_t graph_connectivity_checksum = 0;
        contractor::files::readGraph(
            config.GetPath(".osrm.hsgr"), metrics, graph_connectivity_checksum);

        if (turns_connectivity_checksum != graph_connectivity_checksum)
        {
            throw util::exception(
                "Connectivity checksum " + std::to_string(graph_connectivity_checksum) + " in " +
                config.GetPath(".osrm.hsgr").string() + " does not equal to checksum " +
                std::to_string(turns_connectivity_checksum) + " in " +
                config.GetPath(".osrm.edges").string());
        }
    }

    if (boost::filesystem::exists(config.GetPath(".osrm.partition")))
    {
        auto mlp = make_partition_view(memory_ptr, layout, "/mld/multilevelpartition");
        partitioner::files::readPartition(config.GetPath(".osrm.partition"), mlp);
    }

    if (boost::filesystem::exists(config.GetPath(".osrm.cells")))
    {
        auto storage = make_cell_storage_view(memory_ptr, layout, "/mld/cellstorage");
        partitioner::files::readCells(config.GetPath(".osrm.cells"), storage);
    }

    if (boost::filesystem::exists(config.GetPath(".osrm.cell_metrics")))
    {
        auto exclude_metrics =
            make_cell_metric_view(memory_ptr, layout, "/mld/metrics/" + metric_name);
        std::unordered_map<std::string, std::vector<customizer::CellMetricView>> metrics = {
            {metric_name, std::move(exclude_metrics)},
        };
        customizer::files::readCellMetrics(config.GetPath(".osrm.cell_metrics"), metrics);
    }

    if (boost::filesystem::exists(config.GetPath(".osrm.mldgr")))
    {
        auto graph_view = make_multi_level_graph_view(memory_ptr, layout, "/mld/multilevelgraph");
        std::uint32_t graph_connectivity_checksum = 0;
        partitioner::files::readGraph(
            config.GetPath(".osrm.mldgr"), graph_view, graph_connectivity_checksum);

        if (turns_connectivity_checksum != graph_connectivity_checksum)
        {
            throw util::exception(
                "Connectivity checksum " + std::to_string(graph_connectivity_checksum) + " in " +
                config.GetPath(".osrm.mldgr").string() + " does not equal to checksum " +
                std::to_string(turns_connectivity_checksum) + " in " +
                config.GetPath(".osrm.edges").string());
        }
    }

    // load maneuver overrides
    {
        auto views =
            make_maneuver_overrides_views(memory_ptr, layout, "/common/maneuver_overrides");
        extractor::files::readManeuverOverrides(
            config.GetPath(".osrm.maneuver_overrides"), std::get<0>(views), std::get<1>(views));
    }
}
}
}
