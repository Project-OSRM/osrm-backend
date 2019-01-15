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
using Monitor = SharedMonitor<SharedRegionRegister>;

struct RegionHandle
{
    std::unique_ptr<SharedMemory> memory;
    char *data_ptr;
    std::uint16_t shm_key;
};

RegionHandle setupRegion(SharedRegionRegister &shared_register,
                         const storage::BaseDataLayout &layout)
{
    // This is safe because we have an exclusive lock for all osrm-datastore processes.
    auto shm_key = shared_register.ReserveKey();

    // ensure that the shared memory region we want to write to is really removed
    // this is only needef for failure recovery because we actually wait for all clients
    // to detach at the end of the function
    if (storage::SharedMemory::RegionExists(shm_key))
    {
        util::Log(logWARNING) << "Old shared memory region " << (int)shm_key << " still exists.";
        util::UnbufferedLog() << "Retrying removal... ";
        storage::SharedMemory::Remove(shm_key);
        util::UnbufferedLog() << "ok.";
    }

    io::BufferWriter writer;
    serialization::write(writer, layout);
    auto encoded_static_layout = writer.GetBuffer();

    // Allocate shared memory block
    auto regions_size = encoded_static_layout.size() + layout.GetSizeOfLayout();
    util::Log() << "Data layout has a size of " << encoded_static_layout.size() << " bytes";
    util::Log() << "Allocating shared memory of " << regions_size << " bytes";
    auto memory = makeSharedMemory(shm_key, regions_size);

    // Copy memory static_layout to shared memory and populate data
    char *shared_memory_ptr = static_cast<char *>(memory->Ptr());
    auto data_ptr =
        std::copy_n(encoded_static_layout.data(), encoded_static_layout.size(), shared_memory_ptr);

    return RegionHandle{std::move(memory), data_ptr, shm_key};
}

bool swapData(Monitor &monitor,
              SharedRegionRegister &shared_register,
              const std::map<std::string, RegionHandle> &handles,
              int max_wait)
{
    std::vector<RegionHandle> old_handles;

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

                for (auto &pair : handles)
                {
                    SharedMemory::Remove(pair.second.shm_key);
                }
                return false;
            }
        }
        else
        {
            lock.lock();
        }

        for (auto &pair : handles)
        {
            auto region_id = shared_register.Find(pair.first);
            if (region_id == SharedRegionRegister::INVALID_REGION_ID)
            {
                region_id = shared_register.Register(pair.first, pair.second.shm_key);
            }
            else
            {
                auto &shared_region = shared_register.GetRegion(region_id);

                old_handles.push_back(RegionHandle{
                    makeSharedMemory(shared_region.shm_key), nullptr, shared_region.shm_key});

                shared_region.shm_key = pair.second.shm_key;
                shared_region.timestamp++;
            }
        }
    }

    util::Log() << "All data loaded. Notify all client about new data in:";
    for (const auto &pair : handles)
    {
        util::Log() << pair.first << "\t" << static_cast<int>(pair.second.shm_key);
    }
    monitor.notify_all();

    for (auto &old_handle : old_handles)
    {
        util::UnbufferedLog() << "Marking old shared memory region "
                              << static_cast<int>(old_handle.shm_key) << " for removal... ";

        // SHMCTL(2): Mark the segment to be destroyed. The segment will actually be destroyed
        // only after the last process detaches it.
        storage::SharedMemory::Remove(old_handle.shm_key);
        util::UnbufferedLog() << "ok.";

        util::UnbufferedLog() << "Waiting for clients to detach... ";
        old_handle.memory->WaitForDetach();
        util::UnbufferedLog() << " ok.";

        shared_register.ReleaseKey(old_handle.shm_key);
    }

    util::Log() << "All clients switched.";

    return true;
}
}

void populateLayoutFromFile(const boost::filesystem::path &path, storage::BaseDataLayout &layout)
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
            layout.SetBlock(entry.name, Block{number_of_elements, entry.size, entry.offset});
        }
    }
}

Storage::Storage(StorageConfig config_) : config(std::move(config_)) {}

