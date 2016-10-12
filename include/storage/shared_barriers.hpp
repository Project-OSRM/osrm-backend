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
        : current_regions_mutex(boost::interprocess::open_or_create, "current_regions"),
          regions_1_mutex(boost::interprocess::open_or_create, "regions_1"),
          regions_2_mutex(boost::interprocess::open_or_create, "regions_2")
    {
    }

    static void resetCurrentRegions()
    {
        boost::interprocess::named_sharable_mutex::remove("current_regions");
    }
    static void resetRegions1()
    {
        boost::interprocess::named_sharable_mutex::remove("regions_1");
    }
    static void resetRegions2()
    {
        boost::interprocess::named_sharable_mutex::remove("regions_2");
    }

    boost::interprocess::named_upgradable_mutex current_regions_mutex;
    boost::interprocess::named_sharable_mutex regions_1_mutex;
    boost::interprocess::named_sharable_mutex regions_2_mutex;
};
}
}

#endif // SHARED_BARRIERS_HPP
