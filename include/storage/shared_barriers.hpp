#ifndef SHARED_BARRIERS_HPP
#define SHARED_BARRIERS_HPP

#include <boost/interprocess/sync/named_condition.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>

#include "util/retry_lock.hpp"

namespace osrm
{
namespace storage
{

struct SharedBarriers
{

    SharedBarriers()
        : region_mutex(boost::interprocess::open_or_create, "osrm-region"),
          region_condition(boost::interprocess::open_or_create, "osrm-region-cv")
    {
    }

    static void remove()
    {
        boost::interprocess::named_mutex::remove("osrm-region");
        boost::interprocess::named_condition::remove("osrm-region-cv");
    }

    static RetryLock getLockWithRetry(int timeout_seconds)
    {
        return RetryLock(timeout_seconds, "osrm-region");
    }

    boost::interprocess::named_mutex region_mutex;
    boost::interprocess::named_condition region_condition;
};
}
}

#endif // SHARED_BARRIERS_HPP