int Storage::Run(int max_wait, const std::string &dataset_name, bool only_metric)
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

    // Populate a memory layout into stack memory
    std::vector<SharedDataIndex::AllocatedRegion> regions;
    std::map<std::string, RegionHandle> handles;

    // We keep this handles to read-only regions
    // that we don't update to be able to cross-validate
    // data when loading it
    std::vector<RegionHandle> readonly_handles;

    if (only_metric)
    {
        auto region_id = shared_register.Find(dataset_name + "/static");
        if (region_id == storage::SharedRegionRegister::INVALID_REGION_ID)
        {
            throw util::exception("Cannot update the metric to a dataset that does not exist yet.");
        }
        auto static_region = shared_register.GetRegion(region_id);
        auto static_memory = makeSharedMemory(static_region.shm_key);

        std::unique_ptr<storage::BaseDataLayout> static_layout =
            std::make_unique<storage::ContiguousDataLayout>();
        io::BufferReader reader(reinterpret_cast<char *>(static_memory->Ptr()),
                                static_memory->Size());
        serialization::read(reader, *static_layout);
        auto layout_size = reader.GetPosition();
        auto *data_ptr = reinterpret_cast<char *>(static_memory->Ptr()) + layout_size;

        regions.push_back({data_ptr, std::move(static_layout)});
        readonly_handles.push_back({std::move(static_memory), data_ptr, static_region.shm_key});
    }
    else
    {
        std::unique_ptr<storage::BaseDataLayout> static_layout =
            std::make_unique<storage::ContiguousDataLayout>();
        Storage::PopulateLayoutWithRTree(*static_layout);
        std::vector<std::pair<bool, boost::filesystem::path>> files = Storage::GetStaticFiles();
        Storage::PopulateLayout(*static_layout, files);
        auto static_handle = setupRegion(shared_register, *static_layout);
        regions.push_back({static_handle.data_ptr, std::move(static_layout)});
        handles[dataset_name + "/static"] = std::move(static_handle);
    }

    std::unique_ptr<storage::BaseDataLayout> updatable_layout =
        std::make_unique<storage::ContiguousDataLayout>();
    std::vector<std::pair<bool, boost::filesystem::path>> files = Storage::GetUpdatableFiles();
    Storage::PopulateLayout(*updatable_layout, files);
    auto updatable_handle = setupRegion(shared_register, *updatable_layout);
    regions.push_back({updatable_handle.data_ptr, std::move(updatable_layout)});
    handles[dataset_name + "/updatable"] = std::move(updatable_handle);

    SharedDataIndex index{std::move(regions)};

    if (!only_metric)
    {
        PopulateStaticData(index);
    }
    PopulateUpdatableData(index);

    swapData(monitor, shared_register, handles, max_wait);

    return EXIT_SUCCESS;
}

std::vector<std::pair<bool, boost::filesystem::path>> Storage::GetStaticFiles()
{
    constexpr bool REQUIRED = true;
    constexpr bool OPTIONAL = false;

    std::vector<std::pair<bool, boost::filesystem::path>> files = {
        {OPTIONAL, config.GetPath(".osrm.cells")},
        {OPTIONAL, config.GetPath(".osrm.partition")},
        {REQUIRED, config.GetPath(".osrm.icd")},
        {REQUIRED, config.GetPath(".osrm.properties")},
        {REQUIRED, config.GetPath(".osrm.nbg_nodes")},
        {REQUIRED, config.GetPath(".osrm.ebg_nodes")},
        {REQUIRED, config.GetPath(".osrm.tls")},
        {REQUIRED, config.GetPath(".osrm.tld")},
        {REQUIRED, config.GetPath(".osrm.timestamp")},
        {REQUIRED, config.GetPath(".osrm.maneuver_overrides")},
        {REQUIRED, config.GetPath(".osrm.edges")},
        {REQUIRED, config.GetPath(".osrm.names")},
        {REQUIRED, config.GetPath(".osrm.ramIndex")}};

    for (const auto &file : files)
    {
        if (file.first == REQUIRED && !boost::filesystem::exists(file.second))
        {
            throw util::exception("Could not find required filed: " + std::get<1>(file).string());
        }
    }

    return files;
}

std::vector<std::pair<bool, boost::filesystem::path>> Storage::GetUpdatableFiles()
{
    constexpr bool REQUIRED = true;
    constexpr bool OPTIONAL = false;

    std::vector<std::pair<bool, boost::filesystem::path>> files = {
        {OPTIONAL, config.GetPath(".osrm.mldgr")},
        {OPTIONAL, config.GetPath(".osrm.cell_metrics")},
        {OPTIONAL, config.GetPath(".osrm.hsgr")},
        {REQUIRED, config.GetPath(".osrm.datasource_names")},
        {REQUIRED, config.GetPath(".osrm.geometry")},
        {REQUIRED, config.GetPath(".osrm.turn_weight_penalties")},
        {REQUIRED, config.GetPath(".osrm.turn_duration_penalties")}};

    for (const auto &file : files)
    {
        if (file.first == REQUIRED && !boost::filesystem::exists(file.second))
        {
            throw util::exception("Could not find required filed: " + std::get<1>(file).string());
        }
    }

    return files;
}

