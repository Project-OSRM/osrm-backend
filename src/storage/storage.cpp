#include "storage/storage.hpp"

#include "storage/compact_dataset.hpp"
#include "storage/io.hpp"
#include "storage/serialization.hpp"
#include "storage/shared_datatype.hpp"
#include "storage/shared_memory.hpp"
#include "storage/shared_monitor.hpp"
#include "storage/tar.hpp"

#include "util/exception.hpp"
#include "util/log.hpp"

#ifdef __linux__
#include <sys/mman.h>
#endif

#include <boost/interprocess/sync/file_lock.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <chrono>
#include <thread>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

namespace osrm::storage
{
namespace
{
using Monitor = SharedMonitor<SharedRegionRegister>;

struct RegionHandle
{
    std::unique_ptr<SharedMemory> memory;
    char *data_ptr;
    ProjID proj_id;
};

RegionHandle setupRegion(SharedRegionRegister &shared_register,
                         const storage::BaseDataLayout &layout)
{
    // This is safe because we have an exclusive lock for all osrm-datastore processes.
    ProjID proj_id = shared_register.ReserveKey();

    // ensure that the shared memory region we want to write to is really removed
    // this is only needed for failure recovery because we actually wait for all clients
    // to detach at the end of the function
    if (storage::RegionExists(proj_id))
    {
        util::Log(logWARNING) << "Old shared memory region " << (int)proj_id << " still exists.";
        util::UnbufferedLog() << "Retrying removal of shared memory region " << (int)proj_id;
        storage::Remove(proj_id);
        storage::WaitForDetach(proj_id, 5000);
        util::UnbufferedLog() << "ok.";
    }

    io::BufferWriter writer;
    serialization::write(writer, layout);
    auto encoded_static_layout = writer.GetBuffer();

    // Allocate shared memory block
    auto regions_size = encoded_static_layout.size() + layout.GetSizeOfLayout();
    util::Log() << "Data layout has a size of " << encoded_static_layout.size() << " bytes";
    util::Log() << "Allocating shared memory of " << regions_size << " bytes";
    auto memory = makeSharedMemory(proj_id, regions_size);

    // Copy memory static_layout to shared memory and populate data
    char *shared_memory_ptr = static_cast<char *>(memory->Ptr());
    auto data_ptr =
        std::copy_n(encoded_static_layout.data(), encoded_static_layout.size(), shared_memory_ptr);

    return RegionHandle{std::move(memory), data_ptr, proj_id};
}

void populateArchive(const SharedDataIndex &index, const std::filesystem::path &path)
{
    tar::FileReader reader(path, tar::FileReader::VerifyFingerprint);
    std::vector<tar::FileReader::FileEntry> entries;
    reader.List(std::back_inserter(entries));
    for (const auto &entry : entries)
    {
        if (entry.name.ends_with(".meta") || entry.name == "/ch/connectivity_checksum" ||
            entry.name == "/mld/connectivity_checksum" || !index.HasBlock(entry.name))
            continue;
        reader.ReadInto(
            entry.name, index.GetBlockPtr<char>(entry.name), index.GetBlockSize(entry.name));
    }
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
            auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(max_wait);
            while (!lock.try_lock())
            {
                if (std::chrono::steady_clock::now() >= deadline)
                {
                    util::Log(logERROR) << "Could not acquire current region lock after "
                                        << max_wait << " seconds. Data update failed.";

                    for (auto &pair : handles)
                    {
                        Remove(pair.second.proj_id);
                    }
                    return false;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
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
                region_id = shared_register.Register(pair.first, pair.second.proj_id);
            }
            else
            {
                auto &shared_region = shared_register.GetRegion(region_id);

                old_handles.push_back(RegionHandle{
                    makeSharedMemory(shared_region.proj_id), nullptr, shared_region.proj_id});

                shared_region.proj_id = pair.second.proj_id;
                shared_region.timestamp++;
            }
        }
    }

    util::Log() << "All data loaded. Notify all clients about new data in:";
    for (const auto &pair : handles)
    {
        util::Log() << pair.first << "\t" << static_cast<int>(pair.second.proj_id);
    }
    monitor.notify_all();

