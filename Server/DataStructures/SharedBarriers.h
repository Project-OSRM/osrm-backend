#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>

struct SharedBarriers {

   SharedBarriers () :  update_ongoing(false), number_of_queries(0) { }

   // Mutex to protect access to the boolean variable
   boost::interprocess::interprocess_mutex   pending_update_mutex;
   boost::interprocess::interprocess_mutex   update_mutex;
   boost::interprocess::interprocess_mutex   query_mutex;

   // Condition that no update is running
   boost::interprocess::interprocess_condition  no_running_queries_condition;

   // Is there an ongoing update?
   bool update_ongoing;
   // Is there any query?
   int number_of_queries;
};
