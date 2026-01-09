#ifndef SHARED_MEMORY_HPP
#define SHARED_MEMORY_HPP

#include "storage/shared_datatype.hpp"
#include "util/log.hpp"

#include <boost/interprocess/exceptions.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <chrono>
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
#include <thread>

namespace osrm::storage
{

// Returns directory for OSRM lock files (SHM_LOCK_DIR env var or system temp)
inline std::filesystem::path getLockDir()
{
    if (const char *lock_dir = std::getenv("SHM_LOCK_DIR"))
    {
        std::filesystem::path dir(lock_dir);
        if (!std::filesystem::exists(dir))
        {
            throw util::exception("SHM_LOCK_DIR directory does not exist: " + dir.string() +
                                  SOURCE_REF);
        }
        return dir;
    }
    return std::filesystem::temp_directory_path();
}

struct OSRMLockFile
{
    OSRMLockFile(const ShmKey shm_key)
    {
        std::filesystem::path filename =
            std::filesystem::path("osrm-" + std::to_string(shm_key) + ".lock");
        lock_file = getLockDir() / filename;
    }
    const std::filesystem::path to_path() { return lock_file; }
    operator const std::filesystem::path::value_type *() const noexcept
    {
        return lock_file.c_str();
    }

  private:
    std::filesystem::path lock_file;
};

class OSRMShmName
{
  public:
    OSRMShmName(const ShmKey shm_key) { name = "osrm-" + std::to_string(shm_key); }
    operator const char *() const noexcept { return name.c_str(); }

  private:
    std::string name;
};

using namespace boost::interprocess;

#ifndef _WIN32
class SharedMemory
{
  public:
    void *Ptr() const { return region.get_address(); }
    std::size_t Size() const { return region.get_size(); }

    SharedMemory(const SharedMemory &) = delete;
    SharedMemory &operator=(const SharedMemory &) = delete;

    SharedMemory(const OSRMLockFile &lock_file, const ShmKey shm_key, const uint64_t size = 0)
    {
        xsi_key xsi_key(lock_file, shm_key);
        // open only
        if (0 == size)
        {
            shm = xsi_shared_memory(open_only, xsi_key);

            util::Log(logDEBUG) << "opening " << shm.get_shmid() << " from id " << (int)shm_key;

            region = mapped_region(shm, read_only);
        }
        // open or create
        else
        {
            shm = xsi_shared_memory(open_or_create, xsi_key, size);
            util::Log(logDEBUG) << "opening/creating " << shm.get_shmid() << " from id " << shm_key
                                << " with size " << size;
#ifdef __linux__
            if (-1 == shmctl(shm.get_shmid(), SHM_LOCK, nullptr))
            {
                if (ENOMEM == errno)
                {
                    util::Log(logWARNING) << "could not lock shared memory to RAM";
                }
            }
#endif
            region = mapped_region(shm, read_write);
        }
    }

    /**
     * @brief Test if a shared memory region exists
     *
     * @param key A ShmKey
     * @return bool Returns true if the region exists
     */
    static bool RegionExists(const ShmKey shm_key)
    {
        try
        {
            OSRMLockFile lock_file(shm_key);
            xsi_key xsi_key(lock_file, shm_key);
            xsi_shared_memory xsi(open_only, xsi_key);
            return true;
        }
        catch (...)
        {
        }
        return false;
    }

    /**
     * @brief Destroys the shared memory region
     *
     * @param key A valid ShmKey
     * @return bool returns false on error.
     */
    static bool Remove(const ShmKey shm_key)
    {
        OSRMLockFile lock_file(shm_key);
        xsi_key xsi_key(lock_file, shm_key);
        xsi_shared_memory xsi(open_only, xsi_key);
        util::Log(logDEBUG) << "deallocating prev memory " << xsi.get_shmid();
        return xsi_shared_memory::remove(xsi.get_shmid());
    }

    /**
     * @brief Waits for all processes to detach from the shared memory region
     *
     * @param key A ShmKey
     * @param timeout Timeout in ms
     */
    static void WaitForDetach(const ShmKey shm_key, int timeout)
    {
        try
        {
            OSRMLockFile lock_file(shm_key);
            xsi_key xsi_key(lock_file, shm_key);
            xsi_shared_memory xsi(open_only, xsi_key);
            while (true)
            {
                ::shmid_ds xsi_ds;
                if (::shmctl(xsi.get_shmid(), IPC_STAT, &xsi_ds) < 0)
                    break;
                if (xsi_ds.shm_nattch == 0)
                    break;
                if (--timeout < 0)
                    break;
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
        catch (interprocess_exception &)
        {
            // already detached
        }
    }

  private:
    xsi_shared_memory shm;
    mapped_region region;
};
#else

// POSIX shared mem
// Currently used for Windows, but could be used for all
class SharedMemory
{
    SharedMemory(const SharedMemory &) = delete;
    SharedMemory &operator=(const SharedMemory &) = delete;

  public:
    void *Ptr() const { return region.get_address(); }
    std::size_t Size() const { return region.get_size(); }

    SharedMemory(const OSRMLockFile &, const ShmKey shm_key, const uint64_t size = 0)
    {
        OSRMShmName name(shm_key);
        if (0 == size)
        { // read_only
            shm_o = shared_memory_object(open_only, name, read_only);
            region = mapped_region(shm_o, read_only);
        }
        else
        { // writeable pointer
            shm_o = shared_memory_object(open_or_create, name, read_write);
            shm_o.truncate(size);
            region = mapped_region(shm_o, read_write);

            util::Log(logDEBUG) << "writeable memory allocated " << size << " bytes";
        }
    }

    static bool RegionExists(const ShmKey shm_key)
    {
        OSRMShmName name(shm_key);
        try
        {
            shared_memory_object shm(open_only, name, read_write);
            return true;
        }
        catch (...)
        {
        }
        return false;
    }

    static bool Remove(const ShmKey shm_key)
    {
        OSRMShmName name(shm_key);
        util::Log(logDEBUG) << "deallocating prev memory for name " << name;
        return shared_memory_object::remove(name);
    }

    /**
     * @brief Waits for all processes to detach from the shared memory region
     *
     * @param key A ShmKey
     * @param timeout Timeout in ms
     */
    static void WaitForDetach(const ShmKey shm_key, int timeout)
    {
        OSRMShmName name(shm_key);
        try
        {
            while (true)
            {
                shared_memory_object shm_o(open_only, name, read_write);
                if (--timeout < 0)
                    break;
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
        catch (interprocess_exception &)
        {
            // already detached
        }
    }

  private:
    shared_memory_object shm_o;
    mapped_region region;
};
#endif

std::unique_ptr<SharedMemory> makeSharedMemory(const ShmKey shm_key, const uint64_t size = 0);

} // namespace osrm::storage

#endif // SHARED_MEMORY_HPP
