#ifndef SHARED_BARRIERS_HPP
#define SHARED_BARRIERS_HPP

#include <boost/interprocess/sync/named_sharable_mutex.hpp>
#include <boost/interprocess/sync/named_upgradable_mutex.hpp>

namespace osrm
{
namespace storage
{

struct SharedBarriers
{

    SharedBarriers()
        : current_region_mutex(boost::interprocess::open_or_create, "current_region"),
          region_1_mutex(boost::interprocess::open_or_create, "region_1"),
          region_2_mutex(boost::interprocess::open_or_create, "region_2")
    {
    }

    static void resetCurrentRegion()
    {
        boost::interprocess::named_sharable_mutex::remove("current_region");
    }
    static void resetRegion1() { boost::interprocess::named_sharable_mutex::remove("region_1"); }
    static void resetRegion2() { boost::interprocess::named_sharable_mutex::remove("region_2"); }

    boost::interprocess::named_upgradable_mutex current_region_mutex;
    boost::interprocess::named_sharable_mutex region_1_mutex;
    boost::interprocess::named_sharable_mutex region_2_mutex;
};
}
}

#endif // SHARED_BARRIERS_HPP
