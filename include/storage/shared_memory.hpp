#ifndef SHARED_MEMORY_HPP
#define SHARED_MEMORY_HPP

#include "storage/shared_datatype.hpp"

#include <boost/interprocess/exceptions.hpp>
#include <boost/interprocess/mapped_region.hpp>
#ifndef _WIN32
#include <boost/interprocess/xsi_shared_memory.hpp>
#else
#include <boost/interprocess/shared_memory_object.hpp>
#endif

#ifdef __linux__
#include <sys/ipc.h>
#include <sys/shm.h>
#endif

#include <cstdint>

#include <filesystem>

namespace osrm::storage
{

class SharedMemory
{
    SharedMemory(const SharedMemory &) = delete;
    SharedMemory &operator=(const SharedMemory &) = delete;

  public:
    SharedMemory(const ProjID proj_id, const uint64_t size = 0);

    void *Ptr() const { return region.get_address(); }
    std::size_t Size() const { return region.get_size(); }

  private:
    boost::interprocess::mapped_region region;
};

/**
 * @brief Returns the directory to use for OSRM lock files
 *
 * Returns the contents of the environment variable SHM_LOCK_DIR if set else the system
 * temp directory.
 *
 * @return std::filesystem::path The directory to usew for lock files
 */
std::filesystem::path getLockDir();

std::unique_ptr<SharedMemory> makeSharedMemory(const ProjID proj_id, const uint64_t size = 0);

/**
 * @brief Tests if a shared memory region exists
 *
 * @param key A ProjID
 * @return bool Returns true if the region exists
 */
bool RegionExists(const ProjID proj_id);
/**
 * @brief Destroys the shared memory region
 *
 * @param key A valid ProjID
 * @return bool returns false on error.
 */
bool Remove(const ProjID proj_id);
/**
 * @brief Waits for all processes to detach from the shared memory region
 *
 * @param key A ProjID
 * @param timeout Timeout in ms
 */
void WaitForDetach(const ProjID proj_id, int timeout);

} // namespace osrm::storage

#endif // SHARED_MEMORY_HPP
