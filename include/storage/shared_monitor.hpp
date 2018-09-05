#ifndef SHARED_MONITOR_HPP
#define SHARED_MONITOR_HPP

#include "storage/shared_datatype.hpp"

#include <boost/format.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/interprocess_semaphore.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

#if defined(__linux__)
// See issue #3911, boost interprocess is broken with a glibc > 2.25
// #define USE_BOOST_INTERPROCESS_CONDITION 1
#endif

namespace osrm
{
namespace storage
{

namespace
{
namespace bi = boost::interprocess;

template <class Lock> class InvertedLock
{
    Lock &lock;

  public:
    InvertedLock(Lock &lock) : lock(lock) { lock.unlock(); }
    ~InvertedLock() { lock.lock(); }
};
} // namespace

// The shared monitor implementation based on a semaphore and mutex
template <typename Data> struct SharedMonitor
{
    using mutex_type = bi::interprocess_mutex;

    SharedMonitor(const Data &initial_data)
    {
        shmem = bi::shared_memory_object(bi::open_or_create, Data::name, bi::read_write);

        bi::offset_t size = 0;
        if (shmem.get_size(size) && size == 0)
        {
            shmem.truncate(rounded_internal_size + sizeof(Data));
            region = bi::mapped_region(shmem, bi::read_write);
            new (&internal()) InternalData;
            new (&data()) Data(initial_data);
        }
        else
        {
            region = bi::mapped_region(shmem, bi::read_write);
        }
    }

    SharedMonitor()
    {
        try
        {
            shmem = bi::shared_memory_object(bi::open_only, Data::name, bi::read_write);

            bi::offset_t size = 0;
            if (!shmem.get_size(size) || size != rounded_internal_size + sizeof(Data))
            {
                auto message =
                    boost::format("Wrong shared memory block '%1%' size %2%, expected %3% bytes") %
                    (const char *)Data::name % size % (rounded_internal_size + sizeof(Data));
                throw util::exception(message.str() + SOURCE_REF);
            }

            region = bi::mapped_region(shmem, bi::read_write);
        }
        catch (const bi::interprocess_exception &exception)
        {
            auto message = boost::format("No shared memory block '%1%' found, have you forgotten "
                                         "to run osrm-datastore?") %
                           (const char *)Data::name;
            throw util::exception(message.str() + SOURCE_REF);
        }
    }

    Data &data() const
    {
        auto region_pointer = reinterpret_cast<char *>(region.get_address());
        return *reinterpret_cast<Data *>(region_pointer + rounded_internal_size);
    }

    mutex_type &get_mutex() const { return internal().mutex; }

#if USE_BOOST_INTERPROCESS_CONDITION
    template <typename Lock> void wait(Lock &lock) { internal().condition.wait(lock); }

    void notify_all() { internal().condition.notify_all(); }
#else
    template <typename Lock> void wait(Lock &lock)
    {
        auto semaphore = internal().enqueue_semaphore();
        {
            InvertedLock<Lock> inverted_lock(lock);
            semaphore->wait();
        }
    }

    void notify_all()
    {
        bi::scoped_lock<mutex_type> lock(internal().mutex);
        while (!internal().empty())
        {
            internal().dequeue_semaphore()->post();
        }
    }
#endif

    static void remove() { bi::shared_memory_object::remove(Data::name); }
    static bool exists()
    {
        try
        {
            bi::shared_memory_object shmem_open =
                bi::shared_memory_object(bi::open_only, Data::name, bi::read_only);
        }
        catch (const bi::interprocess_exception &exception)
        {
            return false;
        }
        return true;
    }

  private:
#if USE_BOOST_INTERPROCESS_CONDITION
    struct InternalData
    {
        mutex_type mutex;
        bi::interprocess_condition condition;
    };

#else
    // Implement a conditional variable using a queue of semaphores.
    // OSX checks the virtual address of a mutex in pthread_cond_wait and fails with EINVAL
    // if the shared block is used by different processes. Solutions based on waiters counting
    // like two-turnstile reusable barrier or boost/interprocess/sync/spin/condition.hpp
    // fail if a waiter is killed.

    // Buffer size needs to be large enough to hold all the semaphores for every
    // listener you want to support.
    static constexpr int buffer_size = 4096 * 4;

    struct InternalData
    {
        InternalData() : head(0), tail(0)
        {
            for (int index = 0; index < buffer_size; ++index)
            {
                invalidate_semaphore(get_semaphore(index));
            }
        };

        auto size() const { return head >= tail ? head - tail : buffer_size + head - tail; }

        auto empty() const { return tail == head; }

        auto enqueue_semaphore()
        {
            auto semaphore = get_semaphore(head);
            head = (head + 1) % buffer_size;

            if (size() >= buffer_size / 2)
                throw util::exception(std::string("ring buffer is too small") + SOURCE_REF);

            if (is_semaphore_valid(semaphore))
            {
                // SEM_DESTROY(3): Destroying a semaphore that other processes or threads
                // are currently blocked on (in sem_wait(3)) produces undefined behavior.
                // To prevent undefined behavior number of active semaphores is limited
                // to a half of buffer size. It is not a strong guarantee of well-defined behavior,
                // but minimizes a risk to destroy a semaphore that is still in use.
                semaphore->~interprocess_semaphore();
            }

            return new (semaphore) bi::interprocess_semaphore(0);
        }

        auto dequeue_semaphore()
        {
            auto semaphore = get_semaphore(tail);
            tail = (tail + 1) % buffer_size;
            return semaphore;
        }

      public:
        mutex_type mutex;

      private:
        auto get_semaphore(std::size_t index)
        {
            return reinterpret_cast<bi::interprocess_semaphore *>(
                buffer + index * sizeof(bi::interprocess_semaphore));
        }

        void invalidate_semaphore(void *semaphore) const
        {
            std::memset(semaphore, 0xff, sizeof(bi::interprocess_semaphore));
        }

        bool is_semaphore_valid(void *semaphore) const
        {
            char invalid[sizeof(bi::interprocess_semaphore)];
            invalidate_semaphore(invalid);
            return std::memcmp(semaphore, invalid, sizeof(invalid)) != 0;
        }

        std::size_t head, tail;
        char buffer[buffer_size * sizeof(bi::interprocess_semaphore)];
    };

    static_assert(buffer_size >= 2, "buffer size is too small");

#endif
    static constexpr int rounded_internal_size =
        ((sizeof(InternalData) + alignof(Data) - 1) / alignof(Data)) * alignof(Data);
    static_assert(rounded_internal_size < sizeof(InternalData) + sizeof(Data),
                  "Data and internal data need to fit into shared memory");

    InternalData &internal() const
    {
        return *reinterpret_cast<InternalData *>(reinterpret_cast<char *>(region.get_address()));
    }

    bi::shared_memory_object shmem;
    bi::mapped_region region;
};
} // namespace storage
} // namespace osrm

#undef USE_BOOST_INTERPROCESS_CONDITION

#endif // SHARED_MONITOR_HPP
