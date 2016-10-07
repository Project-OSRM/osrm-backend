#ifndef SHARED_MEMORY_HPP
#define SHARED_MEMORY_HPP

#include "util/exception.hpp"
#include "util/simple_logger.hpp"

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

// #include <cstring>
#include <cstdint>

#include <algorithm>
#include <exception>

namespace osrm
{
namespace storage
{

struct OSRMLockFile
{
    boost::filesystem::path operator()()
    {
        boost::filesystem::path temp_dir = boost::filesystem::temp_directory_path();
        boost::filesystem::path lock_file = temp_dir / "osrm.lock";
        return lock_file;
    }
};

#ifndef _WIN32
class SharedMemory
{

    // Remove shared memory on destruction
    class shm_remove
    {
      private:
        int m_shmid;
        bool m_initialized;

      public:
        void SetID(int shmid)
        {
            m_shmid = shmid;
            m_initialized = true;
        }

        shm_remove() : m_shmid(INT_MIN), m_initialized(false) {}

        shm_remove(const shm_remove &) = delete;
        shm_remove &operator=(const shm_remove &) = delete;

        ~shm_remove()
        {
            if (m_initialized)
            {
                util::SimpleLogger().Write(logDEBUG) << "automatic memory deallocation";
                if (!boost::interprocess::xsi_shared_memory::remove(m_shmid))
                {
                    util::SimpleLogger().Write(logDEBUG) << "could not deallocate id " << m_shmid;
                }
            }
        }
    };

  public:
    void *Ptr() const { return region.get_address(); }

    SharedMemory(const SharedMemory &) = delete;
    SharedMemory &operator=(const SharedMemory &) = delete;

    template <typename IdentifierT>
    SharedMemory(const boost::filesystem::path &lock_file,
                 const IdentifierT id,
                 const uint64_t size = 0,
                 bool read_write = false,
                 bool remove_prev = true)
        : key(lock_file.string().c_str(), id)
    {
        if (0 == size)
        { // read_only
            shm = boost::interprocess::xsi_shared_memory(boost::interprocess::open_only, key);

            util::SimpleLogger().Write(logDEBUG) << "opening " << shm.get_shmid() << " from id "
                                                 << id;
            region = boost::interprocess::mapped_region(
                shm,
                (read_write ? boost::interprocess::read_write : boost::interprocess::read_only));
        }
        else
        { // writeable pointer
            // remove previously allocated mem
            if (remove_prev)
            {
                Remove(key);
            }
            shm = boost::interprocess::xsi_shared_memory(
                boost::interprocess::open_or_create, key, size);
            util::SimpleLogger().Write(logDEBUG) << "opening/creating " << shm.get_shmid()
                                                 << " from id " << id << " with size " << size;
#ifdef __linux__
            if (-1 == shmctl(shm.get_shmid(), SHM_LOCK, nullptr))
            {
                if (ENOMEM == errno)
                {
                    util::SimpleLogger().Write(logWARNING) << "could not lock shared memory to RAM";
                }
            }
#endif
            region = boost::interprocess::mapped_region(shm, boost::interprocess::read_write);

            remover.SetID(shm.get_shmid());
            util::SimpleLogger().Write(logDEBUG) << "writeable memory allocated " << size
                                                 << " bytes";
        }
    }

    template <typename IdentifierT> static bool RegionExists(const IdentifierT id)
    {
        bool result = true;
        try
        {
            OSRMLockFile lock_file;
            boost::interprocess::xsi_key key(lock_file().string().c_str(), id);
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
        boost::interprocess::xsi_key key(lock_file().string().c_str(), id);
        return Remove(key);
    }

  private:
    static bool RegionExists(const boost::interprocess::xsi_key &key)
    {
        bool result = true;
        try
        {
            boost::interprocess::xsi_shared_memory shm(boost::interprocess::open_only, key);
        }
        catch (...)
        {
            result = false;
        }
        return result;
    }

    static bool Remove(const boost::interprocess::xsi_key &key)
    {
        bool ret = false;
        try
        {
            boost::interprocess::xsi_shared_memory xsi(boost::interprocess::open_only, key);
            util::SimpleLogger().Write(logDEBUG) << "deallocating prev memory " << xsi.get_shmid();
            ret = boost::interprocess::xsi_shared_memory::remove(xsi.get_shmid());
        }
        catch (const boost::interprocess::interprocess_exception &e)
        {
            if (e.get_error_code() != boost::interprocess::not_found_error)
            {
                throw;
            }
        }
        return ret;
    }

    boost::interprocess::xsi_key key;
    boost::interprocess::xsi_shared_memory shm;
    boost::interprocess::mapped_region region;
    shm_remove remover;
};
#else
// Windows - specific code
class SharedMemory
{
    SharedMemory(const SharedMemory &) = delete;
    SharedMemory &operator=(const SharedMemory &) = delete;
    // Remove shared memory on destruction
    class shm_remove
    {
      private:
        char *m_shmid;
        bool m_initialized;

