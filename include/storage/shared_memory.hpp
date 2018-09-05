#ifndef SHARED_MEMORY_HPP
#define SHARED_MEMORY_HPP

#include "util/exception.hpp"
#include "util/exception_utils.hpp"
#include "util/log.hpp"

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
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

#include <algorithm>
#include <exception>
#include <thread>

#include "storage/shared_memory_ownership.hpp"

namespace osrm
{
namespace storage
{

struct OSRMLockFile
{
    template <typename IdentifierT> boost::filesystem::path operator()(const IdentifierT &id)
    {
        boost::filesystem::path temp_dir = boost::filesystem::temp_directory_path();
        boost::filesystem::path lock_file = temp_dir / ("osrm-" + std::to_string(id) + ".lock");
        return lock_file;
    }
};

#ifndef _WIN32
class SharedMemory
{
  public:
    void *Ptr() const { return region.get_address(); }
    std::size_t Size() const { return region.get_size(); }

    SharedMemory(const SharedMemory &) = delete;
    SharedMemory &operator=(const SharedMemory &) = delete;

    template <typename IdentifierT>
    SharedMemory(const boost::filesystem::path &lock_file,
                 const IdentifierT id,
                 const uint64_t size = 0)
        : key(lock_file.string().c_str(), id)
    {
        // open only
        if (0 == size)
        {
            shm = boost::interprocess::xsi_shared_memory(boost::interprocess::open_only, key);

            util::Log(logDEBUG) << "opening " << (int)shm.get_shmid() << " from id " << (int)id;

            region = boost::interprocess::mapped_region(shm, boost::interprocess::read_only);
        }
        // open or create
        else
        {
            shm = boost::interprocess::xsi_shared_memory(
                boost::interprocess::open_or_create, key, size);
            util::Log(logDEBUG) << "opening/creating " << shm.get_shmid() << " from id " << id
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
            region = boost::interprocess::mapped_region(shm, boost::interprocess::read_write);
        }
    }

    template <typename IdentifierT> static bool RegionExists(const IdentifierT id)
    {
        bool result = true;
        try
        {
            OSRMLockFile lock_file;
            boost::interprocess::xsi_key key(lock_file(id).string().c_str(), id);
            result = RegionExists(key);
        }
        catch (...)
        {
            result = false;
        }
        return result;
    }

    template <typename IdentifierT> static bool Remove(const IdentifierT id)
    {
        OSRMLockFile lock_file;
        boost::interprocess::xsi_key key(lock_file(id).string().c_str(), id);
        return Remove(key);
    }

#ifdef __linux__
    void WaitForDetach()
    {
        auto shmid = shm.get_shmid();
        ::shmid_ds xsi_ds;
        const auto errorToMessage = [](int error) -> std::string {
            switch (error)
            {
            case EPERM:
                return "EPERM";
                break;
            case EACCES:
                return "ACCESS";
                break;
            case EINVAL:
                return "EINVAL";
                break;
            case EFAULT:
                return "EFAULT";
                break;
            default:
                return "Unknown Error " + std::to_string(error);
                break;
            }
        };

        do
        {
            // On OSX this returns EINVAL for whatever reason, hence we need to disable it
            int ret = ::shmctl(shmid, IPC_STAT, &xsi_ds);
            if (ret < 0)
            {
                auto error_code = errno;
                throw util::exception("shmctl encountered an error: " + errorToMessage(error_code) +
                                      SOURCE_REF);
            }
            BOOST_ASSERT(ret >= 0);

            std::this_thread::sleep_for(std::chrono::microseconds(100));
        } while (xsi_ds.shm_nattch > 1);
    }
#else
    void WaitForDetach()
    {
        util::Log(logDEBUG)
            << "Shared memory support for non-Linux systems does not wait for clients to "
               "dettach. Going to sleep for 50ms.";
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
#endif

  private:
    static bool RegionExists(const boost::interprocess::xsi_key &key)
    {
        bool result = true;
        try
        {
            boost::interprocess::xsi_shared_memory shm(boost::interprocess::open_only, key);
        }
        catch (const boost::interprocess::interprocess_exception &e)
        {
            if (e.get_error_code() != boost::interprocess::not_found_error)
            {
                throw;
            }
            result = false;
        }

        return result;
    }

    static bool Remove(const boost::interprocess::xsi_key &key)
    {
        boost::interprocess::xsi_shared_memory xsi(boost::interprocess::open_only, key);
        util::Log(logDEBUG) << "deallocating prev memory " << xsi.get_shmid();
        return boost::interprocess::xsi_shared_memory::remove(xsi.get_shmid());
    }

    boost::interprocess::xsi_key key;
    boost::interprocess::xsi_shared_memory shm;
    boost::interprocess::mapped_region region;
};
#else
// Windows - specific code
class SharedMemory
{
    SharedMemory(const SharedMemory &) = delete;
    SharedMemory &operator=(const SharedMemory &) = delete;