std::string Storage::PopulateLayoutWithRTree(storage::BaseDataLayout &layout)
{
    // Figure out the path to the rtree file (it's not a tar file)
    auto absolute_file_index_path = boost::filesystem::absolute(config.GetPath(".osrm.fileIndex"));

    // Convert the boost::filesystem::path object into a plain string
    // that can then be stored as a member of an allocator object
    auto rtree_filename = absolute_file_index_path.string();

    // Here, we hardcode the special file_index_path block name.
    // The important bit here is that the "offset" is set to zero
    layout.SetBlock("/common/rtree/file_index_path", make_block<char>(rtree_filename.length() + 1));

    return rtree_filename;
}

/**
 * This function examines all our data files and figures out how much
 * memory needs to be allocated, and the position of each data structure
 * in that big block.  It updates the fields in the layout parameter.
 */
void Storage::PopulateLayout(storage::BaseDataLayout &layout,
                             const std::vector<std::pair<bool, boost::filesystem::path>> &files)
{
    for (const auto &file : files)
    {
        if (boost::filesystem::exists(file.second))
        {
            populateLayoutFromFile(file.second, layout);
        }
    }
}

void Storage::PopulateStaticData(const SharedDataIndex &index)
{
    // read actual data into shared memory object //

    // store the filename of the on-disk portion of the RTree
    {
        const auto file_index_path_ptr = index.GetBlockPtr<char>("/common/rtree/file_index_path");
        // make sure we have 0 ending
        std::fill(file_index_path_ptr,
                  file_index_path_ptr + index.GetBlockSize("/common/rtree/file_index_path"),
                  0);
        const auto absolute_file_index_path =
            boost::filesystem::absolute(config.GetPath(".osrm.fileIndex")).string();
        BOOST_ASSERT(static_cast<std::size_t>(index.GetBlockSize(
                         "/common/rtree/file_index_path")) >= absolute_file_index_path.size());
        std::copy(
            absolute_file_index_path.begin(), absolute_file_index_path.end(), file_index_path_ptr);
    }

    // Name data
    {
        auto name_table = make_name_table_view(index, "/common/names");
        extractor::files::readNames(config.GetPath(".osrm.names"), name_table);
    }

    // Timestamp mark
    {
        auto timestamp_ref = make_timestamp_view(index, "/common/timestamp");
        std::string ts;
        extractor::files::readTimestamp(config.GetPath(".osrm.timestamp"), ts);
        if (!ts.empty())
        {
            memcpy(const_cast<char *>(timestamp_ref.data()), ts.data(), ts.size());
        }
    }

    // Turn lane data
    {
        auto turn_lane_data = make_lane_data_view(index, "/common/turn_lanes");
        extractor::files::readTurnLaneData(config.GetPath(".osrm.tld"), turn_lane_data);
    }

    // Turn lane descriptions
    {
        auto views = make_turn_lane_description_views(index, "/common/turn_lanes");
        extractor::files::readTurnLaneDescriptions(
            config.GetPath(".osrm.tls"), std::get<0>(views), std::get<1>(views));
    }

    // Load edge-based nodes data
    {
        auto node_data = make_ebn_data_view(index, "/common/ebg_node_data");
        extractor::files::readNodeData(config.GetPath(".osrm.ebg_nodes"), node_data);
    }

    // Load original edge data
    {
        auto turn_data = make_turn_data_view(index, "/common/turn_data");

        auto connectivity_checksum_ptr =
            index.GetBlockPtr<std::uint32_t>("/common/connectivity_checksum");

        guidance::files::readTurnData(
            config.GetPath(".osrm.edges"), turn_data, *connectivity_checksum_ptr);
    }

    // Loading list of coordinates
    {
        auto views = make_nbn_data_view(index, "/common/nbn_data");
        extractor::files::readNodes(
            config.GetPath(".osrm.nbg_nodes"), std::get<0>(views), std::get<1>(views));
    }

    // store search tree portion of rtree
    {
        auto rtree = make_search_tree_view(index, "/common/rtree");
        extractor::files::readRamIndex(config.GetPath(".osrm.ramIndex"), rtree);
    }

    // FIXME we only need to get the weight name
    std::string metric_name;
    // load profile properties
    {
        const auto profile_properties_ptr =
            index.GetBlockPtr<extractor::ProfileProperties>("/common/properties");
        extractor::files::readProfileProperties(config.GetPath(".osrm.properties"),
                                                *profile_properties_ptr);

        metric_name = profile_properties_ptr->GetWeightName();
    }

    // Load intersection data
    {
        auto intersection_bearings_view =
            make_intersection_bearings_view(index, "/common/intersection_bearings");
        auto entry_classes = make_entry_classes_view(index, "/common/entry_classes");
        extractor::files::readIntersections(
            config.GetPath(".osrm.icd"), intersection_bearings_view, entry_classes);
    }

    if (boost::filesystem::exists(config.GetPath(".osrm.partition")))
    {
        auto mlp = make_partition_view(index, "/mld/multilevelpartition");
        partitioner::files::readPartition(config.GetPath(".osrm.partition"), mlp);
    }

    if (boost::filesystem::exists(config.GetPath(".osrm.cells")))
    {
        auto storage = make_cell_storage_view(index, "/mld/cellstorage");
        partitioner::files::readCells(config.GetPath(".osrm.cells"), storage);
    }

    if (boost::filesystem::exists(config.GetPath(".osrm.cell_metrics")))
    {
        auto exclude_metrics = make_cell_metric_view(index, "/mld/metrics/" + metric_name);
        std::unordered_map<std::string, std::vector<customizer::CellMetricView>> metrics = {
            {metric_name, std::move(exclude_metrics)},
        };
        customizer::files::readCellMetrics(config.GetPath(".osrm.cell_metrics"), metrics);
    }

    // load maneuver overrides
    {
        auto views = make_maneuver_overrides_views(index, "/common/maneuver_overrides");
        extractor::files::readManeuverOverrides(
            config.GetPath(".osrm.maneuver_overrides"), std::get<0>(views), std::get<1>(views));
    }
}

