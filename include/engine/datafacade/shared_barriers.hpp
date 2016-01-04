#ifndef SHARED_BARRIERS_HPP
#define SHARED_BARRIERS_HPP

#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/named_condition.hpp>

struct SharedBarriers
{

    SharedBarriers()
        : pending_update_mutex(boost::interprocess::open_or_create, "pending_update"),
          update_mutex(boost::interprocess::open_or_create, "update"),
          query_mutex(boost::interprocess::open_or_create, "query"),
          no_running_queries_condition(boost::interprocess::open_or_create, "no_running_queries"),
          update_ongoing(false), number_of_queries(0)
    {
    }

    // Mutex to protect access to the boolean variable
    boost::interprocess::named_mutex pending_update_mutex;
    boost::interprocess::named_mutex update_mutex;
    boost::interprocess::named_mutex query_mutex;

    // Condition that no update is running
    boost::interprocess::named_condition no_running_queries_condition;

    // Is there an ongoing update?
    bool update_ongoing;
    // Is there any query?
    int number_of_queries;
};

#endif // SHARED_BARRIERS_HPP
