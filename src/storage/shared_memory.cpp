#include "storage/shared_memory.hpp"
#include "storage/shared_datatype.hpp"
#include <boost/interprocess/shared_memory_object.hpp>
#include <cstring>
#include <string>
#include <thread>

namespace osrm::storage
{
using namespace boost::interprocess;

struct OSRMLockFile
{
    OSRMLockFile(const ProjID proj_id)
    {
        std::filesystem::path filename =
            std::filesystem::path("osrm-" + std::to_string(proj_id) + ".lock");
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

#ifndef _WIN32

SharedMemory::SharedMemory(const ProjID proj_id, const uint64_t size)
{
    OSRMLockFile lock_file(proj_id);
    xsi_key xsi_key(lock_file, proj_id);
    // open only
    if (size == 0)
    {
        xsi_shared_memory xsi_shm(open_only, xsi_key);

        util::Log(logDEBUG) << "opening " << xsi_shm.get_shmid() << " from id " << (int)proj_id;

        region = mapped_region(xsi_shm, read_only);
    }
    // open or create
    else
    {
        xsi_shared_memory xsi_shm(open_or_create, xsi_key, size);
        util::Log(logDEBUG) << "opening/creating " << xsi_shm.get_shmid() << " from id " << proj_id
                            << " with size " << size;
#ifdef __linux__
        if (-1 == shmctl(xsi_shm.get_shmid(), SHM_LOCK, nullptr))
        {
            if (ENOMEM == errno)
            {
                util::Log(logWARNING) << "could not lock shared memory to RAM";
            }
        }
#endif
        region = mapped_region(xsi_shm, read_write);
    }
}

bool RegionExists(const ProjID proj_id)
{
    try
    {
        OSRMLockFile lock_file(proj_id);
        xsi_key xsi_key(lock_file, proj_id);
        xsi_shared_memory xsi(open_only, xsi_key);
        return true;
    }
    catch (...)
    {
    }
    return false;
}

bool Remove(const ProjID proj_id)
{
    OSRMLockFile lock_file(proj_id);
    xsi_key xsi_key(lock_file, proj_id);
    xsi_shared_memory xsi(open_only, xsi_key);
    util::Log(logDEBUG) << "deallocating prev memory " << xsi.get_shmid();
    return xsi_shared_memory::remove(xsi.get_shmid());
}

void WaitForDetach(const ProjID proj_id, int timeout)
{
    try
    {
        OSRMLockFile lock_file(proj_id);
        xsi_key xsi_key(lock_file, proj_id);
        xsi_shared_memory xsi(open_only, xsi_key);
        while (true)
        {
            ::shmid_ds xsi_ds;
            if (::shmctl(xsi.get_shmid(), IPC_STAT, &xsi_ds) < 0)
            {
                if (errno != EIDRM)
                    throw util::exception("Error while waiting for clients to detach: " +
                                          std::string(strerror(errno)) + " at " + SOURCE_REF);
                break;
            }
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

#else

class OSRMShmName
{
  public:
    OSRMShmName(const ProjID proj_id) { name = "osrm-" + std::to_string(proj_id); }
    operator const char *() const noexcept { return name.c_str(); }

  private:
    std::string name;
};

SharedMemory::SharedMemory(const ProjID proj_id, const uint64_t size)
{
    OSRMShmName name(proj_id);
    if (size == 0)
    { // read_only
        shared_memory_object shm_obj(shared_memory_object(open_only, name, read_only));
        region = mapped_region(shm_obj, read_only);
    }
    else
    { // writeable pointer
        shared_memory_object shm_obj(shared_memory_object(open_or_create, name, read_write));
        shm_obj.truncate(size);
        region = mapped_region(shm_obj, read_write);

        util::Log(logDEBUG) << "writeable memory allocated " << size << " bytes";
    }
}

bool RegionExists(const ProjID proj_id)
{
    OSRMShmName name(proj_id);
    try
    {
        shared_memory_object shm_obj(open_only, name, read_write);
        return true;
    }
    catch (...)
    {
    }
    return false;
}

bool Remove(const ProjID proj_id)
{
    OSRMShmName name(proj_id);
    util::Log(logDEBUG) << "deallocating prev memory for name " << name;
    return shared_memory_object::remove(name);
}

void WaitForDetach(const ProjID proj_id, int timeout)
{
    OSRMShmName name(proj_id);
    try
    {
        while (true)
        {
            shared_memory_object shm_obj(open_only, name, read_write);
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

#endif

std::unique_ptr<SharedMemory> makeSharedMemory(const ProjID proj_id, const uint64_t size)
{
    OSRMLockFile lock_file(proj_id);
    try
    {
        if (!std::filesystem::exists(lock_file.to_path()))
        {
            if (size == 0)
            {
                throw util::exception("Lock file does not exist, exiting. " + SOURCE_REF);
            }
            else
            {
                std::ofstream ofs(lock_file.to_path());
            }
        }
        return std::make_unique<SharedMemory>(proj_id, size);
    }
    catch (const interprocess_exception &e)
    {
        util::Log(logERROR) << "Error while attempting to allocate shared memory: " << e.what()
                            << ", code: " << e.get_error_code()
                            << ", lock file: " << lock_file.to_path();
        throw util::exception(e.what() + SOURCE_REF);
    }
}

std::filesystem::path getLockDir()
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

} // namespace osrm::storage
