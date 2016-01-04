#include "util/version.hpp"
#include "util/simple_logger.hpp"
#include "engine/datafacade/shared_barriers.hpp"

#include <iostream>

int main()
{
    LogPolicy::GetInstance().Unmute();
    try
    {
        SimpleLogger().Write() << "starting up engines, " << OSRM_VERSION;
        SimpleLogger().Write() << "Releasing all locks";
        SharedBarriers barrier;
        barrier.pending_update_mutex.unlock();
        barrier.query_mutex.unlock();
        barrier.update_mutex.unlock();
    }
    catch (const std::exception &e)
    {
        SimpleLogger().Write(logWARNING) << "[excpetion] " << e.what();
    }
    return 0;
}