void Storage::PopulateUpdatableData(const SharedDataIndex &index)
{
    // load compressed geometry
    {
        auto segment_data = make_segment_data_view(index, "/common/segment_data");
        extractor::files::readSegmentData(config.GetPath(".osrm.geometry"), segment_data);
    }

    {
        const auto datasources_names_ptr =
            index.GetBlockPtr<extractor::Datasources>("/common/data_sources_names");
        extractor::files::readDatasources(config.GetPath(".osrm.datasource_names"),
                                          *datasources_names_ptr);
    }

    // load turn weight penalties
    {
        auto turn_duration_penalties = make_turn_weight_view(index, "/common/turn_penalty");
        extractor::files::readTurnWeightPenalty(config.GetPath(".osrm.turn_weight_penalties"),
                                                turn_duration_penalties);
    }

    // load turn duration penalties
    {
        auto turn_duration_penalties = make_turn_duration_view(index, "/common/turn_penalty");
        extractor::files::readTurnDurationPenalty(config.GetPath(".osrm.turn_duration_penalties"),
                                                  turn_duration_penalties);
    }

    // FIXME we only need to get the weight name
    std::string metric_name;
    // load profile properties
    {
        extractor::ProfileProperties properties;
        extractor::files::readProfileProperties(config.GetPath(".osrm.properties"), properties);

        metric_name = properties.GetWeightName();
    }

    if (boost::filesystem::exists(config.GetPath(".osrm.hsgr")))
    {
        const std::string metric_prefix = "/ch/metrics/" + metric_name;
        auto contracted_metric = make_contracted_metric_view(index, metric_prefix);
        std::unordered_map<std::string, contractor::ContractedMetricView> metrics = {
            {metric_name, std::move(contracted_metric)}};

        std::uint32_t graph_connectivity_checksum = 0;
        contractor::files::readGraph(
            config.GetPath(".osrm.hsgr"), metrics, graph_connectivity_checksum);

        auto turns_connectivity_checksum =
            *index.GetBlockPtr<std::uint32_t>("/common/connectivity_checksum");
        if (turns_connectivity_checksum != graph_connectivity_checksum)
        {
            throw util::exception(
                "Connectivity checksum " + std::to_string(graph_connectivity_checksum) + " in " +
                config.GetPath(".osrm.hsgr").string() + " does not equal to checksum " +
                std::to_string(turns_connectivity_checksum) + " in " +
                config.GetPath(".osrm.edges").string());
        }
    }

    if (boost::filesystem::exists(config.GetPath(".osrm.cell_metrics")))
    {
        auto exclude_metrics = make_cell_metric_view(index, "/mld/metrics/" + metric_name);
        std::unordered_map<std::string, std::vector<customizer::CellMetricView>> metrics = {
            {metric_name, std::move(exclude_metrics)},
        };
        customizer::files::readCellMetrics(config.GetPath(".osrm.cell_metrics"), metrics);
    }

    if (boost::filesystem::exists(config.GetPath(".osrm.mldgr")))
    {
        auto graph_view = make_multi_level_graph_view(index, "/mld/multilevelgraph");
        std::uint32_t graph_connectivity_checksum = 0;
        customizer::files::readGraph(
            config.GetPath(".osrm.mldgr"), graph_view, graph_connectivity_checksum);

        auto turns_connectivity_checksum =
            *index.GetBlockPtr<std::uint32_t>("/common/connectivity_checksum");
        if (turns_connectivity_checksum != graph_connectivity_checksum)
        {
            throw util::exception(
                "Connectivity checksum " + std::to_string(graph_connectivity_checksum) + " in " +
                config.GetPath(".osrm.hsgr").string() + " does not equal to checksum " +
                std::to_string(turns_connectivity_checksum) + " in " +
                config.GetPath(".osrm.edges").string());
        }
    }
}
}
}
