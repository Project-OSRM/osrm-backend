#include "storage/shared_barriers.hpp"
#include "util/simple_logger.hpp"

#include <iostream>

int main()
{
    osrm::util::LogPolicy::GetInstance().Unmute();
    osrm::util::SimpleLogger().Write() << "Releasing all locks";
    osrm::storage::SharedBarriers barrier;
    barrier.pending_update_mutex.unlock();
    barrier.query_mutex.unlock();
    return 0;
}
