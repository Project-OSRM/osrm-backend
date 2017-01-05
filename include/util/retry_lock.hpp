#ifndef OSRM_RETRY_TIMED_LOCK_HPP
#define OSRM_RETRY_TIMED_LOCK_HPP

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

class RetryLock
{
  public:
    RetryLock(int timeout_seconds, const char *name)
        : timeout_seconds(timeout_seconds), name(name),
          mutex(std::make_unique<boost::interprocess::named_mutex>(
              boost::interprocess::open_or_create, name)),
          internal_lock(*mutex, boost::interprocess::defer_lock)
    {
    }

    bool TryLock()
    {
        if (timeout_seconds >= 0)
        {
            return internal_lock.timed_lock(boost::posix_time::microsec_clock::universal_time() +
                                            boost::posix_time::seconds(timeout_seconds));
        }
        else
        {
            internal_lock.lock();
            return true;
        }
    }

    void ForceLock()
    {
        mutex.reset();
        boost::interprocess::named_mutex::remove(name);
        mutex = std::make_unique<boost::interprocess::named_mutex>(
            boost::interprocess::open_or_create, name);
    }

  private:
    int timeout_seconds;
    const char *name;
    std::unique_ptr<boost::interprocess::named_mutex> mutex;
    boost::interprocess::scoped_lock<boost::interprocess::named_mutex> internal_lock;
};

#endif
