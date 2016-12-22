#include "storage/shared_barriers.hpp"
#include "util/log.hpp"

#include <iostream>

int main()
{
    osrm::util::LogPolicy::GetInstance().Unmute();
    osrm::util::Log() << "Releasing all locks";

    osrm::storage::SharedBarriers::resetCurrentRegion();
    osrm::storage::SharedBarriers::resetRegion1();
    osrm::storage::SharedBarriers::resetRegion2();

    return 0;
}
