#include "util/simple_logger.hpp"
#include "engine/datafacade/shared_barriers.hpp"

#include <iostream>

int main()
{
    osrm::util::LogPolicy::GetInstance().Unmute();
    try
    {
        osrm::util::SimpleLogger().Write() << "Releasing all locks";
        osrm::engine::datafacade::SharedBarriers barrier;
        barrier.pending_update_mutex.unlock();
        barrier.query_mutex.unlock();
        barrier.update_mutex.unlock();
    }
    catch (const std::exception &e)
    {
        osrm::util::SimpleLogger().Write(logWARNING) << "[excpetion] " << e.what();
    }
    return 0;
}
