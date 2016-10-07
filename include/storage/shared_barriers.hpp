#ifndef SHARED_BARRIERS_HPP
#define SHARED_BARRIERS_HPP

#include <boost/interprocess/sync/named_condition.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/named_sharable_mutex.hpp>

namespace osrm
{
namespace storage
{
struct SharedBarriers
{

    SharedBarriers()
        : pending_update_mutex(boost::interprocess::open_or_create, "pending_update"),
          query_mutex(boost::interprocess::open_or_create, "query")
    {
    }

    // Mutex to protect access to the boolean variable
    boost::interprocess::named_mutex pending_update_mutex;
    boost::interprocess::named_sharable_mutex query_mutex;
};
}
}

#endif // SHARED_BARRIERS_HPP
