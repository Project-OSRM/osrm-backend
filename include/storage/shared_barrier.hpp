#ifndef SHARED_BARRIER_HPP
#define SHARED_BARRIER_HPP

#include "storage/shared_datatype.hpp"

#include <boost/format.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>

#include <iostream>

namespace osrm
{
namespace storage
{

namespace
{
namespace bi = boost::interprocess;
}

struct SharedBarrier
{
    struct internal_data
    {
        typedef bi::interprocess_mutex mutex_type;
        typedef bi::interprocess_condition condvar_type;

        internal_data() : region(REGION_NONE), timestamp(0){};

        mutex_type mutex;
        condvar_type condvar;
        SharedDataType region;
        unsigned timestamp;
    };

    typedef internal_data::mutex_type mutex_type;
    typedef internal_data::condvar_type condvar_type;

    SharedBarrier(bi::open_or_create_t)
    {
        shmem = bi::shared_memory_object(bi::open_or_create, block_name, bi::read_write);

        const void *address = reinterpret_cast<void *>(block_address);
        bi::offset_t size = 0;
        if (!shmem.get_size(size) || size != block_size)
        {
            shmem.truncate(block_size);
            region = bi::mapped_region(shmem, bi::read_write, 0, block_size, address, block_flags);
            new (region.get_address()) internal_data;
        }
        else
        {
            region = bi::mapped_region(shmem, bi::read_write, 0, block_size, address, block_flags);
        }
    }

    SharedBarrier(bi::open_only_t)
    {
        try
        {
            shmem = bi::shared_memory_object(bi::open_only, block_name, bi::read_write);

            bi::offset_t size = 0;
            if (!shmem.get_size(size) || size != block_size)
            {
                auto message =
                    boost::format("Wrong shared memory block size %1%, expected %2% bytes") % size %
                    static_cast<const std::size_t>(block_size);
                throw util::exception(message.str() + SOURCE_REF);
            }
        }
        catch (const bi::interprocess_exception &exception)
        {
            throw util::exception(
                std::string(
                    "No shared memory blocks found, have you forgotten to run osrm-datastore?") +
                SOURCE_REF);
        }

        const void *address = reinterpret_cast<void *>(block_address);
        region = bi::mapped_region(shmem, bi::read_write, 0, block_size, address, block_flags);
    }

    auto &GetMutex() const { return internal()->mutex; }

    template <typename L> void Wait(L &lock) const { internal()->condvar.wait(lock); }

    void NotifyAll() const { internal()->condvar.notify_all(); }

    void SetRegion(SharedDataType region) const
    {
        internal()->region = region;
        internal()->timestamp += 1;
    }

    auto GetRegion() const { return internal()->region; }

    auto GetTimestamp() const { return internal()->timestamp; }

    static void Remove() { bi::shared_memory_object::remove(block_name); }

  private:
    bi::shared_memory_object shmem;
    bi::mapped_region region;

    static constexpr const char *const block_name = "osrm-region";
    static constexpr std::size_t block_size = sizeof(internal_data);
#if defined(__APPLE__)
    // OSX pthread_cond_wait implementation internally checks address of the mutex
    // so a fixed mapping address is needed to share control memory block between
    // processes and engine instances in one process
    // https://github.com/Project-OSRM/osrm-backend/issues/3619
    static constexpr std::intptr_t block_address = 0x00000caffee00000;
    static constexpr bi::map_options_t block_flags = MAP_FIXED;
#else
    static constexpr std::intptr_t block_address = 0x0;
    static constexpr bi::map_options_t block_flags = bi::default_map_options;
#endif

    inline internal_data *internal() const
    {
        return reinterpret_cast<internal_data *>(region.get_address());
    }
};
}
}

#endif // SHARED_BARRIER_HPP