    for (auto &old_handle : old_handles)
    {
        util::UnbufferedLog() << "Marking old shared memory region "
                              << static_cast<int>(old_handle.proj_id) << " for removal... ";

        // SHMCTL(2): Mark the segment to be destroyed. The segment will actually be destroyed
        // only after the last process detaches it.
        if (storage::Remove(old_handle.proj_id))
        {
            util::UnbufferedLog() << "waiting for clients to detach...";
            storage::WaitForDetach(old_handle.proj_id, 5000);
            util::UnbufferedLog() << "ok.";
        }
        else
        {
            util::UnbufferedLog() << "failed.";
        }
        shared_register.ReleaseKey(old_handle.proj_id);
    }

    util::Log() << "All clients switched.";

    return true;
}
} // namespace

void populateLayoutFromFile(const std::filesystem::path &path,
                            storage::BaseDataLayout &layout,
                            const std::function<bool(const std::string &)> &include)
{
    tar::FileReader reader(path, tar::FileReader::VerifyFingerprint);

    std::vector<tar::FileReader::FileEntry> entries;
    reader.List(std::back_inserter(entries));

    for (const auto &entry : entries)
    {
        if (entry.name == COMPACT_VERSION_ENTRY)
            continue;
        if (include && !include(entry.name))
            continue;
        const auto name_end = entry.name.rfind(".meta");
        if (name_end == std::string::npos)
        {
            auto number_of_elements = reader.ReadElementCount64(entry.name);
            layout.SetBlock(entry.name, Block{number_of_elements, entry.size, entry.offset});
        }
    }
}

Storage::Storage(StorageConfig config_)
    : config(std::move(config_)), m_query_data(osrm::storage::hasQueryData(config.base_path))
{
}

