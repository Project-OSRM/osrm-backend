#include "util/simple_logger.hpp"
#include "storage/shared_barriers.hpp"

#include <iostream>

int main() try
{
    osrm::util::LogPolicy::GetInstance().Unmute();
    osrm::util::SimpleLogger().Write() << "Releasing all locks";
    osrm::storage::SharedBarriers barrier;
    barrier.pending_update_mutex.unlock();
    barrier.query_mutex.unlock();
    barrier.update_mutex.unlock();
    return 0;
}
catch (const std::exception &e)
{
    osrm::util::SimpleLogger().Write(logWARNING) << "[excpetion] " << e.what();
    return EXIT_FAILURE;
}