  public:
    void *Ptr() const { return region.get_address(); }
    std::size_t Size() const { return region.get_size(); }

    SharedMemory(const boost::filesystem::path &lock_file, const int id, const uint64_t size = 0)
    {
        sprintf(key, "%s.%d", "osrm.lock", id);
        if (0 == size)
        { // read_only
            shm = boost::interprocess::shared_memory_object(
                boost::interprocess::open_only, key, boost::interprocess::read_only);
            region = boost::interprocess::mapped_region(shm, boost::interprocess::read_only);
        }
        else
        { // writeable pointer
            shm = boost::interprocess::shared_memory_object(
                boost::interprocess::open_or_create, key, boost::interprocess::read_write);
            shm.truncate(size);
            region = boost::interprocess::mapped_region(shm, boost::interprocess::read_write);

            util::Log(logDEBUG) << "writeable memory allocated " << size << " bytes";
        }
    }

    static bool RegionExists(const int id)
    {
        bool result = true;
        try
        {
            char k[500];
            build_key(id, k);
            result = RegionExists(k);
        }
        catch (...)
        {
            result = false;
        }
        return result;
    }

    static bool Remove(const int id)
    {
        char k[500];
        build_key(id, k);
        return Remove(k);
    }

    void WaitForDetach()
    {
        // FIXME this needs an implementation for Windows
        util::Log(logDEBUG) << "Shared memory support for Windows does not wait for clients to "
                               "dettach. Going to sleep for 50ms.";
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

  private:
    static void build_key(int id, char *key) { sprintf(key, "%s.%d", "osrm.lock", id); }

    static bool RegionExists(const char *key)
    {
        bool result = true;
        try
        {
            boost::interprocess::shared_memory_object shm(
                boost::interprocess::open_only, key, boost::interprocess::read_write);
        }
        catch (...)
        {
            result = false;
        }
        return result;
    }

    static bool Remove(char *key)
    {
        util::Log(logDEBUG) << "deallocating prev memory for key " << key;
        return boost::interprocess::shared_memory_object::remove(key);
    }

    char key[500];
    boost::interprocess::shared_memory_object shm;
    boost::interprocess::mapped_region region;
};
#endif

template <typename IdentifierT, typename LockFileT = OSRMLockFile>
std::unique_ptr<SharedMemory> makeSharedMemory(const IdentifierT &id, const uint64_t size = 0)
{
    static_assert(sizeof(id) == sizeof(std::uint16_t), "Key type is not 16 bits");
    try
    {
        LockFileT lock_file;
        if (!boost::filesystem::exists(lock_file(id)))
        {
            if (0 == size)
            {
                throw util::exception("lock file does not exist, exiting" + SOURCE_REF);
            }
            else
            {
                boost::filesystem::ofstream ofs(lock_file(id));
            }
        }
        return std::make_unique<SharedMemory>(lock_file(id), id, size);
    }
    catch (const boost::interprocess::interprocess_exception &e)
    {
        util::Log(logERROR) << "Error while attempting to allocate shared memory: " << e.what()
                            << ", code " << e.get_error_code();
        throw util::exception(e.what() + SOURCE_REF);
    }
}
} // namespace storage
} // namespace osrm

#endif // SHARED_MEMORY_HPP