int Storage::Run(int max_wait, const std::string &dataset_name, bool only_metric)
{
    const auto compact = isCompactDataset(config.base_path);
    BOOST_ASSERT_MSG(config.IsValid(), "Invalid storage config");

    util::LogPolicy::GetInstance().Unmute();

    std::filesystem::path lock_path = storage::getLockDir() / "osrm-datastore.lock";
    if (!std::filesystem::exists(lock_path))
    {
        std::ofstream ofs(lock_path);
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
    if (only_metric && (m_query_data || compact))
    {
        if (shared_register.Find(dataset_name + "/static") ==
            storage::SharedRegionRegister::INVALID_REGION_ID)
            throw util::exception("Cannot update the metric to a dataset that does not exist yet.");
        only_metric = false;
    }

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
        auto static_memory = makeSharedMemory(static_region.proj_id);

        std::unique_ptr<storage::BaseDataLayout> static_layout =
            std::make_unique<storage::ContiguousDataLayout>();
        io::BufferReader reader(reinterpret_cast<char *>(static_memory->Ptr()),
                                static_memory->Size());
        serialization::read(reader, *static_layout);
        auto layout_size = reader.GetPosition();
        auto *data_ptr = reinterpret_cast<char *>(static_memory->Ptr()) + layout_size;

        if (!static_layout->HasBlock("/common/ebg_node_data/nodes"))
            only_metric = false;
        if (only_metric)
        {
            regions.push_back({data_ptr, std::move(static_layout)});
            readonly_handles.push_back({std::move(static_memory), data_ptr, static_region.proj_id});
        }
    }
    if (!only_metric)
    {
        std::unique_ptr<storage::BaseDataLayout> static_layout =
            std::make_unique<storage::ContiguousDataLayout>();
        Storage::PopulateLayoutWithRTree(*static_layout);
        Storage::PopulateLayoutWithOpenAreaRTree(*static_layout);
        std::vector<std::pair<bool, std::filesystem::path>> files = Storage::GetStaticFiles();
        Storage::PopulateLayout(*static_layout, files);
        auto static_handle = setupRegion(shared_register, *static_layout);
        regions.push_back({static_handle.data_ptr, std::move(static_layout)});
        handles[dataset_name + "/static"] = std::move(static_handle);
    }

    std::unique_ptr<storage::BaseDataLayout> updatable_layout =
        std::make_unique<storage::ContiguousDataLayout>();
    std::vector<std::pair<bool, std::filesystem::path>> files = Storage::GetUpdatableFiles();
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

std::vector<std::pair<bool, std::filesystem::path>> Storage::GetStaticFiles()
{
    constexpr bool IS_REQUIRED = true;
    constexpr bool IS_OPTIONAL = false;

    std::vector<std::pair<bool, std::filesystem::path>> files = {
        {IS_OPTIONAL, config.GetPath(".osrm.cells")},
        {IS_OPTIONAL, config.GetPath(".osrm.partition")},
        // only present when the profile meshed pedestrian areas
        {IS_OPTIONAL, config.GetPath(".osrm.openareas")},
        {std::filesystem::exists(config.GetPath(".osrm.openareas")),
         config.GetPath(".osrm.openareas.ramIndex")},
        {IS_REQUIRED, config.GetPath(".osrm.ebg_nodes")},
        {IS_REQUIRED, config.GetPath(".osrm.maneuver_overrides")},
        {IS_REQUIRED, config.GetPath(".osrm.nbg_nodes")},
        {IS_REQUIRED, config.GetPath(".osrm.ramIndex")},
        {IS_REQUIRED, config.GetPath(".osrm.properties")},
        {IS_REQUIRED, config.GetPath(".osrm.timestamp")}};

    for (const auto &file : {".osrm.edges", ".osrm.names", ".osrm.icd", ".osrm.tls", ".osrm.tld"})
    {
        if (config.IsRequiredConfiguredInput(file))
        {
            files.push_back({IS_REQUIRED, config.GetPath(file)});
        }
    }

    for (const auto &file : files)
    {
        if (file.first == IS_REQUIRED && !std::filesystem::exists(file.second))
        {
            throw util::exception("Could not find required file(s): " + std::get<1>(file).string());
        }
    }

    if (m_query_data)
        std::erase_if(files,
                      [&](const auto &file)
                      {
                          for (const auto &suffix :
                               {".osrm.partition", ".osrm.ebg_nodes", ".osrm.edges", ".osrm.icd"})
                              if (file.second ==
                                  std::filesystem::path(config.base_path.string() + suffix))
                                  return true;
                          return false;
                      });

    return files;
}

std::vector<std::pair<bool, std::filesystem::path>> Storage::GetUpdatableFiles()
{
    constexpr bool IS_REQUIRED = true;
    constexpr bool IS_OPTIONAL = false;

    std::vector<std::pair<bool, std::filesystem::path>> files = {
        {IS_OPTIONAL, config.GetPath(".osrm.mldgr")},
        {IS_OPTIONAL, config.GetPath(".osrm.cell_metrics")},
        {IS_OPTIONAL, config.GetPath(".osrm.hsgr")},
        {IS_REQUIRED, config.GetPath(".osrm.datasource_names")},
        {IS_REQUIRED, config.GetPath(".osrm.geometry")},
        {IS_REQUIRED, config.GetPath(".osrm.turn_weight_penalties")},
        {IS_REQUIRED, config.GetPath(".osrm.turn_duration_penalties")}};

    for (const auto &file : files)
    {
        if (file.first == IS_REQUIRED && !std::filesystem::exists(file.second))
        {
            throw util::exception("Could not find required file(s): " + std::get<1>(file).string());
        }
    }

    if (m_query_data)
    {
        files.clear();
        files.emplace_back(true, config.GetPath(".osrm.query"));
    }
    return files;
}

std::string Storage::PopulateLayoutWithRTree(storage::BaseDataLayout &layout)
{
    // Figure out the path to the rtree file (it's not a tar file)
    auto absolute_file_index_path = std::filesystem::absolute(config.GetPath(".osrm.fileIndex"));

    // Convert the std::filesystem::path object into a plain string
    // that can then be stored as a member of an allocator object
    auto rtree_filename = absolute_file_index_path.string();

    // Here, we hardcode the special file_index_path block name.
    // The important bit here is that the "offset" is set to zero
    layout.SetBlock("/common/rtree/file_index_path", make_block<char>(rtree_filename.length() + 1));

    return rtree_filename;
}

/**
 * The same again for the r-tree over the meshed open areas, which has a leaf file of its
 * own.  Deliberately a separate function rather than another block in the one above:
 * MMapMemoryAllocator registers the returned string as an entire memory region with the
 * block at offset zero, so a layout holding two of these needs two regions and two
 * strings to point them at.
 *
 * @return the leaf file's path, or an empty string when the dataset has no meshed areas.
 */
std::string Storage::PopulateLayoutWithOpenAreaRTree(storage::BaseDataLayout &layout)
{
    if (!std::filesystem::exists(config.GetPath(".osrm.openareas.fileIndex")))
    {
        return {};
    }

    auto filename = std::filesystem::absolute(config.GetPath(".osrm.openareas.fileIndex")).string();
    layout.SetBlock("/common/open_areas/rtree/file_index_path",
                    make_block<char>(filename.length() + 1));
    return filename;
}

/**
 * This function examines all our data files and figures out how much
 * memory needs to be allocated, and the position of each data structure
 * in that big block.  It updates the fields in the layout parameter.
 */
void Storage::PopulateLayout(storage::BaseDataLayout &layout,
                             const std::vector<std::pair<bool, std::filesystem::path>> &files)
{
    for (const auto &file : files)
    {
        if (std::filesystem::exists(file.second))
        {
            populateLayoutFromFile(
                file.second,
                layout,
                [&](const auto &name)
                {
                    if (!m_query_data || file.second != config.GetPath(".osrm.query"))
                        return true;
                    if (name.starts_with("/common/turn_data/") &&
                        !config.IsRequiredConfiguredInput(".osrm.edges"))
                        return false;
                    return config.IsRequiredConfiguredInput(".osrm.icd") ||
                           (!name.starts_with("/common/intersection_bearings/") &&
                            !name.starts_with("/common/entry_classes"));
                });
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
            std::filesystem::absolute(config.GetPath(".osrm.fileIndex")).string();
        BOOST_ASSERT(static_cast<std::size_t>(index.GetBlockSize(
                         "/common/rtree/file_index_path")) >= absolute_file_index_path.size());
        std::copy(
            absolute_file_index_path.begin(), absolute_file_index_path.end(), file_index_path_ptr);
    }

    // and the same for the area r-tree's leaf file, when there is one
    if (std::filesystem::exists(config.GetPath(".osrm.openareas.fileIndex")))
    {
        const auto path_ptr = index.GetBlockPtr<char>("/common/open_areas/rtree/file_index_path");
        std::fill(
            path_ptr, path_ptr + index.GetBlockSize("/common/open_areas/rtree/file_index_path"), 0);
        const auto absolute_path =
            std::filesystem::absolute(config.GetPath(".osrm.openareas.fileIndex")).string();
        std::copy(absolute_path.begin(), absolute_path.end(), path_ptr);
    }

    for (const auto &file : GetStaticFiles())
    {
        if (std::filesystem::exists(file.second))
            populateArchive(index, file.second);
    }

    if (!m_query_data && config.IsRequiredConfiguredInput(".osrm.edges") &&
        index.GetBlockSize("/common/connectivity_checksum") != sizeof(std::uint32_t))
        throw util::exception("Invalid connectivity checksum in " +
                              config.GetPath(".osrm.edges").string());
}

void Storage::PopulateUpdatableData(const SharedDataIndex &index)
{
    for (const auto &file : GetUpdatableFiles())
    {
        if (std::filesystem::exists(file.second))
            populateArchive(index, file.second);
    }

    if (m_query_data)
    {
        if (config.IsRequiredConfiguredInput(".osrm.edges"))
        {
            const auto checksum = *index.GetBlockPtr<std::uint32_t>("/common/connectivity_checksum");
            for (const auto *entry : {"/mld/connectivity_checksum", "/ch/connectivity_checksum"})
            {
                if (index.HasBlock(entry) &&
                    (index.GetBlockSize(entry) != sizeof(std::uint32_t) ||
                     *index.GetBlockPtr<std::uint32_t>(entry) != checksum))
                    throw util::exception("Query graph and guidance connectivity checksums do not match");
            }
        }
        return;
    }

    for (const auto &[suffix, entry] : {std::pair{".osrm.hsgr", "/ch/connectivity_checksum"},
                                        std::pair{".osrm.mldgr", "/mld/connectivity_checksum"}})
    {
        const auto path = config.GetPath(suffix);
        if (!std::filesystem::exists(path))
            continue;
        tar::FileReader reader(path, tar::FileReader::VerifyFingerprint);
        std::uint32_t checksum;
        reader.ReadInto(entry, checksum);
        if (config.IsRequiredConfiguredInput(".osrm.edges"))
        {
            const auto guidance_checksum =
                *index.GetBlockPtr<std::uint32_t>("/common/connectivity_checksum");
            if (checksum != guidance_checksum)
            {
                throw util::exception("Connectivity checksum " + std::to_string(checksum) + " in " +
                                      path.string() + " does not equal to checksum " +
                                      std::to_string(guidance_checksum) + " in " +
                                      config.GetPath(".osrm.edges").string());
            }
        }
    }
}
} // namespace osrm::storage
