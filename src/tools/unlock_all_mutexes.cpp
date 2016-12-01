#include "storage/shared_barriers.hpp"
#include "util/log.hpp"

#include <iostream>

int main()
{
    osrm::util::LogPolicy::GetInstance().Unmute();
    osrm::util::Log() << "Releasing all locks";

    osrm::storage::SharedBarriers::resetCurrentRegions();
    osrm::storage::SharedBarriers::resetRegions1();
    osrm::storage::SharedBarriers::resetRegions2();

    return 0;
}
