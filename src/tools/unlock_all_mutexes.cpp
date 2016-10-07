#include "storage/shared_barriers.hpp"
#include "util/simple_logger.hpp"

#include <iostream>

int main()
{
    osrm::util::LogPolicy::GetInstance().Unmute();
    osrm::util::SimpleLogger().Write() << "Releasing all locks";
    osrm::storage::SharedBarriers barriers;
    boost::interprocess::named_upgradable_mutex::remove("current_regions");
    boost::interprocess::named_sharable_mutex::remove("regions_1");
    boost::interprocess::named_sharable_mutex::remove("regions_2");
    return 0;
}
