#include "storage/shared_memory.hpp"

namespace osrm::storage
{

std::unique_ptr<SharedMemory> makeSharedMemory(const ShmKey shm_key, const uint64_t size)
{
    OSRMLockFile lock_file(shm_key);
    try
    {
        if (!std::filesystem::exists(lock_file.to_path()))
        {
            if (0 == size)
            {
                throw util::exception("Lock file does not exist, exiting. " + SOURCE_REF);
            }
            else
            {
                std::ofstream ofs(lock_file.to_path());
            }
        }
        return std::make_unique<SharedMemory>(lock_file, shm_key, size);
    }
    catch (const boost::interprocess::interprocess_exception &e)
    {
        util::Log(logERROR) << "Error while attempting to allocate shared memory: " << e.what()
                            << ", code: " << e.get_error_code()
                            << ", lock file: " << lock_file.to_path();
        throw util::exception(e.what() + SOURCE_REF);
    }
}
} // namespace osrm::storage