      public:
        void SetID(char *shmid)
        {
            m_shmid = shmid;
            m_initialized = true;
        }

        shm_remove() : m_shmid("undefined"), m_initialized(false) {}

        shm_remove(const shm_remove &) = delete;
        shm_remove &operator=(const shm_remove &) = delete;

        ~shm_remove()
        {
            if (m_initialized)
            {
                util::SimpleLogger().Write(logDEBUG) << "automatic memory deallocation";
                if (!boost::interprocess::shared_memory_object::remove(m_shmid))
                {
                    util::SimpleLogger().Write(logDEBUG) << "could not deallocate id " << m_shmid;
                }
            }
        }
    };

  public:
    void *Ptr() const { return region.get_address(); }

    SharedMemory(const boost::filesystem::path &lock_file,
                 const int id,
                 const uint64_t size = 0,
                 bool read_write = false,
                 bool remove_prev = true)
    {
        sprintf(key, "%s.%d", "osrm.lock", id);
        if (0 == size)
        { // read_only
            shm = boost::interprocess::shared_memory_object(
                boost::interprocess::open_only,
                key,
                read_write ? boost::interprocess::read_write : boost::interprocess::read_only);
            region = boost::interprocess::mapped_region(
                shm, read_write ? boost::interprocess::read_write : boost::interprocess::read_only);
        }
        else
        { // writeable pointer
            // remove previously allocated mem
            if (remove_prev)
            {
                Remove(key);
            }
            shm = boost::interprocess::shared_memory_object(
                boost::interprocess::open_or_create, key, boost::interprocess::read_write);
            shm.truncate(size);
            region = boost::interprocess::mapped_region(shm, boost::interprocess::read_write);

            remover.SetID(key);
            util::SimpleLogger().Write(logDEBUG) << "writeable memory allocated " << size
                                                 << " bytes";
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
        bool ret = false;
        try
        {
            util::SimpleLogger().Write(logDEBUG) << "deallocating prev memory for key " << key;
            ret = boost::interprocess::shared_memory_object::remove(key);
        }
        catch (const boost::interprocess::interprocess_exception &e)
        {
            if (e.get_error_code() != boost::interprocess::not_found_error)
            {
                throw;
            }
        }
        return ret;
    }

    char key[500];
    boost::interprocess::shared_memory_object shm;
    boost::interprocess::mapped_region region;
    shm_remove remover;
};
#endif

template <typename IdentifierT, typename LockFileT = OSRMLockFile>
SharedMemory *makeSharedMemory(const IdentifierT &id,
                               const uint64_t size = 0,
                               bool read_write = false,
                               bool remove_prev = true)
{
    try
    {
        LockFileT lock_file;
        if (!boost::filesystem::exists(lock_file()))
        {
            if (0 == size)
            {
                throw util::exception("lock file does not exist, exiting");
            }
            else
            {
                boost::filesystem::ofstream ofs(lock_file());
            }
        }
        return new SharedMemory(lock_file(), id, size, read_write, remove_prev);
    }
    catch (const boost::interprocess::interprocess_exception &e)
    {
        util::SimpleLogger().Write(logWARNING) << "caught exception: " << e.what() << ", code "
                                               << e.get_error_code();
        throw util::exception(e.what());
    }
}
}
}

#endif // SHARED_MEMORY_HPP
